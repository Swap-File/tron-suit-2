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
	display.drawPixel(hand_location_x + 8 + glove0.gloveX, hand_location_y + 7 - glove0.gloveY, WHITE);
	display.drawPixel(hand_location_x + glove1.gloveX, hand_location_y + 7 - glove1.gloveY, WHITE);

	//background overlay with trails, also shows HUD changes
	display.drawRect(trails_location_x - 1, trails_location_y - 1, 18, 10, WHITE);
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 8; y++) {
			if (gloveindicator[x][y] >= 50){  //This sets where decayed pixels disappear from HUD
				display.drawPixel(trails_location_x + x, trails_location_y + y, WHITE);
			}
		}
	}


	//sms display
	display.drawRect(sms_location_x - 1, sms_location_y - 1, 18, 10, WHITE);
	display.setCursor(sms_location_x + 16 + sms_scroll_pos, sms_location_y);
	display.print(sms_message);
	sms_text_ending_pos = display.getCursorX(); //save menu text length for elsewhere
	//pad out message for contiual scrolling in the HUD, gives me more text at once
	display.setCursor(sms_text_ending_pos + 19, sms_location_y );
	for (uint8_t i = 0; i < 7; i++){
		display.print(sms_message[i]);
	}


	//this gets updated during writing out the main LED display, its the last thing to do
	display.drawRect(realtime_location_x - 1, realtime_location_y - 1, 18, 10, WHITE);

	draw_disc(disc0.inner_offset_requested, disc0.inner_magnitude_requested, inner_disc_location_x, inner_disc_location_y);

	draw_disc(disc0.outer_offset_requested, disc0.outer_magnitude_requested, outer_disc_location_x, outer_disc_location_y);

}


boolean read_menu_pixel(uint8_t x, uint8_t y){

	//add 7 sub y to flip on y axis
	if (display.readPixel(menu_location_x + x, menu_location_y + (7 - y))){
		return true;
	}
	return false;
}

boolean read_sms_pixel(uint8_t x, uint8_t y){

	//add 7 sub y to flip on y axis
	if (display.readPixel(sms_location_x + x, sms_location_y + (7 - y))){
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
	else if (magnitude > 32 && magnitude <= 47){
		for (uint8_t i = 0; i < 16; i++) {
			if (i <= magnitude - 32) display.drawPixel(x_offset + circle_x[i], y_offset + circle_y[i], WHITE);
			if (i <= magnitude - 32) display.drawPixel(x_offset + circle_x[i + 15], y_offset + circle_y[i + 15], WHITE);
		}
	}
	else if (magnitude > 47 && magnitude <= 62){
		for (uint8_t i = 0; i < 16; i++) {
			if (i > magnitude - 47) display.drawPixel(x_offset + circle_x[i], y_offset + circle_y[i], WHITE);
			if (i > magnitude - 47) display.drawPixel(x_offset + circle_x[i + 15], y_offset + circle_y[i + 15], WHITE);
		}
	}
	else if (magnitude > 62 && magnitude <= 77){
		for (uint8_t i = 0; i < 16; i++) {

		}
	}

	//blank the beam pixel
	//if (disc0.packet_beam < 17 && disc0.packet_beam > 0 && disc0.fade_level < 255){
	//	display.drawPixel(circle_xcoord(disc0.packet_beam), circle_ycoord(disc0.packet_beam), BLACK);
	//	display.drawPixel(circle_xcoord(30 - disc0.packet_beam), circle_ycoord(30 - disc0.packet_beam), BLACK);
	//}

}


void menu_map(uint8_t direction){
	
	
	//set initial scroll on and scroll off variables
	//can/will be overridden by menu below if desired
	switch (direction){
		
	case HAND_DIRECTION_LEFT:
		scroll_mode = SCROLL_MODE_INCOMING;  //start new scroll action
		menu_scroll_start_left();
		menu_scroll_end_left();
		break;
	case HAND_DIRECTION_RIGHT:
		scroll_mode = SCROLL_MODE_INCOMING;  //start new scroll action
		menu_scroll_start_right();
		menu_scroll_end_right();
		break;
	case HAND_DIRECTION_UP:
		scroll_mode = SCROLL_MODE_INCOMING;  //start new scroll action
		menu_scroll_start_up();
		menu_scroll_end_up();
		break;
	case HAND_DIRECTION_DOWN:
		scroll_mode = SCROLL_MODE_INCOMING;  //start new scroll action
		menu_scroll_start_down();
		menu_scroll_end_down();
		break;
	}


	switch (menu_mode){

	case MENU_FFT_H:
		scroll_mode = SCROLL_MODE_COMPLETE; 
		switch (direction){
		case HAND_DIRECTION_LEFT:
			fftmode = FFT_MODE_HORZ_BARS_LEFT;
			break;
		case HAND_DIRECTION_RIGHT:
			fftmode = FFT_MODE_HORZ_BARS_RIGHT;
			break;
		case HAND_DIRECTION_UP:
			menu_mode = MENU_FFT_V;
			fftmode = FFT_MODE_VERT_BARS_STATIC;
			break;
		case HAND_DIRECTION_DOWN:
			menu_mode = MENU_FFT_V;
			fftmode = FFT_MODE_VERT_BARS_STATIC;
			break;
		}
		break;

	case MENU_FFT_V:
		scroll_mode = SCROLL_MODE_COMPLETE;
		switch (direction){
		case HAND_DIRECTION_LEFT:
			menu_mode = MENU_FFT_H; 
			fftmode = FFT_MODE_HORZ_BARS_STATIC;
			break;
		case HAND_DIRECTION_RIGHT:
			menu_mode = MENU_FFT_H;
			fftmode = FFT_MODE_HORZ_BARS_STATIC;
			break;
		case HAND_DIRECTION_UP:
			fftmode = FFT_MODE_VERT_BARS_UP;
			break;
		case HAND_DIRECTION_DOWN:
			fftmode = FFT_MODE_VERT_BARS_DOWN;
			break;
		}
		break;


	case MENU_FFT_H_or_V:
		switch (direction){
		case HAND_DIRECTION_LEFT:
			fftmode = FFT_MODE_HORZ_BARS_STATIC;
			menu_mode = MENU_FFT_H; //new menu screen 
			break;
		case HAND_DIRECTION_RIGHT:
			fftmode = FFT_MODE_HORZ_BARS_STATIC;
			menu_mode = MENU_FFT_H;
			break;
		case HAND_DIRECTION_UP:
			fftmode = FFT_MODE_VERT_BARS_STATIC;
			menu_mode = MENU_FFT_V;
			break;
		case HAND_DIRECTION_DOWN:
			fftmode = FFT_MODE_VERT_BARS_STATIC;
			menu_mode = MENU_FFT_V;
			break;
		}
		break;


case MENU_DEFAULT:
case MENU_SUIT:
	switch (direction){
	case HAND_DIRECTION_LEFT:
	
		break;
	case HAND_DIRECTION_RIGHT:
		menu_mode = MENU_FFT;
		break;
	case HAND_DIRECTION_UP:
		menu_mode = MENU_DISC; //new menu screen 
		break;
	case HAND_DIRECTION_DOWN:
		menu_mode = MENU_DISC; //new menu screen 
		
		break;
	}
	break;
	

	case MENU_DISC:
		switch (direction){
		case HAND_DIRECTION_LEFT:
			break;
		case HAND_DIRECTION_RIGHT:
			menu_mode = MENU_MAG;
			break;
		case HAND_DIRECTION_UP:
			menu_mode = MENU_SUIT;
			break;
		case HAND_DIRECTION_DOWN:
			menu_mode = MENU_SUIT;
			break;
		}
		break;


	case MENU_MAG:
		switch (direction){
		case HAND_DIRECTION_SHORT_PRESS:
			menu_mode = MENU_SPIN;
			break;
		}
		break;
	
	case MENU_SPIN:
		switch (direction){
		case HAND_DIRECTION_SHORT_PRESS:
			menu_mode = MENU_MAG; 
			break;
		}
		break;

case MENU_FFT:
		switch (direction){
		case HAND_DIRECTION_LEFT:
			menu_mode = MENU_DEFAULT;
			break;
		case HAND_DIRECTION_RIGHT:
			menu_mode = MENU_FFT_H_or_V;
			break;
		case HAND_DIRECTION_UP:
			menu_mode = MENU_FFT;
			break;
		case HAND_DIRECTION_DOWN:
			menu_mode = MENU_FFT;
			break;
		}
		break;
	}




}

void print_menu_mode(void ){
	switch (menu_mode){
	case MENU_DEFAULT:
		display.print("DEFAULT");
		break;
	case MENU_DISC:
		display.print("DISC");
		break;
	case MENU_SUIT:
		display.print("SUIT");
		break;
	case MENU_MAG:
		display.print("MAG");
		break;
	case MENU_SPIN:
		display.print("SPIN");
		break;
	case MENU_FFT_H_or_V:
		display.print("FFT");
		break;
	case MENU_FFT_H:
		display.print("FFTH");
		break;
	case MENU_FFT_V:
		display.print("FFTV");
		break;
	default:
		display.print(menu_mode);
	}
}
//locations to start a scroll movement from
inline void menu_scroll_start_left(void){
	scroll_pos_x = 18;
	scroll_pos_y = 0;
}

inline void menu_scroll_start_right(void){
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
inline void menu_scroll_end_left(void){
	scroll_end_pos_x = -128;  //negative longer than longest text message, the slack will get taken up by the shift code
	scroll_end_pos_y = 0;
}

inline void menu_scroll_end_right(void){
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
