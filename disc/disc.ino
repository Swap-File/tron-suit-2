
#include <OneWire.h> //crc8 
#include <cobs.h> //cobs encoder and decoder 

#include <OctoWS2811.h> //DMA strip output

#include <Metro.h> //timers

#include "FastLED.h" 
#include "hsv2rgb.h"

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"


// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

//#define TESTING

MPU6050 mpu;

const int config = WS2811_GRB | WS2811_800kHz;
const int ledsPerStrip = 30;
DMAMEM int displayMemory[ledsPerStrip * 6];
int drawingMemory[ledsPerStrip * 6];
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

uint8_t disc_mode = 0;

uint32_t inner_opening_time = 0;
uint32_t outer_opening_time = 0;
boolean opening_inner = false;
boolean opening_outer = false;
uint8_t opening_inner_index = 0;
uint8_t opening_outer_index = 0;


uint8_t saved_outer_index_acceleration = 0;
uint8_t saved_inner_index_acceleration = 0;

CHSV color1 = CHSV(128, 255, 255);
CHSV color2 = CHSV(128, 255, 255);

CHSV *color1_inner = &color1;
CHSV *color2_inner = &color2;
CHSV *color1_outer = &color1;
CHSV *color2_outer = &color2;


uint8_t inner_offset = 0;
uint8_t inner_magnitude = 16;
uint8_t outer_offset = 0;
uint8_t outer_magnitude = 16;

//serial com data
#define INCOMING_BUFFER_SIZE 128
uint8_t incoming_raw_buffer[INCOMING_BUFFER_SIZE];
uint8_t incoming_index = 0;
uint8_t incoming_decoded_buffer[INCOMING_BUFFER_SIZE];

Metro FPSdisplay = Metro(1000);

uint32_t idle_microseconds = 0;
uint32_t cooldown = 0;

uint8_t cpu_usage = 0;
uint8_t crc_error = 0;
uint8_t framing_error = 0;
//double temperature = 0;
uint8_t packets_in_counter = 0;  //counts up
uint8_t packets_in_per_second = 0; //saves the value
uint8_t packets_out_counter = 0;  //counts up
uint8_t packets_out_per_second = 0; //saves the value

double xy_accel_magnitude_smoothed = 0;
double saved_xy_accel_magnitude_smoothed = 0;

#define LED_PIN 13 
uint8_t blinkState = 0x00;


uint8_t reported_disc_outer_mode = 0;
uint8_t reported_disc_index = 0;
uint8_t reported_disc_width = 0;

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

uint16_t disc_voltage = 1024;

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
	mpuInterrupt = true;
}



// ================================================================
// ===                      INITIAL SETUP                       ===
// ================================================================

void setup() {

	//Inner LEDs are 0 to 29
	//Outer LEDs are 120 to 129


	// join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
	Wire.begin();
	TWBR = 24; // 400kHz I2C clock (200kHz if CPU is 8MHz)
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
	Fastwire::setup(400, true);
#endif

	Serial.begin(115200);
	Serial1.begin(115200);

	// initialize device
	Serial.println(F("Initializing I2C devices..."));
	mpu.initialize();

	// verify connection
	Serial.println(F("Testing device connections..."));
	Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

	// load and configure the DMP
	Serial.println(F("Initializing DMP..."));
	devStatus = mpu.dmpInitialize();

	// supply your own gyro offsets here, scaled for min sensitivity
	mpu.setXGyroOffset(109);
	mpu.setYGyroOffset(53);
	mpu.setZGyroOffset(54);
	mpu.setXAccelOffset(-1057); // 1688 factory default for my test chip
	mpu.setYAccelOffset(669); // 1688 factory default for my test chip
	mpu.setZAccelOffset(1482); // 1688 factory default for my test chip

	// make sure it worked (returns 0 if so)
	if (devStatus == 0) {
		// turn on the DMP, now that it's ready
		Serial.println(F("Enabling DMP..."));
		mpu.setDMPEnabled(true);

		// enable Arduino interrupt detection
		Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
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

	//battery meter pin, 0-1023  
	pinMode(23, INPUT);


	leds.begin();
	leds.setPixel(120 + 6, 0x0000ff);

}



// ================================================================
// ===                    MAIN PROGRAM LOOP                     ===
// ================================================================

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

			Serial.println(disc_mode);


		}
		// other program behavior stuff here
		// .
		// .
		// .
		// if you are really paranoid you can frequently test in between other
		// stuff to see if mpuInterrupt is true, and if so, "break;" from the
		// while() loop to immediately process the MPU data
		// .
		// .
		// .

		SerialUpdate();

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

		int inner_index = ((int)floor((temp + 0.5) + 30)) % 30;
		int outer_index = ((29 - inner_index) + 32) % 30;

		//instant accel mag
		double xy_accel_magnitude = sqrt((long)aaReal.x*aaReal.x + aaReal.y*aaReal.y);

		//delayed a bit...
		xy_accel_magnitude_smoothed = xy_accel_magnitude_smoothed * .8 + xy_accel_magnitude * .2;

		double temp2 = (atan2(aaReal.x, aaReal.y) * 4.77) + 15;
		int inner_index_acceleration = ((int)floor((temp2 + 0.5) + 30)) % 30;
		int outer_index_acceleration = ((29 - inner_index_acceleration) + 32) % 30;

	

		boolean cooldowncomplete = false;
		if (millis() - cooldown > 250){
			cooldowncomplete = true;
		}


		if (disc_mode == 0 && opening_outer == false){
			inner_magnitude = 0;
			outer_magnitude = 0;

			if (xy_accel_magnitude > 4000){
				outer_opening_time = millis();
				opening_outer = true;
				opening_outer_index = outer_index_acceleration;
				cooldown = millis();
			}
		}


		if (disc_mode == 1 && opening_inner == false && cooldowncomplete){

			inner_magnitude = 0;

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
				outer_magnitude = location;
			}
			else {
				opening_outer = false;
			}
		}


		if (opening_inner == true){
			inner_index = opening_inner_index;

			int location = map(millis() - inner_opening_time, 0, 500, 0, 16);

			if (location < 16){
				inner_magnitude = location;
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


			if (xy_accel_magnitude >5000 && cooldowncomplete){
				disc_mode = 3;
			}

		}



		if (disc_mode == 2){
			inner_magnitude = 16;
			outer_magnitude = 16;
		}


		if (disc_mode == 3){
	
			if (xy_accel_magnitude > 3000){
				uint8_t magnitude_temp = constrain(map(xy_accel_magnitude, 3000, 10000, 1, 10), 1, 10);

				inner_magnitude = magnitude_temp;
				outer_magnitude = magnitude_temp;

				if ((saved_inner_index_acceleration + 1) % 30 == inner_index_acceleration){
					color1_outer->hue = color1_outer->hue + 1;

				}
				if (saved_inner_index_acceleration  == (inner_index_acceleration + 1 ) % 30 ){
					color1_outer->hue = color1_outer->hue - 1;

				}
				saved_inner_index_acceleration = inner_index_acceleration;
				saved_outer_index_acceleration = outer_index_acceleration;
				saved_xy_accel_magnitude_smoothed = xy_accel_magnitude_smoothed ;
			}
			else{
				uint8_t magnitude_temp = constrain(map(saved_xy_accel_magnitude_smoothed, 3000, 10000, 1, 10), 1, 10);

				inner_magnitude = magnitude_temp;
				outer_magnitude = magnitude_temp;
			}


			inner_index = saved_inner_index_acceleration;
			outer_index = saved_outer_index_acceleration;

		}


		//906 is low when plugged into batteries, about 3.55 v per cell
		disc_voltage = disc_voltage * .97 + analogRead(23) * .03;

		//Serial.println(disc_voltage);

		CHSV tempcolor = CHSV(0, 5, 0);

		byte raw_buffer[35];

		raw_buffer[0] = tempcolor.h;
		raw_buffer[1] = tempcolor.s;
		raw_buffer[2] = tempcolor.v;

		raw_buffer[3] = reported_disc_outer_mode;
		raw_buffer[4] = reported_disc_index;
		raw_buffer[5] = reported_disc_width;
		raw_buffer[6] = OneWire::crc8(raw_buffer, 5);
		uint8_t encoded_buffer[8];  //one extra to hold cobs data
		uint8_t encoded_size = COBSencode(raw_buffer, 7, encoded_buffer);
		Serial1.write(encoded_buffer, encoded_size);
		Serial1.write(0x00);

		// blink LED to indicate activity
		blinkState++;
		digitalWrite(LED_PIN, bitRead(blinkState, 2));



		for (int i = 0; i < 30 * 8; i++){
			leds.setPixel(i, 0);
		}


		leds.setPixel(120 + outer_index, 0x00FFFF);
		leds.setPixel(inner_index, 0x00FFFF);

		for (int imag = 0; imag <= 16; imag++) {
			//set outer LEDs
			if (imag <= outer_magnitude && imag > 0){
				//set pixels

				//calulate crossfade for multicolor action
				//first pixel must be color 1!
				CHSV temp;

				temp.h = map(imag, 0, 16, color1_outer->h, color2_outer->h);
				temp.s = map(imag, 0, 16, color1_outer->s, color2_outer->s);
				temp.v = map(imag, 0, 16, color1_outer->v, color2_outer->v);

				uint32_t tempagain = HSV_to_RGB(temp);

				int i;
				i = (outer_index + imag - 1 + 60) % 30;
				leds.setPixel(120 + i, tempagain);
				i = (outer_index - imag + 1 + 60) % 30;
				leds.setPixel(120 + i, tempagain);
			}
			else{
				//clear unused pixels
				int i;
				i = (outer_index + imag - 1 + 60) % 30;
				leds.setPixel(120 + i, 0);
				i = (outer_index - imag + 1 + 60) % 30;
				leds.setPixel(120 + i, 0);
			}


			//set inner LEDs
			if (imag <= inner_magnitude && imag > 0){
				//set pixels

				//calulate crossfade for multicolor action
				//first pixel must be color 1!
				CHSV temp;

				temp.h = map(imag, 0, 16, color1_inner->h, color2_inner->h);
				temp.s = map(imag, 0, 16, color1_inner->s, color2_inner->s);
				temp.v = map(imag, 0, 16, color1_inner->v, color2_inner->v);

				uint32_t tempagain = HSV_to_RGB(temp);

				int i;
				i = (inner_index + imag - 1 + 60) % 30;
				leds.setPixel(i, tempagain);
				i = (inner_index - imag + 1 + 60) % 30;
				leds.setPixel(i, tempagain);
			}
			else{
				//clear unused pixels
				int i;
				i = (inner_index + imag - 1 + 60) % 30;
				leds.setPixel(i, 0);
				i = (inner_index - imag + 1 + 60) % 30;
				leds.setPixel(i, 0);
			}
		}


		//double check if the LEDs are busy, but that should never happen at 100hz
		if (!leds.busy()){
			leds.show();
		}


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

				onPacket(incoming_decoded_buffer, decoded_length);
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



void onPacket(const uint8_t* buffer, size_t size)
{

	//format of packet is
	//H-0 S-0 V-0 H-1 S-1 V-1 INNER-INDEX INNER-WIDTH OUTER-INDEX OUTER-WIDTH CRC

	//check for framing errors
	if (size != 11){
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






		}
	}
}

uint32_t HSV_to_RGB(CHSV hsv_input){
	CRGB temp_rgb;
	hsv2rgb_rainbow(hsv_input, temp_rgb);
	return((temp_rgb.red << 16) | (temp_rgb.green << 8) | temp_rgb.blue);
}
