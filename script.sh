#!/bin/bash


# This script takes as command line argument a file an checks if the number words, lines and chars is ok
# or if there are non-ascii characters inside the file or if the file contains one of the words in the
# array of words.If one of those situatuins is encounterd it exits with a specific code.


if [ $# -ne 1 ]; then
    echo "Usage: $0 <file>"
    exit 1
fi

file="$1"
file_name=$(basename "$file")
if [ ! -f "$file" ]; then
    echo "Error: $file is not a regular file."
    exit 1
fi

words_array=("corrupted" "dangerous" "risk" "attack" "malware" "malicious")

chmod +r "$file"

char_count=$(wc -m < "$file")
word_count=$(wc -w < "$file")
line_count=$(wc -l < "$file")

if [ "$line_count" -lt 3 ] || [ "$word_count" -gt 100000 ] || [ "$char_count" -gt 20000 ]; then
    echo "$file_name"
    chmod -r "$file"
    exit 0
fi

if grep -q -P '[^\x00-\x7F]' "$file"; then
    echo "$file_name"
    chmod -r "$file"
    exit 0
fi

for word in "${words_array[@]}"; do
    if grep -q -e "$word" "$file"; then
        echo "$file_name"
        chmod -r "$file"
        exit 0
    fi
done

echo "SAFE"
chmod -r "$file"
exit 2
