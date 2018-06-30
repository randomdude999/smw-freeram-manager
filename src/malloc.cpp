// adapted from arlib. avoids linking against libstdc++ on windows
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include "malloc.hpp"
#ifdef _WIN32
static void malloc_fail(size_t size) {
	if (size > 0) printf("malloc failed, size %" PRIuPTR "\n", size);
	else puts("malloc failed, size unknown");
	abort();
}

void* malloc_check(size_t size) {
	void* ret=malloc(size);
	if (size && !ret) malloc_fail(size);
	return ret;
}
void* operator new(size_t n, void* p) {
	return p;
}
void* operator new(size_t n) {
	return malloc_check(n);
}
void* operator new[](size_t n) {
	return malloc_check(n);
}
void operator delete(void* p) {
	free(p);
}
void operator delete(void* p, size_t a) {
	free(p);
}
extern "C" void __cxa_pure_virtual() {
	puts("__cxa_pure_virtual"); 
	abort();
}
#endif
