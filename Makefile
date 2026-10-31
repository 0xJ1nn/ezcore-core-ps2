# Build and test for the PS2 HLE core modules.
# Targets promised by README.md: test, asan, clean. The full gate
# (scripts/gate.sh, dist) is slice S1's deliverable and is not created here.

CC ?= cc
CFLAGS ?= -std=c11 -O2
WARN := -Wall -Wextra -Werror -pedantic
INC := -Iinclude -Itests/unit

SRC := src/ps2/cpu/cp0_exception.c src/ps2/intc/intc.c
TEST_SRC := tests/unit/test_cp0_exception.c tests/unit/test_intc.c

BUILD := build
ASAN_BUILD := build/asan
ASAN_FLAGS := -fsanitize=address,undefined -fno-sanitize-recover=all \
              -fno-omit-frame-pointer -g -O1

TEST_BINS := $(BUILD)/test_cp0_exception $(BUILD)/test_intc
ASAN_BINS := $(ASAN_BUILD)/test_cp0_exception $(ASAN_BUILD)/test_intc

.PHONY: all test asan clean

all: test

$(BUILD) $(ASAN_BUILD):
	mkdir -p $@

$(BUILD)/test_cp0_exception: tests/unit/test_cp0_exception.c tests/unit/test_harness.h $(SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) $(INC) -o $@ tests/unit/test_cp0_exception.c $(SRC)

$(BUILD)/test_intc: tests/unit/test_intc.c tests/unit/test_harness.h $(SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) $(INC) -o $@ tests/unit/test_intc.c $(SRC)

test: $(TEST_BINS)
	@set -e; for t in $(TEST_BINS); do \
	  echo "RUN $$t"; \
	  ./$$t; \
	done

$(ASAN_BUILD)/test_cp0_exception: tests/unit/test_cp0_exception.c tests/unit/test_harness.h $(SRC) | $(ASAN_BUILD)
	$(CC) $(CFLAGS) $(WARN) $(ASAN_FLAGS) $(INC) -o $@ tests/unit/test_cp0_exception.c $(SRC)

$(ASAN_BUILD)/test_intc: tests/unit/test_intc.c tests/unit/test_harness.h $(SRC) | $(ASAN_BUILD)
	$(CC) $(CFLAGS) $(WARN) $(ASAN_FLAGS) $(INC) -o $@ tests/unit/test_intc.c $(SRC)

asan: $(ASAN_BINS)
	@set -e; for t in $(ASAN_BINS); do \
	  echo "RUN (asan) $$t"; \
	  ./$$t; \
	done

clean:
	rm -rf $(BUILD)
