CC ?= cc
CFLAGS ?= -Wall -Wextra -Werror -std=c11 -Iinclude

TEST_SRCS = tests/unit/test_ps2_aud_002_output_determinism.c
SRC = src/ps2/audio/output_determinism.c

.PHONY: test asan clean
test:
	$(CC) $(CFLAGS) $(TEST_SRCS) $(SRC) -o /tmp/ps2_aud_002_test && /tmp/ps2_aud_002_test

asan:
	$(CC) $(CFLAGS) -fsanitize=address,undefined -g $(TEST_SRCS) $(SRC) -o /tmp/ps2_aud_002_asan && /tmp/ps2_aud_002_asan

clean:
	rm -f /tmp/ps2_aud_002_test /tmp/ps2_aud_002_asan
