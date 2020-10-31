
AUTOMAKE_OPTIONS = subdir-objects


AM_CFLAGS = -std=gnu99 -Wall -Werror=implicit-function-declaration
#   -fdiagnostics-color=always # -Wformat-overflow=0
# Need to disable color for Eclipse


bin_PROGRAMS = bin/stash

bin_stash_SOURCES =      \
	src/main.c       \
	src/stash.c      \
	src/stash_log.c  \
	src/stash_file.c \
	src/buffer.c     \
	src/list.c       \
	src/util.c
