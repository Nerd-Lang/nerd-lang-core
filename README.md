> 🚧 **Early days.** This is a first step toward LLM-native programming - a language machines write, humans audit. Lots of unknowns ahead: the implementation might change completely, and the experiment itself might not work out. Not ready for real use yet. Ideas and contributions very welcome.
>
> If you're into transformers and token optimization, or you miss the days of writing C and assembly - this might be a fun playground.

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

**FizzBuzz** — the classic test, dense but readable:
```
fn fizzbuzz n
repeat n times as i
  if i mod 15 eq zero out "FizzBuzz" else if i mod three eq zero out "Fizz" else if i mod five eq zero out "Buzz" else out i
done

fn main
call fizzbuzz 15
```

**Factorial** — loops and counters:
```
fn factorial n
let result one
let x n
while x gt one
  let result result times x
  dec x
done
ret result
```

**Math operations:**
```
fn add a b
ret a plus b

fn abs x
if x lt zero ret neg x else ret x
```

## Token Efficiency

| Language | FizzBuzz | Math (4 fn) | Savings |
|----------|----------|-------------|---------|
| **NERD** | **49** | **32** | - |
| JavaScript | 99 | 70 | 50-54% |
| Python | 73 | 47 | 32-33% |
| TypeScript | 126 | 96 | 61-67% |

## Quick Start (macOS Apple Silicon)

```bash
# Install
curl -L https://github.com/Nerd-Lang/nerd-lang-core/releases/latest/download/nerd-darwin-arm64 -o nerd
chmod +x nerd

# Write a program
echo 'fn main
out "Hello from NERD"
out five plus three' > hello.nerd

# Compile and run
./nerd compile hello.nerd -o hello.ll
clang -O2 hello.ll -o hello
./hello
# Output:
# Hello from NERD
# 8
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

# FizzBuzz
./nerd compile ../examples/fizzbuzz.nerd -o fizzbuzz.ll
clang -O2 fizzbuzz.ll -o fizzbuzz
./fizzbuzz

# Factorial with loops
./nerd compile ../examples/loops.nerd -o loops.ll
clang -O2 loops.ll -o loops
./loops
```

## Language Features

| Feature | Syntax | Status |
|---------|--------|--------|
| Functions | `fn name args... ret value` | Done |
| Variables | `let x value` | Done |
| Math | `plus minus times over mod` | Done |
| Comparison | `eq ne gt lt ge le` | Done |
| Output | `out value` | Done |
| Conditionals | `if cond stmt else stmt` | Done |
| Loops | `repeat n times as i ... done` | Done |
| While | `while cond ... done` | Done |
| Negation | `neg x` | Done |
| Counters | `inc x` / `dec x` | Done |
| Stdlib | `math sqrt/pow/sin/cos/...` | Done |

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

- [Specification](https://www.nerd-lang.org/docs/spec) - Language spec and syntax reference
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
