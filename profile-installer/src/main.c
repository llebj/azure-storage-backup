#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <systemd/sd-id128.h>

#include "db-interface.h"
#include "lib.h"
#include "parser.h"
#include "vendor/sqlite3/sqlite3.h"

#define APP_ID		SD_ID128_MAKE(e1,dc,79,91,d6,3a,45,36,09,0d,04,2a,c8,2a,50,91)

struct hook_ctx {
	int fd_in;
	size_t in_consumed,
	       in_size;
	int fd_out;
	size_t out_written;
};

static enum InstallerCode fill(void *ctx, char *buf, size_t buf_size, size_t *consumed)
{
    struct hook_ctx *hook_ctx = ctx;
    size_t remaining = hook_ctx->in_size - hook_ctx->in_consumed,
	   wanted = remaining < buf_size ? remaining : buf_size;
    size_t bytes_read = read(hook_ctx->fd_in, buf, wanted);
    if (bytes_read <= 0) {
	    return PI_READ_FAIL;
    }

    *consumed = bytes_read;
    hook_ctx->in_consumed += *consumed;

    return PI_OK;
}

static enum InstallerCode flush(void *ctx, char *buf, size_t buf_size, size_t *written)
{
    struct hook_ctx *hook_ctx = ctx;
    size_t bytes_written = write(hook_ctx->fd_out, buf, buf_size);
    if (bytes_written < 0) {
	    return PI_WRITE_FAIL;
    }

    *written = bytes_written;
    hook_ctx->out_written += buf_size;

    return PI_OK;
}

enum InstallerCode read_profiles(int fd, ssize_t size, char *buf);
struct profile * map_profiles(struct file_profile *file_profiles, size_t count);
struct slice slice_from(char *s);
char * make_message(char *format, ...);

static char db_path[]				= "/home/dev/.local/share/az-backup.db";
static char input_hook_path[]			= "./template.hook";
static char dest_suffix[]			= "/.snapshots";

static char source_token[]			= "Source";
static char destination_token[]			= "Destination";
static char type_token[]			= "Type";

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <path>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	struct stat sb;
	if (stat(argv[1], &sb) == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}
	if (!S_ISREG(sb.st_mode)) {
		fprintf(stderr, "File %s is not a regular file\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	int fd;
	if ((fd = open(argv[1], O_RDONLY)) == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	char *fb;
	if ((fb = malloc(sizeof *fb * sb.st_size + 1)) == NULL) {
		close(fd);
		fprintf(stderr, "Failed to allocate file buffer.\n");
		exit(EXIT_FAILURE);
	}

	if (read_profiles(fd, sb.st_size, fb) != PI_OK) {
		close(fd);
		free(fb);
		fprintf(stderr, "Failed to read profile contents.\n");
		exit(EXIT_FAILURE);
	};

	struct ini ini;
	struct slice fb_slice = {
		.start = fb,
		.size = sb.st_size
	};
	if (parse_ini(&fb_slice, &ini) != PI_OK) {
		close(fd);
		free(fb);
		fprintf(stderr, "Failed to parse file to INI.\n");
		exit(EXIT_FAILURE);
	}

	struct file_profile *file_profiles;
	if ((file_profiles = malloc(sizeof *file_profiles * ini.size_section)) == NULL) {
		close(fd);
		free(ini.attributes);
		free(ini.sections);
		free(fb);
		fprintf(stderr, "Failed to allocate file profiles buffer.\n");
		exit(EXIT_FAILURE);
	}
	if (parse_profiles(&ini, file_profiles) != PI_OK) {

		close(fd);
		free(file_profiles);
		free(ini.attributes);
		free(ini.sections);
		free(fb);
		fprintf(stderr, "Failed to parse profiles.\n");
		exit(EXIT_FAILURE);
	}

	// compute fingerprints for all new profiles
	struct profile *new_profiles = map_profiles(file_profiles, ini.size_section);
	if (new_profiles == NULL) {
		fprintf(stderr, "Failed to map profiles.\n");
		exit(EXIT_SUCCESS);
	}

	struct sqlite3 *db = NULL;
	if (sqlite3_open_v2(db_path, &db,
				SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) {
		fprintf(stderr, "Failed to open program database: %s\n", sqlite3_errmsg(db));
		// TODO: free all of the profile memory.
		exit(EXIT_FAILURE);
	}

	// reconcile_profiles
	reconcile_profiles(db, new_profiles, ini.size_section);

	// open template for reading
	// extract_hook_contents
	// for hooks
	//	open output hook for writing
	//	write hook

	struct stat template_file;
	stat(input_hook_path, &template_file);
	int template_fd = open(input_hook_path, O_RDONLY);

	for (size_t prof_i = 0; prof_i < ini.size_section; ++prof_i) {
		uint8_t mask = 1;
		struct profile profile = new_profiles[prof_i];
		if (profile.trigger_type == 0) {
			// This should never happen.
			fprintf(stderr, "Trigger type cannot be 0\n");
			exit(EXIT_FAILURE);
		}

		uint8_t trigger_flags = profile.trigger_type;
		for ( ; ; ) {
			if (trigger_flags == 0) {
				break;
			}

			uint8_t trigger_bit = profile.trigger_type & mask;
			mask <<= 1;
			if (trigger_bit == 0) {
				continue;
			}

			trigger_flags = trigger_flags & ~trigger_bit;
			enum TriggerType trigger = (enum TriggerType) trigger_bit;

			struct sub subs[3];

			struct slice source = slice_from(profile.source);
			subs[0] = (struct sub) {
				.token = slice_from(source_token),
				.replacement = source
			};

			struct slice mod_source = source;
			bool source_path_is_terminated = source.start[source.size - 1] = '/';
			if (source_path_is_terminated) {
				// Remove the trailing '/' so that when we concatenate
				// the destination we don't get a duplicate
				mod_source.size -= 1;
			}

			size_t dest_size = mod_source.size + strlen(dest_suffix) + 1;
			char *dest = malloc(dest_size);
			char *end = mempcpy(dest, mod_source.start, mod_source.size);
			memcpy(end, dest_suffix, strlen(dest_suffix));
			subs[1] = (struct sub) {
				.token = slice_from(destination_token),
				.replacement = (struct slice) {
					.start = dest,
					.size = dest_size - 1
				}
			};

			subs[2] = (struct sub) {
				.token = slice_from(type_token),
			};
			switch (trigger) {
			case PreInstall:
				subs[2].replacement.start = "PreInstall";
				break;
			case PostInstall:
				subs[2].replacement.start = "PostInstall";
				break;
			default:
				fprintf(stderr, "Cannot process trigger type %d\n", trigger);
				exit(EXIT_FAILURE);
				break;
			}
			subs[2].replacement.size = strlen(subs[2].replacement.start);

			char *output_file_path = make_message("./50-%" PRIu64 "-%d.hook", profile.fingerprint, trigger);
			// Create the file with 0700 mode, if it doesn't already exist.
			int output_fd = open(output_file_path, O_WRONLY | O_CREAT, S_IRWXU);
			struct hook_ctx ctx = {
				.fd_in = template_fd,
				.fd_out = output_fd,
				.in_size = template_file.st_size,
				.in_consumed = 0,
				.out_written = 0
			};

			process_stream(fill, flush, &ctx, subs, sizeof subs / sizeof *subs);

			close(output_fd);
			// We are reading the template file so it must be re-set.
			lseek(template_fd, 0, SEEK_SET);
		}
	}
}

enum InstallerCode read_profiles(int fd, ssize_t size, char *buf)
{
	size_t bytes_read = 0;
	for ( ; bytes_read < size; ) {
		ssize_t new_bytes = read(fd, buf + bytes_read, size - bytes_read);
		if (new_bytes < 0) {
			break;
		}
		bytes_read += new_bytes;
	}
	if (bytes_read < size) {
		fprintf(stderr, "read_profiles: read %ld bytes, expected %ld\n", bytes_read, size);
		return PI_READ_FAIL;
	}
	buf[size] = '\0';

	return PI_OK;
}

struct profile * map_profiles(struct file_profile *file_profiles, size_t count)
{
	if (file_profiles == NULL) {
		return NULL;
	}

	struct profile *profiles = NULL;
	if ((profiles = malloc(sizeof *profiles * count)) == NULL) {
		fprintf(stderr, "Failed to allocate buffer for profiles.\n");
		exit(EXIT_FAILURE);
	}

	size_t max_source = 0;
	for (size_t i = 0; i < count; ++i) {
		size_t current_len = strlen(file_profiles[i].source);
		if (current_len > max_source) {
			max_source = current_len;
		}
	}

	sd_id128_t app_id = APP_ID;
	sd_id128_t raw_machine_id;
	if (sd_id128_get_machine_app_specific(app_id, &raw_machine_id) != 0) {
		fprintf(stderr, "Failed to generate machine ID.\n");
		return NULL;
	}
	// TODO: Can this contain an embedded NUL?
	char * machine_id = SD_ID128_TO_STRING(raw_machine_id);

	char * hash_buf = NULL;
	// `SD_ID128_STRING_MAX` already includes for NUL terminator.
	size_t buf_len = max_source + SD_ID128_STRING_MAX;
	if ((hash_buf = malloc(sizeof *hash_buf * buf_len)) == NULL) {
		fprintf(stderr, "Failed to allocate hash source buffer.\n");
		return NULL;
	}
	// The machine ID forms part of the hash for every profile fingerprint.
	memcpy(hash_buf, machine_id, SD_ID128_STRING_MAX);

	for (size_t current = 0; current < count; ++current) {
		struct file_profile current_profile = file_profiles[current];
		int src_len = strlen(current_profile.source);

		size_t hash_source_len = SD_ID128_STRING_MAX + src_len;
		for (size_t i = SD_ID128_STRING_MAX - 1, j = 0; i < hash_source_len; ++i, ++j) {
			hash_buf[i] = current_profile.source[j];
		}
		uint64_t hash = poly_hash(hash_buf);

		char *name_buf = NULL;
		// +1 need to account for the NUL terminator.
		size_t name_buf_len = sizeof *name_buf * strlen(current_profile.name) + 1;
		if ((name_buf = malloc(name_buf_len)) == NULL) {
			fprintf(stderr, "Failed to allocate profile name buffer.\n");
			exit(EXIT_FAILURE);
		}
		memcpy(name_buf, current_profile.name, name_buf_len);

		char *src_buf = NULL;
		size_t src_buf_len = sizeof *src_buf * src_len + 1;
		if ((src_buf = malloc(src_buf_len)) == NULL) {
			fprintf(stderr, "Failed to allocate profile source buffer.\n");
			exit(EXIT_FAILURE);
		}
		memcpy(src_buf, current_profile.source, src_buf_len);

		profiles[current].fingerprint = hash;
		profiles[current].name = name_buf;
		profiles[current].source = src_buf;
		profiles[current].trigger_type = current_profile.type;
	}

	free(hash_buf);

	return profiles;
}

struct slice slice_from(char *s)
{
	return (struct slice) {
		.start = s,
		.size = strlen(s)
	};
}

char * make_message(char *format, ...)
{
	int n = 0;
	size_t size = 0;
	char *p = NULL;
	va_list ap;

	/* Determine required size.  */

	va_start(ap, format);
	n = vsnprintf(p, size, format, ap);
	va_end(ap);

	if (n < 0) {
		return NULL;
	}

	size = (size_t) n + 1;      /* One extra byte for '\0' */
	p = malloc(size);
	if (p == NULL) {
		return NULL;
	}

	va_start(ap, format);
	n = vsnprintf(p, size, format, ap);
	va_end(ap);

	if (n < 0) {
		free(p);
		return NULL;
	}

	return p;
}
