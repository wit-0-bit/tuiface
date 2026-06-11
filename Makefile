# Top-level developer entry points. The Pebble SDK build itself is `pebble
# build` (see README); this Makefile wraps formatting and the host test suite.

FORMAT_SRCS = $(wildcard src/c/*.c src/c/*.h) \
              src/pkjs/index.js \
              test/test_watchface.c test/pebble_mock.c test/pebble.h test/test_pkjs.js

.PHONY: format format-check test build

format:
	clang-format -i $(FORMAT_SRCS)

format-check:
	clang-format --dry-run -Werror $(FORMAT_SRCS)

test: format-check
	$(MAKE) -C test test
	node test/test_pkjs.js

# Requires the pebble-env virtualenv to be active.
build: format-check
	pebble build
