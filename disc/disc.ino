#include <OneWire.h> //crc8 
#include <cobs.h> //cobs encoder and decoder 

#define USE_OCTOWS2811
#include <OctoWS2811.h> //DMA strip output
#include <FastLED.h> //LED handling

#include <Metro.h> //timers

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

#include <ADC.h>

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

//#define TESTING

MPU6050 mpu;

ADC *adc = new ADC(); // adc object

#define NUM_LEDS_PER_STRIP 30
#define NUM_STRIPS 8
CRGB actual_output[NUM_STRIPS * NUM_LEDS_PER_STRIP];
CHSV target_output[NUM_STRIPS * NUM_LEDS_PER_STRIP];
boolean dimmable[NUM_STRIPS * NUM_LEDS_PER_STRIP];

uint8_t disc_mode = 2;
uint8_t effect_mode = 0x10;
uint8_t fade_level = 0;

//pacman opening data
uint32_t inner_opening_time = 0;
uint32_t outer_opening_time = 0;
boolean opening_inner = false;
boolean opening_outer = false;
uint8_t opening_inner_index = 0;
uint8_t opening_outer_index = 0;

uint8_t flow_offset = 0;
uint8_t saved_outer_index_acceleration = 0;
uint8_t saved_inner_index_acceleration = 0;

//main two colors with starter values
CHSV color1 = CHSV(128, 255, 255);
CHSV color2 = CHSV(0, 255, 255);


const uint8_t blur_rate = 2;

//buffers to hold data as it streams in
int8_t stream_head = 0;
CHSV color1_streaming[16];
CHSV color2_streaming[16];



//pointers to colors, set by effect modes
CHSV *outer_stream = color2_streaming;
CHSV *inner_stream = color1_streaming;


//effect mode 2 variables
uint8_t inner_offset_requested = 0;
uint8_t outer_offset_requested = 0;
uint8_t inner_magnitude_displayed = 16;
uint8_t outer_magnitude_displayed = 16;
uint8_t inner_index = 16;
uint8_t outer_index = 16;
uint8_t inner_magnitude_requested = 16;
uint8_t outer_magnitude_requested = 16;

//serial com data
#define INCOMING_BUFFER_SIZE 128
uint8_t incoming_raw_buffer[INCOMING_BUFFER_SIZE];
uint8_t incoming_index = 0;
uint8_t incoming_decoded_buffer[INCOMING_BUFFER_SIZE];
uint8_t last_packet_sequence_number = 0;

//fps display and debug
Metro FPSdisplay = Metro(1000);

uint32_t cooldown = 0;

//logging data
uint8_t cpu_usage = 0;
uint32_t idle_microseconds = 0;
uint8_t crc_error = 0;
uint8_t framing_error = 0;
double temperature = 0.0;
double voltage = 24.0;
uint8_t packets_in_counter = 0;  //counts up
uint8_t packets_in_per_second = 0; //saves the value
uint8_t packets_out_counter = 0;  //counts up
uint8_t packets_out_per_second = 0; //saves the value

double xy_accel_magnitude_smoothed = 0;
double saved_xy_accel_magnitude_smoothed = 0;

//start voltage at full
uint16_t disc_voltage = 1024;

//activity LED and counter to slow pulses
#define LED_PIN 13 
uint8_t blinkState = 0x00;

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
	mpuInterrupt = true;
}

void setup() {

	//battery meter pin
	pinMode(A9, INPUT);
	adc->setReference(ADC_REF_EXTERNAL, ADC_0);
	adc->setAveraging(32, ADC_0);
	adc->setResolution(16, ADC_0);
	adc->startContinuous(A9, ADC_0);

	//temp sensor
	adc->setReference(ADC_REF_INTERNAL, ADC_1);
	adc->setAveraging(32, ADC_1);
	adc->setResolution(16, ADC_1);
	adc->startContinuous(38, ADC_1);

	//Inner LEDs are 0 to 29
	//Outer LEDs are 120 to 129

	// join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
	Wire.begin();
	TWBR = 24; // 400kHz I2C clock (200kHz if CPU is 8MHz)
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
	Fastwire::setup(400, true);
#endif

	Serial.begin(115200); //Debug
	Serial1.begin(115200); //Xbee

	// initialize device
	Serial.println(F("Initializing I2C devices..."));
	mpu.initialize();

	// verify connection
	Serial.println(F("Testing device connections..."));
	Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

	// load and configure the DMP
	Serial.println(F("Initializing DMP..."));
	devStatus = mpu.dmpInitialize();

	//collected from calibration app
	mpu.setXGyroOffset(109);
	mpu.setYGyroOffset(53);
	mpu.setZGyroOffset(54);
	mpu.setXAccelOffset(-1057);
	mpu.setYAccelOffset(669);
	mpu.setZAccelOffset(1482);

	// make sure it worked (returns 0 if so)
	if (devStatus == 0) {
		// turn on the DMP, now that it's ready
		Serial.println(F("Enabling DMP..."));
		mpu.setDMPEnabled(true);

		// enable Teensy 3.1 interrupt detection
		Serial.println(F("Enabling interrupt detection (Teensy 3.1 pin 22)..."));
		pinMode(22, INPUT);
		attachInterrupt(22, dmpDataReady, RISING);
		mpuIntStatus = mpu.getIntStatus();

		// set our DMP Ready flag so the main loop() function knows it's okay to use it
		Serial.println(F("DMP ready! Waiting for first interrupt..."));
		dmpReady = true;

		// get expected DMP packet size for later comparison
		packetSize = mpu.dmpGetFIFOPacketSize();
	}
	else {
		// ERROR!
		// 1 = initial memory load failed
		// 2 = DMP configuration updates failed
		// (if it's going to break, usually the code will be 1)
		Serial.print(F("DMP Initialization failed (code "));
		Serial.print(devStatus);
		Serial.println(F(")"));
	}

	// configure LED for output
	pinMode(LED_PIN, OUTPUT);



	//wipe and then put a single color in the streaming buffer for debug
	for (int i = 0; i < 16; i++) {
		color2_streaming[i] = CHSV(0, 0, 0);
		color1_streaming[i] = CHSV(0, 0, 0);
	}

	//start the leds
	LEDS.addLeds<OCTOWS2811>(actual_output, NUM_LEDS_PER_STRIP);
	for (int i = 0; i < NUM_STRIPS * NUM_LEDS_PER_STRIP; i++) {
		actual_output[i] = CRGB(0, 0, 0);
		target_output[i] = CHSV(0, 0, 0);
		dimmable[i] = true;
	}

	LEDS.show();

	//final reset to start it all
	mpu.resetFIFO();
}

void loop() {
	// if programming failed, don't try to do anything
	if (!dmpReady) return;

	long int idle_start_timer = 0;

	// wait for MPU interrupt or extra packet(s) available
	while (!mpuInterrupt && fifoCount < packetSize) {

		if (FPSdisplay.check()){
			cpu_usage = 100 - (idle_microseconds / 10000);
			idle_microseconds = 0;
			packets_in_per_second = packets_in_counter;
			packets_in_counter = 0;
			packets_out_per_second = packets_out_counter;
			packets_out_counter = 0;
			Serial.print(packets_in_per_second);
			Serial.print(" ");
			Serial.print(packets_out_per_second);
			Serial.print(" ");
			Serial.print(cpu_usage);
			Serial.print(" ");
			Serial.println(disc_mode);
			Serial.print("Voltage: ");
			Serial.println(voltage);
			Serial.print("Temp in C: ");
			Serial.println(temperature);

		}

		/* Increment flow via timer
		if (flow_speed.check()){
		increment_stream();
		}
		*/

		SerialUpdate();

		//set color modes from data
		outer_stream = bitRead(effect_mode, 0) ? color1_streaming : color2_streaming;
		inner_stream = bitRead(effect_mode, 1) ? color2_streaming : color1_streaming;


		if (idle_start_timer == 0){

			idle_start_timer = micros();
		}
	}

	idle_microseconds = idle_microseconds + (micros() - idle_start_timer);


	// reset interrupt flag and get INT_STATUS byte
	mpuInterrupt = false;
	mpuIntStatus = mpu.getIntStatus();

	// get current FIFO count
	fifoCount = mpu.getFIFOCount();

	// check for overflow (this should never happen unless our code is too inefficient)
	if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
		// reset so we can continue cleanly
		mpu.resetFIFO();
		Serial.println(F("FIFO overflow!"));

		// otherwise, check for DMP data ready interrupt (this should happen frequently)
	}
	else if (mpuIntStatus & 0x02) {
		// wait for correct available data length, should be a VERY short wait
		while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

		// read a packet from FIFO
		mpu.getFIFOBytes(fifoBuffer, packetSize);

		// track FIFO count here in case there is > 1 packet available
		// (this lets us immediately read more without waiting for an interrupt)
		fifoCount -= packetSize;

		mpu.dmpGetQuaternion(&q, fifoBuffer);
		mpu.dmpGetGravity(&gravity, &q);
		mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

		mpu.dmpGetAccel(&aa, fifoBuffer);
		mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);

#ifdef TESTING
		Serial.print("ypr\t");
		Serial.print(ypr[0] * 180 / M_PI);
		Serial.print("\t");
		Serial.print(ypr[1] * 180 / M_PI);
		Serial.print("\t");
		Serial.print(ypr[2] * 180 / M_PI);
		Serial.print("\t");

		Serial.print("areal\t");
		Serial.print(gravity.x);
		Serial.print("\t");
		Serial.print(gravity.y);
		Serial.print("\t");
		Serial.print(gravity.z);
#endif

		//4.77 = (30/2) / PI + 0.5 physical offset
		double temp = (atan2(gravity.x, gravity.y) * 4.77) + 15;

		while (temp < 0){
			temp = temp + 29;
		}
		while (temp > 29){
			temp = temp - 29;
		}

		inner_index = ((int)floor((temp + 0.5) + 30)) % 30;
		outer_index = ((29 - inner_index) + 32) % 30;

		//instant accel mag
		double xy_accel_magnitude = sqrt((long)aaReal.x*aaReal.x + aaReal.y*aaReal.y);

		//smooth a bit...
		xy_accel_magnitude_smoothed = xy_accel_magnitude_smoothed * .8 + xy_accel_magnitude * .2;

		double temp2 = (atan2(aaReal.x, aaReal.y) * 4.77) + 15;
		int inner_index_acceleration = ((int)floor((temp2 + 0.5) + 30)) % 30;
		int outer_index_acceleration = ((29 - inner_index_acceleration) + 32) % 30;


		boolean cooldowncomplete = false;
		if (millis() - cooldown > 250){
			cooldowncomplete = true;
		}

		if (disc_mode == 0 && opening_outer == false){
			inner_magnitude_displayed = 0;
			outer_magnitude_displayed = 0;

			if (xy_accel_magnitude > 4000){
				outer_opening_time = millis();
				opening_outer = true;
				opening_outer_index = outer_index_acceleration;
				cooldown = millis();
			}
		}


		if (disc_mode == 1 && opening_inner == false && cooldowncomplete){

			inner_magnitude_displayed = 0;

			if (xy_accel_magnitude > 4000){
				inner_opening_time = millis();
				opening_inner = true;
				opening_inner_index = inner_index_acceleration;

			}
		}

		if (opening_outer == true){

			disc_mode = 1;
			outer_index = opening_outer_index;

			int location = map(millis() - outer_opening_time, 0, 500, 0, 16);

			if (location < 16){
				outer_magnitude_displayed = location;
			}
			else {
				opening_outer = false;
			}
		}


		if (opening_inner == true){
			inner_index = opening_inner_index;

			int location = map(millis() - inner_opening_time, 0, 500, 0, 16);

			if (location < 16){
				inner_magnitude_displayed = location;
			}
			else {
				disc_mode = 2;
				opening_inner = false;
			}
		}


		if (disc_mode >= 2){

			if (abs(aaReal.z) > 8000 && cooldowncomplete){
				disc_mode = 2;
			}

			if (xy_accel_magnitude > 5000 && cooldowncomplete){
				disc_mode = 3;
			}

		}

		if (disc_mode == 2){
			inner_magnitude_displayed = inner_magnitude_requested;
			outer_magnitude_displayed = outer_magnitude_requested;
		}

		if (disc_mode == 3){

			//maybe adjust the effect for range 
			//stuff the streaming buffers with color data
			for (uint8_t current_pixel = 0; current_pixel < 16; current_pixel++) {

				//check the 0-15 thing?
				CHSV temp = CHSV(0, 0, 0);
				color1_streaming[stream_head] = map_hsv(current_pixel, 0, 15, &color1, &temp);
				color2_streaming[stream_head] = map_hsv(current_pixel, 0, 15, &color2, &temp);

			}

			if (xy_accel_magnitude > 3000){
				uint8_t magnitude_temp = constrain(map(xy_accel_magnitude, 3000, 10000, 1, 10), 1, 10);

				inner_magnitude_displayed = magnitude_temp;
				outer_magnitude_displayed = magnitude_temp;

				saved_inner_index_acceleration = inner_index_acceleration;
				saved_outer_index_acceleration = outer_index_acceleration;
				saved_xy_accel_magnitude_smoothed = xy_accel_magnitude_smoothed;
			}
			else{
				uint8_t magnitude_temp = constrain(map(saved_xy_accel_magnitude_smoothed, 3000, 10000, 1, 10), 1, 10);

				inner_magnitude_displayed = magnitude_temp;
				outer_magnitude_displayed = magnitude_temp;
			}

			inner_index = saved_inner_index_acceleration;
			outer_index = saved_outer_index_acceleration;


		}
		
		// blink LED to indicate activity
		blinkState++;
		digitalWrite(LED_PIN, bitRead(blinkState, 2));

		//polar rendering
		//renders for 16 pixels - each pixel is mirrored on either side of disc
		//pixel 1 and 16 overlaps - pixel 0 is off
		for (uint8_t current_pixel = 0; current_pixel < 17; current_pixel++) {

			//set inner 30 LEDs
			if (current_pixel > 0 && current_pixel <= inner_magnitude_displayed){

				//streaming input render
				int absolute_LED_index;
				CHSV LED_color = inner_stream [(current_pixel - 1 + stream_head) % 16];

				absolute_LED_index =((60 + inner_index + (current_pixel - 1) + inner_offset_requested) % 30);
				target_output[absolute_LED_index] = LED_color;
				dimmable[absolute_LED_index] = last_packet_sequence_number == current_pixel ? false : true;

				absolute_LED_index = ((60 + inner_index - ((current_pixel - 1) + inner_offset_requested)) % 30);
				target_output[absolute_LED_index] = LED_color;
				dimmable[absolute_LED_index] = last_packet_sequence_number == current_pixel ? false : true;

			}
			else{
				//clear unused pixels
				int absolute_LED_index;

				absolute_LED_index = ((60 + inner_index + (current_pixel - 1) + inner_offset_requested) % 30);
				target_output[absolute_LED_index] = CHSV(0, 0, 0);
				dimmable[absolute_LED_index] =true;

				absolute_LED_index =  ((60 + inner_index - ((current_pixel - 1) + inner_offset_requested)) % 30);
				target_output[absolute_LED_index] = CHSV(0, 0, 0);	
				dimmable[absolute_LED_index] =  true;
	
			}

			//set outer LEDs
			if (current_pixel > 0 && current_pixel <= outer_magnitude_displayed){

				//streaming input render
				int absolute_LED_index;
				CHSV LED_color = outer_stream[(current_pixel - 1 + stream_head) % 16];

				absolute_LED_index = 120 + ((60 + outer_index + (current_pixel - 1) + outer_offset_requested) % 30);
				target_output[absolute_LED_index] = LED_color;
				dimmable[absolute_LED_index] = last_packet_sequence_number == current_pixel ? false : true;

				absolute_LED_index = 120 + ((60 + outer_index - ((current_pixel - 1) + outer_offset_requested)) % 30);
				target_output[absolute_LED_index] = LED_color;
				dimmable[absolute_LED_index] = last_packet_sequence_number == current_pixel ? false : true;

			}
			else{
				//clear unused pixels
				int absolute_LED_index;

				absolute_LED_index = 120 + ((60 + outer_index + (current_pixel - 1) + outer_offset_requested) % 30);
				target_output[absolute_LED_index] = CHSV(0, 0, 0);
				dimmable[absolute_LED_index] = true;

				absolute_LED_index = 120 + ((60 + outer_index - ((current_pixel - 1) + outer_offset_requested)) % 30);
				target_output[absolute_LED_index] = CHSV(0, 0, 0);
				dimmable[absolute_LED_index] = true;
			}
		}

		//Linear Blurring & Dimming Code
		for (int i = 0; i < NUM_STRIPS * NUM_LEDS_PER_STRIP; i++) {

			//convert HSV to RGB
			CRGB temp_rgb = target_output[i];
			if (dimmable[i]) temp_rgb.fadeToBlackBy(fade_level);

			//if shutting off or turning on a LED, do so immediately
			if ((temp_rgb == CRGB(0, 0, 0) || actual_output[i] == CRGB(0, 0, 0)) || 1==1  ){
				actual_output[i] = temp_rgb;
			}
			//if changing a LED, blur
			else{
				if (actual_output[i].r < temp_rgb.r) temp_rgb.r = min(actual_output[i].r + blur_rate, temp_rgb.r);
				if (actual_output[i].r > temp_rgb.r) temp_rgb.r = max(actual_output[i].r - blur_rate, temp_rgb.r);

				if (actual_output[i].g < temp_rgb.g) temp_rgb.g = min(actual_output[i].g + blur_rate, temp_rgb.g);
				if (actual_output[i].g > temp_rgb.g) temp_rgb.g = max(actual_output[i].g - blur_rate, temp_rgb.g);

				if (actual_output[i].b < temp_rgb.b) temp_rgb.b = min(actual_output[i].b + blur_rate, temp_rgb.b);
				if (actual_output[i].b > temp_rgb.b) temp_rgb.b = max(actual_output[i].b - blur_rate, temp_rgb.b);

				actual_output[i] = temp_rgb;
			}
		}

		LEDS.show();

		//906 * 64 is low when plugged into batteries, about 3.55 v per cell
		voltage = voltage * .95 + .05 * (((uint16_t)adc->analogReadContinuous(ADC_0)) / 4083.375); //voltage

		//temp sensor from https://github.com/manitou48/teensy3/blob/master/chiptemp.pde
		temperature = temperature * .95 + .05 * (25 - (((uint16_t)adc->analogReadContinuous(ADC_1)) - 38700) / -35.7); //temp in C

	}
}


void SerialUpdate(void){
	while (Serial1.available()){

		//read in a byte
		incoming_raw_buffer[incoming_index] = Serial1.read();

		//check for end of packet
		if (incoming_raw_buffer[incoming_index] == 0x00){

			//try to decode
			uint8_t decoded_length = COBSdecode(incoming_raw_buffer, incoming_index, incoming_decoded_buffer);

			//check length of decoded data (cleans up a series of 0x00 bytes)
			if (decoded_length > 0){
				receivePacket(incoming_decoded_buffer, decoded_length);
			}

			//reset index
			incoming_index = 0;
		}
		else{
			//read data in until we hit overflow, then hold at last position
			if (incoming_index < INCOMING_BUFFER_SIZE)
				incoming_index++;
		}
	}
}

void sendPacket(){
	//send reply

	byte raw_buffer[11];
	raw_buffer[0] = inner_index;
	raw_buffer[1] = inner_magnitude_displayed;
	raw_buffer[2] = disc_mode; //add disc_mode and indicator together
	raw_buffer[3] = packets_in_per_second;
	raw_buffer[4] = packets_out_per_second;
	raw_buffer[5] = cpu_usage;
	raw_buffer[6] = (uint8_t)(voltage * 100);
	raw_buffer[7] = (uint8_t)(temperature * 100);
	raw_buffer[8] = crc_error;
	raw_buffer[9] = framing_error;

	raw_buffer[10] = OneWire::crc8(raw_buffer, 9);

	uint8_t encoded_buffer[12];  //one extra to hold cobs data
	uint8_t encoded_size = COBSencode(raw_buffer, 11, encoded_buffer);
	Serial1.write(encoded_buffer, encoded_size);
	Serial1.write(0x00);

	if (packets_out_counter < 255){
		packets_out_counter++;
	}

}

void receivePacket(const uint8_t* buffer, size_t size)
{
	//format of packet is
	//H-0 S-0 V-0 H-1 S-1 V-1 INNER-INDEX-OFFSET INNER-MAGNITUDE OUTER-INDEX-OFFSET OUTER-MAGNITUDE
	//PACKET_INDEX FADE MODE CRC

	//Packet index can double as pulse beam

	//mode bits
	//0 & 1 are color mapping
	//4 is flow direction inner
	//5 could be pulsing on and off?


	//check for framing errors
	if (size != 14){
		framing_error++;
	}
	else{
		//check for crc errors
		byte crc = OneWire::crc8(buffer, size - 2);
		if (crc != buffer[size - 1]){
			crc_error++;
		}
		else{

			//increment packet stats counter
			if (packets_in_counter < 255){
				packets_in_counter++;
			}

			//sendPacket();

			color1 = CHSV(buffer[0], buffer[1], buffer[2]);
			color2 = CHSV(buffer[3], buffer[4], buffer[5]);

			inner_offset_requested = ((uint8_t)buffer[6]) % 30;
			outer_offset_requested = ((uint8_t)buffer[7]) % 30;

			inner_magnitude_requested = ((uint8_t)buffer[8]) % 17;
			outer_magnitude_requested = ((uint8_t)buffer[9]) % 17;

			fade_level = buffer[10];
			effect_mode = buffer[11];

			if (last_packet_sequence_number != buffer[12]){
				last_packet_sequence_number = buffer[12];
				increment_stream();
			}
		}
	}
}

void increment_stream(){
	//don't update the streams with incoming data if in mode 3 for smooth exit
	if (disc_mode != 3){
		//sender MUST change to std stream direction when disc enters mode 3 or exit from mode will look odd
		boolean invert_stream_direction = bitRead(effect_mode, 4);

		//adjust indexes based on direction
		invert_stream_direction ? stream_head++ : stream_head--;

		//wrap index
		stream_head = (stream_head + 16) % 16;


		Serial.print("loading ");
		Serial.print(stream_head);
		Serial.print(" h " );
		Serial.print(color1.h);
		Serial.print(" s ");
		Serial.print(color1.s);
		Serial.print(" v ");
		Serial.println(color1.v);

		//add to head or tail of streaming array based on direction
		if (invert_stream_direction){
			color1_streaming[stream_head] = color1;
			color2_streaming[stream_head] = color2;
		}
		else{
			uint8_t stream_tail = (stream_head + 15) % 16;
			color1_streaming[stream_tail] = color1;
			color2_streaming[stream_tail] = color2;

				
		}
	}
}

CHSV map_hsv(uint8_t input, uint8_t in_min, uint8_t in_max, CHSV* out_min, CHSV* out_max){
	return CHSV(
		(input - in_min) * (out_max->h - out_min->h) / (in_max - in_min) + out_min->h, \
		(input - in_min) * (out_max->s - out_min->s) / (in_max - in_min) + out_min->s, \
		(input - in_min) * (out_max->v - out_min->v) / (in_max - in_min) + out_min->v);
}



