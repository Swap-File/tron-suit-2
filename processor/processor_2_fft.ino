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
		EQdisplayValueMax16[i] = max(max(EQdisplayValueMax16[i] * .98, n), 4);
		EQdisplayValue16[i] = constrain(map(n, 0, EQdisplayValueMax16[i], 0, 255), 0, 255);

		// downsample 16 samples to 8
		if (i & 0x01){
			EQdisplayValueMax8[i >> 1] = (EQdisplayValueMax16[i] + EQdisplayValueMax16[i - 1]) >> 1;
			EQdisplayValue8[i >> 1] = (EQdisplayValue16[i] + EQdisplayValue16[i - 1]) >> 1;
		}

	}

	if (fftmode == FFT_MODE_HORZ_BARS_RIGHT ){
		//move eq data left 1
		for (uint8_t x = 1; x < 16; x++) {
			for (uint8_t y = 0; y < 8; y++) {
				EQdisplay[x - 1][y] = EQdisplay[x][y];
			}
		}
	}
	else if (fftmode == FFT_MODE_HORZ_BARS_LEFT){
		//move eq data right 1
		for (uint8_t x = 15; x > 0; x--) {
			for (uint8_t y = 0; y < 8; y++) {
				EQdisplay[x][y] = EQdisplay[x - 1][y];
			}
		}
	}
	else if (fftmode == FFT_MODE_VERT_BARS_UP){
		//move eq data up  1
		for (uint8_t y = 7; y >0; y--) {
			for (uint8_t x = 0; x < 16; x++) {
				EQdisplay[x][y] = EQdisplay[x][y - 1];
			}
		}
	}
	else if (fftmode == FFT_MODE_VERT_BARS_DOWN){
		//move eq data up  1
		for (uint8_t y = 1; y < 8; y++) {
			for (uint8_t x = 0; x < 16; x++) {
				EQdisplay[x][y-1] = EQdisplay[x][y];
			}
		}
	}
	else if (fftmode == FFT_MODE_OFF){
		for (uint8_t y = 0; y < 8; y++) {
			for (uint8_t x = 0; x < 16; x++) {
				EQdisplay[x][y] = CHSV(0, 0, 0);
			}
		}
	}


	if (fftmode == FFT_MODE_HORZ_BARS_RIGHT || fftmode == FFT_MODE_HORZ_BARS_LEFT){
		for (uint8_t i = 0; i < 8; i++) {

			//make the tip of the color be color 2
			CHSV temp_color;
			calcfftcolor(&temp_color, EQdisplayValue8[i]);

			if (fftmode == FFT_MODE_HORZ_BARS_RIGHT )  EQdisplay[15][i] = temp_color;
			else if (fftmode == FFT_MODE_HORZ_BARS_LEFT) EQdisplay[0][i] = temp_color;
		}

	}

	if (fftmode == FFT_MODE_HORZ_BARS_STATIC){
		for (uint8_t i = 0; i < 8; i++) {

			//make the tip of the color be color 2
			CHSV temp_color;
			calcfftcolor(&temp_color, EQdisplayValue8[i]);

			for (uint8_t index = 0; index < 16; index++){
				EQdisplay[index][i] = temp_color;
			}
		}
	}


	if (fftmode == FFT_MODE_VERT_BARS_UP || fftmode == FFT_MODE_VERT_BARS_DOWN){
		for (uint8_t i = 0; i < 16; i++) {

			//make the tip of the color be color 2
			CHSV temp_color;
			calcfftcolor(&temp_color, EQdisplayValue16[i]);

			if (fftmode == FFT_MODE_VERT_BARS_UP)	     EQdisplay[i][0] = temp_color;
			else if (fftmode == FFT_MODE_VERT_BARS_DOWN) EQdisplay[i][7] = temp_color;
		
		}
	}

	if (fftmode == FFT_MODE_VERT_BARS_STATIC){
		for (uint8_t i = 0; i < 16; i++) {

			//make the tip of the color be color 2
			CHSV temp_color;
			calcfftcolor(&temp_color, EQdisplayValue16[i]);

			for (uint8_t index = 0; index < 8; index++){
				EQdisplay[i][index] = temp_color;
			}
		}
	}
}

void calcfftcolor(CHSV * temp_color, uint8_t input){

	//make the tip of the color be color 2
	*temp_color = (input > 240) ? map_hsv(input, 240, 255, &color1, &color2) : color1;

	//scale the brightness //what if color2 is dimmer? look into this.
	temp_color->v = scale8(temp_color->v, input);
	//temp_color.v = EQdisplayValue8[i];

	return;
}
