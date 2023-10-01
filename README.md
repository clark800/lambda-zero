# Introduction

Lambda Zero is a minimalist pure lazy functional programming language bootstrapped from about 2500 lines of C. It can be thought of as pythonic haskell with no IO. Here "no IO" means that the main function maps strings to strings, so there is no concept of IO *within the language*, but the interpreter maps the input and output of main to stdin and stdout. Due to laziness, it's possible to build interactive GUI applications using an adapter (see the Conway's Game of Life sample in the repo).

Features include:
* All the standard Lambda Calculus features (first class functions, closures,
    currying, recursion, nested function definitions, ...)
* Lazy evaluation (infinite data structures, increased modularity, ...)
* Algebraic data types (enumerations, tuples, lists, trees, records/structs,
    maybe/optional types, either types, monads, ...)
* Pattern matching (case expressions, tuple destructuring, ...)
* Automatic garbage collection
* Semantic whitespace
* Unicode support (UTF8)
* User-defined lexically-scoped operator syntax and semantics
* Hindley-Milner static type inference (implemented in Lambda Zero)
* A self-interpreter

# Sample Code

### Hello World

    main(input) ≔ "hello world"

### Factorial

    n ! ≔ n ⦊ (case 0 ↦ 1; case ↑ n′ ↦ n ⋅ n′ !)

### Quicksort

    sort ≔ (case [] ↦ []; case n ∷ ns ↦ sort(ns ¦ (≤ n)) ⧺ [n] ⧺ sort(ns ¦ (> n)))

### Infinite list of natural numbers

    iterate(f, x) ≔ x ∷ iterate(f, f(x))
    (…) ≔ iterate(↑)
    naturals ≔ 0 …

### Infinite list of Fibonacci numbers

    fibonaccis ≔ f(0, 1) where f(m, n) ≔ m ∷ f(n, m + n)

### Infinite list of prime numbers

    primes ≔ p(2 …) where p ≔ (case [] ↦ []; case n ∷ ns ↦ n ∷ p(ns ¦ (% n ≠ 0)))

# Building

Make sure a C compiler is installed and run the `make` script in
the bootstrap-interpreter directory:

    cd bootstrap-interpreter
    ./make

# Running

From the bootstrap-interpreter directory:

    ./run SOURCEFILE

For example, try this sample program that prints out an infinite list of prime
numbers:

    ./run test/samples/primes.zero

The `run` script prepends the [prelude](libraries/prelude.zero)
to the `SOURCEFILE` and passes the result to `main`.

# Stability

Breaking changes may be made at any time.
