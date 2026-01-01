# Introducing NERD: The World's First Programming Language Not Built for Humans

*50-70% fewer tokens. Native compilation. Humans observe, not edit.*

---

## What a Year

2025 was the year LLMs became serious software engineers. Claude got good. Opus 4.5 is phenomenal. GPT-5 shipped. Cursor, Windsurf, Claude Code — suddenly AI wasn't just autocompleting brackets, it was writing entire features.

I spent the year shipping code I didn't write. Reviewing PRs authored by machines. Debugging logic that Claude generated faster than I could type the requirements.

And somewhere in there, a question started nagging at me.

---

## The Question

**40% of code is now written by LLMs.** Google's reporting double-digit percentages. Organizations across industries are confirming it. Cursor's stats prove it daily. Many teams already have 40% as their target. That number is growing.

I was using Claude Code the other day, watching it generate TypeScript for a microservice. And a thought hit me:

**Why is Claude writing code that I'm supposed to read?**

I wasn't going to read it. Not really. I'd skim it, maybe check if the function names made sense, run the tests. But line-by-line code review? That's increasingly rare when AI writes the first draft.

So why are we making AI write in a format optimized for human readers who aren't really reading?

---

## The Historical Stack

Let's trace how we got here:

```
1950s: Machine code (humans write binary)
1960s: Assembly (humans write mnemonics)
1970s: C (humans write structured text)
1980s: C++, OOP (humans write objects)
1990s: Java, Python (humans write even higher abstractions)
2000s: Frameworks (humans write less, frameworks do more)
2020s: AI assistants (AI writes, humans review)
```

Each step added abstraction to make it easier for HUMANS to express intent. The machine never needed `public static void main`. That exists because humans needed to read and write it.

But now AI is the primary author. And AI doesn't need human-friendly syntax.

---

## Others Have Noticed: TOON

I'm not the first to see this.

[TOON](https://github.com/toon-format/toon) (Token-Oriented Object Notation) is a data format designed for LLM efficiency. Same insight: JSON is verbose, tokens cost money, so compress the data you send to models. TOON achieves ~40% fewer tokens than JSON while improving accuracy.

Smart. But TOON optimizes *data*. What about the *code*?

If we're compressing JSON to save tokens, why are we still generating verbose TypeScript? Why `function calculateUserAuthenticationResult(userData: UserData): AuthResult` when the LLM doesn't need any of that verbosity?

TOON was a great initiator. It proved the thesis: token efficiency matters. But it's still playing the middle ground — optimizing data humans might still read, while the code remains bloated.

What if we went further? What if we stopped optimizing code for humans entirely?

---

## What If We Removed the Human From Authorship?

Not from oversight. Not from decision-making. Just from the mechanical act of writing and editing code.

The model would be:

```
Human: "Build me a login API with rate limiting"
   ↓
AI writes something (not human-readable code)
   ↓
That something compiles and runs
   ↓
Human sees a translated view (read-only, for understanding)
   ↓
Human: "Add email verification"
   ↓
AI modifies the something
   ↓
Repeat
```

The human is a **stakeholder**, not an **author**.

---

## The First Objection: "But I Need to Debug!"

Do you though?

When your Java code runs, do you debug the JVM bytecode? When your JavaScript runs, do you debug V8's internal representation?

No. You debug at the abstraction layer you authored. The layers below are hidden.

What if the "authored layer" was natural language, and everything below was AI-generated?

Debugging becomes: "Hey Claude, the login is failing for users with + in their email. What's happening?"

And Claude explains, because Claude wrote it.

---

## The Second Objection: "But Compliance Requires Readable Code!"

This is real. Banks, healthcare, government — they often require human-auditable source code.

But "auditable" doesn't mean "authored by humans." It means "humans can verify what it does."

A translated view could be MORE auditable than spaghetti code a junior dev wrote at 2am. It could show:
- What data flows where
- What constraints are enforced
- What external systems are called
- What security measures are in place

In plain English. With diagrams.

---

## The Token Problem

Here's where it gets practical. And expensive.

When Claude writes TypeScript, it burns tokens on:
- `public static void`
- `import { something } from 'somewhere'`
- `try { } catch (e) { }`
- Curly braces, semicolons, indentation
- Descriptive variable names like `userAuthenticationResponse`

All of that is for HUMAN readers. Claude doesn't need it. But you're paying for every single token.

Let's do the math:

| What You're Paying For | Who Actually Needs It |
|------------------------|----------------------|
| `const userAuthenticationResult =` | Humans |
| `// Check if user exists in database` | Humans |
| `if (user === null) { return { error: "..." } }` | Humans |
| Indentation, formatting, whitespace | Humans |
| Import statements | Build tools |

**You're burning 70%+ of your tokens on human-readability that no human will read.**

The insight: **machine code is smaller than source code**. The verbosity is for humans.

---

## The LLM Tokenization Insight

Here's what changed our approach: **LLMs tokenize English words efficiently**.

Symbols like `{`, `}`, `=>`, `===` often fragment into multiple tokens. But common English words like "plus", "minus", "if", "return" are single tokens.

So instead of making code MORE cryptic (like traditional compression), we should make it MORE English — just dense English.

---

## Designing NERD

**N**o **E**ffort **R**equired, **D**one.

This is the world's first programming language **not optimized for humans**.

Not "easier for humans" or "AI-friendly" or "human-readable with AI features" — none of that middle ground. NERD is an intermediate language. Dense. Terse. Machine-optimized. Like assembly was for processors, but for LLMs.

The foundational philosophy:

**Humans cannot edit NERD code.**

Not "shouldn't" — *cannot*. It's not designed for human authorship. You don't open a NERD file and fix a bug. You tell the AI "the login is broken for users with spaces in their email" and the AI fixes the NERD.

You observe. You direct. You verify. But you don't edit.

This isn't a limitation — it's the point. If 40% of code is machine-written today, and tomorrow it's 90%, why maintain the pretense that humans are authors? They're stakeholders. Reviewers. Product owners. But the mechanical act of writing code? That's the machine's job now.

The key insight: **humans should observe, not author.**

1. **Machines write it** — humans never touch the source
2. **Machines execute it** — compiles to native code via LLVM
3. **Humans observe it** — auditable, transparent, but read-only
4. **Every word = 1 token** — common English words, no symbol fragmentation

Design goals:
1. **Not human-friendly** — dense, terse, machine-optimized
2. **Human-observable** — auditable, verifiable, transparent
3. **Dramatically smaller** — 50-70% fewer tokens than traditional code
4. **LLM-native** — words that tokenize efficiently, no symbols

Here's what we came up with:

**TypeScript:**
```typescript
function add(a, b) { return a + b; }
function sub(a, b) { return a - b; }
function mul(a, b) { return a * b; }
function div(a, b) { return a / b; }
```

**NERD:**
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

**67% fewer tokens. Same functionality.**

---

## A More Complex Example

**TypeScript Calculator:**
```typescript
type Input = { a: number; b: number; op: number };
type Result = { ok: number } | { err: string };

function calc(input: Input): Result {
  if (input.op === 0) return { ok: input.a + input.b };
  if (input.op === 1) return { ok: input.a - input.b };
  if (input.op === 2) return { ok: input.a * input.b };
  if (input.op === 3) return { ok: input.a / input.b };
  return { err: "unknown" };
}
```

**NERD:**
```
type input num num int
type result ok num or err str

fn calc input result
if third eq zero ret ok first plus second
if third eq one ret ok first minus second
if third eq two ret ok first times second
if third eq three ret ok first over second
ret err unknown
```

**59% fewer tokens. Same logic.**

---

## The Economics of NERD

Let's talk money.

| Metric | TypeScript | NERD | Savings |
|--------|------------|------|---------|
| Math (4 functions) | 96 tokens | 32 tokens | **67%** |
| Calculator | 135 tokens | 55 tokens | **59%** |

If you're building with AI, every request costs tokens. Every iteration costs tokens. Every "make it handle edge cases" costs tokens.

With NERD:
- **Generation is 2-3x cheaper** — Same logic, fewer tokens
- **Iteration is faster** — Less to generate, less to parse
- **Context windows go further** — Fit more code in the same context

This isn't about elegance. It's about economics.

---

## What the Human Sees

The human doesn't read NERD. They see a generated view:

```
┌─────────────────────────────────────────┐
│ Calculator Function                      │
├─────────────────────────────────────────┤
│ Input: two numbers + operation code      │
│ Output: result OR error message          │
│                                          │
│ Operations:                              │
│   0 = add                                │
│   1 = subtract                           │
│   2 = multiply                           │
│   3 = divide                             │
│                                          │
│ Error: "unknown" for invalid operation   │
└─────────────────────────────────────────┘
```

They can see data flow. They can understand behavior. But they cannot edit.

To change something, they say: "Add a power operation as operation 4."

And the AI modifies the NERD.

**No syntax to memorize. No documentation to reference. No Stack Overflow to search.**

Just describe what you want. Done.

---

## The Architecture

```
┌─────────────────────────────┐
│     HUMAN LAYER             │
│  "Add rate limiting"        │
│                             │
│  Sees: Diagrams, English    │
│  Cannot: Edit code          │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│     AI LAYER                │
│                             │
│  Reads: Human intent + NERD │
│  Writes: Modified NERD      │
│  Cost: 50-70% less tokens   │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│     NERD LAYER              │
│                             │
│  English words, no symbols  │
│  Types, Functions, Modules  │
│  Optimized for LLMs         │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│     COMPILER                │
│                             │
│  NERD → LLVM IR → Native    │
│  Deterministic              │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│     NATIVE BINARY           │
│                             │
│  Runs on x86_64, ARM64      │
│  No runtime dependencies    │
└─────────────────────────────┘
```

---

## Why This Matters

### 1. The Economics

40% of code is machine-written today. Tomorrow it's 60%. Next year, 80%. Eventually, 95%.

Every token in that 95% costs money. Every character takes time. Every verbose keyword, every descriptive variable name, every nicely formatted brace — all of it burns through context windows and API budgets.

If NERD uses 50-70% fewer tokens for the same logic:
- **Generation is 2-3x cheaper**
- **Iteration is 2-3x faster**
- **Context windows stretch 2-3x further**

At scale, this isn't optimization. It's a different cost structure entirely.

### 2. The Consistency

The AI always writes in the same format. No style debates. No linting. No "I would have done it differently." The code is deterministic, predictable, machine-generated.

### 3. The Observability

Humans can't edit NERD directly — but they can observe it. And the observation layer can be MORE informative than traditional source code:
- What data flows where
- What constraints are enforced
- What security measures are in place
- What could go wrong

In plain English. With diagrams. More auditable than the spaghetti code a tired engineer wrote at 2am.

### 4. The End of Syntax Memorization

Remember memorizing `public static void main(String[] args)`? Or `if __name__ == "__main__"`? Or the difference between `==` and `===`?

All of that was for humans to write code.

If humans don't write code anymore, why memorize syntax? You describe what you want. The machine handles the rest.

**No Effort Required, Done.**

---

## Standard Library

NERD includes a standard library organized like Rust/Java modules:

| Module | Purpose |
|--------|---------|
| `core` | Built-in operators (plus, minus, eq, etc.) |
| `math` | Numeric operations (abs, sqrt, pow) |
| `str` | String operations (len, concat, split) |
| `list` | Collections (map, filter, reduce) |
| `time` | Date/time handling |
| `err` | Error handling |
| `http` | HTTP client |
| `json` | JSON encoding/decoding |

Usage:
```
let x math sqrt y
let s str concat a b
let data json parse text
```

---

## What's Next

The bootstrap compiler is working and compiles NERD to native code via LLVM. To make it production-ready:

1. **Implement standard library** — String, list, HTTP, JSON modules
2. **Build the human view** — Generate diagrams from NERD
3. **Fine-tune models** — Train models specifically on NERD generation

---

## The Philosophical Question

Is this a good idea?

There's something unsettling about code humans don't write or edit. It feels like giving up control.

But consider:
- We don't write machine code. Compilers do.
- We don't manage memory addresses. Operating systems do.
- We don't hand-optimize database queries. Query planners do.

Each abstraction felt like "giving up control" at the time. Each one freed us to work at a higher level.

Source code was the layer where humans expressed intent. But if machines can understand natural language intent directly, source code becomes just another intermediate representation — like bytecode, like assembly, like machine code.

**NERD is what source code becomes when humans stop pretending they need to write it.**

Not human-friendly. Not human-editable. Just an efficient, observable intermediate layer between human intent and machine execution.

---

## Why "NERD"?

**N**o **E**ffort **R**equired, **D**one.

Not because it's easy to write — humans don't write it at all.

Not because it's easy to read — it's dense, terse, machine-optimized.

"No effort required" because the human effort is describing intent, not wrestling with syntax. You say what you want. The machine handles everything else.

**Done** because it compiles, it runs, it works. From natural language to native binary.

---

## The Irony

Here's what's funny: a language designed to NOT be human-friendly turns out to be more readable than traditional code.

Why? Because it's just English words. `a plus b` is clearer than `a + b` when you think about it. `ret ok result` tells you exactly what's happening.

Dense, yes. Terse, yes. But paradoxically... kind of readable.

Just don't try to write it yourself.

---

## Try It Yourself

The NERD specification is at: [spec/NERD-SPEC.md](../../spec/NERD-SPEC.md)

The bootstrap compiler is working. It compiles NERD to native binaries via LLVM. Built from scratch in C, no dependencies.

```bash
# macOS (Apple Silicon)
curl -L https://raw.githubusercontent.com/Nerd-Lang/nerd-lang-core/main/bin/nerd-darwin-arm64 -o nerd && chmod +x nerd

# Or build from source
git clone https://github.com/Nerd-Lang/nerd-lang-core.git
cd nerd-lang-core/bootstrap && make
```

---

## The Bet

In five years, most production code won't be written by humans. It'll be generated, maintained, and modified by AI. Humans will describe intent in natural language, review behavior through visualizations, and ship products without ever touching a semicolon.

When that happens, the languages we use today — TypeScript, Python, Rust, Go — will feel like asking Claude to write in Latin. Beautiful for its time. But not optimized for what's actually happening.

NERD is an experiment in what comes next. A language where humans aren't the authors. Where "readable" doesn't mean "human-readable." Where the machine writes what the machine executes, and humans observe from a layer above.

---

## A Humble Disclaimer

This could be completely wrong.

Maybe humans will always want to edit code directly. Maybe the 40% plateaus and never becomes 90%. Maybe the whole premise is flawed.

NERD is a side project. An exploration. It doesn't have a rich ecosystem of libraries. It doesn't transpile to JavaScript or interop with existing codebases. It's built from scratch — a bootstrap compiler in C, compiling to LLVM IR, with no external dependencies.

That's intentional. The point isn't to replace TypeScript tomorrow. The point is to ask: *what would a language look like if we stopped pretending humans write code?*

If the answer is interesting, great. If not, it was a worthy experiment.

Contributions welcome. The whole thing is open source.

---

## Roadmap

What's next for NERD:

**Near term:**
- [ ] More built-in functions (string operations, list manipulation)
- [ ] Standard library expansion (HTTP client, JSON parsing)
- [ ] Better error messages in the compiler
- [ ] WASM target (browser support)

**Medium term:**
- [ ] Human View generator — visualize NERD as diagrams and English
- [ ] IDE integration — syntax highlighting, LSP support
- [ ] More examples and documentation

**Long term:**
- [ ] Fine-tuned models for NERD generation
- [ ] Integration with Claude Code and other AI coding tools
- [ ] Production deployments and case studies

If any of this sounds interesting, come build with us.

---

*This started as a question during a coding session: "Why is Claude writing code I'm supposed to read?"*

*It became a specification. Then a compiler. The answer to that question is now running on a machine.*

**40% of code is written by machines. When that becomes 80%, then 95% — why should it look like something humans wrote?**

**No Effort Required, Done.**
