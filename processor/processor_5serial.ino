inline void SerialUpdate(void){
	if (GloveSend.check()){

		uint8_t raw_buffer[14];

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
		raw_buffer[10] = 128; //disc0.fade_level;  //fade
		raw_buffer[11] = 0x00;  //mode
		raw_buffer[12] = disc0.packet_sequence_number;  //sequence
		raw_buffer[13] = disc0.packet_beam;  //sequence
		raw_buffer[14] = OneWire::crc8(raw_buffer, 13);

		if (disc0.packet_sequence_number > 29){
			disc0.packet_sequence_number = 0;
		}
		uint8_t encoded_buffer[16];
		uint8_t encoded_size = COBSencode(raw_buffer, 15, encoded_buffer);

		Serial2.write(encoded_buffer, encoded_size);
		Serial2.write(0x00);
		if (serial2stats.packets_out_per_second < 255){
			serial2stats.packets_out_per_second++;
		}

		raw_buffer[9];

		raw_buffer[0] = glove0.output_rgb_led.r;
		raw_buffer[1] = glove0.output_rgb_led.g;
		raw_buffer[2] = glove0.output_rgb_led.b;
		raw_buffer[3] = glove0.output_white_led;
		raw_buffer[4] = glove1.output_rgb_led.r;
		raw_buffer[5] = glove1.output_rgb_led.g;
		raw_buffer[6] = glove1.output_rgb_led.b;
		raw_buffer[7] = glove1.output_white_led;

		raw_buffer[8] = OneWire::crc8(raw_buffer, 7);

		// encoded_buffer[9];
		encoded_size = COBSencode(raw_buffer, 9, encoded_buffer);


		Serial1.write(encoded_buffer, encoded_size);
		Serial1.write(0x00);

		if (serial1stats.packets_out_per_second < 255){
			serial1stats.packets_out_per_second++;
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
				uint8_t decoded_length = COBSdecode(incoming1_raw_buffer, incoming1_index, incoming1_decoded_buffer);

				//check length of decoded data (cleans up a series of 0x00 bytes)
				if (decoded_length > 0){
					onPacket1(incoming1_decoded_buffer, decoded_length);
				}

				//reset index
				incoming1_index = 0;
			}
			else{
				//read data in until we hit overflow then start over
				if (incoming1_index < INCOMING1_BUFFER_SIZE) incoming1_index++;
				else incoming1_index = 0;
			}
		}

		while (Serial2.available()){

			//read in a byte
			incoming2_raw_buffer[incoming2_index] = Serial2.read();

			//check for end of packet
			if (incoming2_raw_buffer[incoming2_index] == 0x00){
				//try to decode
				uint8_t decoded_length = COBSdecode(incoming2_raw_buffer, incoming2_index, incoming2_decoded_buffer);

				//check length of decoded data (cleans up a series of 0x00 bytes)
				if (decoded_length > 0){
					onPacket2(incoming2_decoded_buffer, decoded_length);
				}

				//reset index
				incoming2_index = 0;
			}
			else{
				//read data in until we hit overflow then start over
				if (incoming2_index < INCOMING2_BUFFER_SIZE) incoming2_index++;
				else incoming2_index = 0;
			}
		}

		//blindly empty serial 3 for now
		while (Serial3.available()){
			Serial3.read();
		}
	}

}


inline void onPacket2(const uint8_t* buffer, size_t size)
{

	if (size != 11){
		serial2stats.framing_errors++;
	}
	else{
		uint8_t crc = OneWire::crc8(buffer, size - 2);
		if (crc != buffer[size - 1]){
			serial2stats.crc_errors++;
		}
		else{

			if (serial2stats.packets_in_per_second < 255){
				serial2stats.packets_in_per_second++;
			}


			disc0.battery_voltage = buffer[6];
			disc0.cpu_temp = buffer[7];
			disc0.crc_errors = buffer[8];
			disc0.framing_errors = buffer[9];

		}
	}
}

inline void onPacket1(const uint8_t* buffer, size_t size)
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

			current_glove->aaRealX = buffer[6] << 8 | buffer[7];
			current_glove->aaRealY = buffer[8] << 8 | buffer[9];
			current_glove->aaRealZ = buffer[10] << 8 | buffer[11];

			current_glove->gravityX = buffer[12] << 8 | buffer[13];
			current_glove->gravityY = buffer[14] << 8 | buffer[15];
			current_glove->gravityZ = buffer[16] << 8 | buffer[17];

			current_glove->color_sensor_R = buffer[18] << 8 | buffer[19];
			current_glove->color_sensor_G = buffer[20] << 8 | buffer[21];
			current_glove->color_sensor_B = buffer[22] << 8 | buffer[23];
			current_glove->color_sensor_Clear = buffer[24] << 8 | buffer[25];

			//only allow one finger at a time
			if (buffer[26] == 0x0E){
				current_glove->finger1 = true;
				current_glove->finger2 = false;
				current_glove->finger3 = false;
				current_glove->finger4 = false;
				current_glove->gesture_finger = 1;
			}
			else if (buffer[26] == 0x0D){
				current_glove->finger1 = false;
				current_glove->finger2 = true;
				current_glove->finger3 = false;
				current_glove->finger4 = false;
				current_glove->gesture_finger = 2;
			}
			else if (buffer[26] == 0x0B){
				current_glove->finger1 = false;
				current_glove->finger2 = false;
				current_glove->finger3 = true;
				current_glove->finger4 = false;
				current_glove->gesture_finger = 3;
			}
			else if (buffer[26] == 0x07){
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
		

			current_glove->packets_in_per_second = buffer[27];
			current_glove->packets_out_per_second = buffer[28];

			current_glove->framing_errors = buffer[29];
			current_glove->crc_errors = buffer[30];

			current_glove->cpu_usage = buffer[31];
			current_glove->cpu_temp = buffer[32];

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

			//check for gestures

			if (current_glove->finger4 == 1){

				menu_mode = MENU_DEFAULT;
				fftmode = FFT_MODE_OFF;
				scroll_mode = SCROLL_MODE_COMPLETE;
			}

			if (!current_glove->finger1 &&  !current_glove->finger2 && !current_glove->finger3 && current_glove->gesture_in_progress == true){


				if (current_glove->gloveY <= 0)      menu_map(HAND_DIRECTION_DOWN);
				else if (current_glove->gloveY >= 7) menu_map(HAND_DIRECTION_UP);
				else if (current_glove->gloveX <= 0) menu_map(HAND_DIRECTION_LEFT);
				else if (current_glove->gloveX >= 7) menu_map(HAND_DIRECTION_RIGHT);

				//disable scroll mode on all other fingers
				//if (current_glove->gesture_finger != 1) scroll_mode = SCROLL_MODE_COMPLETE;
			}

			//if a single finger is pressed down....
			if (current_glove->finger1 || current_glove->finger2 || current_glove->finger3){
				if (current_glove->gesture_in_progress == false){
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
