#ifndef MAIN
#define MAIN

#include <stdint.h>

enum InstallerCode {
	PI_OK = 0,
	PI_FAIL = 1,
	PI_ALLOC_FAIL = 2,
	PI_READ_FAIL = 3
};
enum ProfileStatus {
	Active,
	Retired
};
enum TriggerType {
	PreInstall = 1,
	PostInstall = 2
};

uint64_t poly_hash(char *string);

#endif
