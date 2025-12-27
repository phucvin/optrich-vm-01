#!/bin/bash

# List of test binaries
TESTS=("test_lexer" "test_parser" "test_store" "test_integration" "test_array" "test_multi_module" "test_span")
FAILED=0

echo "Running tests..."

for t in "${TESTS[@]}"; do
    if [ ! -f "./$t" ]; then
        echo "Error: Binary ./$t not found. Did you run 'make'?"
        FAILED=1
        continue
    fi

    EXPECTED="tests/$t.expected_stdout"
    if [ ! -f "$EXPECTED" ]; then
        echo "Error: Expected output file $EXPECTED not found."
        FAILED=1
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
