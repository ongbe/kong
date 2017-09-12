#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t string_to_bytes(void *dest, size_t len, char *str)
{
	// FIXME: ugly algorithm

	char buffer[1024];
	char *index_str = str;
	/*void*/ char *index_dest = (char *)dest;

	while (index_str < str + strlen(str)) {
		sscanf(index_str, "%[^#\r\n]", buffer);
		index_str += strlen(buffer) + 1; // +1 means skipping #

		if (index_dest + strlen(buffer) > (char *)dest + len)
			break;

		if (buffer[0] == 0x30 && (buffer[1] == 0x58 || buffer[1] == 0x78)) {
			for (size_t i = 2; i < strlen(buffer); i += 2) {
				char tmp_str[3];
				int tmp_int;
				tmp_str[0] = buffer[i];
				tmp_str[1] = buffer[i+1];
				tmp_str[2] = 0;
				tmp_int = strtol(tmp_str, NULL, 16);
				memset(index_dest++, tmp_int, 1);
			}
		} else {
			memcpy(index_dest, buffer, strlen(buffer));
			index_dest += strlen(buffer);
		}
	}

	return index_dest - (char *)dest;
}
