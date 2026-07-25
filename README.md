# Salsa20 Strem Cipher

An educational C implementation of the **Salsa20** stream cipher designed to demonstrate state matrix construction, quarter-round transformations, key/nonce management, and file encryption/decryption mechanics.

> [!NOTE]
> **Educational Notice:** This repository is intended solely for studying cryptography and stream cipher mechanics. It is **not** audited or intended for production use. 

---

## Features 

- **Standard Salsa20/20 Core:** Implements the 20-round double-round loop (column and row rounds) over a 64-byte internal state array ($4 \times 4$ matrix).
- **File-Based Workflow:** Reads input plaintext from disk, auto-generates a 256-bit key and random nonces, and serializes all cryptographic material.
- **Automated Verification:** Encrypts `plain.txt` to `encoded.txt`, reconstructs state from generated key/nonce files, decrypts the ciphertext, and validates parity using `strcmp`.
- **Dynamic Block Handling:** Processes arbitrary-length text dynamically across multiple 64-byte blocks by advancing the counter word.

---

## Internal Cipher Mechanics

1. **State Matrix:** Constructs a 16-word (`uint32_t`) array containing:
   - 4 Constants ("expand 32-byte k")
   - 8 Key words (256 bits)
   - 2 Nonce words (64 bits)
   - 2 Counter words (64 bits)
2. **Quarter Round:** Applies addition (`+`), bitwise XOR (`^`), and left rotation (`ROTATE32`) across state words.
3. **Keystream Generation:** Adds the original state matrix back to the transformed state matrix modulo $2^{32}$.
4. **Encryption/Decryption:** Performs bitwise XOR between keystream blocks and target text.

---

## Build & Run

### Build

Compilation can be performed using the included `Makefile`:

```bash
make
```

### Run

You can execute the compiled `salsa` binary directly or use the `make run` routine: 

```bash
# Run directly
./salsa

# Or via the Makefile routine
make run
```