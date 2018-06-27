#include "interface/freeram.h"
#include <stdio.h>

int main() {
	if(!freeram_loadlib()) {
		fprintf(stderr, "Couldn't load library");
		return 1;
	}
	char* error;
	freeram_handle* handle = freeram_open("test.smc", &error);
	if(!handle) {
		fprintf(stderr, "Couldn't load ramfile: %s", error);
		return 1;
	}
	fprintf(stderr, "Claimed freeram: %08X", freeram_get_ram(handle, 32, "test", "+addr +clear_ow"));

	freeram_close(handle);
	freeram_unloadlib();
}