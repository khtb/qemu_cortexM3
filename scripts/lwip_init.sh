#!/usr/bin/env bash
# lwip_init.sh: Adds lwIP as a Git submodule

LWIP_URL="https://github.com/lwip-tcpip/lwip.git"
DEST_DIR="lib/lwip"

echo "Initializing lwIP submodule..."

if [ ! -d ".git" ]; then
    echo "Error: Not a git repository. Please initialize git first."
    exit 1
fi

git submodule add --depth 1 "$LWIP_URL" "$DEST_DIR"

if [ $? -eq 0 ]; then
    echo "lwIP submodule added successfully in $DEST_DIR."
else
    echo "Failed to add lwIP submodule."
    exit 1
fi
