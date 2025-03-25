#!/usr/bin/env sh

# This script is used to convert all the data from a given benchmark file and output it as an csv file
# This script expects to have the file path as its first argument
root="$(cd "$(dirname "$0")" && cd .. && pwd)"
file="$(realpath "$1")"

if [ "$file" = "" ]; then
    echo "No file path provided!"
    exit 1
fi

if ! [ -e "$file" ] && ! [ -r "$1" ]; then
    echo "The given file '$file' doesnt exist or is not readable!"
    exit 1
fi
filename="$(basename "$file" | sed -e "s/\..*//g")"

# And now its pretty easy. Just search for all lines that contain a | symbol, then remove the first and last occurence of said symbol in this line, and then swap all | symbols for commas.

# Make sure the directory exists
mkdir -p "$root/test/results/test_data"
mkdir -p "$root/test/results/test_data/csv"

# 1. Load the file
# 2. Search for the | symbol
# 3. Search for all | symbols and replace them with , plus remove all spaces
# 4. Remove the last | and spaces
# 5. Remove the first | and space
# 6. Remove all ' MB' and ' ms' occurences
# 7. Save the csv file
cat "$file" \
    | grep "|" \
    | sed -E "s/[ ]+\|[ ]/,/g" \
    | sed -E "s/[ ]+\|//g" \
    | sed -E "s/\|[ ]//g" \
    | sed -E "s/[ ]MB|[ ]ms//g" \
    > "$root/test/results/test_data/csv/$filename.csv"
