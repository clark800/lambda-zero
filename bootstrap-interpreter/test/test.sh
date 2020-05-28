#!/bin/sh

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NOCOLOR='\033[0m'

DIR=$(dirname "$0")
LIB="$DIR/../../libraries"
CMD="$DIR/../main -t"

if test "$#" -gt 0 && test "$1" = "meta"; then
    META=1
    "$DIR/../../self-interpreter/make"
    CMD="$DIR/../../self-interpreter/main"
else
    META=0
fi

header() {
    printf "%b\n" "${BLUE}========== $1 ==========${NOCOLOR}"
}

check() {
    label="$1"
    expected_output="$2"
    output="$3"
    if test "$output" = "$expected_output"; then
        printf "%b %s\n" "${GREEN}PASS${NOCOLOR}" "$label"
        return 0
    else
        printf "%b %s\n" "${RED}FAIL${NOCOLOR}" "$label"
        echo "== EXPECTED =="
        echo "$expected_output"
        echo "=== OUTPUT ==="
        echo "$output"
        return 1
    fi
}

insert_newlines() {
  sed 's/\\n/\
/g'
}

oneline_suite() {
    testcases_path="$DIR/$1"
    name="$(basename "$testcases_path" ".test")"
    failures=0
    prelude=""
    shift
    for filename in "$@"; do
        prelude=$(printf "%s\n%s" "$prelude" "$(cat "$filename")")
    done
    header "$name"
    while IFS='' read -r line; do
        read -r output_line
        expected_output=$(printf "%s" "$output_line" | insert_newlines)
        sedline=$(printf "%s" "$line" | insert_newlines)
        input=$(printf "%s\n%s" "$prelude" "$sedline")
        # sed command removes any leading newlines (when prelude is empty)
        output=$(printf "%s" "$input" | sed '/./,$!d' | $CMD 2>&1)
        if ! check "$line" "$expected_output" "$output"; then
            failures=$((failures+1))
        fi
    done << EOF
$(grep -v "====" "$testcases_path")
EOF
    test "$failures" -eq 0
}

summarize() {
    failures="$1"
    header "summary"
    if test "$failures" -eq 0; then
        printf "%b" "${GREEN}PASS${NOCOLOR} "
        echo "all tests passed"
        exit 0
    else
        printf "%b" "${RED}FAIL${NOCOLOR} "
        echo "$failures test suite(s) failed"
        exit 1
    fi
}

SUITES="tokens.test quote.test brackets.test lambda.test syntax.test adt.test"
PRELUDE_SUITES="arithmetic.test definition.test sections.test tuples.test math.test prelude.test show.test infinite.test"
META_PRELUDE_SUITES="arithmetic.test definition.test sections.test tuples.test math.test prelude.test"

run() {
    suite_failures=0
    if [ "$META" -eq 1 ]; then
        # prevent segfaults due to high recursion depth
        ulimit -s unlimited || true
        PRELUDE_SUITES=$META_PRELUDE_SUITES
    fi

    for suite in $SUITES; do
        if ! oneline_suite "$suite"; then
            suite_failures=$((suite_failures+1))
        fi
    done
    for suite in $PRELUDE_SUITES; do
        if ! oneline_suite "$suite" "$LIB/operators.zero" "$LIB/prelude.zero"
        then
            suite_failures=$((suite_failures+1))
        fi
    done
    summarize "$suite_failures"
}

run
