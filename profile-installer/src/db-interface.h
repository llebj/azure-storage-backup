#include <stddef.h>
#include <stdint.h>

#include "./vendor/sqlite3/sqlite3.h"

#include "lib.h"

struct profile {
	uint32_t id;
	uint64_t fingerprint;
	uint32_t version;
	char *name;
	char *source;
	uint8_t trigger_type;
	enum ProfileStatus status;
};

enum InstallerCode get_current_profiles(struct sqlite3 *db, struct profile **profiles,
		size_t *profiles_size);
enum InstallerCode reconcile_profiles(struct sqlite3 *db,
		struct profile *new_profiles, size_t new_profiles_len);
