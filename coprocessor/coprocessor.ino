#include <OneWire.h>
#include <PacketSerial.h>
#include <helper_3dmath.h> 
#include "I2Cdev.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

#define TESTING
PacketSerial serial1;

uint8_t local_crc_error_1 = 0;
uint8_t local_framing_error_1 = 0;
uint8_t local_packets_out_counter_1 = 0;
uint8_t local_packets_in_counter_1 = 0;
uint8_t local_packets_out_per_second_1 = 0;
uint8_t local_packets_in_per_second_1 = 0;


VectorInt16 aaReal_1;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld_1;    // [x, y, z]            world-frame accel sensor measurements
int16_t  ypr_1[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
uint8_t  remote_crc_error_1 = 0;
uint8_t  remote_framing_error_1 = 0;
byte fingers_1 = 0x00; 
uint8_t whiteled_1 = 0xff;
int16_t rgb_sample_1[3];  //color data from sensor
double rgb_sample_average_1[3] = {0,0,0};

uint8_t rgb_output_1[3];  //color data to strip
uint8_t temperature_1 = 0; // in centigrade
uint8_t remote_packets_out_per_second_1 = 0;
uint8_t remote_packets_in_per_second_1 = 0;

uint8_t cpu_usage_1 = 0;  //in percentage

unsigned long fps_time = 0;

void setup() {
  // wait for ready
  Serial.begin(115200);

  Serial.println("get");
  // set the slaveSelectPin as an output:
  pinMode(2,INPUT);

  serial1.setPacketHandler(&onPacket1);
  serial1.begin(115200,1);
}

void loop() {

  if (micros() - fps_time > 1000000){
    //cpu_usage = 100 - (idle_microseconds / 10000);
    local_packets_out_per_second_1 = local_packets_out_counter_1;
    local_packets_in_per_second_1 = local_packets_in_counter_1;
    //idle_microseconds = 0;
    local_packets_out_counter_1 = 0;
    local_packets_in_counter_1 = 0;

    fps_time = micros();
  }



  serial1.update(); 
}

void onPacket1(const uint8_t* buffer, size_t size)
{
  if (local_packets_in_counter_1 < 255){
    local_packets_in_counter_1++;
  }

  if (size != 34){
    local_framing_error_1++;
  }
  else{

    byte crc = OneWire::crc8(buffer, size-2);
    if (crc != buffer[size-1]){
      local_crc_error_1++;
    }
    else{

      ypr_1[0] = buffer[0] << 8 | buffer[1];
      ypr_1[1] = buffer[2] << 8 | buffer[3];
      ypr_1[2] = buffer[4] << 8 | buffer[5];

      aaReal_1.x = buffer[6] << 8 | buffer[7];
      aaReal_1.y = buffer[8] << 8 | buffer[9];
      aaReal_1.z = buffer[10] << 8 | buffer[11];

      aaWorld_1.x = buffer[12] << 8 | buffer[13];
      aaWorld_1.y = buffer[14] << 8 | buffer[15];
      aaWorld_1.z = buffer[16] << 8 | buffer[17];

      rgb_sample_1[0]= buffer[18] << 8 | buffer[19];
      rgb_sample_1[1]= buffer[20] << 8 | buffer[21];
      rgb_sample_1[2]= buffer[22] << 8 | buffer[23];
      rgb_sample_1[3]= buffer[24] << 8 | buffer[25];
      

      fingers_1 =  buffer[26]; 

      remote_packets_in_per_second_1 =  buffer[27]; 
      remote_packets_out_per_second_1 =  buffer[28];

      remote_framing_error_1 =  buffer[29]; 
      remote_crc_error_1 =  buffer[30];

      cpu_usage_1 = buffer[31];

      temperature_1 = buffer[32];

#ifdef TESTING
      Serial.print(remote_packets_in_per_second_1);
      Serial.print(" ");
      Serial.print(remote_packets_out_per_second_1);
      Serial.print(" ");
      Serial.print(temperature_1);
      Serial.print(" ");
      Serial.println(cpu_usage_1);
      Serial.print("ypr\t");
      Serial.print(ypr_1[0]);
      Serial.print("\t");
      Serial.print(ypr_1[1]);
      Serial.print("\t");
      Serial.println(ypr_1[2]);


      Serial.print("areal\t");
      Serial.print(aaReal_1.x);
      Serial.print("\t");
      Serial.print(aaReal_1.y);
      Serial.print("\t");
      Serial.println(aaReal_1.z);


      Serial.print("aworld\t");
      Serial.print(aaWorld_1.x);
      Serial.print("\t");
      Serial.print(aaWorld_1.y);
      Serial.print("\t");
      Serial.println(aaWorld_1.z);
      

       
       
       
#endif


Serial.print("rgb\t");


 uint32_t sum = rgb_sample_1[3];
  float r = 0, g = 0, b =0 ;
  if ((digitalRead(2) && sum > 2000) || (digitalRead(2) == 0 && sum > 1000)){
  r = rgb_sample_1[0]; r /= sum;
  g = rgb_sample_1[1]; g /= sum;
  b = rgb_sample_1[2]; b /= sum;
  r *= 256; g *= 256; b *= 256;
  }
  Serial.print("\t");
  Serial.print((int)r, HEX); Serial.print((int)g, HEX); Serial.print((int)b, HEX);

      
      
      
      byte temp[32];
      
      
      temp[0] = (byte)r;
      temp[1] = (byte)g;
      temp[2] = (byte)b;
      
      temp[3] = digitalRead(2);



      temp[4] = OneWire::crc8(temp, 3);
      
           
      
      serial1.send(temp, 5);
      if (local_packets_out_counter_1 < 255){
        local_packets_out_counter_1++;
      }

    }
  }
}












