#!/bin/bash

# 1. Run Make to build everything
echo "Building tests..."
if ! make > /dev/null; then
    echo "Build failed."
    exit 1
fi

# 2. Discover test binaries
# Find files in current directory that start with 'test_', are executable, and are not directories/source files.
# We can just look for the TARGETS defined in Makefile, but to be dynamic as requested:
# We look for files matching ./test_* that are executable.
FAILED=0
echo "Running tests..."

# Using find to get executable files starting with test_ in the current dir
# -maxdepth 1: current dir only
# -type f: files
# -executable: executable permission
# -name "test_*": starts with test_
TESTS=$(find . -maxdepth 1 -type f -executable -name "test_*" | sort)

if [ -z "$TESTS" ]; then
    echo "No tests found."
    exit 1
fi

for t_path in $TESTS; do
    # Strip ./ prefix
    t=$(basename "$t_path")

    # Check if it's likely a source file or object file (paranoid check, though -executable usually filters .cpp/.o unless chmod +x)
    if [[ "$t" == *.cpp ]] || [[ "$t" == *.o ]] || [[ "$t" == *.h ]]; then
        continue
    fi

    EXPECTED="tests/$t.expected_stdout"
    if [ ! -f "$EXPECTED" ]; then
        echo "Warning: Expected output file $EXPECTED not found for binary $t. Skipping."
        continue
    fi

    # Run the test
    ./$t > "$t.actual" 2>&1

    # Diff
    if diff -q "$t.actual" "$EXPECTED" > /dev/null; then
        echo "[PASS] $t"
        rm "$t.actual"
    else
        echo "[FAIL] $t"
        echo "Diff:"
        diff "$t.actual" "$EXPECTED"
        rm "$t.actual"
        FAILED=1
    fi
done

if [ $FAILED -eq 0 ]; then
    echo "All tests passed."
    exit 0
else
    echo "Some tests failed."
    exit 1
fi
