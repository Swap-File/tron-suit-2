
void setup() {
	delay(1000);
	display.begin(SSD1306_SWITCHCAPVCC);
	//display.setRotation(2);
	display.display();

	//noise init
	x_noise = random16();
	y_noise = random16();
	z_noise = random16();

	//must go first so Serial.begin can override pins!!!
	LEDS.addLeds<OCTOWS2811>(actual_output, NUM_LEDS_PER_STRIP);
	for (int i = 0; i < NUM_STRIPS * NUM_LEDS_PER_STRIP; i++) {
		actual_output[i] = CRGB(0, 0, 0);
	}
	for (int i = 0; i < 16; i++) {
		FFTdisplayValueMax16[i] = 4;
	}
	//bump hardwareserial.cpp to 255
	Serial.begin(115200);   //Debug
	Serial1.begin(115200);  //Gloves	
	Serial2.begin(57600);  //Xbee	
	Serial3.begin(115200);  //BT

	//audio library setup
	AudioMemory(3);
	fft256_1.windowFunction(AudioWindowHanning256);
	fft256_1.averageTogether(4);

	//ADC1 setup
	pinMode(A3, INPUT);
	adc->setReference(ADC_REF_DEFAULT, ADC_1);
	adc->setAveraging(32, ADC_1);
	adc->setResolution(16, ADC_1);
	adc->startContinuous(A3, ADC_1);

	//global settings
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setTextWrap(false);



}

uint32_t average_time;

void loop() {

	uint32_t start_time = micros();

	//ringing for incoming calls
	sms_blackout = false;
	if (ring_timer + RINGTIMEOUT > millis()){
		if (ring_timer + RINGTIMEOUT * 2 < millis() & ring_timer + RINGTIMEOUT * 3 > millis()){  //sets on/off cycle
			if ((millis() >> 6) & 0x01)	sms_blackout = true;  //this sets pulsing speed
		}
	}



	if (FPSdisplay.check()){
		//keep re-initing the screen for hot plug support.
		display.reinit();

		//stats_message
		sprintf(stats_message, "JACKET %.2fV DISC %.2fV", voltage, (disc0.battery_voltage / (10.0 * 1.5)));

		Serial.print(voltage);
		Serial.print('\t');
		Serial.println(disc0.battery_voltage);

		glovestats.total_lost_packets += (glovestats.local_packets_out_per_second << 1); //add in both gloves
		glovestats.total_lost_packets -= glovestats.local_packets_in_per_second_glove0;
		glovestats.total_lost_packets -= glovestats.local_packets_in_per_second_glove1;

		glovestats.saved_local_packets_in_per_second_glove0 = glovestats.local_packets_in_per_second_glove0;
		glovestats.saved_local_packets_in_per_second_glove1 = glovestats.local_packets_in_per_second_glove1;
		glovestats.saved_local_packets_out_per_second = glovestats.local_packets_out_per_second;

		glovestats.local_packets_in_per_second_glove0 = 0;
		glovestats.local_packets_in_per_second_glove1 = 0;
		glovestats.local_packets_out_per_second = 0;
		if (glovestats.total_lost_packets > 255) glovestats.total_lost_packets = 0;


		discstats.total_lost_packets += discstats.local_packets_out_per_second;
		discstats.total_lost_packets -= discstats.local_packets_in_per_second;

		discstats.saved_local_packets_out_per_second = discstats.local_packets_out_per_second;
		discstats.saved_local_packets_in_per_second = discstats.local_packets_in_per_second;

		discstats.local_packets_out_per_second = 0;
		discstats.local_packets_in_per_second = 0;
		if (discstats.total_lost_packets > 255) discstats.total_lost_packets = 0;

		bluetoothstats.total_lost_packets += bluetoothstats.local_packets_out_per_second;
		bluetoothstats.total_lost_packets -= bluetoothstats.local_packets_in_per_second;
		bluetoothstats.local_packets_in_per_second = 0;
		bluetoothstats.local_packets_out_per_second = 0;
		if (bluetoothstats.total_lost_packets > 255) bluetoothstats.total_lost_packets = 0;

		//LEDS.getFPS();
		//cpu_usage = 100 - (idle_microseconds / 10000);
		//local_packets_out_per_second_1 = local_packets_out_counter_1;
		//local_packets_in_per_second_1 = local_packets_in_counter_1;
		//idle_microseconds = 0;
		// local_packets_out_counter_1 = 0;
		// local_packets_in_counter_1 = 0;
	}

	//decay the trails of the glove indicator
	if (glovedisplayfade.check()){
		for (int x = 0; x < 16; x++) {
			for (int y = 0; y < 8; y++) {
				if (gloveindicator[x][y] > 0){
					gloveindicator[x][y]--;
				}
			}
		}
	}

	//make new trails
	if (glove0.finger2 == 1){
		gloveindicator[8 + glove0.gloveX][7 - glove0.gloveY] = 100;

	}
	if (glove1.finger2 == 1){
		gloveindicator[glove1.gloveX][7 - glove1.gloveY] = 100;
	}



	//draw most of hud, last bit will be drawn in external helmet bit
	draw_HUD();

	//draw the helmet
	for (uint8_t y = 0; y < 8; y++) {
		for (uint8_t x = 0; x < 16; x++) {

			CRGB background_array = CRGB(0, 0, 0);
			CRGB menu_name = CRGB(0, 0, 0);
			CRGB final_color = CRGB(0, 0, 0);

			if (scroll_mode != SCROLL_MODE_COMPLETE){
				if (read_menu_pixel(x, y))	menu_name = CRGB(180, 180, 180);
			}

			//no overlay for certain modes
			if (menu_mode != MENU_HELMET_EMOTICON_ON_SOUND && menu_mode != MENU_HELMET_EMOTICON_ON_MOTION && menu_mode != MENU_HELMET_PONG_IN){
				if (gloveindicator[x][7 - y] > 0){ // flip Y for helmet external display by subtracting from 7
					background_array = CRGB(gloveindicator[x][7 - y], gloveindicator[x][7 - y], gloveindicator[x][7 - y]);
				}

			}
			//override modes
			if (menu_mode == MENU_HELMET_PONG_IN){
				final_color = Pong_Array[x][y];  //everything else
			}
			else if (menu_mode == MENU_HELMET_EMOTICON_ON_SOUND || menu_mode == MENU_HELMET_EMOTICON_ON_MOTION){
				if (read_emot_Array(x, y)){
					final_color = disc0.color1;
				}
			}
			else{
				//backgrounds
				if (background_mode == BACKGROUND_MODE_FFT){
					if (menu_mode == MENU_TXT_ON){
						if (read_txt_pixel(x, y)){
							CHSV temp = FFT_Array[x][y];
							temp.v = max(temp.v, SMS_MIN_BRIGHTNESS);
							final_color = temp;
						}
					}
					else{
						final_color = FFT_Array[x][y];
					}
				}
				else if (background_mode == BACKGROUND_MODE_NOISE){
					if (menu_mode == MENU_TXT_ON){
						if (read_txt_pixel(x, y)){
							final_color = Noise_Array_Bright[x][y];;
						}
					}
					else{
						final_color = Noise_Array[x][y];
					}
				}
				final_color = background_array | menu_name | final_color;
			}

			//Internal HUD Display
			if (final_color.getAverageLight() > 32){
				display.drawPixel(realtime_location_x + (15 - x), realtime_location_y + 7 - y, WHITE);
			}

			uint8_t tempindex;  //HUD is only 128 LEDs so it will fit in a byte

			//determine if row is even or odd abd place pixel
			if ((y & 0x01) == 1)  tempindex = (y << 4) + 15 - x;  //<< 4 is multiply by 16 pixels per row
			else                  tempindex = (y << 4) + x; //<< 4 is multiply by 16 pixels per row
			//flash display for incoming call
			if (!sms_blackout && !supress_helmet && \
				(startup_mode == STARTUP_MODE_COMPLETED || \
				(startup_mode == STARTUP_MODE_OPENING_HELMET && (x > (7 - startup_mode_masking_helmet) && (x < (8 + startup_mode_masking_helmet)))))){
				actual_output[tempindex] = final_color;
			}
			else{
				actual_output[tempindex] = CRGB(0, 0, 0);
			}
		}
	}




	//draw the suit strips
	for (uint16_t i = 0; i < 38; i++) {

		//chest strips
		if (startup_mode == STARTUP_MODE_DELAY){
			actual_output[(5 * 130) + i] = CRGB(0, 0, 0);
			actual_output[(4 * 130) + i] = CRGB(0, 0, 0);
		}
		else if (startup_mode == STARTUP_MODE_OPENING_SOLID){
			if (startup_mode_masking > i){
				mask_blur_and_output((5 * 130) + i, &color1, i, 19, 1);
				mask_blur_and_output((4 * 130) + i, &color2, i, 19, 1);
			}
			else{
				actual_output[(5 * 130) + i] = CRGB(0, 0, 0);
				actual_output[(4 * 130) + i] = CRGB(0, 0, 0);
			}
		}
		else{
			if (disc0.disc_mode == DISC_MODE_SWIPE){
				mask_blur_and_output((5 * 130) + ((i + (37 - scale8(disc0.current_index, 38))) % 38), &color1, i, map(disc0.current_mag, 0, 16, 0, 19), 255);
				mask_blur_and_output((4 * 130) + ((i + (37 - scale8(disc0.current_index, 38))) % 38), &color2, i, map(disc0.current_mag, 0, 16, 0, 19), 255);
			}
			else{
				mask_blur_and_output((5 * 130) + i, &stream1[(stream_head + i) % 38], i, 19, 1);
				mask_blur_and_output((4 * 130) + i, &stream2[(stream_head + i) % 38], i, 19, 1);
			}
		}

		//arm strips
		if (startup_mode == STARTUP_MODE_DELAY) actual_output[(6 * 130) + i] = CRGB(0, 0, 0);
		else if (startup_mode == STARTUP_MODE_OPENING_SOLID){
			if (startup_mode_masking > (37 - i)){
				mask_blur_and_output((6 * 130) + i, &color1, i, 19, 1);
			}
			else{
				actual_output[(6 * 130) + i] = CRGB(0, 0, 0);
			}

		}
		else if (startup_mode == STARTUP_MODE_OPENING_ARMS){
			if (startup_mode_masking > (37 - i)){
				actual_output[(6 * 130) + i] = (&Noise_Array[0][0])[i];
			}
			else{
				mask_blur_and_output((6 * 130) + i, &color1, i, 19, 1);
			}
		}
		else{
			if (glove1.camera_on){
				blur_cust((6 * 130) + (i), &glove1.cameraflow[(1 + i + glove1.cameraflow_index) % 38], 1);
			}
			else{
				if (background_mode == BACKGROUND_MODE_FFT){
					actual_output[(6 * 130) + i] = (&FFT_Array[0][0])[38 - i];
				}
				else if (background_mode == BACKGROUND_MODE_NOISE){
					actual_output[(6 * 130) + i] = (&Noise_Array[0][0])[i];
				}
			}
		}

		if (startup_mode == STARTUP_MODE_DELAY) actual_output[(7 * 130) + i] = CRGB(0, 0, 0);
		else if (startup_mode == STARTUP_MODE_OPENING_SOLID){
			if (startup_mode_masking > (37 - i)){
				mask_blur_and_output((7 * 130) + i, &color2, i, 19, 1);
			}
			else{
				actual_output[(7 * 130) + i] = CRGB(0, 0, 0);
			}

		}
		else if (startup_mode == STARTUP_MODE_OPENING_ARMS){
			if (startup_mode_masking > (37 - i)){
				actual_output[(7 * 130) + i] = (&Noise_Array[0][0])[i];
			}
			else{
				mask_blur_and_output((7 * 130) + i, &color2, i, 19, 1);
			}
		}
		else{
			if (background_mode == BACKGROUND_MODE_FFT){
				actual_output[(7 * 130) + i] = (&FFT_Array[0][0])[38 - i];
			}
			else if (background_mode == BACKGROUND_MODE_NOISE){
				actual_output[(7 * 130) + i] = (&Noise_Array[0][0])[i];
			}
		}
	}

	//write out HUD
	for (uint16_t i = 0; i < 38; i++) {
		if (actual_output[(7 * 130) + i].getAverageLight() > 32)	display.drawPixel(54, i, WHITE);
		if (actual_output[(5 * 130) + i].getAverageLight() > 32)	display.drawPixel(56, i, WHITE);
		if (disc0.packet_beam == i)									display.drawPixel(58, i, WHITE);
		if (actual_output[(4 * 130) + i].getAverageLight() > 32)	display.drawPixel(60, i, WHITE);
		if (actual_output[(6 * 130) + i].getAverageLight() > 32)	display.drawPixel(62, i, WHITE);
	}

	//we want to use setbrightness so we get dithering
	//but I need brightness when off for color preview
	//this blocks out everything excep the preview
	if (current_brightness != 0){
		LEDS.setBrightness(current_brightness);
	}
	else{
		LEDS.setBrightness(255);
		for (int i = 0; i < NUM_STRIPS * NUM_LEDS_PER_STRIP; i++) {
			actual_output[i] = CRGB(0, 0, 0);
		}
	}

	if (current_brightness != 0 || \
		//this previews colors
		(current_brightness == 0 && (glove1.finger1 || glove1.finger2 || glove0.finger1 || glove0.finger2)) || \
		cust_Startup_color){//custom startup preview code

		//set internal indicators, flip if flipped
		if (disc0.active_primary){
			actual_output[128] = color1;
			actual_output[129] = color2;
		}
		else{
			actual_output[128] = color2;
			actual_output[129] = color1;
		}
		if (cust_Startup_color){
			actual_output[129] = CRGB(0, 0, 0);
		}
	}

	//swap to camera if in cam mode
	if (glove1.camera_on){
		actual_output[129] = glove1.output_rgb_led;
	}
	if (glove0.camera_on){
		actual_output[128] = glove0.output_rgb_led;
	}
	//blank hud if in a countdown
	if (millis() - left_timer < 100 || sms_blackout){
		actual_output[128] = CRGB(0, 0, 0);
	}
	if (millis() - right_timer < 100 || sms_blackout){
		actual_output[129] = CRGB(0, 0, 0);
	}

	//dim hud lights
	actual_output[128] = actual_output[128].nscale8_video(8);
	actual_output[129] = actual_output[129].nscale8_video(8);


	//advance scrolling if timer reached or stop
	if (ScrollSpeed.check()){
		txt_scroll_pos--;
		if (txt_ending_pos < 0){
			txt_scroll_pos = 0;
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

		voltage = voltage * .95 + .05 * ((uint16_t)adc->analogReadContinuous(ADC_1)) / 3516.083016; //voltage
		//dont let octows2811 steal serial3
		//not sure why this works, but reminding the CPU we are using these pins for Serial3 seems to work
		CORE_PIN7_CONFIG = PORT_PCR_PE | PORT_PCR_PS | PORT_PCR_PFE | PORT_PCR_MUX(3);
		CORE_PIN8_CONFIG = PORT_PCR_DSE | PORT_PCR_SRE | PORT_PCR_MUX(3);
	}

	//animation on the suit while the disc opens
	if (startup_mode != STARTUP_MODE_COMPLETED){
		//initial delay to let the disc go a bit...
		if (startup_mode == STARTUP_MODE_DELAY){
			if (millis() - startup_time > 500){
				startup_mode = STARTUP_MODE_OPENING_SOLID;
				startup_time = millis();
				disc0.packet_beam = 0;
				startup_mode_masking = 0;
				startup_mode_masking_helmet = 0;
			}
		}
		//fill the jacket in solid here...
		else if (startup_mode == STARTUP_MODE_OPENING_SOLID){
			startup_mode_masking = map(millis() - startup_time, 0, 1000, 0, 38);
			if (startup_mode_masking > 37){
				startup_mode = STARTUP_MODE_OPENING_ARMS;
				startup_time = millis();
				startup_mode_masking = 0;
				startup_mode_masking_helmet = 0;
			}
		}
		//fill in the arms with noise and sequentially unmask helmet
		else if (startup_mode == STARTUP_MODE_OPENING_ARMS){
			startup_mode_masking = map(millis() - startup_time, 0, 1000, 0, 38);
			//startup_mode_masking_helmet = map(millis() - startup_time, 0, 1000, 0, 8);
			if (startup_mode_masking > 37)	{
				startup_mode = STARTUP_MODE_OPENING_HELMET;
				startup_time = millis();
			}
		}
		else if (startup_mode == STARTUP_MODE_OPENING_HELMET){
			//startup_mode_masking = map(millis() - startup_time, 0, 1000, 0, 38);
			startup_mode_masking_helmet = map(millis() - startup_time, 0, 1000, 0, 8);
			if (startup_mode_masking_helmet > 8)	startup_mode = STARTUP_MODE_COMPLETED;
		}
	}
	 
	//controls the streaming of the colors
	if (Flow.check()){

		uint8_t curoff = quadwave8(flow_offset++);
		uint8_t curoff2 = quadwave8(flow_offset2 += 2);
		discwave = scale8(curoff, 16);
		uint8_t suitwave = scale8(curoff2, 38);

		//calculate current pixels and add to array
		if (disc0.disc_mode == DISC_MODE_SWIPE){
			//to suit
			for (uint8_t current_pixel = 0; current_pixel < 38; current_pixel++) {
				stream1[current_pixel] = color1;
				stream2[current_pixel] = color2;
			}
		}
		else if (disc0.disc_mode == DISC_MODE_IDLE){

			//to suit
			stream1[stream_head] = map_hsv(curoff2, 0, 255, &color1, &color2);
			stream2[stream_head] = map_hsv(curoff2, 0, 255, &color1, &color2);

			if (stream_head == 37)	stream_head = 0;
			else					stream_head++;
		}
	}

	if (fft256_1.available()) {
		fftmath();
		helmet_backgrounds();
	}

	SerialUpdate();

	average_time = average_time * .5 + .5 * (micros() - start_time);
}


CHSV map_hsv(uint8_t input, uint8_t in_min, uint8_t in_max, CHSV* out_starting, CHSV* out_ending) {


	if (input <= in_min) return CHSV(*out_starting);
	if (input >= in_max) return CHSV(*out_ending);

	//calculate shortest path between colors
	int16_t shortest_path = out_ending->h; //no rollover
	if ((((int16_t)out_ending->h) + 256) - ((int16_t)out_starting->h) <= 127) {
		shortest_path += 256;  //rollover
	}
	else if ((int16_t)(out_starting->h) - (((int16_t)out_ending->h) - 255) <= 127) {
		shortest_path -= 256; //rollunder
	}


	return CHSV(
		((input - in_min) * (shortest_path - out_starting->h + 1) / (in_max - in_min + 1) + out_starting->h), \
		(input - in_min) * (out_ending->s - out_starting->s + 1) / (in_max - in_min + 1) + out_starting->s, \
		(input - in_min) * (out_ending->v - out_starting->v + 1) / (in_max - in_min + 1) + out_starting->v);
}


// 	int i = (current_pixel + loc_offset) % 38;// wrap around to get actual location
void mask_blur_and_output(uint16_t i, CHSV* color, uint8_t current_pixel, uint8_t magnitude, uint8_t blur_rate){
	//convert HSV to RGB
	CRGB temp_rgb = *color;

	//masking code
	if (magnitude < 19){  //basic open
		if (current_pixel >= magnitude  && current_pixel <= (38 - magnitude)) temp_rgb = CRGB(0, 0, 0);
	}
	else if (magnitude > 19){ //basic close
		if (current_pixel < (magnitude - 19) || current_pixel >(37 - (magnitude - 20))) temp_rgb = CRGB(0, 0, 0);
	}

	//determine beam pattern
	bool blur = true;

	if (disc0.packet_beam == i % 130) blur = false;

	//dont blur or fade pixels that are turning on
	//dont blur or fade beam pixels
	//turn leds off immediately
	if (blur && color->v != 0 && actual_output[i] != CRGB(0, 0, 0)){

		temp_rgb.fadeToBlackBy(disc0.fade_level); // fade_level

		if (actual_output[i].r < temp_rgb.r) temp_rgb.r = min(qadd8(actual_output[i].r, blur_rate), temp_rgb.r);
		else if (actual_output[i].r > temp_rgb.r) temp_rgb.r = max(qsub8(actual_output[i].r, blur_rate), temp_rgb.r);

		if (actual_output[i].g < temp_rgb.g) temp_rgb.g = min(qadd8(actual_output[i].g, blur_rate), temp_rgb.g);
		else if (actual_output[i].g > temp_rgb.g) temp_rgb.g = max(qsub8(actual_output[i].g, blur_rate), temp_rgb.g);

		if (actual_output[i].b < temp_rgb.b) temp_rgb.b = min(qadd8(actual_output[i].b, blur_rate), temp_rgb.b);
		else if (actual_output[i].b > temp_rgb.b) temp_rgb.b = max(qsub8(actual_output[i].b, blur_rate), temp_rgb.b);
	}

	actual_output[i] = temp_rgb;
}

void blur_cust(uint16_t i, CHSV* color, uint8_t blur_rate){
	CRGB temp_rgb = *color;

	if (color->v != 0 && actual_output[i] != CRGB(0, 0, 0)){

		temp_rgb.fadeToBlackBy(disc0.fade_level); // fade_level

		if (actual_output[i].r < temp_rgb.r) temp_rgb.r = min(qadd8(actual_output[i].r, blur_rate), temp_rgb.r);
		else if (actual_output[i].r > temp_rgb.r) temp_rgb.r = max(qsub8(actual_output[i].r, blur_rate), temp_rgb.r);

		if (actual_output[i].g < temp_rgb.g) temp_rgb.g = min(qadd8(actual_output[i].g, blur_rate), temp_rgb.g);
		else if (actual_output[i].g > temp_rgb.g) temp_rgb.g = max(qsub8(actual_output[i].g, blur_rate), temp_rgb.g);

		if (actual_output[i].b < temp_rgb.b) temp_rgb.b = min(qadd8(actual_output[i].b, blur_rate), temp_rgb.b);
		else if (actual_output[i].b > temp_rgb.b) temp_rgb.b = max(qsub8(actual_output[i].b, blur_rate), temp_rgb.b);
	}

	actual_output[i] = temp_rgb;
}


boolean read_emot_Array(uint8_t x, uint8_t y){

	//draw the ^ ^ or ^ - or - ^ for winking
	if ((glove1.finger1 || glove1.finger2) && leading_glove == 0){
		if (x == 1 && y == 5) return true;
		if (x == 2 && y == 5) return true;
		if (x == 3 && y == 5) return true;
		if (x == 4 && y == 5) return true;
		if (x == 5 && y == 5) return true;
	}
	else{
		if (x == 1 && y == 5) return true;
		if (x == 2 && y == 6) return true;
		if (x == 3 && y == 7) return true;
		if (x == 4 && y == 6) return true;
		if (x == 5 && y == 5) return true;
	}

	if ((glove0.finger1 || glove0.finger2) && leading_glove == 1){
		if (x == 10 && y == 5) return true;
		if (x == 11 && y == 5) return true;
		if (x == 12 && y == 5) return true;
		if (x == 13 && y == 5) return true;
		if (x == 14 && y == 5) return true;
	}
	else{
		if (x == 10 && y == 5) return true;
		if (x == 11 && y == 6) return true;
		if (x == 12 && y == 7) return true;
		if (x == 13 && y == 6) return true;
		if (x == 14 && y == 5) return true;
	}


	

	if (menu_mode == MENU_HELMET_EMOTICON_ON_SOUND){
		//unmute on finger press
		if (glove0.finger1 || glove0.finger2 || glove1.finger1 || glove1.finger2){
			newsmile = qadd8(FFTdisplayValue1, scale8(FFTdisplayValue1, FFTdisplayValue1));
		}
	}
	else if (menu_mode == MENU_HELMET_EMOTICON_ON_MOTION){
		if ((glove0.finger1 || glove0.finger2) && leading_glove == 0) newsmile = glove0.gloveY * 30;
		else if ((glove1.finger1 || glove1.finger2) && leading_glove == 1) newsmile = glove1.gloveY * 30;
	}

	uint8_t smile_open = scale8(newsmile, 4);

	//bottom row
	if (smile_open > 0){
		if (x < 11 && x >4 && y == 0) return true;
	}
	//second row if exactly 2
	if (smile_open == 2){
		if (x < 12 && x >3 && y == 1) return true;
	}
	//second row if greater than  2
	if (smile_open > 2){
		if ((x == 11 || x == 4) && y == 1) return true;
	}
	//top row
	if (smile_open >= 3){
		if (x < 12 && x >3 && y == 2) return true;
	}

	return false;
}