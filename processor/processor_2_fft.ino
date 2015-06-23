inline void fftmath(void){

	for (uint8_t i = 0; i < 16; i++) {
		uint16_t n = 1000 * fft256_1.read((i * 2), (i * 2) + 2);

		//de-emphasize lower frequencies
		switch (i){
		case 0:  n = max(n - 150, 0); break;
		case 1:  n = max(n - 50, 0);  break;
		case 2:  n = max(n - 15, 0);  break;
		case 3:  n = max(n - 10, 0);  break;
		default: n = max(n - 3, 0);   break;
		}

		//falloff controll
		FFTdisplayValueMax16[i] = max(max(FFTdisplayValueMax16[i] * .98, n), 4);
		FFTdisplayValue16[i] = constrain(map(n, 0, FFTdisplayValueMax16[i], 0, 255), 0, 255);

		// downsample 16 samples to 8
		if (i & 0x01){
			FFTdisplayValueMax8[i >> 1] = (FFTdisplayValueMax16[i] + FFTdisplayValueMax16[i - 1]) >> 1;
			FFTdisplayValue8[i >> 1] = (FFTdisplayValue16[i] + FFTdisplayValue16[i - 1]) >> 1;
		}

	}


	if (fft_mode == FFT_MODE_HORZ_BARS_RIGHT){
		//move eq data left 1
		for (uint8_t x = 1; x < 16; x++) {
			for (uint8_t y = 0; y < 8; y++) {
				FFT_Array[x - 1][y] = FFT_Array[x][y];
			}
		}
	}
	else if (fft_mode == FFT_MODE_HORZ_BARS_LEFT){
		//move eq data right 1
		for (uint8_t x = 15; x > 0; x--) {
			for (uint8_t y = 0; y < 8; y++) {
				FFT_Array[x][y] = FFT_Array[x - 1][y];
			}
		}
	}
	else if (fft_mode == FFT_MODE_VERT_BARS_UP){
		//move eq data up  1
		for (uint8_t y = 7; y > 0; y--) {
			for (uint8_t x = 0; x < 16; x++) {
				FFT_Array[x][y] = FFT_Array[x][y - 1];
			}
		}
	}
	else if (fft_mode == FFT_MODE_VERT_BARS_DOWN){
		//move eq data up  1
		for (uint8_t y = 1; y < 8; y++) {
			for (uint8_t x = 0; x < 16; x++) {
				FFT_Array[x][y - 1] = FFT_Array[x][y];
			}
		}
	}



	if (fft_mode == FFT_MODE_HORZ_BARS_RIGHT || fft_mode == FFT_MODE_HORZ_BARS_LEFT){
		for (uint8_t i = 0; i < 8; i++) {

			//make the tip of the color be color 2
			CHSV temp_color;
			calcfftcolor(&temp_color, FFTdisplayValue8[i]);

			if (fft_mode == FFT_MODE_HORZ_BARS_RIGHT)  FFT_Array[15][i] = temp_color;
			else if (fft_mode == FFT_MODE_HORZ_BARS_LEFT) FFT_Array[0][i] = temp_color;
		}

	}

	if (fft_mode == FFT_MODE_HORZ_BARS_STATIC){
		for (uint8_t i = 0; i < 8; i++) {

			//make the tip of the color be color 2
			CHSV temp_color;
			calcfftcolor(&temp_color, FFTdisplayValue8[i]);

			for (uint8_t index = 0; index < 16; index++){
				FFT_Array[index][i] = temp_color;
			}
		}
	}


	if (fft_mode == FFT_MODE_VERT_BARS_UP || fft_mode == FFT_MODE_VERT_BARS_DOWN){
		for (uint8_t i = 0; i < 16; i++) {

			//make the tip of the color be color 2
			CHSV temp_color;
			calcfftcolor(&temp_color, FFTdisplayValue16[i]);

			if (fft_mode == FFT_MODE_VERT_BARS_UP)	     FFT_Array[i][0] = temp_color;
			else if (fft_mode == FFT_MODE_VERT_BARS_DOWN) FFT_Array[i][7] = temp_color;

		}
	}

	if (fft_mode == FFT_MODE_VERT_BARS_STATIC){
		for (uint8_t i = 0; i < 16; i++) {

			//make the tip of the color be color 2
			CHSV temp_color;
			calcfftcolor(&temp_color, FFTdisplayValue16[i]);

			for (uint8_t index = 0; index < 8; index++){
				FFT_Array[i][index] = temp_color;
			}
		}
	}

}

void calcfftcolor(CHSV * temp_color, uint8_t input){

	//make the tip of the color be color 2
	*temp_color = (input > 240) ? map_hsv(input, 240, 255, &color1, &color2) : color1;

	//scale the brightness //what if color2 is dimmer? look into this.
	temp_color->v = scale8(temp_color->v, input);
	//temp_color.v = FFTdisplayValue8[i];

	return;
}


inline void helmet_backgrounds(){

	CRGB black = CRGB::Black;
	

	BrightPalette = CRGBPalette16(
		map_hsv(0, 0, 15, &color1, &color2), map_hsv(1, 0, 15, &color1, &color2),
		map_hsv(2, 0, 15, &color1, &color2), map_hsv(3, 0, 15, &color1, &color2),
		map_hsv(4, 0, 15, &color1, &color2), map_hsv(5, 0, 15, &color1, &color2),
		map_hsv(6, 0, 15, &color1, &color2), map_hsv(7, 0, 15, &color1, &color2),
		map_hsv(8, 0, 15, &color1, &color2), map_hsv(9, 0, 15, &color1, &color2),
		map_hsv(10, 0, 15, &color1, &color2), map_hsv(11, 0, 15, &color1, &color2),
		map_hsv(12, 0, 15, &color1, &color2), map_hsv(13, 0, 15, &color1, &color2),
		map_hsv(14, 0, 15, &color1, &color2), map_hsv(15, 0, 15, &color1, &color2));


	 NormalPalette  = CRGBPalette16(
		map_hsv(0, 0, 15, &color1, &color2), map_hsv(1, 0, 15, &color1, &color2), black, black,
		map_hsv(4, 0, 15, &color1, &color2), map_hsv(5, 0, 15, &color1, &color2), black, black,
		map_hsv(8, 0, 15, &color1, &color2), map_hsv(9, 0, 15, &color1, &color2), black, black,
		map_hsv(12, 0, 15, &color1, &color2), map_hsv(13, 0, 15, &color1, &color2), black, black);


	fillnoise8();
	mapNoiseToLEDsUsingPalette();
	
	if (menu_mode == MENU_HELMET_PONG_IN){

		//render the field
		for (int x = 0; x < 16; x++) {
			for (int y = 0; y < 8; y++) {
				//draw the ball:
				if (x == ball_pos[0] && y == ball_pos[1]){
					Pong_Array[x][y] = CRGB(129, 129, 129);
				}//draw the left paddle:
				else if (x == 0 && (y == pong_paddle_l || y == pong_paddle_l + 1 || y == pong_paddle_l - 1)){
					Pong_Array[x][y] = color1;
				}//draw the right paddle:
				else if (x == 15 && (y == pong_paddle_r || y == pong_paddle_r + 1 || y == pong_paddle_r - 1)){
					Pong_Array[x][y] = color2;
				}//blank the rest
				else{
					Pong_Array[x][y] = CRGB(0, 0, 0);
				}
			}
		}

		//wait to increment
		if (pongtime.check()){


			//check for left paddle  collisions
			if (ball_pos[0] == 1 && (pong_ball_vector == PONG_LEFT || pong_ball_vector == PONG_LEFT_UP || pong_ball_vector == PONG_LEFT_DOWN)){
				if (ball_pos[1] == pong_paddle_l)			pong_ball_vector = PONG_RIGHT;
				else if (ball_pos[1] == pong_paddle_l - 1)  pong_ball_vector = PONG_RIGHT_DOWN;
				else if (ball_pos[1] == pong_paddle_l + 1)  pong_ball_vector = PONG_RIGHT_UP;
				pongtime.interval(max((int)pongtime.interval_millis - 10, PONG_MAX_SPEED));
			}
			//check for right paddle  collisions
			else if (ball_pos[0] == 14 && (pong_ball_vector == PONG_RIGHT || pong_ball_vector == PONG_RIGHT_UP || pong_ball_vector == PONG_RIGHT_DOWN)){
				if (ball_pos[1] == pong_paddle_r)			pong_ball_vector = PONG_LEFT;
				else if (ball_pos[1] == pong_paddle_r - 1)	pong_ball_vector = PONG_LEFT_DOWN;
				else if (ball_pos[1] == pong_paddle_r + 1)  pong_ball_vector = PONG_LEFT_UP;
				pongtime.interval(max((int)pongtime.interval_millis - 10, PONG_MAX_SPEED));
			}

			//check for vertical wall collisions
			if (ball_pos[1] == 0){
				if (pong_ball_vector == PONG_LEFT_DOWN)			pong_ball_vector = PONG_LEFT_UP;
				else if (pong_ball_vector == PONG_RIGHT_DOWN)	pong_ball_vector = PONG_RIGHT_UP;
			}
			else if (ball_pos[1] == 7){
				if (pong_ball_vector == PONG_LEFT_UP)		pong_ball_vector = PONG_LEFT_DOWN;
				else if (pong_ball_vector == PONG_RIGHT_UP)	pong_ball_vector = PONG_RIGHT_DOWN;
			}

			//find new position
			switch (pong_ball_vector){
			case PONG_LEFT:			ball_pos[0]--;	break;
			case PONG_RIGHT:		ball_pos[0]++;	break;
			case PONG_LEFT_UP:		ball_pos[0]--;	ball_pos[1]++;	break;
			case PONG_RIGHT_UP:		ball_pos[0]++;	ball_pos[1]++;	break;
			case PONG_LEFT_DOWN:	ball_pos[0]--;	ball_pos[1]--;	break;
			case PONG_RIGHT_DOWN:	ball_pos[0]++;	ball_pos[1]--;	break;
			}

			//check for collisions with horizontal wall, someone scored, dont care who, Im not keeping track
			if (ball_pos[0] < 0 || ball_pos[0] > 15){
				//random ball start x & y (in the middle 2x2)
				ball_pos[0] = random(7, 9);
				ball_pos[1] = random(3, 5);
				pong_ball_vector = random(0, 6);//radom ball vector
				pongtime.interval(PONG_MIN_SPEED);//slow down to starting speed
			}

		}
	}
}


// Fill the x/y array of 8-bit noise values using the inoise8 function.
void fillnoise8() {
	// If we're runing at a low "speed", some 8-bit artifacts become visible
	// from frame-to-frame.  In order to reduce this, we can do some fast data-smoothing.
	// The amount of data smoothing we're doing depends on "speed".
	uint8_t dataSmoothing = 0;
	if (speed_noise < 50) {
		dataSmoothing = 200 - (speed_noise * 4);
	}

	for (int i = 0; i < 16; i++) {
		int ioffset = z_noise_modifier * i;
		for (int j = 0; j < 16; j++) {
			int joffset = z_noise_modifier * j;

			uint8_t data = inoise8(x_noise + ioffset + x_noise_modifier, y_noise + joffset + y_noise_modifier, z_noise);

			// The range of the inoise8 function is roughly 16-238.
			// These two operations expand those values out to roughly 0..255
			// You can comment them out if you want the raw noise data.
			data = qsub8(data, 16);
			data = qadd8(data, scale8(data, 39));

			if (dataSmoothing) {
				uint8_t olddata = noise[i][j];
				uint8_t newdata = scale8(olddata, dataSmoothing) + scale8(data, 256 - dataSmoothing);
				data = newdata;
			}

			noise[i][j] = data;
		}
	}

	z_noise += speed_noise;

	// apply slow drift to X and Y, just for visual variation.
	x_noise += speed_noise / 8;
	y_noise -= speed_noise / 16;
}

void mapNoiseToLEDsUsingPalette()
{
	static uint8_t ihue = 0;

	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 8; j++) {
			// We use the value at the (i,j) coordinate in the noise
			// array for our brightness, and the flipped value from (j,i)
			// for our pixel's index into the color palette.

			uint8_t index = noise[j][i];
			uint8_t bri = noise[i][j];

			// if this palette is a 'loop', add a slowly-changing base value
			if (colorLoop_noise) {
				index += ihue;
			}

			// brighten up, as the color palette itself often contains the 
			// light/dark dynamic range desired
			if (bri > 127) {
				bri = 255;
			}
			else {
				bri = dim8_raw(bri * 2);
			}

			Noise_Array[i][j] = ColorFromPalette(NormalPalette, index, bri);
			Noise_Array_Bright[i][j] = ColorFromPalette(BrightPalette, index, bri);
		}
	}

	ihue += 1;
}
