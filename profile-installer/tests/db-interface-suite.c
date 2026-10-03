#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../src/vendor/sqlite3/sqlite3.h"

#include "vendor/unity.h"
#include "vendor/unity_internals.h"

#include "../src/db-interface.h"

char *schema;
struct sqlite3 *db;

char * read_schema(char *schema_file);
bool populate_profile(struct sqlite3_stmt *statement, struct profile *profile);
bool populate_text(struct sqlite3_stmt *statement, int col, char **field);

void setUp(void)
{
	sqlite3_open(":memory:", &db);

	char *error = NULL;
	if (sqlite3_exec(db, schema, NULL, NULL, &error) != SQLITE_OK) {
		char message[256];
		snprintf(message, sizeof(message), "Failed to apply schema: %s\n", error);
		sqlite3_free(error);
		TEST_FAIL_MESSAGE(message);
	}
}

void tearDown(void)
{
	sqlite3_close(db);
}

// --------------------------
// -- get_current_profiles --
// --------------------------

void test_it_populates_a_profile_correctly(void)
{
	// Arrange
	char *sql = 
		"INSERT INTO profiles ("
			"fingerprint,"
			"version,"
			"name,"
			"source,"
			"destination,"
			"trigger_type,"
			"status"
		")\n"
		"VALUES ("
			"?1,"
			"?2,"
			"'test',"
			"'/',"
			"'/.snapshots',"
			"1,"
			"0"
		");";
	struct sqlite3_stmt *statement;
	sqlite3_prepare_v2(db, sql, strlen(sql), &statement, NULL);

	sqlite3_bind_int(statement, 1, 1);
	sqlite3_bind_int(statement, 2, 1);
	if (sqlite3_step(statement) != SQLITE_DONE) {
		TEST_FAIL_MESSAGE("SQLite error occurred.\n");
	}
	
	sqlite3_finalize(statement);

	// Act
	struct profile *profiles = NULL;
	size_t profiles_count = 0;
	enum InstallerCode result = get_current_profiles(db, &profiles, &profiles_count);

	// Assert
	TEST_ASSERT_EQUAL_INT(INSTALLER_OK, result);
	TEST_ASSERT_EQUAL_size_t(1, profiles_count);

	TEST_ASSERT_NOT_NULL(profiles);
	TEST_ASSERT_EQUAL_UINT32(0, profiles->id);
	TEST_ASSERT_EQUAL_UINT64(1, profiles->fingerprint);
	TEST_ASSERT_EQUAL_UINT32(1, profiles->version);
	TEST_ASSERT_EQUAL_STRING("test", profiles->name);
	TEST_ASSERT_EQUAL_STRING("/", profiles->source);
	TEST_ASSERT_EQUAL_STRING("/.snapshots", profiles->destination);
	TEST_ASSERT_EQUAL_INT(1, profiles->trigger_type);
	TEST_ASSERT_EQUAL_INT(Active, profiles->status);
}

void test_it_only_retrieves_the_most_recent_version(void) 
{
	// Arrange
	char *sql = 
		"INSERT INTO profiles ("
			"fingerprint,"
			"version,"
			"name,"
			"source,"
			"destination,"
			"trigger_type,"
			"status"
		")\n"
		"VALUES ("
			"?1,"
			"?2,"
			"'test',"
			"'/',"
			"'/.snapshots',"
			"1,"
			"0"
		");";
	struct sqlite3_stmt *statement;
	sqlite3_prepare_v2(db, sql, strlen(sql), &statement, NULL);

	uint32_t versions[] = {1,3,2};
	size_t limit = sizeof(versions) / sizeof(versions[0]);
	for (size_t i = 0; i < limit; ++i) {
		sqlite3_bind_int(statement, 1, 1);
		sqlite3_bind_int(statement, 2, versions[i]);
		if (sqlite3_step(statement) != SQLITE_DONE) {
			TEST_FAIL_MESSAGE("SQLite error occurred.\n");
		}
		sqlite3_reset(statement);
	}

	sqlite3_finalize(statement);

	// Act
	struct profile *profiles = NULL;
	size_t profiles_count = 0;
	enum InstallerCode result = get_current_profiles(db, &profiles, &profiles_count);

	// Assert
	TEST_ASSERT_EQUAL_INT(INSTALLER_OK, result);
	TEST_ASSERT_EQUAL_size_t(1, profiles_count);
	TEST_ASSERT_NOT_NULL(profiles);
	TEST_ASSERT_EQUAL_UINT32(3, profiles->version);
}

void test_it_returns_profiles_sorted_by_fingerprint_ascending(void)
{
	// Arrange
	char *sql = 
		"INSERT INTO profiles ("
			"fingerprint,"
			"version,"
			"name,"
			"source,"
			"destination,"
			"trigger_type,"
			"status"
		")\n"
		"VALUES ("
			"?1,"
			"1,"
			"'test',"
			"'/',"
			"'/.snapshots',"
			"1,"
			"0"
		");";
	struct sqlite3_stmt *statement;
	sqlite3_prepare_v2(db, sql, strlen(sql), &statement, NULL);

	uint64_t input[] = {3,2,1};
	size_t limit = sizeof(input) / sizeof(input[0]);
	for (size_t i = 0; i < limit; ++i) {
		sqlite3_bind_int(statement, 1, input[i]);
		if (sqlite3_step(statement) != SQLITE_DONE) {
			TEST_FAIL_MESSAGE("SQLite error occurred.\n");
		}
		sqlite3_reset(statement);
	}

	sqlite3_finalize(statement);

	// Act
	struct profile *profiles = NULL;
	size_t profiles_count = 0;
	enum InstallerCode result = get_current_profiles(db, &profiles, &profiles_count);

	// Assert
	TEST_ASSERT_EQUAL_INT(INSTALLER_OK, result);
	TEST_ASSERT_EQUAL_size_t(3, profiles_count);
	TEST_ASSERT_NOT_NULL(profiles);
	uint64_t expected[] = {1,2,3};
	for (size_t i = 0; i < profiles_count; ++i) {
		TEST_ASSERT_EQUAL_UINT64(expected[i], profiles[i].fingerprint);
	}
}

void test_if_no_profiles_exist_it_returns_zero_and_an_empty_pointer(void)
{
	// Arrange
	// Act
	struct profile *profiles = NULL;
	size_t profiles_count = 0;
	enum InstallerCode result = get_current_profiles(db, &profiles, &profiles_count);

	// Assert
	TEST_ASSERT_EQUAL_INT(INSTALLER_OK, result);
	TEST_ASSERT_EQUAL_size_t(0, profiles_count);
	TEST_ASSERT_NULL(profiles);
}

// ------------------------
// -- reconcile_profiles --
// ------------------------

void test_it_retires_a_profile_if_it_no_longer_exists(void) { }

void test_it_retires_all_profiles_if_none_are_provided(void) { }

void test_it_inserts_a_new_profile_for_a_fresh_install(void)
{
	// Arrange
	struct profile profiles[1];
	profiles->fingerprint = 1;
	profiles->name = "test";
	profiles->source = "/";
	profiles->destination = "/.snapshots";
	profiles->trigger_type = 1;

	// Act
	enum InstallerCode result = reconcile_profiles(db, profiles, sizeof(profiles) / sizeof(*profiles));

	// Assert
	char *assert_sql = "SELECT\n"
		"fingerprint,\n"
		"version,\n"
		"name,\n"
		"source,\n"
		"destination,\n"
		"trigger_type,\n"
		"status\n"
		"FROM profiles WHERE fingerprint = ?1;";
	struct sqlite3_stmt *assert_statement;
	sqlite3_prepare_v2(db, assert_sql, strlen(assert_sql), &assert_statement, NULL);
	sqlite3_bind_int(assert_statement, 1, profiles->fingerprint);

	// Unity jumps on test failure so we must extract the contents and call
	// `finalize` before making any assertions.
	struct profile actual = {0};
	int row = sqlite3_step(assert_statement);
	populate_profile(assert_statement, &actual);
	int done = sqlite3_step(assert_statement);
	sqlite3_finalize(assert_statement);

	TEST_ASSERT_EQUAL_INT(SQLITE_ROW, row);
	TEST_ASSERT_EQUAL_INT64(profiles->fingerprint, actual.fingerprint);
	TEST_ASSERT_EQUAL_INT(1, actual.version);
	TEST_ASSERT_EQUAL_STRING(profiles->name, actual.name);
	TEST_ASSERT_EQUAL_STRING(profiles->source, actual.source);
	TEST_ASSERT_EQUAL_STRING(profiles->destination, actual.destination);
	TEST_ASSERT_EQUAL_INT(profiles->trigger_type, actual.trigger_type);
	TEST_ASSERT_EQUAL_INT(Active, actual.status);
	// We only expect a single row of data
	TEST_ASSERT_EQUAL_INT(SQLITE_DONE, done);
}

void test_it_inserts_a_new_profile_for_an_existing_install(void)
{
	// Arrange
	struct profile profiles[2];

	profiles[0].fingerprint = 1;
	profiles[0].name = "root";
	profiles[0].source = "/";
	profiles[0].destination = "/.snapshots";
	profiles[0].trigger_type = 1;

	profiles[1].fingerprint = 2;
	profiles[1].name = "home";
	profiles[1].source = "/home";
	profiles[1].destination = "/.snapshots";
	profiles[1].trigger_type = 1;

	// Insert the 'root' profile as the existing profile.
	char *init_sql =
		"INSERT INTO profiles (\n"
			"fingerprint, version, name, source, destination, trigger_type, status)\n"
		"VALUES\n"
			"(1, 1, 'root', '/', '/.snapshots', 1, 0);";
	sqlite3_exec(db, init_sql, NULL, NULL, NULL);

	// Act
	enum InstallerCode result = reconcile_profiles(db, profiles, sizeof(profiles) / sizeof(*profiles));

	// Assert
	char *assert_sql = "SELECT\n"
		"fingerprint,\n"
		"version,\n"
		"name,\n"
		"source,\n"
		"destination,\n"
		"trigger_type,\n"
		"status\n"
		"FROM profiles ORDER BY fingerprint;";
	struct sqlite3_stmt *assert_statement;
	sqlite3_prepare_v2(db, assert_sql, strlen(assert_sql), &assert_statement, NULL);

	struct profile actual[2] = {0};
	int row_one = sqlite3_step(assert_statement);
	populate_profile(assert_statement, &actual[0]);
	int row_two = sqlite3_step(assert_statement);
	populate_profile(assert_statement, &actual[1]);
	int done = sqlite3_step(assert_statement);
	sqlite3_finalize(assert_statement);

	TEST_ASSERT_EQUAL_INT(SQLITE_ROW, row_one);
	TEST_ASSERT_EQUAL_INT64(profiles[0].fingerprint, actual[0].fingerprint);
	TEST_ASSERT_EQUAL_INT(1, actual[0].version);
	TEST_ASSERT_EQUAL_STRING(profiles[0].name, actual[0].name);
	TEST_ASSERT_EQUAL_STRING(profiles[0].source, actual[0].source);
	TEST_ASSERT_EQUAL_STRING(profiles[0].destination, actual[0].destination);
	TEST_ASSERT_EQUAL_INT(profiles[0].trigger_type, actual[0].trigger_type);
	TEST_ASSERT_EQUAL_INT(Active, actual[0].status);

	// The existing profile is uniquely identified by its fingerprint and
	// version.
	TEST_ASSERT_EQUAL_INT(SQLITE_ROW, row_two);
	TEST_ASSERT_EQUAL_INT64(profiles[1].fingerprint, actual[1].fingerprint);
	TEST_ASSERT_EQUAL_INT(1, actual[1].version);

	// We expect two rows of data; one for the existing profile, and another
	// for the new profile.
	TEST_ASSERT_EQUAL_INT(SQLITE_DONE, done);
}

void test_it_creates_a_new_version_of_an_existing_profile(void) { }

void test_it_does_not_change_a_profile_that_has_not_changed(void) { }

// ----------
// -- main --
// ----------

int main(void)
{
	schema = read_schema("../../scripts/create.sql");
	if (schema == NULL) {
		fprintf(stderr, "Failed to load schema file\n.");
		exit(EXIT_FAILURE);
	}

	UNITY_BEGIN();

	RUN_TEST(test_it_populates_a_profile_correctly);
	RUN_TEST(test_it_only_retrieves_the_most_recent_version);
	RUN_TEST(test_it_returns_profiles_sorted_by_fingerprint_ascending);
	RUN_TEST(test_if_no_profiles_exist_it_returns_zero_and_an_empty_pointer);

	RUN_TEST(test_it_inserts_a_new_profile_for_a_fresh_install);
	RUN_TEST(test_it_inserts_a_new_profile_for_an_existing_install);

	return UNITY_END();
}

char * read_schema(char *schema_file)
{
	int fd;
	if ((fd = open(schema_file, O_RDONLY)) == -1) {
		return NULL;
	}

	struct stat sb;
	if (fstat(fd, &sb) == -1) {
		close(fd);
		return NULL;
	}
	
	char *fb;
	if ((fb = malloc((sizeof *fb * sb.st_size) + 1)) == NULL) {
		close(fd);
		return NULL;
	}

	size_t bytes_read = 0;
	for ( ; bytes_read < sb.st_size; ) {
		ssize_t new_bytes = read(fd, fb + bytes_read, sb.st_size - bytes_read);
		if (new_bytes < 0) {
			break;
		}
		bytes_read += new_bytes;
	}
	if (bytes_read < sb.st_size) {
		close(fd);
		free(fb);
		return NULL;
	}
	fb[sb.st_size] = '\0';

	return fb;
}

bool populate_profile(struct sqlite3_stmt *statement, struct profile *profile)
{
	profile->id = 0;
	profile->fingerprint = sqlite3_column_int64(statement, 0);
	profile->version = sqlite3_column_int(statement, 1);

	if (populate_text(statement, 2, &profile->name) != INSTALLER_OK) {
		return true;
	}
	if (populate_text(statement, 3, &profile->source) != INSTALLER_OK) {
		return true;
	}
	if (populate_text(statement, 4, &profile->destination) != INSTALLER_OK) {
		return true;
	}

	profile->trigger_type = (uint8_t) sqlite3_column_int(statement, 5);
	profile->status = sqlite3_column_int(statement, 6);

	return false;
}

bool populate_text(struct sqlite3_stmt *statement, int col, char **field)
{
	char *buf = NULL;
	const unsigned char *val = sqlite3_column_text(statement, col);
	// +1 need to account for the NUL terminator.
	size_t buf_len = sqlite3_column_bytes(statement, col) + 1;
	if ((buf = malloc(buf_len)) == NULL) {
		fprintf(stderr, "write_text_value: Failed to allocate buffer for col %d.\n", col);
		return true;
	}
	memcpy(buf, val, buf_len);
	*field = buf;

	return false;
}
