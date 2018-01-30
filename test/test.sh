#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NOCOLOR='\033[0m'

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CMD="$DIR/../main -t"

function header {
    echo -e "${BLUE}==== $1 ====${NOCOLOR}"
}

function check {
    local label="$1"
    local expected_output="$2"
    local output="$3"
    if [[ "$output" == "$expected_output" ]]; then
        echo -en "${GREEN}PASS${NOCOLOR} "
        echo "$label"
    else
        echo -en "${RED}FAIL${NOCOLOR} "
        echo "$label"
        echo "== EXPECTED =="
        echo "$expected_output"
        echo "=== OUTPUT ==="
        echo "$output"
    fi
}

function oneline_suite {
    local name="$1"
    local testcases_path="$DIR/$2"
    [ ! -z "$3" ] && local prelude="$(cat "$DIR/$3")\n"
    header "$name"
    grep -v "====" "$testcases_path" | while read -r line; do
        read expected_output
        local input="$prelude$line"
        local output=$(echo -e "$input" | $CMD 2>&1)
        check "$line" "$expected_output" "$output"
    done
}

function suite {
    local name="$1"
    local fixtures="$2"
    local regexp="${3:-.*}"

    header "$name"
    for filepath in $DIR/$fixtures/*.in; do
        local label=$(basename $filepath .in)
        if [[ $label =~ $regexp ]]; then
            local expected_output=$(cat $DIR/$fixtures/$label.out)
            local output=$(cd "$DIR/.." && cat $filepath | $CMD 2>&1)
            check "$label" "$expected_output" "$output"
        fi
    done
}

oneline_suite "lambda" "lambda.test" ""
oneline_suite "arithmetic" "arithmetic.test" ""
oneline_suite "definition" "definition.test" ""
oneline_suite "prelude" "prelude.test" "prelude"
