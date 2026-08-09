#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY_BUF_SIZE 32
#define VAL_BUF_SIZE 1024

enum ProfileType {
	PreInstall
};

struct profile {
	char* name;
	char* source;
	char* destination;
	enum ProfileType type;
};

uint8_t config_line(FILE *fp, char *key, char *value);

int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "%s: incorrect number of parameters profided.\n", argv[0]);
		exit(0);
	}
	FILE *config;
	if ((config = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "%s: failed to open config file: %s\n", argv[0], argv[1]);
		exit(1);
	}

	struct profile profile = {
		.name = NULL,
		.type = PreInstall,
		.source = NULL,
		.destination = NULL
	};
	char key[KEY_BUF_SIZE] = {0},
	     value[VAL_BUF_SIZE] = {0};
	// TODO: Fix termination condition (EOF)
	do {
		if (config_line(config, key, value) != 0) {
			fprintf(stderr, "%s: failed to parse config file.\n", argv[0]);
			exit(2);
		}
		char* ptr;
		if (strcmp(key, "Name") == 0) {
			if ((ptr = malloc(strlen(value))) == NULL) {
				exit(2);
			}
			strcpy(ptr, value);
			profile.name = ptr;
		}
		else if (strcmp(key, "Source") == 0) {
			if ((ptr = malloc(strlen(value))) == NULL) {
				exit(2);
			}
			strcpy(ptr, value);
			profile.source = ptr;
		}
		else if (strcmp(key, "Destination") == 0) {
			if ((ptr = malloc(strlen(value))) == NULL) {
				exit(2);
			}
			strcpy(ptr, value);
			profile.destination = ptr;
		}
		else if (strcmp(key, "Type") == 0) {
			continue;
		}
		else {
			fprintf(stderr, "%s: invalid parameter found in config file: %s\n",
				argv[0], key);
			exit(2);
		}
	} while (!feof(config));
}

uint8_t config_line(FILE *fp, char *key, char *value)
{
	uint8_t result = 0;
	key[0] = '\0';
	value[0] = '\0';
	// We always start by parsing the key.
	char* buf = key;
	uint32_t limit = KEY_BUF_SIZE,
		 current = 0;

	for (int32_t c = getc(fp); c != EOF || c != '\n'; c = getc(fp)) {
		if (isblank(c)) {
			continue;
		}
		if (current >= limit) {
			// TODO: Better error handling.
			result = 1;
			break;
		}
		if (c == '=') {
			buf[current] = '\0';
			// Swap to parsing the value instead of the key.
			buf = value;
			limit = VAL_BUF_SIZE;
			current = 0;
			continue;
		}
		if (c == '\n') {
			buf[current] = '\0';
			// We have finished parsing the current line.
			break;
		}

		buf[current++] = c;
	}

	return result;
}
