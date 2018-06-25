
typedef void* freeram_handle;

// (un)load the library. returns 1 on success, 0 on failure
int freeram_loadlib();
int freeram_unloadlib();
// Open the ram file for the specified ROM. Pass in the name of the ROM, ending with .smc/.sfc.
// returns NULL on failure. If it fails, it sets *error to a string describing what went wrong.
// You need to free that string yourself.
freeram_handle freeram_open(const char* romname, char** error);
// Close the specified handle, writing out the file again. returns 1 on success, 0 on failure
int freeram_close(freeram_handle handle);
// Get and claim a freeram address. returns a negative number on failure, or snes address on success
// -1 - no matching freeram found
// -2 - library not loaded
// -3 - invalid identifier
// -4 - invalid flags
int freeram_get_ram(freeram_handle handle, int size, const char* identifier, const char* flags);
