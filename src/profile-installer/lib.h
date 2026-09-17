#ifndef MAIN
#define MAIN

#include <stdint.h>

enum ProfileStatus {
	Active,
	Retired
};

struct profile {
	uint32_t id;
	uint64_t fingerprint;
	uint32_t version;
	unsigned char *name;
	unsigned char *source;
	unsigned char *destination;
	uint8_t trigger_type;
	enum ProfileStatus status;
};

uint64_t poly_hash(char *string);

#endif
