# Introduction

- Lambda Zero is a minimalist pure lazy functional programming language.
- The language is the untyped lambda calculus plus integers, integer arithmetic,
  and a few Haskell-inspired syntactic sugars evaluated by a Lazy Krivine
  Machine.
- Every Lambda Zero program is a single expression; there are no statements
  or variables in the language.
- The interpreter is less than 1800 lines of strict ANSI C99 and the binary is
  just 32KB dynamically linked and stripped.
- The interpreter can also be built for Linux x86-64 without the C standard
  library in which case it is about 2100 lines of strict ANSI C99 and generates
  a 23KB statically linked stripped binary.
- The interpreter includes reference-counting garbage collection on a free list
  memory allocator, error messaging, and rudimental built-in debugging and
  profiling features.

# Sample Code

### Hello World

    main(input) = "hello world"
    main

### Factorial

    factorial(n) = if (n == 0) then 1 else n * factorial(n - 1)

### Quicksort

    xs ?: f = if (xs.empty) then nil else f(xs.head, xs.tail)
    sort(ns) = ns ?: k -> ks -> sort(ks | (<= k)) ++ k :: sort(ks | (> k))

### Infinite list of natural numbers

    iterate(f, x) = x :: iterate(f, f(x))
    countFrom = iterate(+ 1)
    naturals = countFrom(0)

### Infinite list of prime numbers

    primes = (f(ms) = ms ?: n -> ns -> n :: f(ns |~ n.divides)) f(countFrom(2))

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

    integer = [-]?[0-9]+
    builtin = '+' | '-' | '*' | '/' | 'mod' | '==' | '=/=' | '<' | '>' | '<=' | '>='
    name = ("a token that is not an integer, builtin, delimiter, or arrow")
    expr = integer | builtin | name | (name -> expr) | (expr expr)

### Syntactic sugars

Let `x, y, z` be generic expressions, `f` be a name expression, and `op` be
an operator name expression.

- Left associative spaces: `x y z => ((x y) z)`
- Right associative newlines: `x \n y \n z => (z (y z))`
- Redundant Parentheses: `(x) => x` unless `x` is an operator
 (see Un-infixify sugar below)
- Multiparameter lambdas: `x y -> z => x -> y -> z`
- Definitions (Let expressions): `(f = x) y => (f -> y) x`
- Function definitions: `f x y = z => f = x -> y -> z`
- Recursive definitions: The right hand sise of a function definition can refer
 to the function name, in which case the Y combinator is used to convert the
 definition to a non-recursive one.
- Function calls: `f(x, y) => f x y`
- Tuples: `(x, y, z) => f -> f x y z`
- Infix operators: `x op y => op x y` where `op` is a non-alphanumeric symbol
When operators are chained like `x op y op z` there are precedence rules that
determine how it is parsed.
- Operator names: `(op)` disables the infix operator sugar on `op`
- Sections: `(op y) => x -> (x op y)` and `(y op) => x -> (y op x)`
- If-then-else: `then` and `else` are treated as infix operators that apply the left argument to the right argument
- String literals: `"abc"` desugars to a linked list of ascii character codes

### I/O

There is one type of expression that is treated specially. A function with
the parameter name of `input`, when evaluated, is passed stdin as a string
and the return value is assumed to be a string which is passed to stdout.

A string is a linked list of integers in the range 0-255, where the linked
list is constructed in the standard way for the Lambda Calculus.

### Conclusion

That's the whole language! Take a look at the [prelude](test/prelude)
for more examples.

# Building

Make sure gcc and bash are installed and run the `make` script.

# Running

    echo -n "INPUT" | run SOURCEFILE

For example, try this sample program that prints out an infinite list of prime
numbers:

    echo -n "" | run test/samples/showprimes

The `run` script prepends the [prelude](test/prelude) to the `SOURCEFILE` and
passes the result to `main`.

Note that when using the `run` script, line numbers in error messages will be
offset by the number of lines in the prelude.

# Stability

Lambda Zero is in version 0.x, so breaking changes can be made in any commit.
