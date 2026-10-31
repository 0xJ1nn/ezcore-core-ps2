CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude

SRC = src/ps2/cpu/cpu_timing.c
TEST = tests/unit/test_cpu_syscall_timing.c

.PHONY: test asan clean

test:
	@mkdir -p build
	$(CC) $(CFLAGS) $(TEST) $(SRC) -o build/test_cpu
	./build/test_cpu

asan:
	@mkdir -p build
	$(CC) -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -g -Iinclude $(TEST) $(SRC) -o build/test_cpu_asan
	./build/test_cpu_asan

clean:
	rm -rf build
