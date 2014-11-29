
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>


#include <OneWire.h> //crc8 
#include <cobs.h> //cobs encoder and decoder 
#include <OctoWS2811.h> //DMA strip output

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

void setup() {
	// wait for ready
	Serial.begin(115200);
	Serial1.begin(115200);  //gloves	


	//audio input
	pinMode(A4, INPUT);

	AudioMemory(12);
	fft256_1.windowFunction(AudioWindowHanning256);
	fft256_1.averageTogether(4);


	leds.begin();
	leds.show();
}

int ledmodifier = 1;
int indexled = 0;

void loop() {

	if (micros() - fps_time > 1000000){

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


		Serial.println(" bong");
		//  i2cpackets = 0;
		fps_time = micros();
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
	// if (local_packets_in_counter_1 < 255){
	//   local_packets_in_counter_1++;
	// }

	if (size == 35){
		uint8_t crc = OneWire::crc8(buffer, size - 2);
		if (crc != buffer[size - 1]){
			//local_crc_error_1++;
		}
		else{

			Serial.print(buffer[33]);
			Serial.print(" ");
			Serial.print(buffer[31]);
			Serial.print(" ");
			Serial.println(buffer[32]);


			// if (local_packets_out_counter_1 < 255){
			//  local_packets_out_counter_1++;
			// }

		}
	}
}

