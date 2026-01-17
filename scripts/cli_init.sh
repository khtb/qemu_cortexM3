#!/usr/bin/env bash
# cli_init.sh: Fetches only the necessary FreeRTOS-Plus-CLI files

PROJECT_ROOT=$(pwd)
DEST_DIR="$PROJECT_ROOT/lib/FreeRTOS-Plus-CLI"
RAW_URL="https://raw.githubusercontent.com/FreeRTOS/FreeRTOS/main/FreeRTOS-Plus/Source/FreeRTOS-Plus-CLI"

echo "Fetching necessary FreeRTOS-Plus-CLI files..."

mkdir -p "$DEST_DIR"

files=("FreeRTOS_CLI.c" "FreeRTOS_CLI.h")

for file in "${files[@]}"; do
    echo "Downloading $file..."
    curl -s -o "$DEST_DIR/$file" "$RAW_URL/$file"
    if [ $? -eq 0 ]; then
        echo "$file downloaded successfully."
    else
        echo "Failed to download $file."
        exit 1
    fi
done

echo "Done."
