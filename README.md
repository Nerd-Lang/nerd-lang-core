<p align="center">
  <img src="https://raw.githubusercontent.com/Nerd-Lang/nerd-lang-core/main/docs/site/images/nerd-dark.png" alt="NERD" width="400">
</p>

<h3 align="center">No Effort Required, Done</h3>

<p align="center">
  An intermediate language for machines, not humans.
</p>

---

## The Paradigm Shift

**40% of code is now written by LLMs.** That number is growing.

So why are we still optimizing languages for human authors?

Every `public static void main`, every descriptive variable name, every carefully formatted brace — all designed for humans to read and write. But humans aren't writing anymore. And increasingly, they're not reading line-by-line either.

**NERD is not built for humans. It's built for LLMs.**

- **Machines write it** — humans never touch the source
- **Machines read it** — compiles to native code via LLVM
- **Humans observe it** — auditable, transparent, but not editable
- **50-70% fewer tokens** — same logic, fraction of the cost

## Why "NERD"?

**N**o **E**ffort **R**equired, **D**one.

An intermediate language. Dense. Non-human. Observable but not writable by humans. Like assembly, but for the LLM era.

## What Makes NERD Different

Traditional languages use symbols that fragment into many LLM tokens:
```javascript
function add(a, b) { return a + b; }
```

NERD uses English words - each word = 1 token:
```
fn add a b
ret a plus b
```

No symbols. No braces. No semicolons. Just words.

## Examples

**Math operations:**
```
fn add a b
ret a plus b

fn sub a b
ret a minus b

fn mul a b
ret a times b

fn div a b
ret a over b
```

**Calculator with types:**
```
type result ok num or err str

fn calc a b op
if op eq zero ret ok a plus b
if op eq one ret ok a minus b
if op eq two ret ok a times b
if op eq three ret ok a over b
ret err "unknown operation"
```

## Token Efficiency

| Language | Math (4 fn) | Calculator | Savings |
|----------|-------------|------------|---------|
| **NERD** | **32** | **55** | - |
| JavaScript | 70 | 107 | 48-54% |
| TypeScript | 96 | 135 | 59-67% |
| Java | 77 | 273 | 58-80% |

## Quick Start (macOS Apple Silicon)

```bash
# Install
curl -L https://github.com/Nerd-Lang/nerd-lang-core/releases/latest/download/nerd-darwin-arm64 -o nerd
chmod +x nerd

# Write a program
echo 'fn add a b
ret a plus b

fn mul a b
ret a times b' > math.nerd

# Run it
./nerd run math.nerd
# Output:
# add = 8
# mul = 15
```

## Build from Source

```bash
git clone https://github.com/Nerd-Lang/nerd-lang-core.git
cd nerd-lang-core/bootstrap
make
./nerd --version
```

Requires: C compiler and clang (`xcode-select --install` on macOS)

### Run the Examples

```bash
cd bootstrap

# Compile and run math example
./nerd compile ../examples/math.nerd -o math.ll
cat math.ll test_math.ll > combined.ll
clang -O2 combined.ll -o math
./math
# Output:
# add(5, 3) = 8
# sub(10, 4) = 6
# mul(6, 7) = 42
# div(20, 4) = 5
```

## Standard Library

| Module | Purpose | Status |
|--------|---------|--------|
| `core` | Built-in operators (plus, minus, eq, etc.) | Done |
| `math` | Numeric operations (abs, sqrt, pow, etc.) | Done |
| `str` | String operations (len, concat, split, etc.) | Planned |
| `list` | Collections (map, filter, reduce, etc.) | Planned |
| `time` | Date/time handling | Planned |
| `http` | HTTP client | Planned |
| `json` | JSON encoding/decoding | Planned |

## How It Works

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  NERD Code  │────>│   C Lexer   │────>│  C Parser   │────>│  LLVM IR    │
│  (.nerd)    │     │             │     │   (AST)     │     │  (.ll)      │
└─────────────┘     └─────────────┘     └─────────────┘     └─────────────┘
                                                                   │
                                                                   v
                                                            ┌─────────────┐
                                                            │   clang     │
                                                            │  (native)   │
                                                            └─────────────┘
```

1. **Lexer** - Tokenizes English words into token stream
2. **Parser** - Builds Abstract Syntax Tree from tokens
3. **Codegen** - Generates LLVM IR from AST
4. **LLVM/Clang** - Compiles IR to native binary

## Documentation

- [Specification](spec/NERD-SPEC.md) - Full language spec with standard library
- [Examples](examples/) - Sample NERD programs
- [Bootstrap Compiler](bootstrap/) - Native C compiler source

## The Philosophy

> "When machines write the code, why optimize for humans?"

Traditional languages exist because humans needed to read and write them. Verbose syntax, descriptive names, careful formatting — all for human comprehension.

But if 40% of code is machine-written today, and that's growing — the verbosity is pure waste. Every token costs money. Every character takes time.

NERD flips the model:
- **Not human-friendly** — dense, terse, machine-optimized
- **Human-observable** — you can audit it, understand it, verify it
- **Not human-editable** — you describe changes in natural language, machines update the NERD

The result? Same logic. 50-70% fewer tokens. Faster generation. Lower cost.

Built like Rust was built — from scratch. Pure native compilation.

**No Effort Required, Done.**

## Community

NERD was founded by [Guru Sattanathan](https://www.gnanaguru.com) and is actively looking for community contributions.

This is an early-stage project exploring what programming languages look like when AI writes most of the code. Whether you're into compiler development, language design, or just curious about the future of coding — we'd love to have you involved.

**Get Involved:**

- ⭐ Star this repo to follow along
- 🐛 Open an issue if you find bugs or have ideas
- 🔧 Check out [CONTRIBUTING.md](CONTRIBUTING.md) to start contributing
- 💬 Connect with [Guru on LinkedIn](https://www.linkedin.com/in/gnanaguru/) to join the conversation

## License

Apache 2.0 — See [LICENSE](LICENSE) for details.
