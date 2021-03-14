#!/bin/sh

line_number=0
open_line=0

while read -r line; do
    line_number="$((line_number + 1))"
    if [ "$open_line" -eq 0 ] && echo "$line" | grep -q '{$'; then
        open_line="$line_number"
    elif [ "$open_line" -ne 0 ] && [ "$line" = '}' ]; then
        line_count="$((line_number - open_line - 1))"
        open_line="0"
        echo "$line_count"
    fi
done
