#ifndef MAIN
#define MAIN

#include <stdint.h>

enum InstallerCode {
	INSTALLER_OK = 0,
	INSTALLER_FAIL = 1
};
enum ProfileStatus {
	Active,
	Retired
};
enum TriggerType {
	Timer = 1,
	PreInstall = 2,
	PostInstall = 4
};

uint64_t poly_hash(char *string);

#endif
