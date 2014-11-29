// I2C device class (I2Cdev) demonstration Arduino sketch for MPU6050 class using DMP (MotionApps v2.0)
//#define DEBUG_PRINT
// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"

// This library was 50% faster than FastSPI_LED 3.0 in the case of a single LED (50-60 microseconds versus 120-130)
#include <Adafruit_NeoPixel.h> 

//Just using it for CRC8!
#include <OneWire.h>

#include <cobs.h>

//#define TESTING

#include "Adafruit_TCS34725.h"

// Which pin on the Arduino is connected to the NeoPixels?
#define NEOPIXELS_PIN    9

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      1

// set to false if using a common cathode LED
#define commonAnode false

// our RGB -> eye-recognized gamma color
byte gammatable[256];

#include "MPU6050_6Axis_MotionApps20.h"
//#include "MPU6050.h" // not necessary if using MotionApps include file

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for SparkFun breakout and InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 mpu;
//MPU6050 mpu(0x69); // <-- use for AD0 high
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXELS_PIN, NEO_GRB + NEO_KHZ800);



Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);


#define COLOR_SENSOR_WHITE_LED            11
#define LED_PIN 13 // (Arduino is 13, Teensy is 11, Teensy++ is 6)
bool blinkState = false;

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
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

uint8_t gloveid = 0xff;  //nonsense value

//serial com data
#define INCOMING_BUFFER_SIZE 128
uint8_t incoming_raw_buffer[INCOMING_BUFFER_SIZE];
uint8_t incoming_index = 0;
uint8_t incoming_decoded_buffer[INCOMING_BUFFER_SIZE];

//buffer to hold prepped data
uint8_t encoded_buffer[INCOMING_BUFFER_SIZE]; //must be one byte longer
uint8_t encoded_size = 0;

uint16_t  rgb_sample[4];  //color data from sensor
uint8_t fingers = 0x00; 
uint8_t crc_error = 0;
uint8_t framing_error = 0;
double temperature = 0;
unsigned long fps_time = 0; //keeps track of when the last cycle was
uint8_t packets_in_counter = 0;  //counts up
uint8_t packets_in_per_second = 0; //saves the value
uint8_t packets_out_counter = 0;  //counts up
uint8_t packets_out_per_second = 0; //saves the value

uint32_t idle_microseconds = 0;
uint8_t cpu_usage = 0;
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

  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setPixelColor(0, pixels.Color(127, 0, 0)); // Moderately bright red color.
  pixels.show(); // This sends the updated pixel color to the hardware.



  //start the color sensor
  //Serial.println(F("Initializing TC230 Color Sensor..."));

  if (tcs.begin()) {
    //Serial.println(F("Initializing TCS34725 Color Sensor..."));
  }
  else {
    pixels.setPixelColor(0, pixels.Color(0, 127, 0)); // Moderately bright red color.
    //Serial.println(F("No TCS34725 Color Sensor Found..."));
    while (1); // halt!
  }



  //Serial.println(F("Joining I2C Bus..."));
  // join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
  Wire.begin();
  TWBR = 24; // 400kHz I2C clock (200kHz if CPU is 8MHz)
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  Fastwire::setup(400, true);
#endif

  //Serial.println(F("Initializing Serial Port..."));
  //COBS framing serial packet handler 
  //serial.setPacketHandler(&onPacket);
  //serial.begin(115200);

  Serial.begin(115200);

  // initialize device
  //Serial.println(F("Initializing MPU6050 devices..."));
  mpu.initialize();


  // thanks PhilB for this gamma table!
  // it helps convert RGB colors to what humans see
  for (int i = 0; i < 256; i++) {
    float x = i;
    x /= 255;
    x = pow(x, 2.5);
    x *= 255;

    if (commonAnode) {
      gammatable[i] = 255 - x;
    }
    else {
      gammatable[i] = x;
    }
    //Serial.println(gammatable[i]);
  }

  tcs.setInterrupt(true);

  // verify connection
  //Serial.println(F("Testing MPU6050 connection..."));
  //Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

  // wait for ready
  //Serial.println(F("\nSend any character to begin DMP programming and demo: "));
  // while (Serial.available() && Serial.read()); // empty buffer
  // while (!Serial.available());                 // wait for data
  // while (Serial.available() && Serial.read()); // empty buffer again

  // load and configure the DMP
  //Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  // supply your own gyro offsets here, scaled for min sensitivity
  mpu.setXGyroOffset(220);
  mpu.setYGyroOffset(76);
  mpu.setZGyroOffset(-85);
  mpu.setZAccelOffset(1788); // 1688 factory default for my test chip

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // turn on the DMP, now that it's ready
    //Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);

    //Not using Interrupts for IMU, let the FIFO buffer handle it and poll
    //Color sensor interrupts are more important.
    // enable Arduino interrupt detection
    //Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
    attachInterrupt(0, dmpDataReady, RISING);

    mpuIntStatus = mpu.getIntStatus();

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    //Serial.println(F("DMP ready! Waiting for first interrupt..."));
    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  }
  else {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    //Serial.print(F("DMP Initialization failed (code "));
    //Serial.print(devStatus);
    //Serial.println(F(")"));
  }

  pixels.setPixelColor(0, pixels.Color(0, 0, 0)); // 
  pixels.show();

  // configure LED for output

  pinMode(LED_PIN, OUTPUT);  //indiator LED, onboard yellow
  pinMode(10, INPUT_PULLUP);  //glovemode
   pinMode(4, INPUT);
  pinMode(5, INPUT);
  pinMode(6, INPUT);
  pinMode(7, INPUT);
  if (digitalRead(10)){
    gloveid = 0;
  }
  else{
    gloveid = 1;
  }
  GetTempPrep();

 
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


    // other program behavior stuff here
    if (micros() - fps_time > 1000000){
      cpu_usage = 100 - (idle_microseconds / 10000);
      packets_in_per_second = packets_in_counter;
      packets_out_per_second = packets_out_counter;
      idle_microseconds = 0;
      packets_in_counter = 0;
      packets_out_counter = 0;
      fps_time = micros();
    }

    fingers = 0x00; //reset each loop
    if (digitalRead(4)){ 
      bitSet(fingers, 0); 
    }
    if (digitalRead(5)){ 
      bitSet(fingers, 1); 
    }
    if (digitalRead(6)){ 
      bitSet(fingers, 2); 
    }
    if (digitalRead(7)){ 
      bitSet(fingers, 3); 
    }

    SerialUpdate();

    //dont count first cycle, its not idle time, its required
    if (idle_start_timer == 0){
      idle_start_timer = micros();
    }
  }

  idle_microseconds = idle_microseconds + (micros() - idle_start_timer);


  GetTempPrep();

  // reset interrupt flag and get INT_STATUS byte
  mpuInterrupt = false;
  mpuIntStatus = mpu.getIntStatus();

  // get current FIFO count
  fifoCount = mpu.getFIFOCount();

  // check for overflow (this should never happen unless our code is too inefficient)
  if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
    // reset so we can continue cleanly
    mpu.resetFIFO();
    //Serial.println(F("FIFO overflow!"));

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
    mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);



#ifdef TESTING
    Serial.print("ypr\t");
    Serial.print(ypr[0] * 180 / M_PI);
    Serial.print("\t");
    Serial.print(ypr[1] * 180 / M_PI);
    Serial.print("\t");
    Serial.println(ypr[2] * 180 / M_PI);


    Serial.print("areal\t");
    Serial.print(aaReal.x);
    Serial.print("\t");
    Serial.print(aaReal.y);
    Serial.print("\t");
    Serial.println(aaReal.z);


    Serial.print("aworld\t");
    Serial.print(aaWorld.x);
    Serial.print("\t");
    Serial.print(aaWorld.y);
    Serial.print("\t");
    Serial.println(aaWorld.z);


    Serial.print("Temp: ");
    Serial.println(GetTemp(), 1);
    Serial.println(sizeof(GetTemp()));

#endif


    byte raw_buffer[35];

    raw_buffer[0] = ((((int16_t)(ypr[0] * 18000 / M_PI)) >> 8) & 0xff);
    raw_buffer[1] = ((((int16_t)(ypr[0] * 18000 / M_PI)) >> 0) & 0xff);
    raw_buffer[2] = ((((int16_t)(ypr[1] * 18000 / M_PI)) >> 8) & 0xff);
    raw_buffer[3] = ((((int16_t)(ypr[1] * 18000 / M_PI)) >> 0) & 0xff);
    raw_buffer[4] = ((((int16_t)(ypr[2] * 18000 / M_PI)) >> 8) & 0xff);
    raw_buffer[5] = ((((int16_t)(ypr[2] * 18000 / M_PI)) >> 0) & 0xff);

    raw_buffer[6] = ((aaReal.x >> 8) & 0xff);
    raw_buffer[7] = ((aaReal.x >> 0) & 0xff);
    raw_buffer[8] = ((aaReal.y >> 8) & 0xff);
    raw_buffer[9] = ((aaReal.y >> 0) & 0xff);
    raw_buffer[10] = ((aaReal.z >> 8) & 0xff);
    raw_buffer[11] = ((aaReal.z >> 0) & 0xff);

    raw_buffer[12] = ((aaWorld.x >> 8) & 0xff);
    raw_buffer[13] = ((aaWorld.x >> 0) & 0xff);
    raw_buffer[14] = ((aaWorld.y >> 8) & 0xff);
    raw_buffer[15] = ((aaWorld.y >> 0) & 0xff);
    raw_buffer[16] = ((aaWorld.z >> 8) & 0xff);
    raw_buffer[17] = ((aaWorld.z >> 0) & 0xff);


    tcs.getRawData(&rgb_sample[0], &rgb_sample[1], &rgb_sample[2], &rgb_sample[3]);  //rgbclear

    raw_buffer[18] = ((rgb_sample[0] >> 8) & 0xff);
    raw_buffer[19] = ((rgb_sample[0] >> 0) & 0xff);
    raw_buffer[20] = ((rgb_sample[1] >> 8) & 0xff);
    raw_buffer[21] = ((rgb_sample[1] >> 0) & 0xff);
    raw_buffer[22] = ((rgb_sample[2] >> 8) & 0xff);
    raw_buffer[23] = ((rgb_sample[2] >> 0) & 0xff);
    raw_buffer[24] = ((rgb_sample[3] >> 8) & 0xff);
    raw_buffer[25] = ((rgb_sample[3] >> 0) & 0xff);

    fingers = 0x00; //reset each loop
    if (digitalRead(4)){ 
      bitSet(fingers, 0); 
    }
    if (digitalRead(5)){ 
      bitSet(fingers, 1); 
    }
    if (digitalRead(6)){ 
      bitSet(fingers, 2); 
    }
    if (digitalRead(7)){ 
      bitSet(fingers, 3); 
    }

    raw_buffer[26] = fingers;

    raw_buffer[27] = packets_in_per_second;
    raw_buffer[28] = packets_out_per_second;

    raw_buffer[29] = framing_error;
    raw_buffer[30] = crc_error;

    raw_buffer[31] = cpu_usage;

    raw_buffer[32] = ((((int16_t)(temperature)) >> 0) & 0xff);

    raw_buffer[33] = gloveid;

    raw_buffer[34] = OneWire::crc8(raw_buffer, 33);

    //prep buffer completely
    encoded_size = COBSencode(raw_buffer, 35, encoded_buffer);


    // blink LED to indicate activity
    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState);

    SerialUpdate();
    //do temp calculation last, give it as much time as possible to settle

      temperature = temperature * .95 + .05 * GetTemp();
  }
}

void onPacket(const uint8_t* buffer, size_t size)
{

  //format of packet is
  //R-0 G-0 B-0 COLOR-0 R-1 G-1 B-1 COLOR-1 CRC

  //check for framing errors
  if (size != 9 && size != 35){
    framing_error++;
  }
  else{
    //check for crc errors
    byte crc = OneWire::crc8(buffer, size - 2);
    if (crc != buffer[size - 1]){
      crc_error++;
    }
    else{

      if (size == 9){
        //increment packet stats counter
        if (packets_in_counter < 255){
          packets_in_counter++;
        }

        //detect which glove we are
        uint8_t offset = 0;  
        if (gloveid == 0){
          Serial.write(incoming_raw_buffer, incoming_index);
          Serial.write(0x00);
        }
        else{
          offset = 4;
        }

        pixels.setPixelColor(0, pixels.Color(gammatable[(int)buffer[0 + offset]], gammatable[(int)buffer[1 + offset]], gammatable[(int)buffer[2 + offset]]));
        pixels.show();

        //set white sensor light
        if (buffer[3 + offset] == 0x00){
          tcs.setInterrupt(true);    //LED OFF
        }
        else{
          tcs.setInterrupt(false);    //LED ON
        }

        //send out data from last cycle
        Serial.write(encoded_buffer, encoded_size);
        Serial.write(0x00);

        if (packets_out_counter < 255){
          packets_out_counter++;
        }

      }
      //pass buffer on if we get sent one
      else if (size == 35){
        Serial.write(incoming_raw_buffer, incoming_index);
        Serial.write(0x00);
      }
    }
  }
}


void GetTempPrep(void)
{

  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC

  //dont have time to wait.  gotta go fast.
  //delay(20);            // wait for voltages to become stable.
  //put this prep function before code that will take a while
  //the IMU Stuff takes about 4.5ms, not quite 20, but it seems to work well enough
}

double GetTemp(void)
{
  unsigned int wADC;
  double t;

  // The internal temperature has to be used
  // with the internal reference of 1.1V.
  // Channel 8 can not be selected with
  // the analogRead function yet.


  ADCSRA |= _BV(ADSC);  // Start the ADC

    // Detect end-of-conversion
  while (bit_is_set(ADCSRA, ADSC));

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;

  // The offset of 324.31 could be wrong. It is just an indication.
  t = (wADC - 324.31) / 1.22;

  // The returned temperature is in degrees Celcius.
  return (t);
}


void SerialUpdate(void){
  while (Serial.available()){

    //read in a byte
    incoming_raw_buffer[incoming_index] = Serial.read();

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



