#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../src/vendor/sqlite3/sqlite3.h"

#include "vendor/unity.h"
#include "vendor/unity_internals.h"

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

void test_it_only_retrieves_the_most_recent_version(void) 
{

}

void test_it_returns_profiles_sorted_by_fingerprint_ascending(void) {}

int main(void)
{
	schema = read_schema("../../scripts/create.sql");
	if (schema == NULL) {
		fprintf(stderr, "Failed to load schema file\n.");
		exit(EXIT_FAILURE);
	}

	UNITY_BEGIN();

	RUN_TEST(test_it_only_retrieves_the_most_recent_version);

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
