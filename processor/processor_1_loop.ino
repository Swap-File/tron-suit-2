
void setup() {

	strcpy(sms_message, "TESTING TEXT MESSAGE");

	display.begin(SSD1306_SWITCHCAPVCC);
	display.display();

	//must go first so Serial.begin can override pins!!!
	LEDS.addLeds<OCTOWS2811>(actual_output, NUM_LEDS_PER_STRIP);
	for (int i = 0; i < NUM_STRIPS * NUM_LEDS_PER_STRIP; i++) {
		actual_output[i] = CRGB(0, 0, 0);
	}

	//bump hardwareserial.cpp to 255
	Serial.begin(115200);   //Debug
	Serial1.begin(115200);  //Gloves	
	Serial2.begin(57600);  //Xbee	
	Serial3.begin(115200);  //BT

	//audio library setup
	AudioMemory(4);
	fft256_1.windowFunction(AudioWindowHanning256);
	fft256_1.averageTogether(4);

	//ADC1 setup
	pinMode(A3, INPUT);
	adc->setAveraging(32, ADC_1);
	adc->setResolution(16, ADC_1);

	//global settings
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setTextWrap(false);
}

long int average_time;

void loop() {

	long start_time = micros();


	if (YPRdisplay.check()){

		switch (adc1_mode){
		case BATTERY_METER:
			voltage = voltage * .95 + .05 * (((uint16_t)adc->analogReadContinuous(ADC_1)) / 4083.375); //voltage
			adc->stopContinuous(ADC_1);
			adc->setReference(ADC_REF_INTERNAL, ADC_1);  //modified this to remove calibration, drops time from 2ms to ~2ns
			adc->startContinuous(38, ADC_1);
			adc1_mode = TEMP_SENSOR;
			break;
		case TEMP_SENSOR:
			// temp sensor from https ://github.com/manitou48/teensy3/blob/master/chiptemp.pde
			temperature = temperature * .95 + .05 * (25 - (((uint16_t)adc->analogReadContinuous(ADC_1)) - 38700) / -35.7); //temp in C
			adc->stopContinuous(ADC_1);
			adc->setReference(ADC_REF_EXTERNAL, ADC_1);  //modified this to remove calibration
			adc->startContinuous(A3, ADC_1);
			adc1_mode = BATTERY_METER;
			break;
		}

		//keep re-initing the screen for hot plug support.
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
		Serial.print(average_time);
		Serial.print(" ");
		Serial.println(total_packets_out - total_packets_in);


		glovestats.total_lost_packets += (glovestats.local_packets_out_per_second << 1); //add in both gloves
		glovestats.total_lost_packets -= glovestats.local_packets_in_per_second_glove0;
		glovestats.total_lost_packets -= glovestats.local_packets_in_per_second_glove1;
		glovestats.local_packets_in_per_second_glove0 = 0;
		glovestats.local_packets_in_per_second_glove1 = 0;
		glovestats.local_packets_out_per_second = 0;

		discstats.total_lost_packets += discstats.local_packets_out_per_second;
		discstats.total_lost_packets -= discstats.local_packets_in_per_second;
		discstats.local_packets_in_per_second = 0;
		discstats.local_packets_out_per_second = 0;

		bluetoothstats.total_lost_packets += bluetoothstats.local_packets_out_per_second;
		bluetoothstats.total_lost_packets -= bluetoothstats.local_packets_in_per_second;
		bluetoothstats.local_packets_in_per_second = 0;
		bluetoothstats.local_packets_out_per_second = 0;

		fpscount = 0;
		//cpu_usage = 100 - (idle_microseconds / 10000);
		//local_packets_out_per_second_1 = local_packets_out_counter_1;
		//local_packets_in_per_second_1 = local_packets_in_counter_1;
		//idle_microseconds = 0;
		// local_packets_out_counter_1 = 0;
		// local_packets_in_counter_1 = 0;
	}

	if (glove1.finger3 == 1){
		
		glove0.output_white_led = 0x01;
		

		uint32_t sum = glove0.color_sensor_Clear;
		Serial.println(sum);
		if (sum > 3000 && sum <  20000){

			float r, g, b;
			r = glove0.color_sensor_R; r /= sum;
			g = glove0.color_sensor_G; g /= sum;
			b = glove0.color_sensor_B; b /= sum;
			r *= 256; g *= 256; b *= 256;

			CHSV temp = rgb2hsv_approximate(CRGB(r, g, b));
			//temp.v = min(temp.v, 200);
			temp.s = max(temp.s, 192);
			CRGB temp2;
			hsv2rgb_rainbow(temp, temp2);
			glove0.output_rgb_led = temp2;


			color1 = temp;

		}
		else{
			//ove0.output_rgb_led = CRGB(0, 0, 0);
		}
	}
	else{
		glove0.output_white_led = 0x00;
		glove0.output_rgb_led = CRGB(0, 0, 0);
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

	if (glove1.finger1 == 1){
		gloveindicator[8 + glove0.gloveX][7 - glove0.gloveY] = 100;// flip Y for helmet external display by subtracting from 7
		gloveindicator[glove1.gloveX][7 - glove1.gloveY] = 100;// flip Y for helmet external display by subtracting from 7
	}


	if ((glove1.finger1 || glove1.finger2) && menu_mode == MENU_MAG){
		disc0.outer_magnitude_requested = glove1.calculated_mag;
	}
	

	if ((glove1.finger1 || glove1.finger2) && menu_mode == MENU_SPIN){
		disc0.inner_offset_requested = glove1.disc_offset;
		disc0.outer_offset_requested = glove1.disc_offset;
	}
	

	if (menu_mode != MENU_MAG && menu_mode != MENU_SPIN){
		disc0.outer_magnitude_requested = 16;
		disc0.inner_offset_requested = 0;
		disc0.outer_offset_requested = 0;
	}

	//draw most of hud, last bit will be drawn in external helmet bit
	draw_HUD();

	for (uint8_t y = 0; y < 8; y++) {
		for (uint8_t x = 0; x < 16; x++) {

			CRGB background_array = CRGB(0, 0, 0);
			CRGB menu_name = CRGB(0, 0, 0);
			CRGB final_color = CRGB(0, 0, 0);

			if (scroll_mode != SCROLL_MODE_COMPLETE){
				if (read_menu_pixel(x, y))	menu_name = CRGB(180, 180, 180);
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

			//determin if row is even or odd
			if ((y & 0x01) == 1)  tempindex = (y << 4) + 15 - x;  //<< 4 is multiply by 16 pixels per row
			else                  tempindex = (y << 4) + x; //<< 4 is multiply by 16 pixels per row

			actual_output[tempindex] = final_color;
		}
	}

	//advance scrolling if timer reached or stop

	if (ScrollSpeed.check()){
		sms_scroll_pos--;
		if (sms_text_ending_pos < 0){
			sms_scroll_pos = 0;
		}

		switch (scroll_mode){

		case SCROLL_MODE_INCOMING:
			//take up slack when scrolling in backwards
			if (menu_text_ending_pos < 0 && scroll_pos_x < 0) scroll_pos_x -= menu_text_ending_pos;

			if (scroll_pos_x > 0) scroll_pos_x--;
			else if (scroll_pos_y > 0) scroll_pos_y--;
			else if (scroll_pos_x < 0) scroll_pos_x++;
			else if (scroll_pos_y < 0) scroll_pos_y++;

			if (scroll_pos_y == 0 && scroll_pos_x == 0){
				scroll_mode = SCROLL_MODE_PAUSE;
				scroll_pause_start_time = millis();
			}
			break;
		case SCROLL_MODE_PAUSE:
			if (millis() - scroll_pause_start_time > SCROLL_PAUSE_TIME){
				scroll_mode = SCROLL_MODE_OUTGOING;
			}
			break;
		case SCROLL_MODE_OUTGOING:
			if (scroll_pos_x > scroll_end_pos_x) scroll_pos_x--;
			else if (scroll_pos_y > scroll_end_pos_y) scroll_pos_y--;
			else if (scroll_pos_x < scroll_end_pos_x) scroll_pos_x++;
			else if (scroll_pos_y < scroll_end_pos_y) scroll_pos_y++;

			if (scroll_pos_y == scroll_end_pos_y && scroll_pos_x == scroll_end_pos_x){
				scroll_mode = SCROLL_MODE_COMPLETE;
			}
		}
	}

	if (LEDdisplay.check()){
		display.display();
		LEDS.show();
		fpscount++;
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


	if (fft256_1.available()) {
		fftmath();
	}

	SerialUpdate();

	average_time = average_time * .5 + .5 * (micros() - start_time);
}




CHSV map_hsv(uint8_t input, uint8_t in_min, uint8_t in_max, CHSV* out_min, CHSV* out_max){
	return CHSV(
		(input - in_min) * (out_max->h - out_min->h) / (in_max - in_min) + out_min->h, \
		(input - in_min) * (out_max->s - out_min->s) / (in_max - in_min) + out_min->s, \
		(input - in_min) * (out_max->v - out_min->v) / (in_max - in_min) + out_min->v);
}


