#!/bin/bash
# Script to build MUSL version for MT7621 (MIPS architecture)

echo "Building MUSL version for MT7621..."

# Check if musl-gcc is available
if command -v musl-gcc &> /dev/null; then
    echo "Using musl-gcc to build static binary..."
    musl-gcc -static -Os -o wowlan-mt7621 src/main.c
    echo "Built wowlan-mt7621 with musl-gcc"
else
    echo "musl-gcc not found. Installing musl-dev and building..."
    
    # Try to install musl tools if on a system that supports it
    if command -v apt-get &> /dev/null; then
        apt-get update && apt-get install -y musl-tools
        if command -v musl-gcc &> /dev/null; then
            musl-gcc -static -Os -o wowlan-mt7621 src/main.c
            echo "Built wowlan-mt7621 with musl-gcc"
        else
            echo "musl-gcc still not available, building with gcc static"
            gcc -static -o wowlan-mt7621 src/main.c
        fi
    elif command -v apk &> /dev/null; then
        apk add --no-cache musl-dev musl-tools
        if command -v musl-gcc &> /dev/null; then
            musl-gcc -static -Os -o wowlan-mt7621 src/main.c
            echo "Built wowlan-mt7621 with musl-gcc"
        else
            echo "musl-gcc still not available, building with gcc static"
            gcc -static -o wowlan-mt7621 src/main.c
        fi
    else
        echo "Cannot install musl-gcc. Building with gcc static instead..."
        gcc -static -o wowlan-mt7621 src/main.c
    fi
fi

echo "wowlan-mt7621 build complete!"