
#include <Audio.h>
#include <i2c_t3.h>
#include <SPI.h>
#include <SD.h>

#include <OneWire.h>
#include <PacketSerial.h>


// Function prototypes
void receiveEvent(size_t len);
void requestEvent(void);

AudioInputAnalog         adc1(A0);         
AudioAnalyzeFFT256       fft256_1;     
AudioConnection          patchCord1(adc1, fft256_1);

#define TESTING

volatile boolean reseti2c = false;


PacketSerial serial1;


unsigned long fps_time = 0;


void setup() {
  // wait for ready
  Serial.begin(115200);


  pinMode(A0,INPUT);
  pinMode(2,INPUT_PULLUP);
  AudioMemory(4);
  fft256_1.windowFunction(AudioWindowHanning256);
  fft256_1.averageTogether(4);

  pinMode(A0,INPUT);

  serial1.setPacketHandler(&onPacket1);
  serial1.begin(115200,1);
  //delay(10000);
  

}


void loop() {

  if (micros() - fps_time > 1000000){
    //cpu_usage = 100 - (idle_microseconds / 10000);
    //local_packets_out_per_second_1 = local_packets_out_counter_1;
   //local_packets_in_per_second_1 = local_packets_in_counter_1;
    //idle_microseconds = 0;
   // local_packets_out_counter_1 = 0;
   // local_packets_in_counter_1 = 0;
   // Serial.print(i2cpackets);
     byte buffer[10];
  buffer[0] = 0x00;
  buffer[1] = 0x00;
  buffer[2] = 0xff;
  buffer[3] = 0x00;
  buffer[4] = 0x00;
  buffer[5] = 0xFF;
  buffer[6] = 0x00;
  buffer[7] = 0x00;
  buffer[8] = OneWire::crc8(buffer, 7);
  serial1.send(buffer,9);
  
    Serial.println(" bong");
  //  i2cpackets = 0;
    fps_time = micros();
  }



  uint16_t n;
  int i;

  if (fft256_1.available()) {

  }

  serial1.update(); 
}







void onPacket1(const uint8_t* buffer, size_t size)
{
 // if (local_packets_in_counter_1 < 255){
 //   local_packets_in_counter_1++;
 // }

  if (size == 35){
    byte crc = OneWire::crc8(buffer, size-2);
    if (crc != buffer[size-1]){
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

