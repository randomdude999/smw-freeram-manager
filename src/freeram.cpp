#include <ctype.h> // isalpha, isalnum
#include <stdio.h> // fopen, fread, fclose
#include <string.h> // strlen, strcmp, strerror
#include <stdlib.h> // malloc, calloc, free
#include <errno.h> // errno
#include "malloc.hpp" // custom new/delete

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

bool identifier_is_valid(const char* identifier) {
	if(identifier == NULL) return false;
	if(!isalpha(identifier[0]) && identifier[0] != '_') return false; // will also catch empty string
	for(int i = 1; identifier[i]; i++) {
		if(i > 255) return false;
		if(!isalnum(identifier[i]) && identifier[i] != '_') return false;
	}
	return true;
}

bool flag_is_valid(const char* flagstr) {
	if(flagstr == NULL) return false;
	if(flagstr[0] == '-' || flagstr[0] == '@') flagstr++;
	return identifier_is_valid(flagstr);
}

char* strdup_(const char* inp) {
	char* out = (char*)malloc(strlen(inp)+1);
	strcpy(out, inp);
	return out;
}

char* read_file_into_str(FILE* f) {
	char* buf;
	fseek(f, 0, SEEK_END);
	long length = ftell(f);
	rewind(f);
	buf = (char*)calloc(length+1, 1); // allocate 1 byte after end for a null terminator just in case
	fread(buf, 1, length, f);
	return buf;
}

// do the (both-ends-inclusive) ranges A and B overlap? If yes, return the overlapping segment
// warning: this was written at 1am and it's completely untested
bool ranges_overlap(int a_start, int a_end, int b_start, int b_end, int& overlap_start, int& overlap_end) {
	if(a_start >= b_start && a_start <= b_end) {
		// overlap
		overlap_start = a_start;
		overlap_end = (b_end > a_end) ? a_end : b_end;
		return true;
	} else if(a_start < b_start && a_end >= b_start) {
		// overlap
		overlap_start = b_start;
		overlap_end = (b_end > a_end) ? a_end : b_end;
		return true;
	} else {
		return false;
	}
}

bool validate_ramdesc_json(cJSON* ramdesc) {
	cJSON* tmp;
	cJSON* elem;
	cJSON* ram = cJSON_GetObjectItemCaseSensitive(ramdesc, "ram");
	if(!cJSON_IsArray(ram)) return false;

	cJSON_ArrayForEach(elem, ram) {
		if(!cJSON_IsObject(elem)) return false;

		tmp = cJSON_GetObjectItemCaseSensitive(elem, "address");
		if(!cJSON_IsNumber(tmp)) return false;

		tmp = cJSON_GetObjectItemCaseSensitive(elem, "length");
		if(!cJSON_IsNumber(tmp)) return false;

		cJSON* flags_arr = cJSON_GetObjectItemCaseSensitive(elem, "flags");
		if(!cJSON_IsArray(flags_arr)) return false;

		cJSON_ArrayForEach(tmp, flags_arr) {
			if(!cJSON_IsString(tmp)) return false;
			if(!flag_is_valid(tmp->valuestring)) return false;
		}
	}

	cJSON* claims = cJSON_GetObjectItemCaseSensitive(ramdesc, "claims");
	if(!cJSON_IsObject(claims)) return false;

	// i love how arrayforeach works on objects too
	cJSON_ArrayForEach(elem, claims) {
		if(!cJSON_IsObject(elem)) return false;
		if(!identifier_is_valid(elem->string)) return false; // object key

		tmp = cJSON_GetObjectItemCaseSensitive(elem, "address");
		if(!cJSON_IsNumber(tmp)) return false;

		tmp = cJSON_GetObjectItemCaseSensitive(elem, "length");
		if(!cJSON_IsNumber(tmp)) return false;

		cJSON* flags_arr = cJSON_GetObjectItemCaseSensitive(elem, "flags");
		if(!cJSON_IsArray(flags_arr)) return false;

		cJSON_ArrayForEach(tmp, flags_arr) {
			if(!cJSON_IsString(tmp)) return false;
			if(!flag_is_valid(tmp->valuestring)) return false;
		}
	}

	return true;
}

bool flags_ok(const char** ram_flags, int ram_flagc, const char** claim_flags, int claim_flagc) {
	for(int i = 0; i < claim_flagc; i++) {
		const char* claim_flg = claim_flags[i];
		bool claim_neg = false;
		if(claim_flg[0] == '-') {
			claim_neg = true;
			claim_flg++;
		}
		bool found = false;
		for(int j = 0; j < ram_flagc; j++) {
			if(strcmp(ram_flags[j], claim_flg) == 0) {
				if(claim_neg) return false;
				bool found = true;
				break;
			}
		}
		if(!found && !claim_neg) return false;
	}
	return true;
}

class flag {
public:
	char* name;
	bool explicit_only;
	bool negative;

	flag(const char* string) {
		negative = false;
		explicit_only = false;
		if(string[0] == '-') {
			negative = true;
			string++;
		} else if(string[0] == '@') {
			explicit_only = true;
			string++;
		}
		name = strdup_(string);
	}

	char* tostring() {
		char* buf = (char*)malloc(strlen(name)+2);
		if(negative) {
			buf[0] = '-';
			strcpy(buf+1, name);
		} else if(explicit_only) {
			buf[0] = '@';
			strcpy(buf+1, name);
		} else {
			strcpy(buf, name);
		}
		return buf;
	}

	~flag() {
		free(name);
	}

	bool operator==(flag& other){
		return strcmp(other.name, name) == 0
			&& other.explicit_only == explicit_only
			&& other.negative == negative;
	}

	bool operator!=(flag& other) {
		return !(*this == other);
	}
};

class flaglist {
public:
	flag* flags;
	int count;

	flaglist(int i_count) {
		flags = (flag*)calloc(i_count, sizeof(flag));
		count = 0;
	}
	// use this after calling the int constructor
	void add_flag(const char* flagstr) {
		new(flags+count) flag(flagstr);
		count++;
	}

	flaglist(const char* flagspec) {
		count = 1;
		for(int i = 0; flagspec[i]; i++)
			if(flagspec[i] == ' ') count++;
		flags = (flag*)calloc(count, sizeof(flag));
		count = 0; // to not fuck up add_flag
		const char* cur_flag = flagspec;
		for(int i = 0; true; i++) {
			if(flagspec[i] == ' ' || flagspec[i] == '\0') {
				int flag_len = flagspec+i - cur_flag;
				char* buf = (char*)malloc(flag_len+1);
				strncpy(buf, cur_flag, flag_len);
				buf[flag_len] = '\0'; // strncpy doesn't add a terminator
				add_flag(buf);
				free(buf);
				cur_flag = flagspec+i+1;
			}
			if(flagspec[i] == '\0') break;
		}
	}

	~flaglist() {
		for(int i = 0; i < count; i++) flags[i].~flag();
		free(flags);
	}

	bool operator==(flaglist& other) {
		if(count != other.count) return false;
		for(int i = 0; i < count; i++) {
			if(flags[i] != other.flags[i]) return false;
		}
		return true;
	}

	flag& operator[](int index) {
		return flags[index];
	}
};

class ram_entry {
public:
	int start_addr;
	int length;
	flaglist flags;

	// flagc is only used for preallocating the flag list
	ram_entry(int i_addr, int i_len, int i_flagc) : flags(i_flagc) {
		start_addr = i_addr;
		length = i_len;
		// flags = flaglist(i_flagc);
	}
	// call after calling constructor to add a flag
	void add_flag(const char* flag) {
		flags.add_flag(flag);
	}

	bool flags_compatible(flaglist& i_flags) {
		for(int i = 0; i < i_flags.count; i++) {
			if(i_flags[i].negative) {
				for(int j = 0; j < flags.count; j++)
					if(strcmp(flags[j].name, i_flags[i].name) == 0)
						return false;
			} else {
				bool ok = false;
				for(int j = 0; j < flags.count; j++) {
					if(strcmp(flags[j].name, i_flags[i].name) == 0) {
						ok = true; break;
					}
				}
				if(!ok) return false;
			}
		}
		for(int i = 0; i < flags.count; i++) {
			// if this freeram has any explicit-only flags...
			if(flags[i].explicit_only) {
				// and the request didn't specify said flags...
				bool ok = false;
				for(int j = 0; j < i_flags.count; j++) {
					if(strcmp(i_flags[j].name, flags[i].name) == 0) {
						ok = true; break;
					}
				}
				// then the flags are incompatible
				if(!ok) return false;
			}
		}
		return true;
	}
};

class claim_entry {
public:
	int start_addr;
	int length;
	char* identifier;
	flaglist flags;

	// flagc is only used for preallocating the flag list
	claim_entry(int i_addr, int i_len, const char* i_id, int i_flagc) : flags(i_flagc) {
		start_addr = i_addr;
		length = i_len;
		identifier = strdup_(i_id);
		// flags = flaglist(i_flagc);
	}
	// call after calling constructor to add a flag
	void add_flag(const char* flagstr) {
		flags.add_flag(flagstr);
	}

	~claim_entry() {
		// i think the flaglist gets destroyed automatically?
		free(identifier);
	}
};

class freeram_handle {
private:
	ram_entry* ram_entries;
	long ram_entry_count;

	claim_entry* claim_entries;
	long claim_entry_count;

public:
	char* open_path;

	freeram_handle(cJSON* data, const char* fname) {
		open_path = strdup_(fname);
		cJSON* elem;
		cJSON* tmp;
		cJSON* ram = cJSON_GetObjectItemCaseSensitive(data, "ram");

		int l = cJSON_GetArraySize(ram);
		ram_entries = (ram_entry*)calloc(l, sizeof(ram_entry));
		ram_entry_count = l;

		int i = 0;
		cJSON_ArrayForEach(elem, ram) {
			int start_addr = cJSON_GetObjectItemCaseSensitive(elem, "address")->valueint;

			int length = cJSON_GetObjectItemCaseSensitive(elem, "length")->valueint;

			cJSON* flags_arr = cJSON_GetObjectItemCaseSensitive(elem, "flags");
			int numflags = cJSON_GetArraySize(flags_arr);
			ram_entry* item = new(ram_entries+i) ram_entry(start_addr, length, numflags);
			cJSON_ArrayForEach(tmp, flags_arr) {
				item->add_flag(tmp->valuestring);
			}
			i++;
		}

		cJSON* claims = cJSON_GetObjectItemCaseSensitive(data, "claims");

		l = cJSON_GetArraySize(claims);
		claim_entries = (claim_entry*)calloc(l, sizeof(claim_entry));
		claim_entry_count = l;

		i = 0;
		cJSON_ArrayForEach(elem, claims) {
			const char* id = elem->string;
			int addr = cJSON_GetObjectItemCaseSensitive(elem, "address")->valueint;
			int len = cJSON_GetObjectItemCaseSensitive(elem, "length")->valueint;

			cJSON* flags_arr = cJSON_GetObjectItemCaseSensitive(elem, "flags");
			int numflags = cJSON_GetArraySize(flags_arr);
			claim_entry* item = new(claim_entries+i) claim_entry(addr, len, id, numflags);
			cJSON_ArrayForEach(tmp, flags_arr) {
				item->add_flag(tmp->valuestring);
			}
			i++;
		}
	}

	cJSON* serialize() {
		cJSON* out = cJSON_CreateObject();
		cJSON* ramarr = cJSON_CreateArray();
		for(int i = 0; i < ram_entry_count; i++) {
			cJSON* ramitem = cJSON_CreateObject();
			cJSON_AddNumberToObject(ramitem, "address", ram_entries[i].start_addr);
			cJSON_AddNumberToObject(ramitem, "length", ram_entries[i].length);
			cJSON* flag_arr = cJSON_CreateArray();
			for(int j = 0; j < ram_entries[i].flags.count; j++) {
				char* flg = ram_entries[i].flags[j].tostring();
				cJSON_AddItemToArray(flag_arr, cJSON_CreateString(flg));
				free(flg);
			}
			cJSON_AddItemToObject(ramitem, "flags", flag_arr);
			cJSON_AddItemToArray(ramarr, ramitem);
		}
		cJSON_AddItemToObject(out, "ram", ramarr);

		cJSON* claimarr = cJSON_CreateObject();
		for(int i = 0; i < claim_entry_count; i++) {
			cJSON* claimitem = cJSON_CreateObject();
			cJSON_AddNumberToObject(claimitem, "address", claim_entries[i].start_addr);
			cJSON_AddNumberToObject(claimitem, "length", claim_entries[i].length);
			cJSON* flag_arr = cJSON_CreateArray();
			for(int j = 0; j < claim_entries[i].flags.count; j++) {
				char* flg = claim_entries[i].flags[j].tostring();
				cJSON_AddItemToArray(flag_arr, cJSON_CreateString(flg));
				free(flg);
			}
			cJSON_AddItemToObject(claimitem, "flags", flag_arr);
			cJSON_AddItemToObject(claimarr, claim_entries[i].identifier, claimitem);
		}
		cJSON_AddItemToObject(out, "claims", claimarr);
		return out;
	}

	~freeram_handle() {
		for(int i = 0; i < ram_entry_count; i++)
			ram_entries[i].~ram_entry();
		free(ram_entries);
		for(int i = 0; i < claim_entry_count; i++)
			claim_entries[i].~claim_entry();
		free(claim_entries);
		free(open_path);
	}

	int get_ram(int size, const char* identifier, const char* i_flags) {
		if(!identifier_is_valid(identifier)) return -3;

		int flagc;
		flaglist request_flags = flaglist(i_flags);

		for(int i = 0; i < claim_entry_count; i++) {
			if(strcmp(claim_entries[i].identifier, identifier) == 0) {
				if(claim_entries[i].length != size) return -5;
				if(!(claim_entries[i].flags == request_flags)) return -5;
				return claim_entries[i].start_addr;
			}
		}

		for(int i = 0; i < ram_entry_count; i++) {
			if(!ram_entries[i].flags_compatible(request_flags)) continue; // incompatible flags
			if(ram_entries[i].length < size) continue; // too small even without any claims

			// now we need to figure out which parts (if any) of this freeram are unused
			// and if there are some, is there a large enough block
			int blkstart = ram_entries[i].start_addr;
			int blksize = ram_entries[i].length;
			bool is_used[blksize] = {false}; // compiler should fill rest with false automatically
			for(int j = 0; j < claim_entry_count; j++) {
				int overlap_start, overlap_end;
				if(ranges_overlap(0, blksize-1, claim_entries[j].start_addr-blkstart, claim_entries[j].start_addr-blkstart+claim_entries[j].length-1, overlap_start, overlap_end)) {
					// what the fuck is that line
					// why do i even code at 1:30am
					for(int k = overlap_start; k <= overlap_end; k++) {
						is_used[k] = true;
					}
				}
			}
			int curblk = 0;
			int cursize = 0;
			bool found = false;
			// find potential gaps in this now
			for(int j = 0; j < blksize; j++) {
				if(is_used[j] == true) {
					cursize = 0;
				} else {
					if(cursize == 0) curblk = j;
					cursize++;
					if(cursize >= size) {
						found = true;
						break;
					}
				}
			}
			if(found) {
				int loc = curblk + blkstart;
				// add the claim
				claim_entry_count++;
				claim_entries = (claim_entry*)realloc(claim_entries, claim_entry_count*sizeof(claim_entry));
				claim_entry* item = new(claim_entries+claim_entry_count-1) claim_entry(loc, size, identifier, request_flags.count);
				for(int i = 0; i < request_flags.count; i++) {
					item->add_flag(request_flags[i].tostring());
				}
				return loc;
			} else {
				continue;
			}
		}
		return -1;
	}

	int unclaim_ram(const char* identifier) {
		for(int i = 0; i < claim_entry_count; i++) {
			if(strcmp(claim_entries[i].identifier, identifier) == 0) {
				claim_entries[i].~claim_entry();
				size_t to_move = (claim_entry_count - i - 1) * sizeof(claim_entry);
				memmove(&claim_entries[i], &claim_entries[i+1], to_move);
				claim_entry_count--;
				return 0;
			}
		}
		return -1;
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
		char* tmp = strerror(errno);
		*err_str = (char*)malloc(strlen(tmp) + sizeof("Error opening ramdesc file: "));
		strcpy(*err_str, "Error opening ramdesc file: ");
		strcat(*err_str, tmp);
		free(ramf_name);
		return NULL;
	}
	char* text = read_file_into_str(f);
	fclose(f);
	cJSON* data = cJSON_Parse(text);
	if(data == NULL) {
		int err_pos = cJSON_GetErrorPtr() - text;
		free(text);
		free(ramf_name);
		*err_str = (char*)malloc(256);
		sprintf(*err_str, "Error parsing JSON at position %d", err_pos);
		return NULL;
	}
	free(text);
	if(validate_ramdesc_json(data)) {
		freeram_handle* h = new freeram_handle(data, ramf_name);
		cJSON_Delete(data);
		free(ramf_name);
		return h;
	} else {
		cJSON_Delete(data);
		*err_str = (char*)malloc(256);
		strcpy(*err_str, "Invalid JSON data found");
		free(ramf_name);
		return NULL;
	}
};

EXPORT int freeram_close(freeram_handle* handle) {
	cJSON* obj = handle->serialize();
	const char* path = handle->open_path;
	FILE* f = fopen(path, "w");
	if(!f) {
		return 0;
	}
	char* text = cJSON_Print(obj);
	fputs(text, f);
	fclose(f);
	free(text);
	cJSON_Delete(obj);
	delete handle;
	return 1;

};

EXPORT int freeram_get_ram(freeram_handle* handle, int size, const char* identifier, const char* flags) {
	if(handle == NULL) return -2;
	return handle->get_ram(size, identifier, flags);
};

EXPORT int freeram_unclaim_ram(freeram_handle* handle, const char* identifier) {
	if(handle == NULL) return -2;
	return handle->unclaim_ram(identifier);
}