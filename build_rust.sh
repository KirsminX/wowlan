#!/bin/bash
# Script to build Rust version of wowlan

echo "Building Rust version..."

# Check if rustc is available
if command -v rustc &> /dev/null; then
    echo "Using rustc to build..."
    rustc -O --out-dir . src/main.rs
    if [ -f main ]; then
        mv main wowlan-rust
    fi
    echo "Built wowlan-rust with rustc"
elif command -v cargo &> /dev/null; then
    echo "Using cargo to build..."
    cargo build --release
    if [ -f target/release/wowlan ]; then
        cp target/release/wowlan wowlan-rust
        echo "Built wowlan-rust with cargo"
    else
        echo "Cargo build failed"
        exit 1
    fi
else
    echo "Rust toolchain not found. Installing Rust..."
    
    # Install Rust if not present
    if command -v curl &> /dev/null; then
        curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
        source ~/.cargo/env
        if command -v rustc &> /dev/null; then
            rustc -O --out-dir . src/main.rs
            if [ -f main ]; then
                mv main wowlan-rust
            fi
            echo "Built wowlan-rust with rustc"
        elif command -v cargo &> /dev/null; then
            cargo build --release
            if [ -f target/release/wowlan ]; then
                cp target/release/wowlan wowlan-rust
                echo "Built wowlan-rust with cargo"
            fi
        else
            echo "Failed to build Rust version"
            exit 1
        fi
    else
        echo "Cannot install Rust without curl. Please install Rust manually."
        exit 1
    fi
fi

echo "wowlan-rust build complete!"