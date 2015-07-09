inline void SerialUpdate(void){
	if (DiscSend3.check()){

		disc0.packet_beam--;
		 
		//calculate current pixels and add to array
		if (disc0.disc_mode != DISC_MODE_IDLE){
			//to disc
			disc0.color1 = color1;
			disc0.color2 = color2;
		}
		else{
			if (last_discwave != discwave){
				disc0.color1 = map_hsv(discwave, 0, 15, &color1, &color2);
				disc0.color2 = map_hsv(discwave, 0, 15, &color1, &color2);

				disc0.packet_sequence_number++;
				last_discwave = discwave;
			}
		}

		uint8_t raw_buffer[15];

		raw_buffer[0] = disc0.color1.h;
		raw_buffer[1] = disc0.color1.s;
		raw_buffer[2] = disc0.color1.v;
		raw_buffer[3] = disc0.color2.h;
		raw_buffer[4] = disc0.color2.s;
		raw_buffer[5] = disc0.color2.v;
		raw_buffer[6] = disc0.inner_offset_requested;
		raw_buffer[7] = disc0.outer_offset_requested;
		raw_buffer[8] = disc0.inner_magnitude_requested;
		raw_buffer[9] = disc0.outer_magnitude_requested;
		//raw_buffer[10] = disc0.fade_level; //disc0.fade_level;  //fade
		raw_buffer[10] = current_brightness;
		raw_buffer[11] = disc0.disc_mode & 0x0F; // lower 4 are disc mode itself

		if (!disc0.active_primary){ //flip the colors
			bitSet(raw_buffer[11], 5);
			bitSet(raw_buffer[11], 4);
		}

		raw_buffer[12] = disc0.packet_sequence_number;  //sequence
		raw_buffer[13] = disc0.packet_beam;  //sequence
		raw_buffer[14] = OneWire::crc8(raw_buffer, 14);

		if (disc0.packet_sequence_number > 29){
			disc0.packet_sequence_number = 0;
		}
		uint8_t encoded_buffer[16];
		uint8_t encoded_size = COBSencode(raw_buffer, 15, encoded_buffer);
		Serial2.write(encoded_buffer, encoded_size);
		Serial2.write(0x00);

		if (discstats.local_packets_out_per_second < 255){
			discstats.local_packets_out_per_second++;
		}

	}
	if (GloveSend.check()){


		uint8_t raw_buffer[10];

		raw_buffer[0] = glove0.output_rgb_led.r;
		raw_buffer[1] = glove0.output_rgb_led.g;
		raw_buffer[2] = glove0.output_rgb_led.b;
		raw_buffer[3] = glove0.output_white_led;
		raw_buffer[4] = glove1.output_rgb_led.r;
		raw_buffer[5] = glove1.output_rgb_led.g;
		raw_buffer[6] = glove1.output_rgb_led.b;
		raw_buffer[7] = glove1.output_white_led;

		raw_buffer[8] = OneWire::crc8(raw_buffer, 8);
		uint8_t encoded_buffer[16];
		uint8_t encoded_size = COBSencode(raw_buffer, 9, encoded_buffer);


		Serial1.write(encoded_buffer, encoded_size);
		Serial1.write(0x00);

		if (glovestats.local_packets_out_per_second < 255){
			glovestats.local_packets_out_per_second++;
		}
	}

	//empty buffer up to 5 times to catch up after an error

	for (uint8_t i = 0; i < 5; i++){
		while (Serial1.available()){

			//read in a byte
			incoming1_raw_buffer[incoming1_index] = Serial1.read();

			//check for end of packet
			if (incoming1_raw_buffer[incoming1_index] == 0x00){
				//try to decode
				uint8_t decoded_buffer[BUFFER_SIZE];
				uint8_t decoded_length = COBSdecode(incoming1_raw_buffer, incoming1_index, decoded_buffer);

				//check length of decoded data (cleans up a series of 0x00 bytes)
				if (decoded_length > 0){
					onPacket1(decoded_buffer, decoded_length);
				}

				//reset index
				incoming1_index = 0;
			}
			else{
				//read data in until we hit overflow then start over
				incoming1_index++;
				if (incoming1_index == BUFFER_SIZE) incoming1_index = 0;
			}
		}

		while (Serial2.available()){

			//read in a byte
			incoming2_raw_buffer[incoming2_index] = Serial2.read();

			//check for end of packet
			if (incoming2_raw_buffer[incoming2_index] == 0x00){
				//try to decode

				uint8_t decoded_buffer[BUFFER_SIZE];
				uint8_t decoded_length = COBSdecode(incoming2_raw_buffer, incoming2_index, decoded_buffer);

				//check length of decoded data (cleans up a series of 0x00 bytes)
				if (decoded_length > 0){
					onPacket2(decoded_buffer, decoded_length);
				}

				//reset index
				incoming2_index = 0;
			}
			else{
				//read data in until we hit overflow then start over
				incoming2_index++;
				if (incoming2_index == BUFFER_SIZE) incoming2_index = 0;

			}
		}



		while (Serial3.available()){

			//read in a byte
			incoming3_raw_buffer[incoming3_index] = Serial3.read();

			//check for end of packet
			if (incoming3_raw_buffer[incoming3_index] == 0x00){
				//try to decode

				uint8_t decoded_buffer[BUFFER_SIZE];
				uint8_t decoded_length = COBSdecode(incoming3_raw_buffer, incoming3_index, decoded_buffer);

				//check length of decoded data (cleans up a series of 0x00 bytes)
				if (decoded_length > 0){
					onPacket3(decoded_buffer, decoded_length);
				}

				//reset index
				incoming3_index = 0;
			}
			else{
				//read data in until we hit overflow then start over
				incoming3_index++;
				if (incoming3_index == BUFFER_SIZE) incoming3_index = 0;

			}
		}
	}
}


inline void onPacket2(const uint8_t* buffer, size_t size)
{

	if (size != 11){
		discstats.local_framing_errors++;
	}
	else{
		uint8_t crc = OneWire::crc8(buffer, size - 1);
		if (crc != buffer[size - 1]){
			discstats.local_crc_errors++;
		}
		else{


			if (discstats.local_packets_in_per_second < 255){
				discstats.local_packets_in_per_second++;
			}

			disc0.current_index = buffer[0];
			disc0.current_mag = buffer[1];
			disc0.requested_mode = buffer[2];  //upper 4 bits are a counter, lower 4 are data

			disc0.cpu_usage = buffer[5];

			disc0.battery_voltage = buffer[6];
			disc0.cpu_temp = buffer[7];
			disc0.crc_errors = buffer[8];
			disc0.framing_errors = buffer[9];

			//upper 4 bits are a counter, lower 4 are data
			if (disc0.last_requested_mode != disc0.requested_mode){

				//reset colors if flipped
				if (disc0.requested_mode == DISC_MODE_IDLE)  disc0.active_primary = true;

				if (disc0.disc_mode == DISC_MODE_OFF && (disc0.requested_mode & 0x0F) == DISC_MODE_OPENING && menu_mode != MENU_LOCKED) {
					current_brightness = 255;
					startup_time = millis();
					for (uint8_t current_pixel = 0; current_pixel < 38; current_pixel++) {
						stream1[current_pixel] = CHSV(0, 0, 0);
						stream2[current_pixel] = CHSV(0, 0, 0);
					}
					disc0.packet_beam = 0;
					startup_mode = STARTUP_MODE_DELAY;
					background_mode = BACKGROUND_MODE_NOISE;
					z_noise = 127; //very random background noise
					if (!cust_Startup_color){
						color1 = CHSV(HUE_AQUA, 255, 255); // cyan
						color2 = CHSV(HUE_AQUA, 255, 255);
					}
					cust_Startup_color = false;
				}

				disc0.disc_mode = disc0.requested_mode & 0x0F;
				disc0.last_requested_mode = disc0.requested_mode;

			}
			//Serial.println(disc0.disc_mode);
			//read glove

			//spin code
			if (menu_mode == MENU_SPIN){
				if (leading_glove == 0)  glove_spin((void*)&glove0, (void*)&glove1);
				else 	glove_spin((void*)&glove1, (void*)&glove0);
			}

			uint8_t temp = 29 - scale8(disc0.current_index, 30);
			//swiping control code
			if (disc0.disc_mode == DISC_MODE_SWIPE && disc0.current_mag < 16){

				if (disc0.last_index != temp){
					//color spincode ccw

					if ((disc0.last_index + 1) % 30 == temp){
						//Serial.println("cw");
						if (!disc0.active_primary){
							color1.h = color1.h + 1;
							color1.v = qadd8(color1.v, 2);
							color1.s = qadd8(color1.s, 2);

						}
						else{
							color2.h = color2.h + 1;
							color2.v = qadd8(color2.v, 2);
							color2.s = qadd8(color2.s, 2);
						}
					}
					else if ((disc0.last_index + 29) % 30 == temp){
						if (!disc0.active_primary){
							color1.h = color1.h - 1;
							color1.v = qadd8(color1.v, 2);
							color1.s = qadd8(color1.s, 2);
						}
						else{
							color2.h = color2.h - 1;
							color2.v = qadd8(color2.v, 2);
							color2.s = qadd8(color2.s, 2);
						}
						//Serial.println("ccw");
					}
					//swap active colors
					else if (abs(disc0.last_index - temp) > 10 && abs(disc0.last_index - temp) < 20){
						//Serial.println("flip");
						//supress flip in hand mode
						if (menu_mode != MENU_SPIN && !glove1.finger1  && !glove1.finger2 && !glove0.finger1 && !glove0.finger2){

							disc0.active_primary = !disc0.active_primary;
						}
					}
				}
			}
			else{ //normal idle mode

			}
			disc0.last_index = temp;


		}
	}
}

inline void onPacket3(const uint8_t* buffer, size_t size)
{

	if (size != 2 && size != 7 && size != 71 && size != 91){
		bluetoothstats.local_framing_errors++;
	}
	else{
		uint8_t crc = OneWire::crc8(buffer, size - 1);
		if (crc != buffer[size - 1]){
			bluetoothstats.local_crc_errors++;
		}
		else{
			if (bluetoothstats.local_packets_in_per_second < 255){
				bluetoothstats.local_packets_in_per_second++;
			}


			if (size == 7){
				//packet confirmation
				Serial3.write(0x00);
				Serial3.write(0xFF);
				Serial3.write(0x00);

				color1 = rgb2hsv_approximate(CRGB(buffer[0], buffer[1], buffer[2]));
				color2 = rgb2hsv_approximate(CRGB(buffer[3], buffer[4], buffer[5]));

			}
			else if (size == 71){
				//packet confirmation
				Serial3.write(0x00);
				Serial3.write(0xFF);
				Serial3.write(0x00);

				memcpy(&back_sms[0], &buffer[0], 70);

			}
			else if (size == 91){
				//packet confirmation
				Serial3.write(0x00);
				Serial3.write(0xFF);
				Serial3.write(0x00);

				memcpy(&back_sms[70], &buffer[0], 90);

				//tighten up the null terminator
				int i = 160;
				while (i > 0){
					if (back_sms[--i] != 0x20) break;
				}
				back_sms[++i] = 0;

				//swap back and front buffers
				char * temp = front_sms;
				front_sms = back_sms;
				back_sms = temp;

				//make message live
				live_txt = front_sms;

				//ring for a new message
				ring_timer = millis();
				menu_mode = MENU_TXT_ON; //force into sms display
			}
			else if (size == 2){
				if (buffer[0] == 0x00){
					uint8_t raw_buffer[128];

					CRGB temp_color;
					hsv2rgb_rainbow(color1, temp_color);
					raw_buffer[0] = temp_color.r;
					raw_buffer[1] = temp_color.g;
					raw_buffer[2] = temp_color.b;
					hsv2rgb_rainbow(color2, temp_color);
					raw_buffer[3] = temp_color.r;
					raw_buffer[4] = temp_color.g;
					raw_buffer[5] = temp_color.b;

					raw_buffer[6] = glove0.cpu_usage;
					raw_buffer[7] = glove1.cpu_usage;
					raw_buffer[8] = disc0.cpu_usage;
					raw_buffer[9] = (int8_t)(average_time / 100); //cpu usage

					raw_buffer[10] = glove0.cpu_temp;
					raw_buffer[11] = glove1.cpu_temp;
					raw_buffer[12] = disc0.cpu_temp;
					raw_buffer[13] = (uint8_t)(temperature * 100);

					raw_buffer[14] = disc0.battery_voltage;
					raw_buffer[15] = (uint8_t)(voltage * 100);

					raw_buffer[16] = glovestats.saved_local_packets_in_per_second_glove0;
					raw_buffer[17] = glovestats.saved_local_packets_in_per_second_glove1;
					raw_buffer[18] = glovestats.saved_local_packets_out_per_second;
					raw_buffer[19] = (glovestats.total_lost_packets & 0xff);

					raw_buffer[20] = discstats.saved_local_packets_in_per_second;
					raw_buffer[21] = discstats.saved_local_packets_out_per_second;
					raw_buffer[22] = (discstats.total_lost_packets & 0xff);

					raw_buffer[23] = (((glove0.yaw_compensated) >> 8) & 0xff);
					raw_buffer[24] = (((glove0.yaw_compensated) >> 0) & 0xff);
					raw_buffer[25] = (((glove0.pitch_compensated) >> 8) & 0xff);
					raw_buffer[26] = (((glove0.pitch_compensated) >> 0) & 0xff);
					raw_buffer[27] = (((glove0.roll_compensated) >> 8) & 0xff);
					raw_buffer[28] = (((glove0.roll_compensated) >> 0) & 0xff);

					raw_buffer[29] = (((glove1.yaw_compensated) >> 8) & 0xff);
					raw_buffer[30] = (((glove1.yaw_compensated) >> 0) & 0xff);
					raw_buffer[31] = (((glove1.pitch_compensated) >> 8) & 0xff);
					raw_buffer[32] = (((glove1.pitch_compensated) >> 0) & 0xff);
					raw_buffer[33] = (((glove1.roll_compensated) >> 8) & 0xff);
					raw_buffer[34] = (((glove1.roll_compensated) >> 0) & 0xff);

					raw_buffer[35] = 0x00;

					if (glove0.finger1) bitSet(raw_buffer[35], 0);
					if (glove0.finger2) bitSet(raw_buffer[35], 1);
					if (glove0.finger3) bitSet(raw_buffer[35], 2);
					if (glove0.finger4) bitSet(raw_buffer[35], 3);
					if (glove1.finger1) bitSet(raw_buffer[35], 4);
					if (glove1.finger2) bitSet(raw_buffer[35], 5);
					if (glove1.finger3) bitSet(raw_buffer[35], 6);
					if (glove1.finger4) bitSet(raw_buffer[35], 7);

					raw_buffer[36] = OneWire::crc8(raw_buffer, 36);

					Serial3.write(raw_buffer, 37);

				}
			}

		}
	}
}


inline void onPacket1(const uint8_t* buffer, size_t size)
{

	if (size != 22){
		glovestats.local_framing_errors++;
	}
	else{
		uint8_t crc = OneWire::crc8(buffer, size - 1);
		if (crc != buffer[size - 1]){
			glovestats.local_crc_errors++;
		}
		else{

			GLOVE * current_glove;
			if bitRead(buffer[14], 7){  //bit 0-3 are fingers, bit 7 is glove ID.  bit 4 5 and 6 are unused.
				current_glove = &glove1;
				if (glovestats.local_packets_in_per_second_glove1 < 255){
					glovestats.local_packets_in_per_second_glove1++;
				}

			}
			else
			{
				current_glove = &glove0;
				if (glovestats.local_packets_in_per_second_glove0 < 255){
					glovestats.local_packets_in_per_second_glove0++;
				}
			}

			current_glove->yaw_raw = buffer[0] << 8 | buffer[1];
			current_glove->pitch_raw = buffer[2] << 8 | buffer[3];
			current_glove->roll_raw = buffer[4] << 8 | buffer[5];

			current_glove->yaw_compensated = current_glove->yaw_offset - current_glove->yaw_raw;
			current_glove->pitch_compensated = current_glove->pitch_offset - current_glove->pitch_raw;
			current_glove->roll_compensated = current_glove->roll_offset - current_glove->roll_raw;

			//center data on 18000, meaning 180.00 degrees
			current_glove->yaw_compensated = (current_glove->yaw_compensated + 2 * 36000 + 18000) % 36000;
			current_glove->pitch_compensated = (current_glove->pitch_compensated + 2 * 36000 + 18000) % 36000;
			current_glove->roll_compensated = (current_glove->roll_compensated + 2 * 36000 + 18000) % 36000;

			current_glove->color_sensor_R = buffer[6] << 8 | buffer[7];
			current_glove->color_sensor_G = buffer[8] << 8 | buffer[9];
			current_glove->color_sensor_B = buffer[10] << 8 | buffer[11];
			current_glove->color_sensor_Clear = buffer[12] << 8 | buffer[13];

			current_glove->finger1 = false;
			current_glove->finger2 = false;
			current_glove->finger3 = false;
			current_glove->finger4 = false;
			//only allow one finger at a time
			if ((buffer[14] & 0x0F) == 0x0E){
				current_glove->finger1 = true;
				current_glove->gesture_finger = 1;
			}
			else if ((buffer[14] & 0x0F) == 0x0D){
				current_glove->finger2 = true;
				current_glove->gesture_finger = 2;
			}
			else if ((buffer[14] & 0x0F) == 0x0B){
				current_glove->finger3 = true;
				current_glove->gesture_finger = 3;
			}
			else if ((buffer[14] & 0x0F) == 0x07){
				current_glove->finger4 = true;
				current_glove->gesture_finger = 4;
			}

			//figure out who the leader is
			if (current_glove == &glove1){
				if (((buffer[14] & 0x0F) != 0x0F) && (glove0.finger1 == 0 && glove0.finger2 == 0 && glove0.finger3 == 0 && glove0.finger4 == 0)){
					leading_glove = 1;
				}
			}
			else{
				if (((buffer[14] & 0x0F) != 0x0F) && (glove1.finger1 == 0 && glove1.finger2 == 0 && glove1.finger3 == 0 && glove1.finger4 == 0)){
					leading_glove = 0;
				}
			}

			current_glove->packets_in_per_second = buffer[15];
			current_glove->packets_out_per_second = buffer[16];

			current_glove->framing_errors = buffer[17];
			current_glove->crc_errors = buffer[18];

			current_glove->cpu_usage = buffer[19];
			current_glove->cpu_temp = buffer[20];

			//update the gesture grid (8x8 grid + overlap)
			int temp_gloveX_saved = 0;
			int temp_gloveX = 0;

			if (current_glove == &glove1){
				temp_gloveX_saved = (current_glove->yaw_compensated - 18000) - ((current_glove->pitch_compensated - 18000)*abs(sin((current_glove->roll_raw - 18000)* PI / 18000))) + 18000;
			}
			else{
				temp_gloveX_saved = (current_glove->yaw_compensated - 18000) + ((current_glove->pitch_compensated - 18000)*abs(sin((current_glove->roll_raw - 18000)* PI / 18000))) + 18000;
			}
			temp_gloveX = map(temp_gloveX_saved, 18000 - GLOVE_DEADZONE, 18000 + GLOVE_DEADZONE, 0, 8);

			int temp_gloveY = map((current_glove->pitch_compensated), 18000 - GLOVE_DEADZONE, 18000 + GLOVE_DEADZONE, 0, 8);
			temp_gloveY = constrain(temp_gloveY, 0, 7);

			if (current_glove == &glove0){
				temp_gloveX = constrain(temp_gloveX, -8, 7);
				//only update position if not at the edge
				if (temp_gloveX > -8 && temp_gloveX < 7){
					glove0.gloveY = temp_gloveY;
				}
			}

			if (current_glove == &glove1){
				temp_gloveX = constrain(temp_gloveX, 0, 15);

				//only update position if not at the edge
				if (temp_gloveX > 0 && temp_gloveX < 15){
					glove1.gloveY = temp_gloveY;
				}
			}

			if (temp_gloveY>0 && temp_gloveY < 7){
				current_glove->gloveX = temp_gloveX;
			}

			//calc disc circle location
			temp_gloveX = map(temp_gloveX_saved, 18000 - GLOVE_DEADZONE, 18000 + GLOVE_DEADZONE, -1024, 1024);

			temp_gloveY = map((current_glove->pitch_compensated), 18000 - GLOVE_DEADZONE, 18000 + GLOVE_DEADZONE, -1024, 1024);

			//CHECK THIS FOR X Y STUFF
			int angle = atan2(-(current_glove->pitch_compensated - 18000), -(temp_gloveX_saved - 18000)) * 180 / PI;
			angle = (angle + 360 + 90) % 360;

			//magnitude for spinning 
			current_glove->magnitude = sqrt(temp_gloveX*temp_gloveX + temp_gloveY*temp_gloveY);
			//Serial.println(current_glove->magnitude);
			//only update disc_offset if mag is high enough (ADJUST THIS)
			if (current_glove->magnitude > 300){
				current_glove->disc_offset = constrain(map(angle, 0, 361, 0, 255), 0, 255);
			}


			//y axis only
			current_glove->y_angle = constrain(map((current_glove->pitch_compensated), 0, 36000, -64, 64), -32, 32);

			current_glove->y_angle_noise = constrain(map((current_glove->pitch_compensated), 0, 36000, -256, 256), -128, 128);

			//pong
			if (current_glove == &glove1){
				pong_paddle_l = constrain(map((current_glove->pitch_compensated), 0, 36000, -32, 32), -2, 9);
			}
			else{
				pong_paddle_r = constrain(map((current_glove->pitch_compensated), 0, 36000, -32, 32), -2, 9);

			}

			//noise
			if (menu_mode == MENU_HELMET_NOISE_ON){
				if (leading_glove == 0)  glove_noise((void*)&glove0, (void*)&glove1);
				else 	glove_noise((void*)&glove1, (void*)&glove0);
			}

			//brightness
			if (menu_mode == MENU_PWR_IN){
				if (leading_glove == 0)  glove_pwr((void*)&glove0, (void*)&glove1);
				else 	glove_pwr((void*)&glove1, (void*)&glove0);


			}

			if (!current_glove->finger1 &&  !current_glove->finger2 && !current_glove->finger3 && !current_glove->finger4 && current_glove->gesture_in_progress == true){
				
				if(current_glove->gloveY <= 0)      menu_map(HAND_DIRECTION_DOWN);
				else if (current_glove->gloveY >= 7) menu_map(HAND_DIRECTION_UP);
				else if (current_glove->gloveX <= 0) menu_map(HAND_DIRECTION_RIGHT);
				else if (current_glove->gloveX >= 7) menu_map(HAND_DIRECTION_LEFT);
				else {
					if (current_glove->gesture_finger == 4) menu_map(HAND_BACK);
					//leading gloves count only
					if ((current_glove == &glove1 && leading_glove == 1) || (current_glove == &glove0 && leading_glove == 0)){

						current_glove->finger_timer2 = millis();  //reset the press timer to stop counting
						if (millis() - current_glove->finger_timer < 200) current_glove->finger_presses++;  //presses under 200ms count
						Serial.println(current_glove->finger_presses);
					}
				}
			}

			if (millis() - current_glove->finger_timer2 > 300){ //max 300ms between presses then process it
				if (current_glove->gesture_finger != 4){
					if (current_glove->finger_presses == 1)	menu_map(HAND_SHORT_PRESS_SINGLE);
					else if (current_glove->finger_presses == 2) menu_map(HAND_SHORT_PRESS_DOUBLE);
					else if (current_glove->finger_presses == 3) menu_map(HAND_SHORT_PRESS_TRIPLE);
				}
				else{
					if (current_glove->finger_presses >= 3) menu_map(HAND_SUIT_OFF);
				}
				current_glove->finger_presses = 0;
			}
			//if a single finger is pressed down....
			if (current_glove->finger1 || current_glove->finger2 || current_glove->finger3 || current_glove->finger4){
				if (current_glove->gesture_in_progress == false){
					current_glove->finger_timer = millis();
					current_glove->gesture_in_progress = true;
					current_glove->yaw_offset = current_glove->yaw_raw;
					current_glove->pitch_offset = current_glove->pitch_raw;
					current_glove->roll_offset = current_glove->roll_raw;
				}
			}
			else current_glove->gesture_in_progress = false;


			//toggle opposing glove's camera off and on
			if (menu_mode == MENU_CAMON){
				if (current_glove->finger1 || current_glove->finger2){
					if (current_glove->camera_button_press_handled == false){
						if (current_glove == &glove0){
							//wipe array on turn on
							if (!glove1.camera_on){
								for (uint8_t i = 0; i < 38; i++){
									glove1.cameraflow[i] = CHSV(0, 0, 0);
								}
							}
							else{
								//on exit save color
								if (glove1.cameraflow[glove1.cameraflow_index] != CHSV(0, 0, 0)){
									color2 = glove1.cameraflow[glove1.cameraflow_index];
								}
							}
							glove1.camera_on = !(glove1.camera_on);
						}
						if (current_glove == &glove1){
							//wipe array on turn on
							if (!glove0.camera_on){
								for (uint8_t i = 0; i < 38; i++){
									glove0.cameraflow[i] = CHSV(0, 0, 0);
								}
							}
							else{
								//on exit save color
								if (glove0.cameraflow[glove0.cameraflow_index] != CHSV(0, 0, 0)){
									color1 = glove0.cameraflow[glove0.cameraflow_index];
								}
							}
							glove0.camera_on = !(glove0.camera_on);
						}
						current_glove->camera_button_press_handled = true;
					}
				}
				else{
					current_glove->camera_button_press_handled = false;
				}
			}
			else{
				current_glove->camera_button_press_handled = false;
				glove1.camera_on = false;
				glove0.camera_on = false;
			}


			if (current_glove->camera_on){

				current_glove->output_white_led = 0x01;

				uint32_t sum = current_glove->color_sensor_Clear;
				//Serial.println(sum);
				if (sum > 3000 && sum < 20000){

					float r, g, b;
					r = current_glove->color_sensor_R; r /= sum;
					g = current_glove->color_sensor_G; g /= sum;
					b = current_glove->color_sensor_B; b /= sum;
					r *= 256; g *= 256; b *= 256;

					current_glove->output_hsv_led = rgb2hsv_approximate(CRGB(r, g, b));
					//temp.v = min(temp.v, 200);
					//temp.s = max(temp.s, 192);

					hsv2rgb_rainbow(current_glove->output_hsv_led, current_glove->output_rgb_led);

				}
				else{
					current_glove->output_rgb_led = CRGB(0, 0, 0);
					current_glove->output_hsv_led = CHSV(0, 0, 0);
				}

				if (millis() - current_glove->camera_advance > 50){
					current_glove->camera_advance = millis();
					current_glove->cameraflow_index = (current_glove->cameraflow_index + 1) % 38;
					current_glove->cameraflow[current_glove->cameraflow_index] = current_glove->output_hsv_led;
				}

			}
			else{
				//current_glove->camera_entering = true;
				current_glove->output_white_led = 0x00;
				current_glove->output_rgb_led = CRGB(0, 0, 0);
			}


		}
	}
}

//use void pointer to bypass arduino compiler weirdness
void glove_spin(void *  first_glove, void *  second_glove){

	if (((GLOVE *)first_glove)->finger1 || ((GLOVE *)first_glove)->finger2){
		if (MENU_SWIPE_entering == true){
			disc0.disc_mode = DISC_MODE_SWIPE;
			MENU_SWIPE_entering = false;
			if (millis() - disc0.taptimer < 300 && disc0.current_mag == 16) disc0.active_primary = !disc0.active_primary;
			disc0.taptimer = millis();
		}
		disc0.inner_offset_requested = ((GLOVE *)first_glove)->disc_offset;
		disc0.outer_offset_requested = ((GLOVE *)first_glove)->disc_offset;
		if (((GLOVE *)second_glove)->finger1 || ((GLOVE *)second_glove)->finger2){
			//map magnitude
			if (disc0.mag_saved == false){
				disc0.saved_mag = disc0.current_mag;
				disc0.mag_saved = true;

			}
			disc0.inner_magnitude_requested = constrain(disc0.saved_mag + ((GLOVE *)second_glove)->y_angle, 0, 16);
		}
		else{
			disc0.mag_saved = false;
		}
	}
	else{
		MENU_SWIPE_entering = true;
	}
}

//use void pointer to bypass arduino compiler weirdness
void glove_noise(void *  first_glove, void *  second_glove){

	if (((GLOVE *)first_glove)->finger1 || ((GLOVE *)first_glove)->finger2){
		if (noise_xy_entered == false){
			x_noise_modifier_initial = x_noise_modifier;
			y_noise_modifier_initial = y_noise_modifier;
			noise_xy_entered = true;
		}

		x_noise_modifier = x_noise_modifier_initial - ((((GLOVE *)first_glove)->pitch_compensated - 18000)) / 10;
		y_noise_modifier = y_noise_modifier_initial - ((((GLOVE *)first_glove)->yaw_compensated - 18000)) / 10;
		if (((GLOVE *)second_glove)->finger1 || ((GLOVE *)second_glove)->finger2){
			//map magnitude
			if (noise_z_entered == false){
				z_noise_modifier_initial = z_noise_modifier;
				noise_z_entered = true;

			}
			z_noise_modifier = constrain(z_noise_modifier_initial + (((GLOVE *)second_glove)->y_angle_noise), 0, 128);
		}
		else{
			noise_z_entered = false;
		}
	}
	else{
		noise_xy_entered = false;
	}
}



//use void pointer to bypass arduino compiler weirdness
void glove_pwr(void *  first_glove, void *  second_glove){

	if (((GLOVE *)first_glove)->finger1 || ((GLOVE *)first_glove)->finger2){
		if (brightness_entered == false){
			disc_turned_off = false;
			disc0.disc_mode = DISC_MODE_IDLE;
			brightness_initial = current_brightness;
			brightness_entered = true;
			//starting from off, force modes!
			if (brightness_initial == 0){
				supress_helmet = true;
				background_mode = BACKGROUND_MODE_NOISE;
				z_noise = 127;
			}
		}

		if ((((GLOVE *)first_glove)->y_angle_noise) * 3 > 20){
			current_brightness = constrain(brightness_initial - 20 + (((GLOVE *)first_glove)->y_angle_noise) * 3, 0, 255);
		}
		else if ((((GLOVE *)first_glove)->y_angle_noise) * 3 < -20){
			current_brightness = constrain(brightness_initial + 20 + (((GLOVE *)first_glove)->y_angle_noise) * 3, 0, 255);
		}



	}
	else{
		supress_helmet = false;
		brightness_entered = false;
		if (!disc_turned_off && current_brightness == 0){
			disc0.disc_mode = DISC_MODE_OFF;
			disc_turned_off = true;
		}
	}
}
