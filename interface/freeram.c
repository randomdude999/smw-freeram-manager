// loader for the freeram library
// most of this code is adapted from Asar's asardll.c
#if defined(_WIN32)
#	include <Windows.h>

#	define getlib() LoadLibrary("freeram.dll")
//#	define loadraw(name, target) *((int(**)(void))&target)=(int(*)(void))GetProcAddress((HINSTANCE)asardll, name); require(target)
#	define getptr(name) GetProcAddress((HINSTANCE)freeramdll, name)
#	define closelib() FreeLibrary((HINSTANCE)freeramdll)
#else
#	include <dlfcn.h>
#	include <stdio.h>

#	ifdef __APPLE__
#		define EXTENSION ".dylib"
#	else
#		define EXTENSION ".so"
#	endif

	inline static void * getlib(void)
	{
		const char * names[]={"./libfreeram" EXTENSION, "libfreeram" EXTENSION, NULL};
		for (int i=0;names[i];i++) {
			void * rval=dlopen(names[i], RTLD_LAZY);
			const char*e=dlerror();
			if(e)puts(e);
			if (rval) return rval;
		}
		return NULL;
	}

#	define getptr(name) dlsym(freeramdll, name)
#	define closelib() dlclose(freeramdll)
#endif

#include "freeram.h"

static void* freeramdll = NULL;

static freeram_handle (*dll_openfunc)(char*);
static int (*dll_closefunc)(freeram_handle);
static int (*dll_getramfunc)(freeram_handle, int, char*, char*);

int freeram_loadlib() {
	freeramdll = getlib();
	if(freeramdll == NULL) return 0;
	dll_openfunc = getptr("freeram_open");
	if(!dll_openfunc) {freeramdll = NULL; return 0;}
	dll_closefunc = getptr("freeram_close");
	if(!dll_closefunc) {freeramdll = NULL; return 0;}
	dll_getramfunc = getptr("freeram_get_ram");
	if(!dll_getramfunc) {freeramdll = NULL; return 0;}
	return 1;
}
int freeram_unloadlib() {
	closelib();
}

freeram_handle freeram_open(char* romname) {
	if(!freeramdll) return NULL;
	return dll_openfunc(romname);
}

int freeram_close(freeram_handle handle) {
	if(!freeramdll) return 0;
	return dll_closefunc(handle);
}

int freeram_get_ram(freeram_handle handle, int size, char* identifier, char* flags) {
	if(!freeramdll) return -2;
	return dll_getramfunc(handle, size, identifier, flags);
}