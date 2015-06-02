inline void SerialUpdate(void){
	if (DiscSend3.check()){

		disc0.packet_beam--;

		//calculate current pixels and add to array
		if (disc0.disc_mode == DISC_MODE_SWIPE){
			//to disc
			disc0.color1 = color1;
			disc0.color2 = color2;
		}
		else{
			if (last_discwave != discwave){
				disc0.color1 = map_hsv(discwave, 0, 16, &color1, &color2);
				disc0.color2 = map_hsv(discwave, 0, 16, &color1, &color2);

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
		raw_buffer[10] = disc0.fade_level; //disc0.fade_level;  //fade

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
				disc0.disc_mode = disc0.requested_mode & 0x0F;
				disc0.last_requested_mode = disc0.requested_mode;
				Serial.println(disc0.disc_mode);
			}

			//read glove
			if ((glove1.finger1 || glove1.finger2) && menu_mode == MENU_MAG){

				if (MENU_MAG_entering == true){
					disc0.disc_mode = 3;
					MENU_MAG_entering_starting_inner = disc0.inner_magnitude_requested;
					MENU_MAG_entering = false;
				}
				if (leading_glove == 1){
					disc0.inner_magnitude_requested = glove1.calculated_mag;
				}
				else{
					disc0.inner_magnitude_requested = glove0.calculated_mag;
				}
			}
			else{

				MENU_MAG_entering = true;
			}



			if ((glove1.finger1 || glove1.finger2) && menu_mode == MENU_SPIN){
				disc0.inner_offset_requested = glove1.disc_offset;
				disc0.outer_offset_requested = glove1.disc_offset;
			}

			uint8_t temp = 29 - scale8(disc0.current_index, 30);
			//swiping control code
			if (disc0.disc_mode == DISC_MODE_SWIPE){

				if (disc0.last_index != temp){
					//color spincode ccw

					if ((disc0.last_index + 1) % 30 == temp){
						Serial.println("cw");
						if (!disc0.active_primary){
							color1.h = color1.h + 1;
							color1.v =qadd8(color1.v, 2);
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
						Serial.println("ccw");
					}
					//swap active colors
					else if (abs(disc0.last_index - temp) > 10 && abs(disc0.last_index - temp) < 20){
						Serial.println("flip");
						disc0.active_primary = !disc0.active_primary;
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

				color1 = rgb2hsv_rainbow(CRGB(buffer[0], buffer[1], buffer[2]));
				color2 = rgb2hsv_rainbow(CRGB(buffer[3], buffer[4], buffer[5]));

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


			//only allow one finger at a time
			if ((buffer[14] & 0x0F) == 0x0E){
				current_glove->finger1 = true;
				current_glove->finger2 = false;
				current_glove->finger3 = false;
				current_glove->finger4 = false;
				current_glove->gesture_finger = 1;
			}
			else if ((buffer[14] & 0x0F) == 0x0D){
				current_glove->finger1 = false;
				current_glove->finger2 = true;
				current_glove->finger3 = false;
				current_glove->finger4 = false;
				current_glove->gesture_finger = 2;
			}
			else if ((buffer[14] & 0x0F) == 0x0B){
				current_glove->finger1 = false;
				current_glove->finger2 = false;
				current_glove->finger3 = true;
				current_glove->finger4 = false;
				current_glove->gesture_finger = 3;
			}
			else if ((buffer[14] & 0x0F) == 0x07){
				current_glove->finger1 = false;
				current_glove->finger2 = false;
				current_glove->finger3 = false;
				current_glove->finger4 = true;
				current_glove->gesture_finger = 4;
			}
			else{
				current_glove->finger1 = false;
				current_glove->finger2 = false;
				current_glove->finger3 = false;
				current_glove->finger4 = false;
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

			//update the gesture grid (8x8 grid + overlap 
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

			int angle = atan2(temp_gloveY, temp_gloveX) * 180 / PI;
			angle = (angle + 360 + 90) % 360;

			current_glove->disc_offset = constrain(map(angle, 0, 361, 0, 30), 0, 29);

			current_glove->magnitude = sqrt(temp_gloveX*temp_gloveX + temp_gloveY*temp_gloveY);

			// calc mag stuff
			temp_gloveY = map((current_glove->pitch_compensated), 18000 - GLOVE_DEADZONE, 18000 + GLOVE_DEADZONE, 0, 32);
			current_glove->calculated_mag = constrain(temp_gloveY, -16, 16);






			if (!current_glove->finger1 &&  !current_glove->finger2 && !current_glove->finger3 && current_glove->gesture_in_progress == true){
				if (current_glove->gloveY <= 0)      menu_map(HAND_DIRECTION_DOWN);
				else if (current_glove->gloveY >= 7) menu_map(HAND_DIRECTION_UP);
				else if (current_glove->gloveX <= 0) menu_map(HAND_DIRECTION_RIGHT);
				else if (current_glove->gloveX >= 7) menu_map(HAND_DIRECTION_LEFT);
				else if (millis() - current_glove->finger_timer < 200) menu_map(HAND_DIRECTION_SHORT_PRESS);

				//disable scroll mode on all other fingers
				//if (current_glove->gesture_finger != 1) scroll_mode = SCROLL_MODE_COMPLETE;
			}

			//if a single finger is pressed down....
			if (current_glove->finger1 || current_glove->finger2 || current_glove->finger3){
				if (current_glove->gesture_in_progress == false){
					current_glove->finger_timer = millis();
					current_glove->gesture_in_progress = true;
					current_glove->yaw_offset = current_glove->yaw_raw;
					current_glove->pitch_offset = current_glove->pitch_raw;
					current_glove->roll_offset = current_glove->roll_raw;
				}
			}
			else current_glove->gesture_in_progress = false;

		}
	}
}

uint8_t combine3(uint8_t input1, uint8_t input2, uint8_t input3){
	return (ctoi(input1) * 100) + (ctoi(input2) * 10) + ctoi(input3);
}


uint8_t ctoi(char input){
	switch (input) {
	case '1':		return 1;
	case '2':		return 2;
	case '3':		return 3;
	case '4':		return 4;
	case '5':		return 5;
	case '6':		return 6;
	case '7':		return 7;
	case '8':		return 8;
	case '9':		return 9;
	default:		return 0;
	}
}

CHSV rgb2hsv_rainbow(CRGB input_color){

	CHSV output_color;

	uint8_t maximum = max(input_color.r, max(input_color.g, input_color.b));
	uint8_t minimum = min(input_color.r, min(input_color.g, input_color.b));

	output_color.v = maximum;
	uint8_t delta = maximum - minimum;

	output_color.s = (maximum == 0) ? 0 : (255 * delta / maximum);

	if (maximum == minimum) {
		output_color.h = 0;
	}
	else {
		if (maximum == input_color.r){
			//Serial.println("red is max");
			if (input_color.g >= input_color.b){
				if ((input_color.r - input_color.g) <= (delta / 3)){
					//Serial.println("32-64");
					output_color.h = map((input_color.r - input_color.g), delta / 3, 0, 32, 64);
				}
				else{
					//Serial.println("0-32");
					output_color.h = map((input_color.r - input_color.g), delta, delta / 3, 0, 32);
				}
			}
			else{
				//Serial.println("208-255");
				output_color.h = map((input_color.r - input_color.b), 0, delta, 208, 255);
			}
		}
		else if (maximum == input_color.g){
			//Serial.println("green is max");
			if (input_color.r >= input_color.b){
				//Serial.println("64-96");
				output_color.h = map((input_color.g - input_color.r), 0, delta, 64, 96);
			}
			else{
				if (input_color.g - input_color.b <= delta / 3){
					//Serial.println("96-128");
					output_color.h = map((input_color.g - input_color.b), delta, delta / 3, 96, 128);
				}
				else{
					//Serial.println("128-132?");
					output_color.h = map((input_color.g - input_color.b), delta / 3, delta, 128, 160);
				}
			}
		}
		else if (maximum == input_color.b){
			//Serial.println("blue is max");
			if (input_color.g > input_color.r){
				//Serial.println("132?-160");
				output_color.h = map((input_color.b - input_color.g), -delta / 3, delta, 128, 160);
			}
			else{
				//Serial.println("160-208");
				output_color.h = map((input_color.b - input_color.r), delta, 0, 160, 208);
			}
		}
	}

	return output_color;
}