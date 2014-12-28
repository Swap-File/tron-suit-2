//ripped this out of PacketSerial.h so that I can use it directly...
uint8_t COBSencode(const uint8_t* source, uint8_t size, uint8_t* destination)
{
	uint8_t read_index = 0;
	uint8_t write_index = 1;
	uint8_t code_index = 0;
	uint8_t code = 1;

	while (read_index < size)
	{
		if (source[read_index] == 0)
		{
			destination[code_index] = code;
			code = 1;
			code_index = write_index++;
			read_index++;
		}
		else
		{
			destination[write_index++] = source[read_index++];
			code++;

			if (code == 0xFF)
			{
				destination[code_index] = code;
				code = 1;
				code_index = write_index++;
			}
		}
	}

	destination[code_index] = code;

	return write_index;
}

//ripped this out of PacketSerial.h so that I can use it directly...
uint8_t COBSdecode(const uint8_t* source, uint8_t size, uint8_t* destination)
{
	uint8_t read_index = 0;
	uint8_t write_index = 0;
	uint8_t code;
	uint8_t i;

	while (read_index < size)
	{
		code = source[read_index];

		if (read_index + code > size && code != 1)
		{
			return 0;
		}

		read_index++;

		for (i = 1; i < code; i++)
		{
			destination[write_index++] = source[read_index++];
		}

		if (code != 0xFF && read_index != size)
		{
			destination[write_index++] = '\0';
		}
	}

	return write_index;
}