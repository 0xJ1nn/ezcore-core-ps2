#!/bin/sh
set -eu

echo "gate: HEAD=$(git rev-parse HEAD)"

make clean
make test
make asan

# no-firmware check: no non-text artifacts under src/include/tests
if find src include tests -type f 2>/dev/null | grep -Ei '\.(bin|rom|iso|elf|irx|dump|key)$' ; then
    echo "gate: firmware-like artifact found" >&2
    exit 1
fi

echo "gate: PASS"
exit 0
