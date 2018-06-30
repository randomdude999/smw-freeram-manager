A freeram manager for SMW (and possibly other SNES games)

This is a dynamic library and a C interface for managing freeram.
For the format of the ramdesc file, see ramfile_description.txt.

Building: Use cmake. On linux, cmake . && make

Usage in C:

#include "freeram.h"

freeram_loadlib();
char* error;
freeram_handle h = freeram_open(rom_name, &error);
if(!h) {
	printf("Error opening ramdesc file: %s", error);
}
// Requests a free ram address with size of needed_size bytes, an ID "some_id"
// unique for this freeram, and the flags "addr" and "clear_lvlload" (with the
// standard ramdesc, this would mean a freeram in the range $0000-$1FFF which
// is cleared on level load).
// If you know that there's a flag that your freeram must not have, you can use
// "-flag", such as -dp to request freeram outside the direct page.
int addr = freeram_get_ram(h, needed_size, "some_id", "addr clear_lvlload");
if(addr < 0) {
	// handle the error. check freeram.h for possible errors
}

// when you run the same command again later, you'll just get the same ram
// again. This means you can run it every time you need the address and not
// worry if it has been allocated already

// if you aren't using a ram address anymore, run:
freeram_unclaim_ram(h, "some_id");

// once you are done with allocating/freeing ram, run:
freeram_close(h);
freeram_unloadlib();
