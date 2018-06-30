void* operator new(size_t n, void* p);
void* operator new(size_t n);
void* operator new[](size_t n);
void operator delete(void* p);
void operator delete(void* p, size_t n);
extern "C" void __cxa_pure_virtual();
