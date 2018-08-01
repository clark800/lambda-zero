# Introduction

- Lambda Zero is a minimalist pure lazy functional programming language.
- The language is the untyped lambda calculus plus integers, integer arithmetic,
  and a few Haskell-inspired syntactic sugars evaluated by a Lazy Krivine
  Machine.
- Every Lambda Zero program is a single expression; there are no statements
  or variables in the language.
- The interpreter is less than 2000 lines of strict ANSI C99 and the binary is
  just ~60KB dynamically linked and stripped.
- The interpreter can also be built for Linux x86-64 without the C standard
  library in which case it is less than 2500 lines of strict ANSI C99 and
  generates a ~30KB statically linked stripped binary.
- The interpreter includes reference-counting garbage collection on a free list
  memory allocator, error messaging, and rudimental built-in debugging features.

# Sample Code

### Hello World

    main(input) := "hello world"

### Factorial

    factorial(n) := (n = 0) ? 1 || n * factorial(n - 1)

### Quicksort

    sort(ns) := (
        isEmpty(ns) ? []
        (n, ns') := ns
        sort(ns' | (<= n)) ++ [n] ++ sort(ns' | (> n))
    )

### Infinite list of natural numbers

    iterate(f, x) := x :: iterate(f, f(x))
    countFrom := iterate((+ 1))
    naturals := countFrom(0)

### Infinite list of prime numbers

    primes := (
        filterPrime((n, ns)) := n ∷ filterPrime(ns | n' -> n' % n != 0)
        filterPrime(countFrom(2))
    )

# Motivation

The Lambda Calculus is incredibly elegant and can be a very powerful
high-level programming language with just a few simple optimizations and
syntactic sugars.

Lambda Zero allows you to program in the Lambda Calculus with just the
minimal amount of added features to make it practical and readable.
Lambda Zero combines the elegance of Haskell with the simplicity of LISP.

# Use Cases

The primary use case of Lambda Zero is to bootstrap a future higher level
language "Lambda One" i.e. Lambda Zero is a sublanguage of Lambda One and the
Lambda One compiler is written entirely in Lambda Zero.

Lambda Zero is also a good way to learn about functional programming since it
is much simpler than other languages like Haskell, but operates on the same
foundation.

Lambda Zero is not a general purpose language; the interpreter is optimized
for simplicity over performance and the language does not allow any I/O
besides accepting stdio as a string and returning a string which is sent to
stdout by the interpreter.

# Tutorial

### Core Language

First we will describe the core language without syntactic sugars, which is very
small. These are the only things you can do in the core language:
- Define a one-parameter function with the arrow operator: `(x -> body)`
- Call a one-parameter function by putting a space between the function name `f`
  and the argument `x`: `(f x)`
- Refer to the parameter of a function by name: `x`
- Refer to an integer: `123`
- Refer to a built-in operator: `+`

For example, we can combine all of these to write a function that returns
its argument plus one (just ignore the parentheses around the plus sign; they
are to disable a syntactic sugar):

    (x -> (((+) x) 1))

Note that you can construct a function that effectively takes two parameters
by making a one-parameter function that returns a one-parameter function,
so that it takes two function applications to get to the body. This is the
concept of currying. For example, to make a function that adds two arguments:

    (x -> (y -> (((+) x) y)))

The desugared language can be described by the grammar rules below
(technically the desugared language is a sublanguage of expr because this
grammar ignores some error cases):

    natural = [0-9]+
    builtin = '+' | '-' | '*' | '/' | '%' | '=' | '!=' | '<' | '>' | '<=' | '>=' | 'error'
    name = ("a token that is not a natural, builtin, delimiter, or arrow")
    expr = natural | builtin | name | (name -> expr) | (expr expr)

### Syntactic sugars

Let `x, y, z` be generic expressions, `f` be a name expression, and `*` be
an operator name expression.

- Left associative spaces: `x y z` desugars to `((x y) z)`
- Right associative newlines: `x \n y \n z` desguars to `(z (y z))`
- Redundant Parentheses: `(x)` desugars to `x` unless `x` is an operator
  (see Operator names sugar below)
- Definitions (Let expressions): `(f := x) y` desugars to `(f -> y) x`
- Trailing definitions: `(x (f := y))` desguars to `(x ((f := y) f))`
  If there is nothing after a definition, it as treated as if the name of the
  definition follows the definition.
- Function definitions: `f x y := z` desugars to `f := x -> y -> z`
- Recursive definitions: The right hand sise of a function definition can refer
 to the function name, in which case the Y combinator is used to convert the
 definition to a non-recursive one.
- Curried function application: `f(x, y)` desugars to `f x y`
- Infix operators: `x * y` desugars to `* x y`
When operators are chained like `x * y * z` there are precedence and associatvitiy rules that determine how it is parsed.
- Operator names: `(*)` disables the infix operator sugar on `*`
- Sections: `(* y)` desguars to `x -> (x * y)` and
  `(y *)` desugars to `x -> (y * x)`
- String literals: `"abc"` desugars to a Church-encoded list of ascii
  character codes.
- Character literals: `'a'` desugars to the ascii character code for `a`
- List literals: `[x, y, z]` desugars to the Church-encoded list of
  `x`, `y`, and `z`.
- Tuple destructuring: `(a, b, c) := x` desugars to the equivalent of
  `a := first(x)`, `b := second(x)`, and `c := third(x)`. Tuples can also be
  used on the left side of `->` and in parameters on the left side of
  function definitions. Destructuring is also recursive e.g.
  `((a, b), (c, d)) := x`.

### I/O

There is no I/O in the lambda zero _language_, but there is some basic I/O
support in the lambda zero _interpreter_. In other words, there is no
syntax in a lamda zero program that actually performs I/O itself, but there is a
special function `main` that the interpreter treats differently to allow I/O.

When the last function definition of a program defines the symbol `main`, the
interpreter assumes that it is a function and passes it a string corresponding
to `stdin` and then assumes that the return value of `main` is a string and
prints it to `stdout`. An error will occur if these assumptions aren't true.

A string is a linked list of integers in the range 0-255, where the linked
list is constructed according to the standard Church encoding.

The input and output are both evaluated lazily, which allows for non-terminating
output and interactive programs. See the samples below for examples of this.

### Conclusion

That's the whole language! Take a look at the [prelude](libraries/prelude.zero)
for more examples.

# Building

Make sure gcc and bash are installed and run the `make` script in the
bootstrap-interpreter directory.

# Running
First `cd` to the bootstrap-interpreter directory.

    ./run SOURCEFILE

For example, try this sample program that prints out an infinite list of prime
numbers:

    ./run ../sample-code/showprimes

This sample program runs an interactive REPL that reverses each line entered:

    ./run ../sample-code/repl

The `run` script prepends the [prelude](libraries/prelude.zero) to the `SOURCEFILE` and
passes the result to `main`.

Note that when using the `run` script, line numbers in error messages will be
offset by the number of lines in the prelude.

# Stability

Lambda Zero is in version 0.x, so breaking changes can be made in any commit.
