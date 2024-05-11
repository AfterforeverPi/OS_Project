#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage: $0 <file_path>"
    exit 1
fi

dest_path="$2"
file_path="$1"
keywords=("malicious" "virus" "dangerous" "attack")

chmod 744 "${file_path}"

for keyword in "${keywords[@]}"; do
    if grep -q "$keyword" "$file_path"; then
        chmod 000 "${file_path}"
        mv "${file_path}" "${dest_path}"
    fi
done



