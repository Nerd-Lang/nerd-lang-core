# Contributing to NERD

Thanks for your interest in contributing to NERD! This is an experimental project exploring what programming languages look like when humans aren't the primary authors.

## Project Philosophy

NERD is designed for LLMs, not humans. Before contributing, understand the core principles:

1. **Token efficiency over readability** - Every design decision optimizes for fewer tokens
2. **English words over symbols** - Words tokenize predictably; symbols fragment
3. **Humans observe, not author** - The language isn't meant to be hand-written

## Ways to Contribute

### Good First Contributions

- **Bug reports** - Found something broken? Open an issue
- **Documentation** - Clarify the spec, add examples, improve README
- **Test cases** - More NERD code samples help validate the compiler
- **Typo fixes** - Small fixes are always welcome

### Larger Contributions

- **Standard library modules** - Implement str, list, http, json modules
- **Compiler improvements** - Better error messages, optimizations
- **Human View generator** - Visualize NERD as diagrams/English
- **WASM target** - Browser support

For larger changes, **please open an issue first** to discuss the approach before investing significant time.

## Development Setup

```bash
git clone https://github.com/Nerd-Lang/nerd-lang-core.git
cd nerd-lang-core/bootstrap
make
```

Requirements:
- C compiler (gcc or clang)
- LLVM (for code generation)
- Make

## Submitting Changes

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Make your changes
4. Test your changes (`make test`)
5. Commit with a clear message
6. Open a Pull Request

### Commit Messages

Keep them clear and concise:
```
Add sqrt function to math module
Fix tokenization edge case with negative numbers
Update spec to clarify type syntax
```

## Code Style

For the bootstrap compiler (C code):
- 4-space indentation
- Descriptive function names
- Comments for non-obvious logic

For NERD code examples:
- Follow the spec exactly
- Prefer clarity in examples (they're for documentation)

## Governance

NERD is currently a BDFL (Benevolent Dictator For Life) project maintained by [Guru Sattanathan](https://www.gnanaguru.com). But we're actively looking to grow this into a community-driven effort.

## Join the Community

NERD is just getting started, and we're looking for contributors to help shape its direction.

Whether you're interested in compiler development, language design, developer tooling, or just curious about the future of AI-native programming - there's room to get involved.

**Want to become a core contributor?** Reach out! We're having conversations with people who are excited about this space and want to help build it.

- **Technical discussions** - Open a GitHub issue
- **Want to get more involved?** - Connect with [Guru](https://www.linkedin.com/in/gnanaguru/) on LinkedIn
- **Project website** - [nerd-lang.org](https://nerd-lang.org)

## License

By contributing, you agree that your contributions will be licensed under the Apache 2.0 License.
