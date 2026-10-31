CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2 -g -ffile-prefix-map=$(CURDIR)=.
CPPFLAGS += -Iinclude -Isrc

SRC := \
    src/ps2/spu2/spu2.c

HEADERS := $(sort $(shell find include src -type f -name '*.h'))

TEST := build/test_ps2_aud_001

.PHONY: all test asan clean

all: $(TEST)

$(TEST): tests/test_ps2_aud_001.c $(SRC) $(HEADERS)
	@mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ tests/test_ps2_aud_001.c $(SRC)

test: $(TEST)
	./$(TEST)

asan:
	$(MAKE) clean
	$(MAKE) test CFLAGS="-std=c11 -Wall -Wextra -Werror -pedantic -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer"

clean:
	rm -rf build