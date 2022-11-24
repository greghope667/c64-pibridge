#include <stdio.h>
#include <string.h>

#include "../src/frame.h"

int main(void)
{
	frame buffer;

	for (; ! feof(stdin);) {
		memset(&buffer, 0, sizeof(buffer));

		size_t n = fread(&buffer.data, 1, FRAME_DATA_SIZE, stdin);
		buffer.length = n;

		if (feof(stdin))
			buffer.type = FRAME_EOI;

		fwrite(&buffer, sizeof(buffer), 1, stdout);
		fflush(stdout);
	}
}
