#!/bin/bash
set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NOCOLOR='\033[0m'

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CMD="$DIR/../main -t"

function header {
    echo -e "${BLUE}========== $1 ==========${NOCOLOR}"
}

function check {
    local label="$1"
    local expected_output="$2"
    local output="$3"
    if [[ "$output" == "$expected_output" ]]; then
        echo -en "${GREEN}PASS${NOCOLOR} "
        echo "$label"
        return 0
    else
        echo -en "${RED}FAIL${NOCOLOR} "
        echo "$label"
        echo "== EXPECTED =="
        echo "$expected_output"
        echo "=== OUTPUT ==="
        echo "$output"
        return 1
    fi
}

function oneline_suite {
    local testcases_path="$DIR/$1"
    local name="$(basename "$testcases_path" ".test")"
    local failures=0
    local prelude=""
    if [[ "$#" -eq 2 ]]; then
        prelude="$(cat "$DIR/$2")\n"
    fi
    header "$name"
    while read -r line; do
        read expected_output
        local input="$prelude$line"
        local output=$(echo -e "$input" | $CMD 2>&1)
        check "$line" "$expected_output" "$output" || ((failures++)) || true
    done < <(grep -v "====" "$testcases_path")
    if [[ $failures -eq 0 ]]; then return 0; else return 1; fi
}

function summarize {
    local failures="$1"
    header "summary"
    if [[ "$failures" -eq 0 ]]; then
        echo -en "${GREEN}PASS${NOCOLOR} "
        echo "all tests passed"
        exit 0
    else
        echo -en "${RED}FAIL${NOCOLOR} "
        echo "$failures test suite(s) failed"
        exit 1
    fi
}

function run {
    local failures=0
    declare -a suites=(
        "lambda.test"
        "arithmetic.test"
        "definition.test"
        "prelude.test prelude"
    )
    for suite in "${suites[@]}"; do
        # the next line allows us to continue running tests after a
        # suite fails with set -e mode enabled
        oneline_suite $suite || ((failures++)) || true
    done
    summarize "$failures"
}

run
