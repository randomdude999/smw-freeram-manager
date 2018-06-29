#include "interface/freeram.h"
#include <stdio.h>

int main(int argc, char** argv) {
	if(!freeram_loadlib()) {
		printf("Error: Couldn't load library\n");
		return 1;
	}
	if(argc != 5) {
		printf("Usage: %s <romname> <id> <size> <flags>\n", argv[0]);
		return 1;
	}
	char* error;
	freeram_handle* handle = freeram_open(argv[1], &error);
	if(!handle) {
		printf("Couldn't load ramfile: %s\n", error);
		return 1;
	}
	int size;
	sscanf(argv[3], "%i", &size);
	int freeram = freeram_get_ram(handle, size, argv[2], argv[4]);
	if(freeram < 0) {
		printf("Error: get_ram returned %d\n", freeram);
	} else {
		printf("Claimed freeram: $%06X\n", freeram);
	}

	freeram_close(handle);
	freeram_unloadlib();
}