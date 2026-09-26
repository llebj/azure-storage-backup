#include <stddef.h>
#include <stdint.h>

#include "./vendor/sqlite3/sqlite3.h"

enum InstallerCode {
	INSTALLER_OK = 0,
	INSTALLER_FAIL = 1
};
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

enum InstallerCode get_current_profiles(struct sqlite3 *db, struct profile **profiles,
		size_t *profiles_size);
