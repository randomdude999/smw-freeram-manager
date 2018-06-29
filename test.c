#include "interface/freeram.h"
#include <stdio.h>

int main() {
	if(!freeram_loadlib()) {
		fprintf(stderr, "Couldn't load library\n");
		return 1;
	}
	char* error;
	freeram_handle* handle = freeram_open("test.smc", &error);
	if(!handle) {
		fprintf(stderr, "Couldn't load ramfile: %s\n", error);
		return 1;
	}
	fprintf(stderr, "Claimed freeram: %08X\n", freeram_get_ram(handle, 2, "test", "addr clear_ow"));

	freeram_close(handle);
	freeram_unloadlib();
}