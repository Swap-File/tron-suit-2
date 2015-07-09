inline void draw_HUD(void){

	//HUD!
	display.clearDisplay();

	//draw the boxes are 18x10, thats 16x8 + the boarder pixels

	//draw the menu first for masking purposes
	display.drawRect(menu_location_x - 1, menu_location_y - 1, 18, 10, WHITE);

	display.setCursor(menu_location_x + scroll_pos_x, menu_location_y + scroll_pos_y);

	print_menu_mode();


	menu_text_ending_pos = display.getCursorX(); //save menu text length for elsewhere
	//black out the rest!  probably dont need full width... 
	display.fillRect(menu_location_x - 1, menu_location_y - 1 - 10, display.width() - (menu_location_x - 1), 10, BLACK);
	display.fillRect(menu_location_x - 1, menu_location_y - 1 + 10, display.width() - (menu_location_x - 1), 10, BLACK);
	display.fillRect(menu_location_x - 1 + 18, menu_location_y - 1, display.width() - (menu_location_x - 1 + 18), 10, BLACK);

	//staticmode
	display.drawRect(static_menu_location_x - 1, static_menu_location_y - 1, 35, 10, WHITE);
	display.setCursor(static_menu_location_x, static_menu_location_y);
	print_menu_mode();

	//hand display dots - its important to always know where your hands are.
	display.drawRect(hand_location_x - 1, hand_location_y - 1, 18, 10, WHITE);
	display.drawPixel(hand_location_x + 15 - (8 + glove0.gloveX), hand_location_y + 7 - glove0.gloveY, WHITE);
	display.drawPixel(hand_location_x + 15 - glove1.gloveX, hand_location_y + 7 - glove1.gloveY, WHITE);

	//background overlay with trails, also shows HUD changes
	display.drawRect(trails_location_x - 1, trails_location_y - 1, 18, 10, WHITE);
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 8; y++) {
			if (gloveindicator[x][y] >= 50){  //This sets where decayed pixels disappear from HUD
				display.drawPixel(trails_location_x + (15 - x), trails_location_y + y, WHITE);
			}
		}
	}


	//sms display
	display.drawRect(txt_location_x - 1, txt_location_y - 1, 18, 10, WHITE);
	display.drawRect(txt_location_x - 1, txt_location_y - 1, 52, 10, WHITE);
	display.setCursor(txt_location_x + 16 + txt_scroll_pos, txt_location_y);
	display.print(live_txt);
	txt_ending_pos = display.getCursorX(); //save menu text length for elsewhere
	//pad out message for contiual scrolling in the HUD, gives me more text at once
	display.setCursor(txt_ending_pos + 19, txt_location_y);
	for (uint8_t i = 0; i < 7; i++){
		display.print(live_txt[i]);
	}

	//blank text scrolling
	display.fillRect(52, txt_location_y - 1, display.width() - (52), 8, BLACK);


	//this gets updated during writing out the main LED display, its the last thing to do
	display.drawRect(realtime_location_x - 1, realtime_location_y - 1, 18, 10, WHITE);


	draw_disc(scale8(disc0.current_index, 30), disc0.current_mag, inner_disc_location_x, inner_disc_location_y);


	//INDICATORS!
	display.setCursor(inner_disc_location_x-10, inner_disc_location_y);

	print_glove((void*)&glove0);

	display.setCursor(inner_disc_location_x +15, inner_disc_location_y);
	print_glove((void*)&glove1);



}

void print_glove(void *  first_glove){
	if (((GLOVE *)first_glove)->camera_on)		display.print('C');
	else if(((GLOVE *)first_glove)->finger1)	finger(first_glove, 1);
	else if (((GLOVE *)first_glove)->finger2)	finger(first_glove, 2);
	else if (((GLOVE *)first_glove)->finger3)	finger(first_glove, 3);
	else if (((GLOVE *)first_glove)->finger4)	finger(first_glove, 4);
	else										display.print(' ');
}

void finger(void * glove_f, uint8_t num){
	if (((GLOVE *)glove_f)->gloveY <= 0)		display.write(25); //ascii arrows
	else if (((GLOVE *)glove_f)->gloveY >= 7)	display.write(24);
	else if (((GLOVE *)glove_f)->gloveX <= 0)	display.write(26);
	else if (((GLOVE *)glove_f)->gloveX >= 7)	display.write(27);
	else										display.print(num);
}



boolean read_menu_pixel(uint8_t x, uint8_t y){

	//add 7 sub y to flip on y axis
	if (display.readPixel(menu_location_x + x, menu_location_y + (7 - y))){
		return true;
	}
	return false;
}

boolean read_txt_pixel(uint8_t x, uint8_t y){

	//add 7 sub y to flip on y axis
	if (display.readPixel(txt_location_x + x, txt_location_y + (7 - y))){
		return true;
	}
	return false;
}


void draw_disc(uint8_t index_offset, uint8_t magnitude, uint8_t x_offset, uint8_t y_offset){

	//adjust reference
	index_offset = (index_offset + 15) % 30;
	//draw pointer line
	display.drawLine(x_offset + 6, y_offset + 5, x_offset + circle_x[index_offset], y_offset + circle_y[index_offset], WHITE);
	display.drawLine(x_offset + 6, y_offset + 4, x_offset + circle_x[index_offset], y_offset + circle_y[index_offset], WHITE);

	//draw the circle
	if (magnitude <= 16){
		for (uint8_t i = 0; i < 16; i++) {
			if (i < magnitude){
				display.drawPixel(x_offset + circle_x[(index_offset + i) % 30], y_offset + circle_y[(index_offset + i) % 30], WHITE);
				display.drawPixel(x_offset + circle_x[(index_offset + 30 - i) % 30], y_offset + circle_y[(index_offset + 30 - i) % 30], WHITE);
			}
		}
	}
	else if (magnitude > 16 && magnitude <= 32){
		for (uint8_t i = 0; i < 16; i++) {
			if (i >(magnitude - 17)){
				display.drawPixel(x_offset + circle_x[i], y_offset + circle_y[i], WHITE);
				display.drawPixel(x_offset + circle_x[30 - i], y_offset + circle_y[30 - i], WHITE);
			}
		}
	}
}


void menu_map(uint8_t direction){
	Serial.print(scroll_mode);

	//set initial scroll on and scroll off variables
	//can/will be overridden by menu below if desired
	switch (direction){

	case HAND_DIRECTION_LEFT:
		scroll_mode = SCROLL_MODE_INCOMING;  //start new scroll action
		menu_scroll_start_left();
		menu_scroll_end_right();
		break;
	case HAND_DIRECTION_RIGHT:
		scroll_mode = SCROLL_MODE_INCOMING;  //start new scroll action
		menu_scroll_start_right();
		menu_scroll_end_right();
		break;
	case HAND_DIRECTION_UP:
		scroll_mode = SCROLL_MODE_INCOMING;  //start new scroll action
		menu_scroll_start_up();
		menu_scroll_end_right();
		break;
	case HAND_DIRECTION_DOWN:
		scroll_mode = SCROLL_MODE_INCOMING;  //start new scroll action
		menu_scroll_start_down();
		menu_scroll_end_right();
		break;
	}

	//silent modes for 1st finger
	if (glove0.gesture_finger == 1 || glove1.gesture_finger == 1){
		scroll_mode = SCROLL_MODE_COMPLETE;
	}


	if (direction == HAND_SUIT_OFF){
		//force out of swipe mode
		if (disc0.disc_mode != DISC_MODE_IDLE && disc0.disc_mode != DISC_MODE_OFF)	disc0.disc_mode = DISC_MODE_IDLE;

		//go to root menu
		menu_mode = MENU_LOCKED;
		//clear screen
		scroll_mode = SCROLL_MODE_COMPLETE;
		//reset 
		if (z_noise_modifier == 0) z_noise_modifier = 127;
	}
	//shortcut menu
	else if (glove0.gesture_finger == 3 || glove1.gesture_finger == 3){
		scroll_mode = SCROLL_MODE_COMPLETE;
		switch (direction){
		case HAND_DIRECTION_LEFT:
			//if already in a backgroud menu, jump to the other background
			if (menu_mode == MENU_HELMET_FFT_ON_H_or_V || menu_mode == MENU_HELMET_FFT_ON_H || menu_mode == MENU_HELMET_FFT_ON_V || menu_mode == MENU_HELMET_FFT_ON){
				menu_mode = MENU_HELMET_NOISE_ON;
				background_mode = BACKGROUND_MODE_NOISE;
			}
			else if (menu_mode == MENU_HELMET_NOISE_ON){
				background_mode = BACKGROUND_MODE_FFT;
				if (fft_mode == FFT_MODE_HORZ_BARS_LEFT || fft_mode == FFT_MODE_HORZ_BARS_RIGHT || fft_mode == FFT_MODE_HORZ_BARS_STATIC){
					menu_mode = MENU_HELMET_FFT_ON_H;
				}
				else if (fft_mode == FFT_MODE_VERT_BARS_UP || fft_mode == FFT_MODE_VERT_BARS_DOWN || fft_mode == FFT_MODE_VERT_BARS_STATIC){
					menu_mode = MENU_HELMET_FFT_ON_V;
				}
			}
			else
			{//if not in a background menu, jump into the current background menu
				if (background_mode == BACKGROUND_MODE_FFT){
					if (fft_mode == FFT_MODE_HORZ_BARS_LEFT || fft_mode == FFT_MODE_HORZ_BARS_RIGHT || fft_mode == FFT_MODE_HORZ_BARS_STATIC){
						menu_mode = MENU_HELMET_FFT_ON_H;
					}
					else if (fft_mode == FFT_MODE_VERT_BARS_UP || fft_mode == FFT_MODE_VERT_BARS_DOWN || fft_mode == FFT_MODE_VERT_BARS_STATIC){
						menu_mode = MENU_HELMET_FFT_ON_V;
					}
				}
				else if (background_mode == BACKGROUND_MODE_NOISE){
					menu_mode = MENU_HELMET_NOISE_ON;
				}
			}
			break;
		case HAND_DIRECTION_RIGHT:
			//disc control
			menu_mode = MENU_SPIN;
			break;
		case HAND_DIRECTION_UP:
			//disc control
			menu_mode = MENU_CAMON;
			break;
		case HAND_DIRECTION_DOWN:
			menu_mode = MENU_HELMET_EMOTICON_ON_SOUND;
			break;
		} 
	}
	else if ((glove0.gesture_finger == 4 || glove1.gesture_finger == 4) && direction != HAND_BACK){
		//blackhole these for now
		switch (direction){
		case HAND_DIRECTION_LEFT:
			//menu_mode = MENU_PWR_IN;
		case HAND_DIRECTION_RIGHT:
			////disc control
			//menu_mode = MENU_SPIN;
			break;
		case HAND_DIRECTION_UP:
			//disc control
			//menu_mode = MENU_CAMON;
			break;
		case HAND_DIRECTION_DOWN:
			//menu_mode = MENU_HELMET_EMOTICON_ON_SOUND;
			break;
		}
	}
	else{


		switch (menu_mode){

		case MENU_HELMET_FFT_ON_H:
			scroll_mode = SCROLL_MODE_COMPLETE;
			switch (direction){
			case HAND_BACK: menu_mode = MENU_HELMET_FFT;	break;
			case HAND_DIRECTION_LEFT:
				fft_mode = FFT_MODE_HORZ_BARS_LEFT;
				menu_scroll_end_left();
				break;
			case HAND_DIRECTION_RIGHT:
				fft_mode = FFT_MODE_HORZ_BARS_RIGHT;
				menu_scroll_end_right();
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_HELMET_FFT_ON_V;
				fft_mode = FFT_MODE_VERT_BARS_STATIC;
				menu_scroll_end_up();
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_HELMET_FFT_ON_V;
				fft_mode = FFT_MODE_VERT_BARS_STATIC;
				menu_scroll_end_down();
				break;
			}
			break;

		case MENU_HELMET_FFT_ON_V:
			scroll_mode = SCROLL_MODE_COMPLETE;
			switch (direction){
			case HAND_BACK: menu_mode = MENU_HELMET_FFT;	break;
			case HAND_DIRECTION_LEFT:
				menu_mode = MENU_HELMET_FFT_ON_H;
				fft_mode = FFT_MODE_HORZ_BARS_STATIC;
				menu_scroll_end_left();
				break;
			case HAND_DIRECTION_RIGHT:
				menu_mode = MENU_HELMET_FFT_ON_H;
				fft_mode = FFT_MODE_HORZ_BARS_STATIC;
				menu_scroll_end_right();
				break;
			case HAND_DIRECTION_UP:
				fft_mode = FFT_MODE_VERT_BARS_UP;
				menu_scroll_end_up();
				break;
			case HAND_DIRECTION_DOWN:
				fft_mode = FFT_MODE_VERT_BARS_DOWN;
				menu_scroll_end_down();
				break;
			}
			break;


		case MENU_HELMET_FFT_ON_H_or_V:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_HELMET_FFT;	break;
			case HAND_DIRECTION_LEFT:
				fft_mode = FFT_MODE_HORZ_BARS_STATIC;
				menu_mode = MENU_HELMET_FFT_ON_H; //new menu screen 
				menu_scroll_end_left();
				break;
			case HAND_DIRECTION_RIGHT:
				fft_mode = FFT_MODE_HORZ_BARS_STATIC;
				menu_mode = MENU_HELMET_FFT_ON_H;
				menu_scroll_end_right();
				break;
			case HAND_DIRECTION_UP:
				fft_mode = FFT_MODE_VERT_BARS_STATIC;
				menu_mode = MENU_HELMET_FFT_ON_V;
				menu_scroll_end_up();
				break;
			case HAND_DIRECTION_DOWN:
				fft_mode = FFT_MODE_VERT_BARS_STATIC;
				menu_mode = MENU_HELMET_FFT_ON_V;
				menu_scroll_end_down();
				break;
			}
			break;


		case MENU_LOCKED:
			switch (direction){
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				menu_mode = MENU_HELMET_FFT;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_RED; //new menu screen 
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_PWR_IN; //new menu screen 
				break;
			}
			break;


		case MENU_HELMET:
			switch (direction){
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				menu_mode = MENU_HELMET_FFT; // MENU_FFT;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_COLOR; //new menu screen 
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_DISC; //new menu screen 
				break;
			}
			break;


		case MENU_DISC:
			switch (direction){
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				menu_mode = MENU_SPIN;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_HELMET;
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_CAMERA;
				break;
			}
			break;

		case MENU_CAMERA:
			switch (direction){
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				menu_mode = MENU_CAMON;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_DISC; //new menu screen 
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_TXT; //new menu screen 
				break;
			}
			break;

		case MENU_TXT:
			switch (direction){
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				menu_mode = MENU_TXT_DISPLAY;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_CAMERA; //new menu screen 
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_PWR; //new menu screen 
				break;
			}
			break;

		case MENU_PWR:
			switch (direction){
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				menu_mode = MENU_PWR_IN;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_TXT; //new menu screen 
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_COLOR; //new menu screen 
				break;
			}
			break;

		case MENU_COLOR:
			switch (direction){
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				menu_mode = MENU_RED;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_PWR; //new menu screen 
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_HELMET; //new menu screen 
				break;
			}
			break;

		case MENU_PWR_IN:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_PWR;	break;
			}break;
		case MENU_TXT_ON:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_TXT_DISPLAY;	break;
			}break;
		case MENU_CAMON:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_CAMERA;	break;
			}break;
		case MENU_SPIN:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_DISC;	break;
			}break;
		case MENU_HELMET_NOISE_ON:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_HELMET_NOISE;	break;
			}break;

		case MENU_TXT_DISPLAY:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_TXT;	break;
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				menu_mode = MENU_TXT_ON;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_STATS_LOAD; //new menu screen 
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_SMS_LOAD; //new menu screen 
				break;
			}
			break;

		case MENU_SMS_LOAD:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_TXT;	break;
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				live_txt = front_sms; //load it
				menu_mode = MENU_TXT_DISPLAY;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_TXT_DISPLAY; //new menu screen 
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_WWW_LOAD; //new menu screen 
				break;
			}
			break;
		case MENU_WWW_LOAD:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_TXT;	break;
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				live_txt = www_message; //load it
				menu_mode = MENU_TXT_DISPLAY;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_SMS_LOAD; //new menu screen 
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_STATS_LOAD; //new menu screen 
				break;
			}
			break;

		case MENU_STATS_LOAD:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_TXT;	break;
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				live_txt = stats_message; ///load it
				menu_mode = MENU_TXT_DISPLAY;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_WWW_LOAD; //new menu screen 
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_TXT_DISPLAY; //new menu screen 
				break;
			}
			break;

		case MENU_HELMET_FFT:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_HELMET;	break;
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				menu_mode = MENU_HELMET_FFT_ON_H_or_V;
				background_mode = BACKGROUND_MODE_FFT;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_HELMET_PONG;
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_HELMET_NOISE;
				break;
			}
			break;


		case MENU_HELMET_NOISE:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_HELMET;	break;
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				menu_mode = MENU_HELMET_NOISE_ON;
				background_mode = BACKGROUND_MODE_NOISE;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_HELMET_FFT;
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_HELMET_EMOTICON;
				break;
			}
			break;

		case MENU_HELMET_EMOTICON:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_HELMET;	break;
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				menu_mode = MENU_HELMET_EMOTICON_ON_SOUND;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_HELMET_NOISE;
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_HELMET_PONG;
				break;
			}
			break;

		case MENU_HELMET_EMOTICON_ON_SOUND:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_HELMET_EMOTICON;	break;
			case HAND_SHORT_PRESS_DOUBLE:
				menu_mode = MENU_HELMET_EMOTICON_ON_MOTION;
				break;
			}
			break;


		case MENU_HELMET_EMOTICON_ON_MOTION:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_HELMET_EMOTICON;	break;
			case HAND_SHORT_PRESS_DOUBLE:
				menu_mode = MENU_HELMET_EMOTICON_ON_SOUND;
				break;
			}
			break;



		case MENU_HELMET_PONG:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_HELMET;	break;
			case HAND_DIRECTION_LEFT:
			case HAND_DIRECTION_RIGHT:
				menu_mode = MENU_HELMET_PONG_IN;
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_HELMET_EMOTICON;
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_HELMET_FFT;

				break;
			}
			break;


		case MENU_RED:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_COLOR;	break;
			case HAND_DIRECTION_LEFT:
				color1 = CHSV(HUE_RED, 255, 255);
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = true;
					color2 = CHSV(HUE_RED, 255, 255);
				}
				break;
			case HAND_DIRECTION_RIGHT:
				color2 = CHSV(HUE_RED, 255, 255);
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = true;
					color1 = CHSV(HUE_RED, 255, 255);
				}
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_PINK;
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_ORANGE;
				break;
			}
			break;

		case MENU_ORANGE:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_COLOR;	break;
			case HAND_DIRECTION_LEFT:
				color1 = CHSV(HUE_ORANGE, 255, 255);
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = true;
					color2 = CHSV(HUE_ORANGE, 255, 255);
				}
				break;
			case HAND_DIRECTION_RIGHT:
				color2 = CHSV(HUE_ORANGE, 255, 255);
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = true;
					color1 = CHSV(HUE_ORANGE, 255, 255);
				}
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_RED;
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_YELLOW;
				break;
			}
			break;
		case MENU_YELLOW:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_COLOR;	break;
			case HAND_DIRECTION_LEFT:
				color1 = CHSV(HUE_YELLOW, 255, 255);
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = true;
					color2 = CHSV(HUE_YELLOW, 255, 255);
				}
				break;
			case HAND_DIRECTION_RIGHT:
				color2 = CHSV(HUE_YELLOW, 255, 255);
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = true;
					color1 = CHSV(HUE_YELLOW, 255, 255);
				}
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_ORANGE;
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_GREEN;
				break;
			}
			break;

		case MENU_GREEN:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_COLOR;	break;
			case HAND_DIRECTION_LEFT:
				color1 = CHSV(HUE_GREEN, 255, 255);
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = true;
					color2 = CHSV(HUE_GREEN, 255, 255);
				}
				break;
			case HAND_DIRECTION_RIGHT:
				color2 = CHSV(HUE_GREEN, 255, 255);
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = true;
					color1 = CHSV(HUE_GREEN, 255, 255);
				}
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_YELLOW;
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_CYAN;
				break;
			}
			break;


		case MENU_CYAN:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_COLOR;	break;
			case HAND_DIRECTION_LEFT:
				color1 = CHSV(HUE_AQUA, 255, 255);
				//cyan is normal! shut off custom startup.
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = false;
				}
				break;
			case HAND_DIRECTION_RIGHT:
				color2 = CHSV(HUE_AQUA, 255, 255);
				//cyan is normal! shut off custom startup.
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = false;
				}
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_GREEN;
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_BLUE;
				break;
			}
			break;

		case MENU_BLUE:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_COLOR;	break;
			case HAND_DIRECTION_LEFT:
				color1 = CHSV(HUE_BLUE, 255, 255);
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = true;
					color2 = CHSV(HUE_BLUE, 255, 255);
				}
				break;
			case HAND_DIRECTION_RIGHT:
				color2 = CHSV(HUE_BLUE, 255, 255);
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = true;
					color1 = CHSV(HUE_BLUE, 255, 255);
				}
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_CYAN;
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_PURPLE;
				break;
			}
			break;
		case MENU_PURPLE:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_COLOR;	break;
			case HAND_DIRECTION_LEFT:
				color1 = CHSV(HUE_PURPLE, 255, 255);
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = true;
					color2 = CHSV(HUE_PURPLE, 255, 255);
				}
				break;
			case HAND_DIRECTION_RIGHT:
				color2 = CHSV(HUE_PURPLE, 255, 255);
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = true;
					color1 = CHSV(HUE_PURPLE, 255, 255);
				}
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_BLUE;
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_PINK;
				break;
			}
			break;
		case MENU_PINK:
			switch (direction){
			case HAND_BACK: menu_mode = MENU_COLOR;	break;
			case HAND_DIRECTION_LEFT:
				color1 = CHSV(HUE_PINK, 255, 255);
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = true;
					color2 = CHSV(HUE_PINK, 255, 255);
				}
				break;
			case HAND_DIRECTION_RIGHT:
				color2 = CHSV(HUE_PINK, 255, 255);
				if (disc0.disc_mode == DISC_MODE_OFF){
					cust_Startup_color = true;
					color1 = CHSV(HUE_PINK, 255, 255);
				}
				break;
			case HAND_DIRECTION_UP:
				menu_mode = MENU_PURPLE;
				break;
			case HAND_DIRECTION_DOWN:
				menu_mode = MENU_RED;
				break;
			}
			break;
		}
	}
}

void print_menu_mode(void){
	switch (menu_mode){
	case MENU_LOCKED:
		display.print("LOCKED");
		break;
	case MENU_DISC:
		display.print("DISC");
		break;
	case MENU_TXT:
		display.print("TXT");
		break;
	case MENU_HELMET:
		display.print("HEAD");
		break;
	case MENU_SPIN:
		display.print("D ON");
		break;
	case MENU_HELMET_FFT_ON_H_or_V:
		display.print("FFT");
		break;
	case MENU_HELMET_FFT_ON_H:
		display.print("FFTH");
		break;
	case MENU_HELMET_FFT_ON_V:
		display.print("FFTV");
		break;
	case MENU_CAMERA:
		display.print("CAM");
		break;
	case MENU_HELMET_FFT:
		display.print("FFT");
		break;
	case MENU_HELMET_NOISE:
		display.print("NOISE");
		break;
	case MENU_HELMET_PONG:
		display.print("PONG");
		break;
	case MENU_HELMET_PONG_IN:
		display.print("PONG!");
		break;
	case MENU_HELMET_EMOTICON:
		display.print("EMOT");
		break;
	case MENU_TXT_DISPLAY:
		display.print("DISP");
		break;
	case MENU_STATS_LOAD:
		display.print("STAT");
		break;
	case MENU_WWW_LOAD:
		display.print("WWW");
		break;
	case MENU_SMS_LOAD:
		display.print("SMS");
		break;
	case MENU_TXT_ON:
		display.print("T ON");
		break;
	case MENU_CAMON:
		display.print("C ON");
		break;
	case MENU_HELMET_NOISE_ON:
		display.print("N ON");
		break;

	case MENU_HELMET_EMOTICON_ON_SOUND:
		display.print("S ^_^");
		break;
	case MENU_HELMET_EMOTICON_ON_MOTION:
		display.print("^_^ M");
		break;
	case MENU_PWR:
		display.print("PWR");
		break;
	case MENU_PWR_IN:
		display.print("P ADJ");
		break;
	case MENU_COLOR:
		display.print("COLOR");
		break;
	case MENU_RED:
		display.print("RED");
		break;
	case MENU_ORANGE:
		display.print("ORANGE");
		break;
	case MENU_YELLOW:
		display.print("YELLOW");
		break;
	case MENU_GREEN:
		display.print("GREEN");
		break;
	case MENU_CYAN:
		display.print("CYAN");
		break;
	case MENU_BLUE:
		display.print("BLUE");
		break;
	case MENU_PURPLE:
		display.print("PURPLE");
		break;
	case MENU_PINK:
		display.print("PINK");
		break;
		//fallback to print ID number if not silenced
	default:
		display.print(menu_mode);
	}
}
//locations to start a scroll movement from
inline void menu_scroll_start_right(void){
	scroll_pos_x = 18;
	scroll_pos_y = 0;
}

inline void menu_scroll_start_left(void){
	scroll_pos_x = -128; //negative longer than longest text message, the slack will get taken up by the shift code
	scroll_pos_y = 0;
}

inline void menu_scroll_start_up(void){
	scroll_pos_x = 0;
	scroll_pos_y = 8;
}

inline void menu_scroll_start_down(void){
	scroll_pos_x = 0;
	scroll_pos_y = -8;
}

//locations to stop a scroll movement at
inline void menu_scroll_end_right(void){
	scroll_end_pos_x = -128;  //negative longer than longest text message, the slack will get taken up by the shift code
	scroll_end_pos_y = 0;
}

inline void menu_scroll_end_left(void){
	scroll_end_pos_x = 16;
	scroll_end_pos_y = 0;
}

inline void menu_scroll_end_up(void){
	scroll_end_pos_x = 0;
	scroll_end_pos_y = -8;
}

inline void menu_scroll_end_down(void){
	scroll_end_pos_x = 0;
	scroll_end_pos_y = 8;
}
