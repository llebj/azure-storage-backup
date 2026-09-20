#include <stdint.h>

#include "./vendor/sqlite3/sqlite3.h"

enum ProfileStatus {
	Active,
	Retired
};

struct profile {
	uint32_t id;
	uint64_t fingerprint;
	uint32_t version;
	char *name;
	char *source;
	char *destination;
	uint8_t trigger_type;
	enum ProfileStatus status;
};

struct profile * get_current_profiles(struct sqlite3 *db);
