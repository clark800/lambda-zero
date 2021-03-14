#!/bin/sh

pad="${1:-3}"
bucket=''

while read -r line; do
    if [ "$line" = "$bucket" ]; then
        printf "="
    else
        if [ "$bucket" != '' ]; then echo; fi
        bucket="$line"
        printf "%-${pad}s" "$bucket"
        printf "|="
    fi
done
printf "\n"
