#include "lib.h"
#include <stddef.h>

enum InstallerCode process_stream_sized(
		enum InstallerCode (*fill)(void *ctx, char *, size_t, size_t *),
		enum InstallerCode (*flush)(void *ctx, char *, size_t, size_t *),
		void *ctx,
		struct sub *subs,
		size_t subs_size,
		size_t in_size, size_t out_size);
