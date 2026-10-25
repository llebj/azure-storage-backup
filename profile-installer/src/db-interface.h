#include <stddef.h>
#include <stdint.h>

#include "./vendor/sqlite3/sqlite3.h"

#include "lib.h"

enum InstallerCode get_current_profiles(struct sqlite3 *db, struct profile **profiles,
		size_t *profiles_size);
enum InstallerCode reconcile_profiles(struct sqlite3 *db,
		struct profile *new_profiles, size_t new_profiles_len);
