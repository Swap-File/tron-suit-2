
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#include <OneWire.h> //crc8 
#include <cobs.h> //cobs encoder and decoder 
#include <OctoWS2811.h> //DMA strip output
#include <Metro.h> //timers

typedef struct {
	boolean fresh;

	int16_t yaw_raw;  //yaw pitch and roll in degrees * 1000
	int16_t pitch_raw;
	int16_t roll_raw;

	int16_t yaw_offset;  //yaw pitch and roll in degrees * 1000
	int16_t pitch_offset;
	int16_t roll_offset;

	int16_t yaw_compensated;  //yaw pitch and roll in degrees * 1000
	int16_t pitch_compensated;
	int16_t roll_compensated;

	int16_t aaRealX; // gravity-free accel sensor measurements
	int16_t aaRealY;
	int16_t aaRealZ;

	int16_t aaWorldX; // world-frame accel sensor measurements
	int16_t aaWorldY;
	int16_t aaWorldZ;

	int16_t red; // world-frame accel sensor measurements
	int16_t green;
	int16_t blue;
	int16_t clear;

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

} GLOVE;

typedef struct {
	uint8_t packets_in_per_second;
	uint8_t packets_out_per_second;

	uint8_t framing_errors;
	uint8_t crc_errors;

} SERIALSTATS;

const int ledsPerStrip = 128;

DMAMEM int displayMemory[ledsPerStrip * 6];
int drawingMemory[ledsPerStrip * 6];

const int config = WS2811_GRB | WS2811_800kHz;

//crank up hardwareserial.cpp to 128 to match!
#define INCOMING1_BUFFER_SIZE 128
uint8_t incoming1_raw_buffer[INCOMING1_BUFFER_SIZE];
uint8_t incoming1_index = 0;
uint8_t incoming1_decoded_buffer[INCOMING1_BUFFER_SIZE];

AudioInputAnalog         adc1(A4);
AudioAnalyzeFFT256       fft256_1;
AudioConnection          patchCord1(adc1, fft256_1);

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

#define TESTING

unsigned long fps_time = 0;

GLOVE glove0;
GLOVE glove1;
SERIALSTATS serial1stats;

Metro FPSdisplay = Metro(1000);

Metro YPRdisplay = Metro(100);

void setup() {


	glove0.fresh = false;
	glove1.fresh = false;
	glove1.yaw_offset = 0;
	glove1.pitch_offset = 0;
	glove1.roll_offset = 0;
	glove0.yaw_offset = 0;
	glove0.pitch_offset = 0;
	glove0.roll_offset = 0;

	//must go first so Serial.begin can override pins!!!
	leds.begin();

	//bump hardwareserial.cpp to 255
	Serial.begin(115200);   //USB Debug
	Serial1.begin(115200);  //Gloves	
	Serial2.begin(115200);  //Bluetooth	
	Serial3.begin(115200);  //Xbee	

	pinMode(A4, INPUT); //audio input

	AudioMemory(5);
	fft256_1.windowFunction(AudioWindowHanning256);
	fft256_1.averageTogether(4);

	leds.show();
}

long int magictime = 0;
int ledmodifier = 1;
int indexled = 0;

void loop() {

	if (Serial2.available()){
		Serial2.write(Serial2.read());
		indexled = 0;
	}

	if (Serial3.available()){
		Serial3.write(Serial3.read());
		indexled = 0;
	}

	if (glove1.finger1==0){
		glove1.yaw_offset = glove1.yaw_raw;
		glove1.pitch_offset = glove1.pitch_raw;
		glove1.roll_offset = glove1.roll_raw;

		glove0.yaw_offset = glove0.yaw_raw;
		glove0.pitch_offset = glove0.pitch_raw;
		glove0.roll_offset = glove0.roll_raw;
	}

	if (YPRdisplay.check()){
		Serial.print("ypr\t");
		Serial.print(glove1.finger1);
		Serial.print("\t");
		Serial.print(glove1.yaw_compensated);
		Serial.print("\t");
		Serial.print(glove1.pitch_compensated);
		Serial.print("\t");
		Serial.print(glove1.roll_compensated);
		Serial.print("\t");
		Serial.print(glove0.yaw_compensated);
		Serial.print("\t");
		Serial.print(glove0.pitch_compensated);
		Serial.print("\t");
		Serial.println(glove0.roll_compensated);
	}

	if (FPSdisplay.check()){
	
		leds.setPixel(indexled, 0xff0000);
		indexled = ledmodifier + indexled;
		if (indexled > 127 || indexled < 0){
			ledmodifier = ledmodifier* -1;
			indexled = ledmodifier + indexled;
		}

		leds.show();

		//cpu_usage = 100 - (idle_microseconds / 10000);
		//local_packets_out_per_second_1 = local_packets_out_counter_1;
		//local_packets_in_per_second_1 = local_packets_in_counter_1;
		//idle_microseconds = 0;
		// local_packets_out_counter_1 = 0;
		// local_packets_in_counter_1 = 0;
		// Serial.print(i2cpackets);

		uint8_t raw_buffer[9];

		raw_buffer[0] = 0x00;
		raw_buffer[1] = 0x00;
		raw_buffer[2] = 0xff;
		raw_buffer[3] = 0x00;
		raw_buffer[4] = 0x00;
		raw_buffer[5] = 0xFF;
		raw_buffer[6] = 0x00;
		raw_buffer[7] = 0x00;
		raw_buffer[8] = OneWire::crc8(raw_buffer, 7);

		uint8_t encoded_buffer[9];
		uint8_t encoded_size = COBSencode(raw_buffer, 9, encoded_buffer);


		Serial1.write(encoded_buffer, encoded_size);
		Serial1.write(0x00);

		if (serial1stats.packets_out_per_second < 255){
			serial1stats.packets_out_per_second++;
		}

	}

	float n;
	int i;

	if (fft256_1.available()) {
		// each time new FFT data is available
		// print it all to the Arduino Serial Monitor
		//Serial.print("FFT: ");
		//for (i = 0; i < 40; i++) {
		//n = fft256_1.read(i);
		//if (n >= 0.01) {
		//	Serial.print(n);
		//	Serial.print(" ");
		//	}
		//	else {
		//		Serial.print("  -  "); // don't print "0.00"
		//	}
		//	}
		//	Serial.println();
	}

	SerialUpdate();
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

			current_glove->yaw_raw =  buffer[0] << 8 | buffer[1];
			current_glove->pitch_raw = buffer[2] << 8 | buffer[3];
			current_glove->roll_raw = buffer[4] << 8 | buffer[5];

			current_glove->yaw_compensated = current_glove->yaw_offset - current_glove->yaw_raw;
			current_glove->pitch_compensated = current_glove->pitch_offset - current_glove->pitch_raw;
			current_glove->roll_compensated = current_glove->roll_offset - current_glove->roll_raw;

			current_glove->aaRealX = buffer[6] << 8 | buffer[7];
			current_glove->aaRealY = buffer[8] << 8 | buffer[9];
			current_glove->aaRealZ = buffer[10] << 8 | buffer[11];

			current_glove->aaWorldX = buffer[12] << 8 | buffer[13];
			current_glove->aaWorldY = buffer[14] << 8 | buffer[15];
			current_glove->aaWorldZ = buffer[16] << 8 | buffer[17];

			current_glove->red = buffer[18] << 8 | buffer[19];
			current_glove->green = buffer[20] << 8 | buffer[21];
			current_glove->blue = buffer[22] << 8 | buffer[23];
			current_glove->clear = buffer[24] << 8 | buffer[25];

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


			
	

		}
	}
}

