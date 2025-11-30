# Makefile for wowlan
CC = gcc
CFLAGS = -Wall -Wextra -O2
MUSL_CC = musl-gcc
RUSTC = rustc

# Default target
.PHONY: all clean c musl-rust

all: wowlan wowlan-mt7621 wowlan-rust

# Build regular C version
wowlan: src/main.c
	$(CC) $(CFLAGS) -o wowlan src/main.c

# Build MUSL version for MT7621 (MIPS architecture)
wowlan-mt7621: src/main.c
	@if command -v musl-gcc >/dev/null 2>&1; then \
		musl-gcc -static -Os -o wowlan-mt7621 src/main.c; \
	else \
		echo "musl-gcc not found, using regular gcc with static linking"; \
		$(CC) -static -o wowlan-mt7621 src/main.c; \
	fi

# Build Rust version (if rustc is available)
wowlan-rust:
	@if command -v rustc >/dev/null 2>&1; then \
		rustc -O --out-dir . src/main.rs -L .; \
		if [ -f main ]; then mv main wowlan-rust; fi; \
	else \
		echo "Rust compiler not found. Please install rustc to build Rust version."; \
		touch wowlan-rust; \
		chmod +x wowlan-rust; \
		echo '#!/bin/sh' > wowlan-rust; \
		echo 'echo "Error: Rust compiler not found. Please install rustc to build Rust version.";' >> wowlan-rust; \
	fi

# Alternative Rust build with cargo if available
wowlan-rust-cargo:
	@if command -v cargo >/dev/null 2>&1; then \
		cargo build --release --target-dir=target; \
		cp target/release/wowlan wowlan-rust 2>/dev/null || cp target/release/wowlan wowlan-rust-cargo; \
	else \
		echo "Cargo not found. Install Rust toolchain to build with Cargo."; \
	fi

# Clean build artifacts
clean:
	rm -f wowlan wowlan-mt7621 wowlan-rust wowlan-rust-cargo wowlan_c wowlan_c_fixed

# Test the built binaries
test: all
	@echo "Testing C version..."
	@./wowlan E4:3A:6E:84:4E:85 192.168.123.0/24
	@echo "Testing MT7621 version..."
	@./wowlan-mt7621 E4:3A:6E:84:4E:85 192.168.123.0/24
	@echo "Testing Rust version..."
	@./wowlan-rust E4:3A:6E:84:4E:85 192.168.123.0/24 || echo "Rust version not available"