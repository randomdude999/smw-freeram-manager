#include <ctype.h> // isalpha, isalnum
#include <stdio.h> // fopen, fread, fclose
#include <string.h> // strlen, strcmp
#include <stdlib.h> // malloc, calloc, free
#include "cJSON.h"

#if defined(_WIN32)
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C" __attribute__ ((visibility ("default")))
#endif

bool str_ends_with(const char* str, const char* ending) {
	int str_len = strlen(str);
	int end_len = strlen(ending);
	if(end_len > str_len) return false;
	return strcmp(str + str_len - end_len, ending) == 0;
}

bool identifier_is_valid(char* identifier) {
	if(identifier == NULL) return false;
	if(!isalpha(identifier[0]) && identifier[0] != '_') return false; // will also catch empty string
	for(int i = 1; identifier[i]; i++) {
		if(i > 255) return false;
		if(!isalnum(identifier[i]) && identifier[i] != '_') return false;
	}
	return true;
}

char* read_file_into_str(FILE* f) {
	char* buf;
	fseek(f, 0, SEEK_END);
	long length = ftell(f);
	buf = malloc(length);
	fread(buf, 1, length, f);
	return buf;
}

class invalid_ramf_error {};

struct ram_entry {
	int start_addr;
	int length;
	char** flags;
	int flagc;
};

struct claim_entry {
	int start_addr;
	int length;
	char* identifier;
};

class freeram_handle {
	ram_entry* ram_entries;
	long ram_entry_count;

	claim_entry* claim_entries;
	long claim_entry_count;

public:
	freeram_handle(cJSON* data) {
		cJSON* elem;
		cJSON* tmp;
		cJSON* ram = cJSON_GetObjectItemCaseSensitive(data, "ram");
		if(!cJSON_IsArray(ram)) throw invalid_ramf_error();

		int l = cJSON_GetArraySize(ram);
		ram_entries = (ram_entry*)calloc(l, sizeof(ram_entry));
		ram_entry_count = l;

		int i = 0;
		cJSON_ArrayForEach(elem, ram) {

#define err() {free(ram_entries); throw invalid_ramf_error();}

			if(!cJSON_IsObject(elem)) err();

			tmp = cJSON_GetObjectItemCaseSensitive(elem, "addr");
			if(!cJSON_IsNumber(tmp)) err();
			ram_entries[i].start_addr = tmp->valueint;

			tmp = cJSON_GetObjectItemCaseSensitive(elem, "len");
			if(!cJSON_IsNumber(tmp)) err();
			ram_entries[i].length = tmp->valueint;

			cJSON* flags_arr = cJSON_GetObjectItemCaseSensitive(elem, "flags");
			if(!cJSON_IsArray(flags_arr)) err();
			cJSON_ArrayForEach(tmp, flags_arr) {

			}
			i++;
		}
	}

	~freeram_handle() {
		free(ram_entries);
		free(claim_entries);
	}

	int get_ram(int size, char* identifier, char* flags) {
		if(!identifier_is_valid(identifier)) return -3;

	}
};

EXPORT freeram_handle* freeram_open(const char* romname, char** err_str) {
	*err_str = NULL;
	int new_fname_len;
	if(str_ends_with(romname, ".sfc") || str_ends_with(romname, ".smc")) {
		new_fname_len = strlen(romname) - 4 + sizeof(".ramdesc");
	} else {
		new_fname_len = strlen(romname) + sizeof(".ramdesc");
	}
	char* ramf_name = (char*)calloc(new_fname_len, 1);
	strcpy(ramf_name, romname);
	strcpy(ramf_name + new_fname_len - sizeof(".ramdesc"), ".ramdesc");
	FILE* f = fopen(ramf_name, "r");
	if(!f) {
		
		return NULL;
	}
	free(ramf_name);
	char* text = read_file_into_str(f);
	fclose(f);
	cJSON* data = cJSON_Parse(text);
	free(text);
	if(data == NULL) return NULL;
	try {
		freeram_handle* h = new freeram_handle(data);
		cJSON_Delete(data);
		return h;
	} catch (invalid_ramf_error) {
		cJSON_Delete(data);
		return NULL;
	}
};

EXPORT int freeram_close(freeram_handle* handle) {

};

EXPORT int freeram_get_ram(freeram_handle* handle, int size, char* identifier, char* flags) {
	if(handle == NULL) return -2;
	return handle->get_ram(size, identifier, flags);
};