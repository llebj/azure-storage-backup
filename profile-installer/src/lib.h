#ifndef MAIN
#define MAIN

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SIZE_BUF	4

enum InstallerCode {
	PI_OK		= 0,
	PI_FAIL		= 1,
	PI_ALLOC_FAIL	= 2,
	PI_READ_FAIL	= 3,
	PI_WRITE_FAIL	= 4,
	PI_PARSE_FAIL	= 5
};
enum ProfileStatus {
	Active,
	Retired
};
enum TriggerType {
	PreInstall	= 1,
	PostInstall	= 2
};

struct slice {
	char* start;
	size_t size;
};
struct ini {
	size_t size_section;
	struct section *sections;
	struct attribute *attributes;
};
struct section {
	struct slice header;
	struct slice tag;
	size_t attribs_size;
	size_t attribs_offset;
 };
 struct attribute {
	struct slice key;
	struct slice value;
};

struct profile {
	uint32_t id;
	uint64_t fingerprint;
	uint32_t version;
	char *name;
	char *source;
	uint8_t trigger_type;
	enum ProfileStatus status;
};

struct sub_token {
	bool is_replacement;
	struct slice token;
};
struct sub {
	struct slice token;
	struct slice replacement;
};

struct slice trim(struct slice string);

enum InstallerCode parse_ini(struct slice *buf, struct ini *ini);

uint64_t poly_hash(char *string);

enum InstallerCode process_stream(
		enum InstallerCode (*fill)(void *ctx, char *, size_t, size_t *),
		enum InstallerCode (*flush)(void *ctx, char *, size_t, size_t *),
		void *ctx,
		struct sub *subs,
		size_t subs_size);
struct sub_token next_token(char *in, size_t size);

#endif
