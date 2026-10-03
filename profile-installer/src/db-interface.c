#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vendor/sqlite3/sqlite3.h"

#include "db-interface.h"

#define INITIAL_CAPACITY	4

enum InstallerCode write_profile(struct sqlite3_stmt *statement, struct profile *profile);
enum InstallerCode write_text_field(struct sqlite3_stmt *statement, int col, char **field);
enum InstallerCode write_int64_field(struct sqlite3_stmt *statement, int col, uint64_t *field);
enum InstallerCode write_int32_field(struct sqlite3_stmt *statement, int col, uint32_t *field);

enum InstallerCode create_profile(struct sqlite3 *db, struct profile *profile);

int compare_profiles(const void *pa, const void *pb);

enum InstallerCode get_current_profiles(struct sqlite3 *db, struct profile **profiles,
		size_t *profiles_size)
{
	struct profile *result = NULL;
	size_t result_size = INITIAL_CAPACITY,
	       row_count = 0;

	if ((result = malloc(sizeof *result * result_size)) == NULL) {
		fprintf(stderr, "get_current_profiles: failed to allocate result buffer.\n");
		return INSTALLER_FAIL;
	}

	char *sql = 
		"WITH cte AS (\n"
			"SELECT "
				"fingerprint, version, name, source, destination, trigger_type, status, "
				"row_number() OVER (PARTITION BY fingerprint ORDER BY version DESC) AS row_number\n"
			"FROM profiles\n"
		")\n"
		"SELECT fingerprint, version, name, source, destination, trigger_type, status\n"
		"FROM cte\n"
		"WHERE row_number = 1\n"
		"ORDER BY fingerprint;";
	struct sqlite3_stmt *statement;
	if (sqlite3_prepare_v2(db, sql, strlen(sql), &statement, NULL) != SQLITE_OK) {
		fprintf(stderr, "get_current_profiles: failed to prepare statement: %s\n.",
			sqlite3_errmsg(db));
		free(result);
		return INSTALLER_FAIL;
	}

	int step_result;
	for (step_result = sqlite3_step(statement);
			step_result == SQLITE_ROW;
			step_result = sqlite3_step(statement), ++row_count) {
		for ( ; row_count >= result_size; ) {
			// We have to check for zero, otherwise this loop could spin forever
			size_t new_size = result_size > 0 ? result_size * 2 : INITIAL_CAPACITY;
			struct profile *new_ptr = realloc(result, sizeof *result * new_size);
			if (new_ptr == NULL) {
				fprintf(stderr, "get_current_profiles: failed to re-allocate result buffer.\n");
				free(result);
				sqlite3_finalize(statement);
				return INSTALLER_FAIL;
			}
			result = new_ptr;
			result_size = new_size;
		}

		write_profile(statement, &result[row_count]);
	}
	if (step_result != SQLITE_DONE) {
		fprintf(stderr, "get_current_profiles: failed to evaluate row %ld: %s\n.",
			row_count,
			sqlite3_errmsg(db));
		sqlite3_finalize(statement);
		free(result);
		return INSTALLER_FAIL;
	}

	*profiles = row_count > 0 ? result : NULL;
	*profiles_size = row_count;
	return INSTALLER_OK;
}

enum InstallerCode write_profile(struct sqlite3_stmt *statement, struct profile *profile)
{
	if (statement == NULL) {
		fprintf(stderr, "write_to_profile: statement is invalid.\n");
		return INSTALLER_FAIL;
	}
	if (profile == NULL) {
		fprintf(stderr, "write_to_profile: profile is invalid.\n");
		return INSTALLER_FAIL;
	}

	// The `id` field is the sqlite rowid column. It it not used in the application
	// so we set it to `0` to overwrite the initial garbage value.
	profile->id = 0;

	if (write_int64_field(statement, 0, &profile->fingerprint) != INSTALLER_OK) {
		fprintf(stderr, "write_to_profile: Failed to write profile fingerprint.\n");
		return INSTALLER_FAIL;
	}
	if (write_int32_field(statement, 1, &profile->version) != INSTALLER_OK) {
		fprintf(stderr, "write_to_profile: Failed to write profile version.\n");
		return INSTALLER_FAIL;
	}
	if (write_text_field(statement, 2, &profile->name) != INSTALLER_OK) {
		fprintf(stderr, "write_to_profile: Failed to write profile name.\n");
		return INSTALLER_FAIL;
	}
	if (write_text_field(statement, 3, &profile->source) != INSTALLER_OK) {
		fprintf(stderr, "write_to_profile: Failed to write profile source->\n");
		return INSTALLER_FAIL;
	}
	if (write_text_field(statement, 4, &profile->destination) != INSTALLER_OK) {
		fprintf(stderr, "write_to_profile: Failed to write profile destination->\n");
		return INSTALLER_FAIL;
	}

	uint32_t trigger_type_val = 0;
	if (write_int32_field(statement, 5, &trigger_type_val) != INSTALLER_OK) {
		fprintf(stderr, "write_to_profile: Failed to write profile trigger type.\n");
		return INSTALLER_FAIL;
	}
	profile->trigger_type = (uint8_t) trigger_type_val;

	if (write_int32_field(statement, 6, &profile->status) != INSTALLER_OK) {
		fprintf(stderr, "write_to_profile: Failed to write profile status.\n");
		return INSTALLER_FAIL;
	}

	return INSTALLER_OK;
}

enum InstallerCode write_text_field(struct sqlite3_stmt *statement, int col, char **field)
{
	char *buf = NULL;
	const unsigned char *val = sqlite3_column_text(statement, col);
	// +1 need to account for the NUL terminator.
	size_t buf_len = sqlite3_column_bytes(statement, col) + 1;
	if ((buf = malloc(buf_len)) == NULL) {
		fprintf(stderr, "get_text_value: Failed to allocate buffer for col %d.\n", col);
		return INSTALLER_FAIL;
	}
	memcpy(buf, val, buf_len);
	*field = buf;

	return INSTALLER_OK;
}

enum InstallerCode write_int64_field(struct sqlite3_stmt *statement, int col, uint64_t *field)
{
	*field = sqlite3_column_int64(statement, col);
	return INSTALLER_OK;
}

enum InstallerCode write_int32_field(struct sqlite3_stmt *statement, int col, uint32_t *field)
{
	*field = sqlite3_column_int(statement, col);
	return INSTALLER_OK;
}

enum InstallerCode reconcile_profiles(struct sqlite3 *db,
		struct profile *new_profiles, size_t new_prof_len)
{
	struct profile *current_profiles = NULL;
	size_t cur_prof_len = 0;
	if (get_current_profiles(db, &current_profiles, &cur_prof_len) != INSTALLER_OK) {
		fprintf(stderr, "reconcile_profiles: failed to get current profiles.\n");
		// TODO: free all of the profile memory.
		return INSTALLER_FAIL;
	}
	// sort by fingerprints
	qsort(new_profiles, new_prof_len, sizeof *new_profiles, &compare_profiles);

	for (size_t cp_i = 0, np_i = 0; cp_i < cur_prof_len || np_i < new_prof_len; ) {
		// for new_profiles and current_profiles
		//	if DB key < parsed key or no new profiles
		//		retire existing profile
		//	else if DB key > parsed key or no current profiles
		//		insert new profile
		//	else if profiles have diverged
		//		insert new version of existing profile
		//	else
		//		do nothing

		if (cp_i >= cur_prof_len) {
			// There are no more current profiles so the the current
			// profile must be new.
			create_profile(db, &new_profiles[np_i++]);
			continue;
		}

		struct profile current_prof = current_profiles[cp_i],
			       new_prof = new_profiles[np_i];
		if (current_prof.fingerprint > new_prof.fingerprint) {
			create_profile(db, &new_profiles[np_i++]);
			continue;
		}
		else if (current_prof.fingerprint == new_prof.fingerprint) {
			// bump_version(current_prof, new_prof);
			np_i++;
			cp_i++;
		}
		else {
			// We can't handle this case so we simply exit.
			fprintf(stderr,
				"reconcile_profiles: unsupported operation: cp_i = %ld, np_i = %ld\n", cp_i, np_i);
			exit(EXIT_FAILURE);
		}
	}

	return INSTALLER_OK;
}

enum InstallerCode create_profile(struct sqlite3 *db, struct profile *profile)
{
	enum InstallerCode result = INSTALLER_OK;

	char *sql =
		"INSERT INTO profiles (\n"
			"fingerprint, version, name, source, destination, trigger_type, status)\n"
		"VALUES\n"
			"(?1, ?2, ?3, ?4, ?5, ?6, ?7);";
	struct sqlite3_stmt *statement;

	sqlite3_prepare_v2(db, sql, strlen(sql), &statement, NULL);
	sqlite3_bind_int64(statement, 1, profile->fingerprint);
	sqlite3_bind_int(statement, 2, 1);
	sqlite3_bind_text(statement, 3, profile->name, strlen(profile->name), SQLITE_STATIC);
	sqlite3_bind_text(statement, 4, profile->source, strlen(profile->source), SQLITE_STATIC);
	sqlite3_bind_text(statement, 5, profile->destination, strlen(profile->destination), SQLITE_STATIC);
	sqlite3_bind_int(statement, 6, profile->trigger_type);
	sqlite3_bind_int(statement, 7, Active);
	if (sqlite3_step(statement) != SQLITE_DONE) {
		fprintf(stderr,
			"create_profile: failed to insert profile with fingerprint %ld: %s\n",
			profile->fingerprint,
			sqlite3_errmsg(db));
		result = INSTALLER_FAIL;
	}
	sqlite3_finalize(statement);

	return result;
}

int compare_profiles(const void *pa, const void *pb)
{
	const struct profile *a = pa;
	const struct profile *b = pb;

	return (a->fingerprint > b->fingerprint) - (a->fingerprint < b->fingerprint);
}
