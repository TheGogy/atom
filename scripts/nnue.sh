#!/bin/bash

HEADER_FILE="src/nnue.h"

# Extract the file names from nnue.h
BIG_FILE=$(awk -F'"' '/EvalFileDefaultNameBig/ {print $2}' "$HEADER_FILE")
SMALL_FILE=$(awk -F'"' '/EvalFileDefaultNameSmall/ {print $2}' "$HEADER_FILE")

# Check if the file exists, and download if necessary
check_and_download() {
    local FILE_NAME=$1
    local URL="https://tests.stockfishchess.org/api/nn/$FILE_NAME"

    # Check if the file exists
    if [ ! -f "$FILE_NAME" ]; then
        echo "File $FILE_NAME does not exist."
        read -p "Download NNUE? [Y/n] " answer
        if [[ $answer =~ ^[Nn]$ ]]; then
            echo "Skipping download of $FILE_NAME."
            echo "Warning: this will not compile correctly."
        else
            echo "Downloading $FILE_NAME..."
            wget "$URL" -O "$FILE_NAME"
        fi
    else
        echo "Located $FILE_NAME"
    fi
}

check_and_download "$BIG_FILE"
check_and_download "$SMALL_FILE"
