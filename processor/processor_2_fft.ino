inline void fftmath(void){

	for (uint8_t i = 0; i < 16; i++) {
		uint16_t n = 1000 * fft256_1.read((i * 2), (i * 2) + 2);

		switch (i){
		case 0:  n = max(n - 150, 0); break;
		case 1:  n = max(n - 50, 0);  break;
		case 2:  n = max(n - 15, 0);  break;
		case 3:  n = max(n - 10, 0);  break;
		default: n = max(n - 3, 0);   break;
		}

		EQdisplayValueMax16[i] = max(max(EQdisplayValueMax16[i] * .98, n), 4);
		EQdisplayValue16[i] = constrain(map(n, 0, EQdisplayValueMax16[i], 0, 255), 0, 255);
		
		// downsample 16 samples to 8
		if (i & 0x01){
			EQdisplayValueMax8[i >> 1] = (EQdisplayValueMax16[i] + EQdisplayValueMax16[i - 1]) >> 1;
			EQdisplayValue8[i >> 1] = (EQdisplayValue16[i] + EQdisplayValue16[i - 1]) >> 1;
		}

	}


	if (fftmode == FFT_MODE_HORZ_BARS_LEFT){
		//move eq data left 1
		for (uint8_t x = 1; x < 16; x++) {
			for (uint8_t y = 0; y < 8; y++) {
				EQdisplay[x - 1][y] = EQdisplay[x][y];
			}
		}
	}

	else if (fftmode == FFT_MODE_HORZ_BARS_RIGHT){
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


	if (fftmode == FFT_MODE_HORZ_BARS_RIGHT || fftmode == FFT_MODE_HORZ_BARS_LEFT){
		for (uint8_t i = 0; i < 8; i++) {
			uint8_t hue = 0;
			uint8_t saturation = 255;

			if (color_on){
				hue = constrain(map(EQdisplayValue8[i], 240, 255, 0, 64), 0, 64);
			}

			if (fftmode == FFT_MODE_HORZ_BARS_LEFT){
				EQdisplay[15][i] = CHSV(hue, saturation, EQdisplayValue8[i]);

			}
			else if (fftmode == FFT_MODE_HORZ_BARS_RIGHT){
				EQdisplay[0][i] = CHSV(hue, saturation, EQdisplayValue8[i]);
			}
		}

	}

	if (fftmode == FFT_MODE_HORZ_BARS_STATIC){
		for (uint8_t i = 0; i < 8; i++) {
			uint8_t hue = 0;
			uint8_t saturation = 255;

			if (color_on){
				hue = constrain(map(EQdisplayValue8[i], 240, 255, 0, 64), 0, 64);
			}

			for (uint8_t index = 0; index < 16; index++){
				EQdisplay[index][i] = CHSV(hue, saturation, EQdisplayValue8[i]);
			}
		}
	}


	if (fftmode == FFT_MODE_VERT_BARS_UP || fftmode == FFT_MODE_VERT_BARS_DOWN){
		for (uint8_t i = 0; i < 16; i++) {
			uint8_t hue = 0;
			uint8_t saturation = 255;

			if (color_on){
				hue = constrain(map(EQdisplayValue16[i], 250, 255, 0, 128), 0, 64);
			}
			if (fftmode == FFT_MODE_VERT_BARS_UP){
				EQdisplay[i][0] = CHSV(hue, saturation, EQdisplayValue16[i]);
			}
			else if (fftmode == FFT_MODE_VERT_BARS_DOWN){
				EQdisplay[i][7] = CHSV(hue, saturation, EQdisplayValue16[i]);
			}
		}
	}

	if (fftmode == FFT_MODE_VERT_BARS_STATIC){
		for (uint8_t i = 0; i < 16; i++) {
			uint8_t hue = 0;
			uint8_t saturation = 255;
			if (color_on){
				hue = constrain(map(EQdisplayValue16[i], 250, 255, 0, 128), 0, 64);
			}
			for (uint8_t index = 0; index < 8; index++){
				EQdisplay[i][index] = CHSV(hue, saturation, EQdisplayValue16[i]);
			}
		}
	}
}
