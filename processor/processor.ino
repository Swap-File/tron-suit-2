#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#include <OneWire.h> //crc8 
#include <cobs.h> //cobs encoder and decoder 

#define USE_OCTOWS2811
#include <OctoWS2811.h> //DMA strip output
#include <FastLED.h> //LED handling

#include <Metro.h> //timers

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ADC.h>

//menu scroll-aways to match effect

#define MENU_SCROLL_OFF_UP 1
#define MENU_SCROLL_OFF_DOWN 2
#define MENU_SCROLL_OFF_LEFT 3
#define MENU_SCROLL_OFF_RIGHT 4
uint8_t menu_scroll_off = MENU_SCROLL_OFF_RIGHT;

#define HAND_DIRECTION_LEFT 1
#define HAND_DIRECTION_RIGHT 2
#define HAND_DIRECTION_UP 3
#define HAND_DIRECTION_DOWN 4

#define MENU_DEFAULT 1
#define MENU_FFT_ROOT 2

boolean flow_direction_positive = true;

typedef struct {
	boolean fresh = false;
	boolean finger1_in_progress = false;
	boolean finger2_in_progress = false;
	boolean finger3_in_progress = false;
	boolean finger4_in_progress = false;

	int16_t yaw_raw;  //yaw pitch and roll in degrees * 100
	int16_t pitch_raw;
	int16_t roll_raw;

	int32_t yaw_offset = 0;  //yaw pitch and roll in degrees * 100
	int32_t pitch_offset = 0;
	int32_t roll_offset = 0;

	int32_t yaw_compensated;  //yaw pitch and roll in degrees * 100
	int32_t pitch_compensated;
	int32_t roll_compensated;

	int16_t aaRealX; // gravity-free accel sensor measurements
	int16_t aaRealY;
	int16_t aaRealZ;

	int16_t gravityX; // world-frame accel sensor measurements
	int16_t gravityY;
	int16_t gravityZ;

	int16_t color_sensor_R;
	int16_t color_sensor_G;
	int16_t color_sensor_B;
	int16_t color_sensor_Clear;

	boolean finger1;
	boolean finger2;
	boolean finger3;
	boolean finger4;

	uint8_t packets_in_per_second;
	uint8_t packets_out_per_second;

	uint8_t framing_errors;
	uint8_t crc_errors;

	uint8_t cpu_usage;
	uint8_t cpu_temp;

	//outputs
	CRGB output_rgb_led = CRGB(0, 0, 0);
	uint8_t output_white_led = 0;

	//calculated results
	int8_t gloveX;
	int8_t gloveY;

	uint8_t disc_offset; // 0 to 29
	int16_t magnitude; //about 3000 is edges
} GLOVE;


typedef struct {
	//recieved data
	uint8_t packets_in_per_second;
	uint8_t packets_out_per_second;

	uint8_t framing_errors;
	uint8_t crc_errors;

	uint8_t cpu_usage;
	uint8_t cpu_temp;

	uint8_t saved_angle;
	uint8_t saved_magnitude;

	uint8_t battery_voltage;
	uint8_t current_mode;

	//sending data
	CHSV color1;
	CHSV color2;

	uint8_t inner_offset_requested = 0;
	uint8_t outer_offset_requested = 0;
	uint8_t inner_offset_reported = 0;
	uint8_t outer_offset_reported = 0;

	uint8_t inner_magnitude_requested = 8;
	uint8_t outer_magnitude_requested = 8;
	uint8_t inner_magnitude_reported = 0;
	uint8_t outer_magnitude_reported = 0;

	uint8_t fade_level;
	uint8_t requested_mode;

	uint8_t packet_sequence_number;
	uint8_t packet_beam;

} DISC;

typedef struct {
	uint8_t packets_in_per_second;
	uint8_t packets_out_per_second;

	uint8_t framing_errors;
	uint8_t crc_errors;

} SERIALSTATS;


uint8_t circle_x[30] = { 6, 7, 8, 9, 10, 11, 12, 12, 12, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 1, 2, 3, 4, 5 };
uint8_t circle_y[30] = { 0, 0, 0, 1, 1, 2, 3, 4, 5, 6, 7, 8, 8, 9, 9, 9, 9, 9, 8, 8, 7, 6, 5, 4, 3, 2, 1, 1, 0, 0 };


CHSV color1 = CHSV(0, 255, 255);
CHSV color2 = CHSV(64, 255, 255);

int8_t flow_offset = 0;


#define GLOVE_DEADZONE 3000  //30 degrees
#define GLOVE_MAXIMUM 30000 //90 degrees

#define OLED_DC     18
#define OLED_CS     22
#define OLED_RESET  19
Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);

uint8_t fftmode = 2;
int mask_mode = 0;
uint8_t menu_mode = 1;
double  temperature = 0.0;
double  voltage = 0.0;
char sms_message[160];
int16_t sms_text_ending_pos = 0; //160 * 5 max length
int16_t sms_scroll_pos = 0;
int16_t menu_text_ending_pos = 0;


//scroll state has 3 modes
//scrolling from start position to 0,0
//pausing at 0,0 according to scroll_timer
//scrolling from 0,0 to scroll_off pos
int8_t scroll_pos_x = 0; //starting position to scroll from x
int8_t scroll_pos_y = 0;  //starting position to scroll from y
int8_t scroll_end_pos_x = 0;  //destination to scroll to x
int8_t scroll_end_pos_y = 0; //destination to scroll to y
uint8_t scroll_mode = 0;  //keeps track of scroll state
long int scroll_timer = 0;  //time to pause at 0,0

//crank up hardwareserial.cpp to 128 to match!
#define INCOMING1_BUFFER_SIZE 128
uint8_t incoming1_raw_buffer[INCOMING1_BUFFER_SIZE];
uint8_t incoming1_index = 0;
uint8_t incoming1_decoded_buffer[INCOMING1_BUFFER_SIZE];

#define INCOMING2_BUFFER_SIZE 128
uint8_t incoming2_raw_buffer[INCOMING2_BUFFER_SIZE];
uint8_t incoming2_index = 0;
uint8_t incoming2_decoded_buffer[INCOMING2_BUFFER_SIZE];


ADC *adc = new ADC(); // adc object

AudioInputAnalog         adc1(A9);  //A9 is on ADC0
AudioAnalyzeFFT256       fft256_1;
AudioConnection          patchCord1(adc1, fft256_1);

#define NUM_LEDS_PER_STRIP 130 //longest strip is helmet, 128 LEDS + 2 indicators internal
#define NUM_STRIPS 8 //must be 8, even though only 6 are used
CRGB actual_output[NUM_STRIPS * NUM_LEDS_PER_STRIP];
CRGB target_output[NUM_STRIPS * NUM_LEDS_PER_STRIP];

#define TESTING
int color_on = 1;
unsigned long fps_time = 0;
uint16_t fpscount = 0;

GLOVE glove0;
GLOVE glove1;
SERIALSTATS serial1stats;
SERIALSTATS serial2stats;
DISC disc0;

byte gloveindicator[16][8];

uint8_t oled_dimmer = 0;
CHSV EQdisplay[16][8];
int EQdisplayValueMax[16]; //max vals for normalization over time

Metro FPSdisplay = Metro(1000);
Metro glovedisplayfade = Metro(10);
Metro YPRdisplay = Metro(100);
Metro ScrollSpeed = Metro(40);
Metro GloveSend = Metro(10);
Metro DiscSend = Metro(100);
Metro DiscSend2 = Metro(20);
Metro LEDdisplay = Metro(10);

uint8_t adc1_mode = 0;// round robin poll sensors
void setup() {



	strcpy(sms_message, "TESTING TEXT MESSAGE");

	display.begin(SSD1306_SWITCHCAPVCC);

	display.display();


	//must go first so Serial.begin can override pins!!!
	LEDS.addLeds<OCTOWS2811>(actual_output, NUM_LEDS_PER_STRIP);
	for (int i = 0; i < NUM_STRIPS * NUM_LEDS_PER_STRIP; i++) {
		actual_output[i] = CRGB(0, 0, 0);
		target_output[i] = CHSV(0, 0, 0);
	}

	//bump hardwareserial.cpp to 255
	Serial.begin(115200);   //Debug
	Serial1.begin(115200);  //Gloves	
	Serial2.begin(38400);  //Xbee	
	Serial3.begin(115200);  //BT

	//pinMode(A9, INPUT); //audio input

	AudioMemory(4);
	fft256_1.windowFunction(AudioWindowHanning256);
	fft256_1.averageTogether(4);


	pinMode(A3, INPUT);//A3 is on ADC1
	adc->setAveraging(32, ADC_1);
	adc->setResolution(16, ADC_1);

}

//HUD locations

#define trails_location_x 1
#define trails_location_y 1

#define sms_location_x 1
#define sms_location_y 19

#define menu_location_x 1
#define menu_location_y 10

#define realtime_location_x 18
#define realtime_location_y 1

#define hand_location_x 35
#define hand_location_y 1

#define static_menu_location_x 18
#define static_menu_location_y 10

#define inner_disc_location_x 1
#define inner_disc_location_y 30

#define outer_disc_location_x 15
#define outer_disc_location_y 30


void loop() {

	if (YPRdisplay.check()){


		switch (adc1_mode){
		case 0:
			voltage = voltage * .95 + .05 * (((uint16_t)adc->analogReadContinuous(ADC_1)) / 4083.375); //voltage
			adc->stopContinuous(ADC_1);
			adc->setReference(ADC_REF_INTERNAL, ADC_1);  //modified this to remove calibration, drops time from 2ms to ~2ns
			adc->startContinuous(38, ADC_1);
			adc1_mode = 1;
			break;
		case 1:
			// temp sensor from https ://github.com/manitou48/teensy3/blob/master/chiptemp.pde
			temperature = temperature * .95 + .05 * (25 - (((uint16_t)adc->analogReadContinuous(ADC_1)) - 38700) / -35.7); //temp in C
			adc->stopContinuous(ADC_1);
			adc->setReference(ADC_REF_EXTERNAL, ADC_1);  //modified this to remove calibration
			adc->startContinuous(A3, ADC_1);
			adc1_mode = 0;
			break;
		}


		//blindly keep re-initing the screen for hot plug!
		display.reinit();

		if (0){
			Serial.print("ypr\t");
			Serial.print(sin(glove1.roll_raw * PI / 18000));
			Serial.print("ypr\t");
			Serial.print(glove1.finger1);
			Serial.print("\t");
			Serial.print(glove1.yaw_compensated);
			Serial.print("\t");
			Serial.print(glove1.pitch_compensated);
			Serial.print("\t");
			Serial.print(glove1.roll_compensated);
			Serial.print("\t");
			Serial.print(glove1.yaw_raw);
			Serial.print("\t");
			Serial.print(glove1.pitch_raw);
			Serial.print("\t");
			Serial.print(glove1.roll_raw);
			Serial.print("\t");
			Serial.print(glove1.yaw_offset);
			Serial.print("\t");
			Serial.print(glove1.pitch_offset);
			Serial.print("\t");
			Serial.println(glove1.roll_offset);
		}
	}



	disc0.inner_offset_requested = glove1.disc_offset;
	disc0.outer_offset_requested = glove1.disc_offset;

	if (FPSdisplay.check()){


		Serial.print(serial2stats.packets_in_per_second);
		Serial.print(" ");
		Serial.print(serial2stats.packets_out_per_second);
		Serial.print(" ");
		Serial.println(temperature);
		serial1stats.packets_out_per_second = 0;
		fpscount = 0;
		//cpu_usage = 100 - (idle_microseconds / 10000);
		//local_packets_out_per_second_1 = local_packets_out_counter_1;
		//local_packets_in_per_second_1 = local_packets_in_counter_1;
		//idle_microseconds = 0;
		// local_packets_out_counter_1 = 0;
		// local_packets_in_counter_1 = 0;
	}




	if (glove1.finger2 == 0 || glove1.finger3 == 0){
		if (glove1.finger2 == 0){
			glove0.output_white_led = 0x01;
		}
		uint32_t sum = glove0.color_sensor_Clear;
		float r, g, b;
		r = glove0.color_sensor_R; r /= sum;
		g = glove0.color_sensor_G; g /= sum;
		b = glove0.color_sensor_B; b /= sum;
		r *= 256; g *= 256; b *= 256;

		CHSV temp = rgb2hsv_approximate(CRGB(r, g, b));
		//temp.v = min(temp.v, 200);
		temp.s = max(temp.s, 64);
		CRGB temp2;
		hsv2rgb_rainbow(temp, temp2);
		glove0.output_rgb_led = temp2;

	}
	else{
		glove0.output_white_led = 0x00;
		glove0.output_rgb_led = CRGB(0, 0, 0);
	}



	if (Serial3.available()){
		Serial3.read();

	}


	if (glovedisplayfade.check()){
		for (int x = 0; x < 16; x++) {
			for (int y = 0; y < 8; y++) {
				if (gloveindicator[x][y] > 0){
					gloveindicator[x][y] = gloveindicator[x][y] - 1;
				}
			}
		}
	}

	if (glove1.finger1 == 0){
		gloveindicator[8 + glove0.gloveX][7 - glove0.gloveY] = 100;// flip Y for helmet external display by subtracting from 7
		gloveindicator[glove1.gloveX][7 - glove1.gloveY] = 100;// flip Y for helmet external display by subtracting from 7
	}



	//HUD!
	display.clearDisplay();

	//global settings (might not need to keep setting, figure it out later
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setTextWrap(false);

	//most boxes are widt
	//draw the boxes are 18x10, thats 16x8 + the boarder pixels

	//draw the menu first for masking purposes
	display.drawRect(menu_location_x - 1, menu_location_y - 1, 18, 10, WHITE);

	display.setCursor(menu_location_x + scroll_pos_x, menu_location_y + scroll_pos_y);

	switch (menu_mode){
	case MENU_FFT_ROOT:
		display.print("FFT");
		break;
	default:
		display.print(menu_mode);
	}

	menu_text_ending_pos = display.getCursorX(); //save menu text length for elsewhere
	//black out the rest!  probably dont need full width... 
	display.fillRect(menu_location_x - 1, menu_location_y - 1 - 10, display.width() - (menu_location_x - 1), 10, BLACK);
	display.fillRect(menu_location_x - 1, menu_location_y - 1 + 10, display.width() - (menu_location_x - 1), 10, BLACK);
	display.fillRect(menu_location_x - 1 + 18, menu_location_y - 1, display.width() - (menu_location_x - 1 + 18), 10, BLACK);

	//staticmode
	display.drawRect(static_menu_location_x - 1, static_menu_location_y - 1, 35, 10, WHITE);
	display.setCursor(static_menu_location_x, static_menu_location_y);
	display.print(menu_mode);

	//hand display dots - its important to always know where your hands are.
	display.drawRect(hand_location_x - 1, hand_location_y - 1, 18, 10, WHITE);
	display.drawPixel(hand_location_x + 8 + glove0.gloveX, hand_location_y + 7 - glove0.gloveY, WHITE);
	display.drawPixel(hand_location_x + glove1.gloveX, hand_location_y + 7 - glove1.gloveY, WHITE);

	//background overlay with trails, also shows HUD changes
	display.drawRect(trails_location_x - 1, trails_location_y - 1, 18, 10, WHITE);
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 8; y++) {
			if (gloveindicator[x][y] >= 50){  //This sets where decayed pixels disappear from HUD
				display.drawPixel(trails_location_x + x, trails_location_y + y, WHITE);
			}
		}
	}


	//sms display
	display.drawRect(sms_location_x - 1, sms_location_y - 1, 18, 10, WHITE);
	display.setCursor(sms_location_x + 16 + sms_scroll_pos, sms_location_y);
	display.print(sms_message);
	sms_text_ending_pos = display.getCursorX(); //save menu text length for elsewhere
	//pad out message for contiual scrolling in the HUD, gives me more text at once
	display.setCursor(sms_text_ending_pos + 19, sms_location_y + 1);
	for (uint8_t i = 0; i < 7; i++){
		display.print(sms_message[i]);
	}


	//this gets updated during writing out the main LED display, its the last thing to do
	display.drawRect(realtime_location_x - 1, realtime_location_y - 1, 18, 10, WHITE);

	draw_disc(disc0.inner_offset_requested, disc0.inner_magnitude_requested, inner_disc_location_x, inner_disc_location_y);

	draw_disc(disc0.outer_offset_requested, disc0.outer_magnitude_requested, outer_disc_location_x, outer_disc_location_y);

	for (uint8_t y = 0; y < 8; y++) {
		for (uint8_t x = 0; x < 16; x++) {


			CRGB background_array = CRGB(0, 0, 0);
			CRGB menu_name = CRGB(0, 0, 0);
			CRGB final_color = CRGB(0, 0, 0);


			if (scroll_mode > 0){
				if (read_menu_pixel(x, y)){
					menu_name = CRGB(180, 180, 180);
				}
			}

			if (gloveindicator[x][7 - y] > 0){ // flip Y for helmet external display by subtracting from 7
				background_array = CRGB(gloveindicator[x][7 - y], gloveindicator[x][7 - y], gloveindicator[x][7 - y]);
			}


			if (mask_mode == 1){
				if (read_sms_pixel(x, y) != 0){
					if (menu_mode > 0){
						final_color = EQdisplay[x][y];
					}
				}
			}
			else{
				if (menu_mode > 0){
					final_color = EQdisplay[x][y];
				}
				final_color = background_array | menu_name | final_color;
			}

			//}
			//else{
			//	if (background_array || menu_name || read_sms_pixel(x,y-1) != 0){
			//		final_color = EQdisplay[x][y] ;
			//	}
			//}



			if ((final_color.r & 0xff == 0xff) | ((final_color.g >> 16) & 0xff == 0xff) | ((final_color.b >> 8) & 0xff == 0xff)){
				display.drawPixel(realtime_location_x + x, realtime_location_y + 7 - y, WHITE);
			}


			uint8_t tempindex;  //HUD is only 128 LEDs so it will fit in a byte

			if (y & 0x01 == 1){ // for odd rows scan from opposite side since external display is zig zag wired
				tempindex = (y << 4) + 15 - x;  //  << 4 is multiply by 16 pixels per row
			}
			else{
				tempindex = (y << 4) + x; //  << 4 is multiply by 16 pixels per row
			}

			target_output[tempindex] = final_color;
			actual_output[tempindex] = target_output[tempindex];
		}
	}

	//advance scrolling if timer reached or stop

	if (ScrollSpeed.check()){
		sms_scroll_pos--;
		if (sms_text_ending_pos < 0){
			sms_scroll_pos = 0;
		}

		switch (scroll_mode){
		case 3:
			//take up slack when scrolling in backwards
			if (menu_text_ending_pos < 0 && scroll_pos_x < 0) scroll_pos_x -= menu_text_ending_pos;

			if (scroll_pos_x > 0) scroll_pos_x--;
			else if (scroll_pos_y > 0) scroll_pos_y--;
			else if (scroll_pos_x < 0) scroll_pos_x++;
			else if (scroll_pos_y < 0) scroll_pos_y++;

			if (scroll_pos_y == 0 && scroll_pos_x == 0){
				scroll_mode = 2;
				scroll_timer = millis();
			}
			break;
		case 2:
			if (millis() - scroll_timer > 1000){
				scroll_mode = 1;
			}
			break;
		case 1:
			if (scroll_pos_x > scroll_end_pos_x) scroll_pos_x--;
			else if (scroll_pos_y > scroll_end_pos_y) scroll_pos_y--;
			else if (scroll_pos_x < scroll_end_pos_x) scroll_pos_x++;
			else if (scroll_pos_y < scroll_end_pos_y) scroll_pos_y++;

			if (scroll_pos_y == scroll_end_pos_y && scroll_pos_x == scroll_end_pos_x){
				scroll_mode = 0;
			}
		}
	}

	if (LEDdisplay.check()){
		//show it all
		display.display();

		LEDS.show();
	}

	if (DiscSend.check()){

		//bounce flow_offet between 0 and 16
		if (flow_offset == 0)		flow_direction_positive = true;
		else if (flow_offset == 16)	flow_direction_positive = false;

		flow_direction_positive ? flow_offset++ : flow_offset--;

		//calculate current pixels and add to array
		disc0.color1 = map_hsv(flow_offset, 0, 16, &color1, &color2);
		disc0.color2 = map_hsv(flow_offset, 0, 16, &color1, &color2);
		disc0.fade_level++;
		disc0.packet_sequence_number++;

	}
	if (DiscSend2.check()){

		disc0.packet_beam--;
	}

	if (GloveSend.check()){

		uint8_t raw_buffer[14];

		raw_buffer[0] = disc0.color1.h;
		raw_buffer[1] = disc0.color1.s;
		raw_buffer[2] = disc0.color1.v;
		raw_buffer[3] = disc0.color2.h;
		raw_buffer[4] = disc0.color2.s;
		raw_buffer[5] = disc0.color2.v;
		raw_buffer[6] = disc0.inner_offset_requested;
		raw_buffer[7] = disc0.outer_offset_requested;
		raw_buffer[8] = disc0.inner_magnitude_requested;
		raw_buffer[9] = disc0.outer_magnitude_requested;
		raw_buffer[10] = 128; //disc0.fade_level;  //fade
		raw_buffer[11] = 0x00;  //mode
		raw_buffer[12] = disc0.packet_sequence_number;  //sequence
		raw_buffer[13] = disc0.packet_beam;  //sequence
		raw_buffer[14] = OneWire::crc8(raw_buffer, 13);

		if (disc0.packet_sequence_number > 29){
			disc0.packet_sequence_number = 0;
		}
		uint8_t encoded_buffer[16];
		uint8_t encoded_size = COBSencode(raw_buffer, 15, encoded_buffer);

		Serial2.write(encoded_buffer, encoded_size);
		Serial2.write(0x00);


		raw_buffer[9];

		raw_buffer[0] = glove0.output_rgb_led.r;
		raw_buffer[1] = glove0.output_rgb_led.g;
		raw_buffer[2] = glove0.output_rgb_led.b;
		raw_buffer[3] = glove0.output_white_led;
		raw_buffer[4] = glove1.output_rgb_led.r;
		raw_buffer[5] = glove1.output_rgb_led.g;
		raw_buffer[6] = glove1.output_rgb_led.b;
		raw_buffer[7] = glove1.output_white_led;

		raw_buffer[8] = OneWire::crc8(raw_buffer, 7);

		// encoded_buffer[9];
		encoded_size = COBSencode(raw_buffer, 9, encoded_buffer);


		Serial1.write(encoded_buffer, encoded_size);
		Serial1.write(0x00);

		if (serial1stats.packets_out_per_second < 255){
			serial1stats.packets_out_per_second++;
		}
	}




	if (fft256_1.available()) {

		if (fftmode == 0){
			//move eq data left 1
			for (uint8_t x = 1; x < 16; x++) {
				for (uint8_t y = 0; y < 8; y++) {
					EQdisplay[x - 1][y] = EQdisplay[x][y];
				}
			}
		}

		if (fftmode == 1){
			//move eq data right 1
			for (uint8_t x = 15; x > 0; x--) {
				for (uint8_t y = 0; y < 8; y++) {
					EQdisplay[x][y] = EQdisplay[x - 1][y];
				}
			}
		}

		if (fftmode == 1 || fftmode == 0){
			for (uint8_t i = 0; i < 8; i++) {

				uint16_t n = 1000 * fft256_1.read((i * 5), (i * 5) + 5);

				switch (i){
				case 0:  n = max(n - 150, 0); break;
				case 1:  n = max(n - 15, 0);  break;
				case 2:  n = max(n - 6, 0);   break;
				default: n = max(n - 5, 0);   break;
				}

				//Serial.print((int)(n));
				//Serial.print(' ');

				EQdisplayValueMax[i] = max(max(EQdisplayValueMax[i] * .99, n), 10);

				uint8_t value = map(n, 0, EQdisplayValueMax[i], 0, 255);



				uint8_t hue = 0;
				uint8_t saturation = 255;

				if (0){
					hue = constrain(map(value, 240, 255, 0, 64), 0, 64);
				}
				if (fftmode == 0){

					EQdisplay[15][i] = CHSV(hue, saturation, value);

				}
				if (fftmode == 1){
					EQdisplay[0][i] = CHSV(hue, saturation, value);
				}
			}
			//Serial.println("");
		}

		if (fftmode == 2 || fftmode == 3){
			for (uint8_t i = 0; i < 16; i++) {

				uint16_t n = 1000 * fft256_1.read((i * 2), (i * 2) + 2);

				switch (i){
				case 0:  n = max(n - 150, 0); break;
				case 1:  n = max(n - 50, 0);  break;
				case 2:  n = max(n - 15, 0);  break;
				case 3:  n = max(n - 10, 0);  break;
				default: n = max(n - 3, 0);   break;
				}


				//Serial.print((int)(n));
				//Serial.print(' ');

				EQdisplayValueMax[i] = max(max(EQdisplayValueMax[i] * .98, n), 4);

				//int y = map(n, 0, EQdisplayValueMax[i], 0, 8);
				uint8_t value = constrain(map(n, 0, EQdisplayValueMax[i], 0, 255), 0, 255);

				uint8_t hue = 0;
				uint8_t saturation = 255;

				if (color_on){
					hue = constrain(map(value, 250, 255, 0, 128), 0, 64);
				}

				for (uint8_t index = 0; index < 8; index++){
					EQdisplay[i][index] = CHSV(hue, saturation, value);
				}
			}
			//Serial.println("");
		}
	}

	SerialUpdate();

}

boolean read_menu_pixel(uint8_t x, uint8_t y){

	//add 7 sub y to flip on y axis
	if (display.readPixel(menu_location_x + x, menu_location_y + (7 - y))){
		return true;
	}
	return false;
}

boolean read_sms_pixel(uint8_t x, uint8_t y){

	//add 7 sub y to flip on y axis
	if (display.readPixel(sms_location_x + x, sms_location_y + (7 - y))){
		return true;
	}
	return false;
}

#define MODECHANGEBRIGHTNESS 250  


void readglove(void * temp){

	GLOVE * current_glove = (GLOVE *)temp;
	if (current_glove->finger1 == 1 && current_glove->finger1_in_progress == true){

		if (current_glove->gloveY <= 0){
			menu_map(HAND_DIRECTION_DOWN);
		}
		else
		{
			if (current_glove->gloveY >= 7){
				menu_map(HAND_DIRECTION_UP);
			}

			else{
				if (current_glove->gloveX <= 0){
					menu_map(HAND_DIRECTION_RIGHT);
				}

				if (current_glove->gloveX >= 7){
					menu_map(HAND_DIRECTION_LEFT);
				}
			}
		}
	}

	if (current_glove->finger4 == 0){
		if (current_glove->finger4_in_progress == false){
			current_glove->finger4_in_progress = true;

			//if (menu_mode == 0){
			//	menu_mode = 1;

			//	}
			//	else{
			//	menu_mode = 0;
			//
			//	}

		}
	}
	else{
		current_glove->finger4_in_progress = false;


	}

	if (current_glove->finger1 == 0){
		if (current_glove->finger1_in_progress == false){
			current_glove->finger1_in_progress = true;
			current_glove->yaw_offset = current_glove->yaw_raw;
			current_glove->pitch_offset = current_glove->pitch_raw;
			current_glove->roll_offset = current_glove->roll_raw;

			//TEMP
			glove0.yaw_offset = glove0.yaw_raw;
			glove0.pitch_offset = glove0.pitch_raw;
			glove0.roll_offset = glove0.roll_raw;

		}
	}
	else{
		current_glove->finger1_in_progress = false;
	}
	fpscount++;
}


inline void SerialUpdate(void){

	while (Serial1.available()){

		//read in a byte
		incoming1_raw_buffer[incoming1_index] = Serial1.read();

		//check for end of packet
		if (incoming1_raw_buffer[incoming1_index] == 0x00){
			//try to decode
			uint8_t decoded_length = COBSdecode(incoming1_raw_buffer, incoming1_index, incoming1_decoded_buffer);

			//check length of decoded data (cleans up a series of 0x00 bytes)
			if (decoded_length > 0){
				onPacket1(incoming1_decoded_buffer, decoded_length);
			}

			//reset index
			incoming1_index = 0;
		}
		else{
			//read data in until we hit overflow, then hold at last position
			if (incoming1_index < INCOMING1_BUFFER_SIZE)
				incoming1_index++;
		}
	}


	while (Serial2.available()){

		//read in a byte
		incoming2_raw_buffer[incoming2_index] = Serial2.read();

		//check for end of packet
		if (incoming2_raw_buffer[incoming2_index] == 0x00){
			//try to decode
			uint8_t decoded_length = COBSdecode(incoming2_raw_buffer, incoming2_index, incoming2_decoded_buffer);

			//check length of decoded data (cleans up a series of 0x00 bytes)
			if (decoded_length > 0){
				onPacket2(incoming1_decoded_buffer, decoded_length);
			}

			//reset index
			incoming2_index = 0;
		}
		else{
			//read data in until we hit overflow, then hold at last position
			if (incoming2_index < INCOMING1_BUFFER_SIZE)
				incoming2_index++;
		}
	}


}


inline void onPacket2(const uint8_t* buffer, size_t size)
{

	if (size != 11){
		serial2stats.framing_errors++;

	}
	else{
		uint8_t crc = OneWire::crc8(buffer, size - 2);
		if (crc != buffer[size - 1]){
			serial2stats.crc_errors++;
		}
		else{

			if (serial2stats.packets_in_per_second < 255){
				serial2stats.packets_in_per_second++;
			}

			//Serial.println("Get xbee");


		}
	}
}

inline void onPacket1(const uint8_t* buffer, size_t size)
{

	if (size != 35){
		serial1stats.framing_errors++;
	}
	else{
		uint8_t crc = OneWire::crc8(buffer, size - 2);
		if (crc != buffer[size - 1]){
			serial1stats.crc_errors++;
		}
		else{

			if (serial1stats.packets_in_per_second < 255){
				serial1stats.packets_in_per_second++;
			}

			GLOVE * current_glove;
			if (buffer[33] == 0){
				current_glove = &glove0;
			}
			else if (buffer[33] == 1)
			{
				current_glove = &glove1;
			}

			current_glove->fresh = true;

			current_glove->yaw_raw = buffer[0] << 8 | buffer[1];
			current_glove->pitch_raw = buffer[2] << 8 | buffer[3];
			current_glove->roll_raw = buffer[4] << 8 | buffer[5];

			current_glove->yaw_compensated = current_glove->yaw_offset - current_glove->yaw_raw;
			current_glove->pitch_compensated = current_glove->pitch_offset - current_glove->pitch_raw;
			current_glove->roll_compensated = current_glove->roll_offset - current_glove->roll_raw;

			//center data on 18000, meaning 180.00 degrees
			current_glove->yaw_compensated = (current_glove->yaw_compensated + 2 * 36000 + 18000) % 36000;
			current_glove->pitch_compensated = (current_glove->pitch_compensated + 2 * 36000 + 18000) % 36000;
			current_glove->roll_compensated = (current_glove->roll_compensated + 2 * 36000 + 18000) % 36000;

			current_glove->aaRealX = buffer[6] << 8 | buffer[7];
			current_glove->aaRealY = buffer[8] << 8 | buffer[9];
			current_glove->aaRealZ = buffer[10] << 8 | buffer[11];

			current_glove->gravityX = buffer[12] << 8 | buffer[13];
			current_glove->gravityY = buffer[14] << 8 | buffer[15];
			current_glove->gravityZ = buffer[16] << 8 | buffer[17];

			current_glove->color_sensor_R = buffer[18] << 8 | buffer[19];
			current_glove->color_sensor_G = buffer[20] << 8 | buffer[21];
			current_glove->color_sensor_B = buffer[22] << 8 | buffer[23];
			current_glove->color_sensor_Clear = buffer[24] << 8 | buffer[25];

			current_glove->finger1 = bitRead(buffer[26], 0);
			current_glove->finger2 = bitRead(buffer[26], 1);
			current_glove->finger3 = bitRead(buffer[26], 2);
			current_glove->finger4 = bitRead(buffer[26], 3);

			current_glove->packets_in_per_second = buffer[27];
			current_glove->packets_out_per_second = buffer[28];

			current_glove->framing_errors = buffer[29];
			current_glove->crc_errors = buffer[30];

			current_glove->cpu_usage = buffer[31];
			current_glove->cpu_temp = buffer[32];

			//update the gesture grid (8x8 grid + overlap 
			int temp_gloveX_saved = 0;
			int temp_gloveX = 0;

			if (current_glove == &glove1){
				temp_gloveX_saved = (current_glove->yaw_compensated - 18000) - ((current_glove->pitch_compensated - 18000)*abs(sin((current_glove->roll_raw - 18000)* PI / 18000))) + 18000;
			}
			else{
				temp_gloveX_saved = (current_glove->yaw_compensated - 18000) + ((current_glove->pitch_compensated - 18000)*abs(sin((current_glove->roll_raw - 18000)* PI / 18000))) + 18000;
			}
			temp_gloveX = map(temp_gloveX_saved, 18000 - GLOVE_DEADZONE, 18000 + GLOVE_DEADZONE, 0, 8);

			int temp_gloveY = map((current_glove->pitch_compensated), 18000 - GLOVE_DEADZONE, 18000 + GLOVE_DEADZONE, 0, 8);
			temp_gloveY = constrain(temp_gloveY, 0, 7);

			if (current_glove == &glove0){
				temp_gloveX = constrain(temp_gloveX, -8, 7);
				//only update position if not at the edge
				if (temp_gloveX > -8 && temp_gloveX < 7){
					glove0.gloveY = temp_gloveY;
				}
			}

			if (current_glove == &glove1){
				temp_gloveX = constrain(temp_gloveX, 0, 15);

				//only update position if not at the edge
				if (temp_gloveX > 0 && temp_gloveX < 15){
					glove1.gloveY = temp_gloveY;
				}
			}

			if (temp_gloveY>0 && temp_gloveY < 7){
				current_glove->gloveX = temp_gloveX;
			}

			//calc disc circle location
			temp_gloveX = map(temp_gloveX_saved, 18000 - GLOVE_DEADZONE, 18000 + GLOVE_DEADZONE, -1024, 1024);

			temp_gloveY = map((current_glove->pitch_compensated), 18000 - GLOVE_DEADZONE, 18000 + GLOVE_DEADZONE, -1024, 1024);

			int angle = atan2(temp_gloveY, temp_gloveX) * 180 / PI;
			angle = (angle + 360 + 90) % 360;

			current_glove->disc_offset = constrain(map(angle, 0, 361, 0, 30), 0, 29);

			current_glove->magnitude = sqrt(temp_gloveX*temp_gloveX + temp_gloveY*temp_gloveY);


			//check for gestures
			readglove(&glove0);
			readglove(&glove1);

		}
	}
}

CHSV map_hsv(uint8_t input, uint8_t in_min, uint8_t in_max, CHSV* out_min, CHSV* out_max){
	return CHSV(
		(input - in_min) * (out_max->h - out_min->h) / (in_max - in_min) + out_min->h, \
		(input - in_min) * (out_max->s - out_min->s) / (in_max - in_min) + out_min->s, \
		(input - in_min) * (out_max->v - out_min->v) / (in_max - in_min) + out_min->v);
}



void draw_disc(uint8_t index_offset, uint8_t magnitude, uint8_t x_offset, uint8_t y_offset){

	//adjust reference
	index_offset = (index_offset + 15) % 30;
	//draw pointer line
	display.drawLine(x_offset + 6, y_offset + 5, x_offset + circle_x[index_offset], y_offset + circle_y[index_offset], WHITE);
	display.drawLine(x_offset + 6, y_offset + 4, x_offset + circle_x[index_offset], y_offset + circle_y[index_offset], WHITE);

	//draw the circle
	if (magnitude <= 16){
		for (uint8_t i = 0; i < 16; i++) {
			if (i < magnitude){
				display.drawPixel(x_offset + circle_x[(index_offset + i) % 30], y_offset + circle_y[(index_offset + i) % 30], WHITE);
				display.drawPixel(x_offset + circle_x[(index_offset + 30 - i) % 30], y_offset + circle_y[(index_offset + 30 - i) % 30], WHITE);
			}
		}
	}
	else if (magnitude > 16 && magnitude <= 32){
		for (uint8_t i = 0; i < 16; i++) {
			if (i >(magnitude - 17)){
				display.drawPixel(x_offset + circle_x[i], y_offset + circle_y[i], WHITE);
				display.drawPixel(x_offset + circle_x[30 - i], y_offset + circle_y[30 - i], WHITE);
			}
		}
	}
	else if (magnitude > 32 && magnitude <= 47){
		for (uint8_t i = 0; i < 16; i++) {
			if (i <= magnitude - 32) display.drawPixel(x_offset + circle_x[i], y_offset + circle_y[i], WHITE);
			if (i <= magnitude - 32) display.drawPixel(x_offset + circle_x[i + 15], y_offset + circle_y[i + 15], WHITE);
		}
	}
	else if (magnitude > 47 && magnitude <= 62){
		for (uint8_t i = 0; i < 16; i++) {
			if (i > magnitude - 47) display.drawPixel(x_offset + circle_x[i], y_offset + circle_y[i], WHITE);
			if (i > magnitude - 47) display.drawPixel(x_offset + circle_x[i + 15], y_offset + circle_y[i + 15], WHITE);
		}
	}
	else if (magnitude > 62 && magnitude <= 77){
		for (uint8_t i = 0; i < 16; i++) {


		}
	}

	//blank the beam pixel
	//if (disc0.packet_beam < 17 && disc0.packet_beam > 0 && disc0.fade_level < 255){
	//	display.drawPixel(circle_xcoord(disc0.packet_beam), circle_ycoord(disc0.packet_beam), BLACK);
	//	display.drawPixel(circle_xcoord(30 - disc0.packet_beam), circle_ycoord(30 - disc0.packet_beam), BLACK);
	//}

}




void menu_map(uint8_t direction){

	//set initial scroll on and scroll off variables
	//can/will be overridden by menu below if desired
	switch (direction){
	case HAND_DIRECTION_LEFT:
		menu_scroll_start_left();
		menu_scroll_end_left();
		break;
	case HAND_DIRECTION_RIGHT:
		menu_scroll_start_right();
		menu_scroll_end_right();
		break;
	case HAND_DIRECTION_UP:
		menu_scroll_start_up();
		menu_scroll_end_up();
		break;
	case HAND_DIRECTION_DOWN:
		menu_scroll_start_down();
		menu_scroll_end_down();
		break;
	}


	switch (menu_mode){
	case MENU_FFT_ROOT:
	case MENU_DEFAULT:
		switch (direction){
		case HAND_DIRECTION_LEFT:
			menu_scroll_off = MENU_SCROLL_OFF_LEFT; // menu scrolling exit type
			menu_mode = MENU_FFT_ROOT; //new menu screen 
			break;
		case HAND_DIRECTION_RIGHT:
			menu_scroll_off = MENU_SCROLL_OFF_RIGHT;
			menu_mode = MENU_FFT_ROOT;
			break;
		case HAND_DIRECTION_UP:
			menu_scroll_off = MENU_SCROLL_OFF_UP;
			menu_mode = MENU_FFT_ROOT;
			break;
		case HAND_DIRECTION_DOWN:
			menu_scroll_off = MENU_SCROLL_OFF_DOWN;
			menu_mode = MENU_FFT_ROOT;
			break;
		}
		break;
	}
}

//locations to start a scroll movement from
inline void menu_scroll_start_left(void){
	scroll_mode = 3;
	scroll_pos_x = -128; //negative longer than longest text message, the slack will get taken up by the shift code
	scroll_pos_y = 0;
}

inline void menu_scroll_start_right(void){
	scroll_mode = 3;
	scroll_pos_x = 18;
	scroll_pos_y = 0;
}

inline void menu_scroll_start_up(void){
	scroll_mode = 3;
	scroll_pos_x = 0;
	scroll_pos_y = 8;
}

inline void menu_scroll_start_down(void){
	scroll_mode = 3;
	scroll_pos_x = 0;
	scroll_pos_y = -8;
}

//locations to stop a scroll movement at
inline void menu_scroll_end_left(void){
	scroll_end_pos_x = 16;
	scroll_end_pos_y = 0;
}

inline void menu_scroll_end_right(void){
	scroll_end_pos_x = -128;  //negative longer than longest text message, the slack will get taken up by the shift code
	scroll_end_pos_y = 0;
}

inline void menu_scroll_end_up(void){
	scroll_end_pos_x = 0;
	scroll_end_pos_y = -8;
}

inline void menu_scroll_end_down(void){
	scroll_end_pos_x = 0;
	scroll_end_pos_y = 8;
}
