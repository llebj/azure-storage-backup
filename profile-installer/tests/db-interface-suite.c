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
			"\"test\","
			"\"/\","
			"\"/.snapshots\","
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
			"\"test\","
			"\"/\","
			"\"/.snapshots\","
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
			"\"test\","
			"\"/\","
			"\"/.snapshots\","
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

// ------------------------
// -- reconcile_profiles --
// ------------------------

void test_it_deletes_a_profile_that_no_longer_exists(void) { }

void test_it_inserts_a_new_profile(void) { }

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
