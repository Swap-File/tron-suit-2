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

typedef struct {
	boolean fresh;
	boolean finger1_in_progress;
	boolean finger2_in_progress;
	boolean finger3_in_progress;
	boolean finger4_in_progress;

	int16_t yaw_raw;  //yaw pitch and roll in degrees * 100
	int16_t pitch_raw;
	int16_t roll_raw;

	int32_t yaw_offset;  //yaw pitch and roll in degrees * 100
	int32_t pitch_offset;
	int32_t roll_offset;

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
	CRGB output_rgb_led;
	uint8_t output_white_led;

	//calculated results
	int8_t gloveX;
	int8_t gloveY;

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

	uint8_t inner_offset;
	uint8_t outer_offset;

	uint8_t inner_magnitude;
	uint8_t outer_magnitude;

	uint8_t fade_level;
	uint8_t requested_mode;

	uint8_t packet_sequence_number;

} DISC;

typedef struct {
	uint8_t packets_in_per_second;
	uint8_t packets_out_per_second;

	uint8_t framing_errors;
	uint8_t crc_errors;

} SERIALSTATS;

#define GLOVE_DEADZONE 3000  //30 degrees
#define GLOVE_MAXIMUM 30000 //90 degrees

#define OLED_DC     18
#define OLED_CS     22
#define OLED_RESET  19
Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);

uint8_t fftmode = 2;
int mask_mode = 0;
uint8_t menu_mode = 1;

char sms_message[160];
int16_t sms_text_ending_pos = 0; //160 * 5 max length
int16_t sms_scroll_pos = 0;
int16_t menu_text_ending_pos = 0;

uint8_t scroll_count = 0;
int8_t scroll_pos_x = 0;
int8_t scroll_pos_x_offset = 0; //used for scrolling from the wrong end 

int8_t scroll_pos_y = 0;
uint8_t scroll_mode = 0;
long int scroll_timer = 0;

//crank up hardwareserial.cpp to 128 to match!
#define INCOMING1_BUFFER_SIZE 128
uint8_t incoming1_raw_buffer[INCOMING1_BUFFER_SIZE];
uint8_t incoming1_index = 0;
uint8_t incoming1_decoded_buffer[INCOMING1_BUFFER_SIZE];


ADC *adc = new ADC(); // adc object

AudioInputAnalog         adc1(A9);  //A9 is on ADC0
AudioAnalyzeFFT256       fft256_1;
AudioConnection          patchCord1(adc1, fft256_1);

#define NUM_LEDS_PER_STRIP 128
#define NUM_STRIPS 8
CRGB actual_output[NUM_STRIPS * NUM_LEDS_PER_STRIP];
CRGB target_output[NUM_STRIPS * NUM_LEDS_PER_STRIP];

#define TESTING
int color_on = 1;
unsigned long fps_time = 0;
uint16_t fpscount = 0;

GLOVE glove0;
GLOVE glove1;
SERIALSTATS serial1stats;
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

void setup() {

	strcpy(sms_message, "TESTING TEXT MESSAGE");

	display.begin(SSD1306_SWITCHCAPVCC);

	display.display();

	glove1.finger1_in_progress = false;
	glove0.finger1_in_progress = false;
	glove0.fresh = false;
	glove1.fresh = false;
	glove1.yaw_offset = 0;
	glove1.pitch_offset = 0;
	glove1.roll_offset = 0;
	glove0.yaw_offset = 0;
	glove0.pitch_offset = 0;
	glove0.roll_offset = 0;
	glove0.output_rgb_led = CRGB(0, 0, 0);
	glove0.output_white_led = 0;
	glove1.output_rgb_led = CRGB(0, 0, 0);
	glove1.output_white_led = 0;

	//must go first so Serial.begin can override pins!!!
	LEDS.addLeds<OCTOWS2811>(actual_output, NUM_LEDS_PER_STRIP);
	for (int i = 0; i < NUM_STRIPS * NUM_LEDS_PER_STRIP; i++) {
		actual_output[i] = CRGB(0, 0, 0);
		target_output[i] = CHSV(0, 0, 0);
	}

	//bump hardwareserial.cpp to 255
	Serial.begin(115200);   //Debug
	Serial1.begin(115200);  //Gloves	
	Serial2.begin(115200);  //Xbee	
	Serial3.begin(115200);  //BT

	//pinMode(A9, INPUT); //audio input

	AudioMemory(4);
	fft256_1.windowFunction(AudioWindowHanning256);
	fft256_1.averageTogether(4);


	pinMode(A3, INPUT);//A3 is on ADC1

	adc->setReference(ADC_REF_INTERNAL, ADC_1); //change all 3.3 to 1.2 if you change the reference
	adc->setAveraging(1, ADC_1); // set number of averages
	adc->setResolution(16, ADC_1); // set bits of resolution
	adc->setConversionSpeed(ADC_HIGH_SPEED, ADC_1); // change the conversion speed
	adc->setSamplingSpeed(ADC_HIGH_SPEED, ADC_1); // change the sampling speed
	
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

void loop() {



	


	fpscount++;

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

	if (Serial2.available()){
		Serial2.read();
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

		display.setCursor(menu_location_x + scroll_pos_x + scroll_pos_x_offset, menu_location_y + scroll_pos_y);
		display.print(menu_mode);
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

		//center the text from whatever direction its coming from
		switch (scroll_mode){
		case 3:
			//take up slack when scrolling in from the far side
			if (menu_text_ending_pos < 0 && scroll_pos_x < 0) scroll_pos_x -= menu_text_ending_pos;

			if (scroll_pos_x > 0) scroll_pos_x--;
			if (scroll_pos_y > 0) scroll_pos_y--;
			if (scroll_pos_x < 0) scroll_pos_x++;
			if (scroll_pos_y < 0) scroll_pos_y++;
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
			//scroll off the left side  
			scroll_pos_x--;

			if (menu_text_ending_pos < 0 && scroll_pos_x < -16){
				scroll_mode = 0;
			}
			break;
		}
	}


	//show it all
	display.display();

	LEDS.show();

	if (DiscSend.check()){
		disc0.packet_sequence_number++;
	}

	if (GloveSend.check()){

		uint8_t raw_buffer[14];

		raw_buffer[0] = 0;
		raw_buffer[1] = 255;
		raw_buffer[2] = 255;
		raw_buffer[3] = 128;
		raw_buffer[4] = 255;
		raw_buffer[5] = 255;
		raw_buffer[6] = disc0.packet_sequence_number ;
		raw_buffer[7] = disc0.packet_sequence_number;
		raw_buffer[8] = 8;
		raw_buffer[9] = 8;
		raw_buffer[10] = 0;
		raw_buffer[11] = 0;
		raw_buffer[12] = disc0.packet_sequence_number;
		raw_buffer[13] = OneWire::crc8(raw_buffer, 12);

		if (disc0.packet_sequence_number > 29){
			disc0.packet_sequence_number = 0;
		}
		uint8_t encoded_buffer[14];
		uint8_t encoded_size = COBSencode(raw_buffer, 14, encoded_buffer);
	
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

	if (YPRdisplay.check()){

	

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


	if (FPSdisplay.check()){

		long int start = micros();
		if (adc->isComplete(ADC_1)){

	

			adc->startContinuous(38, ADC_1);
		}
		Serial.println(micros() - start);

		

		fpscount = 0;
		//cpu_usage = 100 - (idle_microseconds / 10000);
		//local_packets_out_per_second_1 = local_packets_out_counter_1;
		//local_packets_in_per_second_1 = local_packets_in_counter_1;
		//idle_microseconds = 0;
		// local_packets_out_counter_1 = 0;
		// local_packets_in_counter_1 = 0;
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

boolean visible_menu(uint8_t x, uint8_t y){

	if (scroll_mode == 3){
		if (scroll_pos_y != 0){

			if (scroll_pos_y < 0){
				if (y >= -scroll_pos_y) return false;
			}
			if (scroll_pos_y > 0){
				if (y <= 8 - scroll_pos_y)  return false;
			}
		}
		if (scroll_pos_x != 0){

			if (scroll_pos_x < 0){
				if (x <= menu_text_ending_pos)  return false;
			}
			if (scroll_pos_x > 0){
				if (x >= scroll_pos_x)  return false;

			}
		}
	}
	if (scroll_mode == 2){
		return false;
	}
	if (scroll_mode == 1){
		if (x <= max(menu_text_ending_pos, scroll_pos_x + 16))  return false;
	}
	return true;

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
#define MODECHANGESTARTPOSX 18
#define MODECHANGESTARTPOSY 8

void readglove(void * temp){

	GLOVE * current_glove = (GLOVE *)temp;
	if (current_glove->finger1 == 1 && current_glove->finger1_in_progress == true){

		if (current_glove->gloveY <= 0){
			Serial.println(" down!");

			scroll_mode = 3;
			scroll_pos_x = 0;
			scroll_pos_y = -MODECHANGESTARTPOSY;

			//root menus
			if (menu_mode == 3){
				menu_mode = 2;
			}
			else if (menu_mode == 2){
				menu_mode = 1;
			}
			else if (menu_mode == 1){
				menu_mode = 3;
			}
			//10s menus
			if (menu_mode >= 10 && menu_mode <= 19){
				menu_mode--;

				//rollaround
				if (menu_mode == 9){
					menu_mode = 12;
				}
			}

			//20s menus
			if (menu_mode >= 20 && menu_mode <= 29){
				menu_mode--;

				//rollaround
				if (menu_mode == 19){
					menu_mode = 24;
				}
			}
			//30s menus
			if (menu_mode >= 30 && menu_mode <= 39){
				menu_mode--;

				//rollaround
				if (menu_mode == 29){
					menu_mode = 31;
				}
			}

		}
		else
		{
			if (current_glove->gloveY >= 7){
				Serial.println(" up!");

				scroll_mode = 3;
				scroll_pos_x = 0;
				scroll_pos_y = MODECHANGESTARTPOSY;



				//root menus
				if (menu_mode == 1){
					menu_mode = 2;
				}
				else if (menu_mode == 2){
					menu_mode = 3;
				}
				else if (menu_mode == 3){
					menu_mode = 1;
				}

				//10s menus
				if (menu_mode >= 10 && menu_mode <= 19){
					menu_mode++;

					//rollaround
					if (menu_mode == 13){
						menu_mode = 10;
					}
				}

				//20s menus
				if (menu_mode >= 20 && menu_mode <= 29){
					menu_mode++;

					//rollaround
					if (menu_mode == 24){
						menu_mode = 20;
					}
				}
				//30s menus
				if (menu_mode >= 30 && menu_mode <= 39){
					menu_mode++;

					//rollaround
					if (menu_mode == 32){
						menu_mode = 30;
					}
				}


			}

			else{
				if (current_glove->gloveX <= 0){
					Serial.println(" right!");

					scroll_mode = 3;
					scroll_pos_x = MODECHANGESTARTPOSX;
					scroll_pos_y = 0;



					if (menu_mode == 30){
						mask_mode = 0;
					}
					if (menu_mode == 31){
						mask_mode = 1;
					}

					if (menu_mode == 20){
						fftmode = 0;
					}
					if (menu_mode == 21){
						fftmode = 1;
					}
					if (menu_mode == 22){
						fftmode = 2;
					}
					if (menu_mode == 23){
						fftmode = 3;
					}

					if (menu_mode == 10){
						color_on = 0;
					}
					if (menu_mode == 11){
						color_on = 1;
					}


					//jump into first layer menu
					if (menu_mode < 10){
						menu_mode = menu_mode * 10;
					}






				}

				if (current_glove->gloveX >= 7){
					Serial.println(" left!");

					scroll_mode = 3;
					scroll_pos_x = -128; //negative a lot, this gets fixed by the shift code
					scroll_pos_y = 0;



					//back out of 10s menu
					if (menu_mode <= 19 && menu_mode >= 10){
						menu_mode = 1;
					}

					//back out of 20s menu
					if (menu_mode <= 29 && menu_mode >= 20){
						menu_mode = 2;
					}

					//back out of 30s menu
					if (menu_mode <= 39 && menu_mode >= 30){
						menu_mode = 3;
					}

					//back out of 40s menu
					if (menu_mode <= 49 && menu_mode >= 40){
						menu_mode = 4;
					}

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
}


void SerialUpdate(void){

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
}

void onPacket1(const uint8_t* buffer, size_t size)
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

			int temp_gloveX = 0;

			if (current_glove == &glove1){
				temp_gloveX = map((current_glove->yaw_compensated - 18000) - ((current_glove->pitch_compensated - 18000)*abs(sin((current_glove->roll_raw - 18000)* PI / 18000))) + 18000, 18000 - GLOVE_DEADZONE, 18000 + GLOVE_DEADZONE, 0, 8);
			}
			else{
				temp_gloveX = map((current_glove->yaw_compensated - 18000) + ((current_glove->pitch_compensated - 18000)*abs(sin((current_glove->roll_raw - 18000)* PI / 18000))) + 18000, 18000 - GLOVE_DEADZONE, 18000 + GLOVE_DEADZONE, 0, 8);
			}

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

			//check for gestures
			readglove(&glove0);
			readglove(&glove1);

		}
	}
}