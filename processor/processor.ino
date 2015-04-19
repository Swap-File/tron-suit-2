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
#define HAND_DIRECTION_LEFT 1
#define HAND_DIRECTION_RIGHT 2
#define HAND_DIRECTION_UP 3
#define HAND_DIRECTION_DOWN 4
#define HAND_DIRECTION_SHORT_PRESS 5

#define FFT_MODE_HORZ_BARS_LEFT 0
#define FFT_MODE_HORZ_BARS_RIGHT 1
#define FFT_MODE_HORZ_BARS_STATIC 2
#define FFT_MODE_VERT_BARS_UP 3
#define FFT_MODE_VERT_BARS_DOWN 4
#define FFT_MODE_VERT_BARS_STATIC 5
#define FFT_MODE_OFF 6
uint8_t fftmode = FFT_MODE_OFF;

#define MENU_DEFAULT 1
#define MENU_FFT_H_or_V 2
#define MENU_FFT_H 3
#define MENU_FFT_V 4
#define MENU_FFT 5
#define MENU_SUIT 6
#define MENU_DISC 7
#define MENU_MAG 8
#define MENU_SPIN 9
uint8_t menu_mode = MENU_DEFAULT;

boolean flow_direction_positive = true;

uint32_t total_packets_in = 0;
uint32_t total_packets_out = 0;

typedef struct {
	boolean gesture_in_progress = false;
	uint8_t gesture_finger;

	int16_t yaw_raw;  //yaw pitch and roll in degrees * 100
	int16_t pitch_raw;
	int16_t roll_raw;

	int32_t yaw_offset = 0;  //yaw pitch and roll in degrees * 100
	int32_t pitch_offset = 0;
	int32_t roll_offset = 0;

	int32_t yaw_compensated;  //yaw pitch and roll in degrees * 100
	int32_t pitch_compensated;
	int32_t roll_compensated;

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
	int8_t calculated_mag;
	uint8_t disc_offset; // 0 to 29
	int16_t magnitude; //about 3000 is edges

	uint32_t finger_timer; //differentiate between short and long presses

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

	uint8_t inner_magnitude_requested = 16;
	uint8_t outer_magnitude_requested = 16;
	uint8_t inner_magnitude_reported = 0;
	uint8_t outer_magnitude_reported = 0;

	uint8_t fade_level;
	uint8_t requested_mode;

	uint8_t packet_sequence_number;
	uint8_t packet_beam;

} DISC;

typedef struct {
	uint8_t local_packets_in_per_second = 0;
	uint8_t local_packets_out_per_second = 0;

	uint8_t local_framing_errors = 0;
	uint8_t local_crc_errors = 0;

	int16_t total_lost_packets = 0;

} SERIALSTATS;

typedef struct {
	uint8_t local_packets_in_per_second_glove0 = 0;
	uint8_t local_packets_in_per_second_glove1 = 0;
	uint8_t local_packets_out_per_second = 0;

	uint8_t local_framing_errors = 0;
	uint8_t local_crc_errors = 0;

	int16_t total_lost_packets = 0;

} SERIALSTATSGLOVE;

//lookup table for 30 pixel circumference circle coordinates
const uint8_t circle_x[30] = { 6, 7, 8, 9, 10, 11, 12, 12, 12, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 1, 2, 3, 4, 5 };
const uint8_t circle_y[30] = { 0, 0, 0, 1, 1, 2, 3, 4, 5, 6, 7, 8, 8, 9, 9, 9, 9, 9, 8, 8, 7, 6, 5, 4, 3, 2, 1, 1, 0, 0 };

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

CHSV color1 = CHSV(0, 255, 255);
CHSV color2 = CHSV(64, 255, 255);

int8_t flow_offset = 0;

#define GLOVE_DEADZONE 3000  //30 degrees
#define GLOVE_MAXIMUM 30000 //90 degrees

#define OLED_DC     18
#define OLED_CS     22
#define OLED_RESET  19
Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);

uint8_t mask_mode = 0;

float temperature = 0.0;
float voltage = 0.0;
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

#define SCROLL_MODE_INCOMING 3
#define SCROLL_MODE_PAUSE 2
#define SCROLL_MODE_OUTGOING 1
#define SCROLL_MODE_COMPLETE 0
uint8_t scroll_mode = SCROLL_MODE_COMPLETE;  //keeps track of scroll state
#define SCROLL_PAUSE_TIME 1000 //time to pause at 0,0
uint32_t scroll_pause_start_time = 0; //keeps track of when pause started

//crank up hardwareserial.cpp to 128 to match!
#define INCOMING1_BUFFER_SIZE 128
uint8_t incoming1_raw_buffer[INCOMING1_BUFFER_SIZE];
uint8_t incoming1_index = 0;
uint8_t incoming1_decoded_buffer[INCOMING1_BUFFER_SIZE];

#define INCOMING2_BUFFER_SIZE 128
uint8_t incoming2_raw_buffer[INCOMING2_BUFFER_SIZE];
uint8_t incoming2_index = 0;
uint8_t incoming2_decoded_buffer[INCOMING2_BUFFER_SIZE];

#define INCOMING3_BUFFER_SIZE 128
uint8_t incoming3_raw_buffer[INCOMING3_BUFFER_SIZE];
uint8_t incoming3_index = 0;
uint8_t incoming3_decoded_buffer[INCOMING3_BUFFER_SIZE];

// adc object for battery and temp meter, will use ADC1
ADC *adc = new ADC();

AudioInputAnalog         adc1(A9);  //A9 is on ADC0
AudioAnalyzeFFT256       fft256_1;
AudioConnection          patchCord1(adc1, fft256_1);

#define NUM_LEDS_PER_STRIP 130 //longest strip is helmet, 128 LEDS + 2 indicators internal
#define NUM_STRIPS 8 //must be 8, even though only 6 are used
CRGB actual_output[NUM_STRIPS * NUM_LEDS_PER_STRIP];

int color_on = 1;

GLOVE glove0;
GLOVE glove1;
SERIALSTATSGLOVE glovestats;
SERIALSTATS discstats;
SERIALSTATS bluetoothstats;
DISC disc0;

byte gloveindicator[16][8];


CHSV EQdisplay[16][8];
int EQdisplayValueMax16[16]; //max vals for normalization over time
uint8_t EQdisplayValue16[16]; //max vals for normalization over time
int EQdisplayValueMax8[8]; //max vals for normalization over time
uint8_t EQdisplayValue8[8]; //max vals for normalization over time

Metro FPSdisplay = Metro(1000);
Metro glovedisplayfade = Metro(10);
Metro YPRdisplay = Metro(100);
Metro ScrollSpeed = Metro(40);
Metro GloveSend = Metro(10);
Metro DiscSend = Metro(50);
Metro DiscSend3 = Metro(20);
Metro LEDdisplay = Metro(10);

//to round robin poll sensors
#define TEMP_SENSOR 0
#define BATTERY_METER 1
uint8_t adc1_mode = TEMP_SENSOR;