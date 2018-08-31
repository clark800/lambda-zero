#include "lib/tree.h"
#include "ast.h"
#include "scan.h"
#include "errors.h"
#include "define.h"
#include "operators.h"

typedef enum {L, R, N} Associativity;
typedef unsigned char Precedence;
typedef struct {
    const char* symbol;
    Precedence leftPrecedence, rightPrecedence;
    Fixity fixity;
    Associativity associativity;
    Node* (*apply)(Node* operator, Node* left, Node* right);
} Rules;

static Node* apply(Node* operator, Node* left, Node* right) {
    return newApplication(getTag(operator), left, right);
}

static int lookupBuiltinCode(Node* token) {
    for (unsigned int i = 0; i < sizeof(BUILTINS)/sizeof(char*); i++)
        if (isThisToken(token, BUILTINS[i]))
            return (int)i;
    return -1;
}

static Node* convertOperator(Node* operator) {
    int code = lookupBuiltinCode(operator);
    Tag tag = getTag(operator);
    return code >= 0 ? newBuiltin(tag, code) : newName(tag);
}

static Node* infix(Node* operator, Node* left, Node* right) {
    return apply(operator, apply(operator,
        convertOperator(operator), left), right);
}

static Node* asterisk(Node* operator, Node* left, Node* right) {
    (void)left, (void)right;
    // rename operator so it will generate an error for being undefined if
    // a prefix asterisk appears outside an ADT definition
    return newName(renameTag(getTag(operator), "(*)"));
}

static unsigned int getCommaListLength(Node* node) {
    return !isCommaList(node) ? 1 : 1 + getCommaListLength(getLeft(node));
}

static Node* newConstructorDefinition(Node* pattern, int i, int n) {
    // pattern is an application of a constructor name to a number of asterisks
    // i is the index of this constructor in this algebraic data type
    // n is the total number of constructors for this algebraic data type

    // verify that all arguments in pattern are asterisks and count to get k
    int k = 0;
    for (; isApplication(pattern); ++k, pattern = getLeft(pattern))
        syntaxErrorIf(!isThisToken(getRight(pattern), "(*)"),
            "constructor parameters must be asterisks", getRight(pattern));

    Tag tag = getTag(pattern);
    // let p_* be constructor parameters (k total)
    // let c_* be constructor names (n total)
    // build: p_1 -> ... -> p_k -> c_1 -> ... -> c_n -> c_i p_1 ... p_k
    Node* constructor = newBlankReference(tag, (unsigned long long)(n - i));
    for (int j = 0; j < k; ++j)
        constructor = newApplication(tag, constructor,
            newBlankReference(tag, (unsigned long long)(n + k - j)));
    for (int j = 0; j < n + k; ++j)
        constructor = newLambda(tag, newBlank(tag), constructor);
    syntaxErrorIf(!isReference(pattern), "invalid constructor name", pattern);
    return newDefinition(tag, pattern, constructor);
}

static Node* extend(Node* list, Node* item) {
    return list == NULL ? item : newCommaList(getTag(item), list, item);
}

static Node* curly(Node* close, Node* open, Node* patterns) {
    syntaxErrorIf(patterns == NULL, "missing patterns", open);
    syntaxErrorIf(!isThisToken(open, "{"), "missing open for", close);
    syntaxErrorIf(isDefinition(patterns), "missing scope", patterns);
    syntaxErrorIf(isPipePair(patterns), "invalid '|' in", open);
    // for each item in the patterns comma list, define a constructor function,
    // then return all of these definitions as a comma list, which the parser
    // converts to a sequence of lines
    int n = (int)getCommaListLength(patterns);
    Node* definitions = NULL;
    for (int i = 0; isCommaList(patterns); ++i, patterns = getLeft(patterns))
        definitions = extend(definitions,
            newConstructorDefinition(getRight(patterns), n - i - 1, n));
    definitions = extend(definitions, newConstructorDefinition(patterns, 0, n));
    return newADT(getTag(open), definitions);
}

static Node* reduceADT(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(!isReference(left) && !isApplication(left),
        "invalid left hand side", operator);
    syntaxErrorIf(!isADT(right), "right side must be an ADT", operator);
    return right;
}

static Node* pipe(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(isDefinition(left), "missing scope", left);
    syntaxErrorIf(isDefinition(right), "missing scope", right);
    return newPipePair(getTag(operator), left, right);
}

static Node* comma(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(isDefinition(left), "missing scope", left);
    syntaxErrorIf(isDefinition(right), "missing scope", right);
    return newCommaList(getTag(operator), left, right);
}

static Node* getHead(Node* node) {
    while (isApplication(node))
        node = getLeft(node);
    return node;
}

static bool isStrictPatternLambda(Node* lambda) {
    return isBlank(getParameter(lambda)) && isBlank(getHead(getBody(lambda)));
}

static bool isLazyPatternLambda(Node* lambda) {
    return isBlank(getParameter(lambda)) && isApplication(getBody(lambda)) &&
        isApplication(getRight(getBody(lambda))) &&
        isBlank(getLeft(getRight(getBody(lambda))));
}

static Node* getPatternExtension(Node* lambda) {
    if (isStrictPatternLambda(lambda))
        return getRight(getBody(lambda));
    if (isLazyPatternLambda(lambda))
        return getHead(getBody(lambda));
    return getBody(lambda);
}

static Node* semicolon(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(!isLambda(left), "expected lambda to left of", operator);
    syntaxErrorIf(!isLambda(right), "expected lambda to right of", operator);
    Tag tag = getTag(operator);
    Node* base = isStrictPatternLambda(left) ? getBody(left) : newApplication(
        tag, newBlankReference(tag, 1), getPatternExtension(left));
    return newLambda(tag, newBlank(tag),
        newApplication(tag, base, getPatternExtension(right)));
}

static int getArgumentCount(Node* application) {
    int i = 0;
    for (Node* n = application; isApplication(n); i++)
        n = getLeft(n);
    return i;
}

static Node* newProjection(Tag tag, int size, int index) {
    Node* projection = newBlankReference(tag,
        (unsigned long long)(size - index));
    for (int i = 0; i < size; i++)
        projection = newLambda(tag, newBlank(tag), projection);
    return projection;
}

Node* newPatternLambda(Node* operator, Node* left, Node* right) {
    // lazy pattern matching
    Tag tag = getTag(operator);
    if (isReference(left))
        return newLambda(tag, left, right);
    syntaxErrorIf(!isApplication(left), "invalid parameter", left);
    // example: (x, y) -> body ---> _ -> (x -> y -> body) first(_) second(_)
    Node* body = right;
    for (Node* items = left; isApplication(items); items = getLeft(items))
        body = newPatternLambda(operator, getRight(items), body);
    for (int i = 0, size = getArgumentCount(left); i < size; i++)
        body = newApplication(tag, body,
            newApplication(tag, newBlankReference(tag, 1),
                newProjection(tag, size, i)));
    return newLambda(tag, newBlank(tag), body);
}

/*
Node* newStrictPatternLambda(Node* operator, Node* left, Node* right) {
    Tag tag = getTag(operator);
    if (isReference(left))
        return newLambda(tag, left, right);
    syntaxErrorIf(!isApplication(left), "invalid parameter", left);
    // example: (x, y) -> body ---> _ -> _ (x -> y -> body)
    for (; isApplication(left); left = getLeft(left))
        right = newPatternLambda(operator, getRight(left), right);
    // discard left, which is the value constructor
    return newLambda(tag, newBlank(tag),
        newApplication(tag, newBlankReference(tag, 1), right));
}
*/

static Node* prefix(Node* operator, Node* left, Node* right) {
    (void)left;     // suppress unused parameter warning
    return apply(operator, convertOperator(operator), right);
}

static Node* negate(Node* operator, Node* left, Node* right) {
    (void)left;     // suppress unused parameter warning
    return infix(operator, newInteger(getTag(operator), 0), right);
}

static Node* unmatched(Node* operator, Node* left, Node* right) {
    syntaxError("missing close for", operator);
    return left == NULL ? right : left; // suppress unused parameter warning
}

static Node* applyToCommaList(Tag tag, Node* base, Node* arguments) {
    if (!isCommaList(arguments))
        return base == NULL ? arguments : newApplication(tag, base, arguments);
    return newApplication(tag, applyToCommaList(tag, base,
        getLeft(arguments)), getRight(arguments));
}

static inline Node* newTuple(Node* open, Node* commaList, const char name[32]){
    unsigned int length = getCommaListLength(commaList) - 1;
    syntaxErrorIf(length > 32, "too many arguments", open);
    Tag tag = newTag(newString(name, length), getTag(open).location);
    return applyToCommaList(getTag(open), newName(tag), commaList);
}

static Node* square(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "["), "missing open for", close);
    Tag tag = getTag(open);
    if (contents == NULL)
        return newNil(tag);
    syntaxErrorIf(isPipePair(contents), "invalid '|' in", contents);
    syntaxErrorIf(isDefinition(contents), "missing scope", contents);
    if (getFixity(open) == OPENCALL) {
        static const char opens[32] = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[";
        syntaxErrorIf(!isCommaList(contents), "missing argument to", open);
        // not really a tuple, but it is the same structure
        return newTuple(open, contents, opens);
    }
    Node* list = newNil(tag);
    if (!isCommaList(contents))
        return prepend(tag, contents, list);
    for(; isCommaList(contents); contents = getLeft(contents))
        list = prepend(tag, getRight(contents), list);
    return prepend(tag, contents, list);
}

static Node* newFilter(Tag tag, Node* left, Node* right) {
    syntaxErrorIf(isCommaList(left), "comma invalid on left of pipe", left);
    syntaxErrorIf(isCommaList(right), "comma invalid on right of pipe", right);
    Node* filter = newName(renameTag(tag, "filter"));
    return newApplication(tag, newApplication(tag, filter, left), right);
}

static Node* parentheses(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "("), "missing open for", close);
    Tag tag = getTag(open);
    if (contents == NULL)
        return newName(renameTag(tag, "()"));
    syntaxErrorIf(isDefinition(contents), "missing scope", contents);
    if (getFixity(open) == OPENCALL) {
        if (isPipePair(contents)) {
            // getLeft(contents) must be a comma list because function
            // is moved inside of parenthesis with a comma afterwards
            Node* function = getLeft(getLeft(contents));
            if (isCommaList(function))
                syntaxError("pipe not inside brackets", contents);
            return newApplication(tag, function, newFilter(tag,
                getRight(getLeft(contents)), getRight(contents)));
        }
        return isCommaList(contents) ? applyToCommaList(tag, NULL, contents) :
            // desugar f() to f(())
            newApplication(tag, contents, newName(renameTag(tag, "()")));
    }

    if (isOperator(contents)) {
        // update rules to favor infix over prefix inside parenthesis
        setRules(contents, false);
        if (isSpecialOperator(contents) && !isComma(contents))
            syntaxError("operator cannot be parenthesized", contents);
        return convertOperator(contents);
    }
    static const char commas[32] = ",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,";
    if (isCommaList(contents))
        return newTuple(open, contents, commas);
    if (isPipePair(contents))
        return newFilter(tag, getLeft(contents), getRight(contents));
    if (isApplication(contents))
        return setLocation(contents, getLocation(open));
    return contents;
}

static Node* eof(Node* operator, Node* open, Node* contents) {
    (void)operator;
    syntaxErrorIf(!isEOF(open), "missing close for", open);
    syntaxErrorIf(isEOF(contents), "no input", open);
    syntaxErrorIf(isCommaList(contents), "comma not inside brackets", contents);
    syntaxErrorIf(isPipePair(contents), "pipe not inside brackets", contents);
    return isDefinition(contents) ? transformDefinition(contents) : contents;
}

// note: must check for pipe pairs, comma lists, and definitions in every
// reducer for operators of lower precedence to ensure that parser-specific
// node types don't reach the evaluator.
static Rules RULES[] = {
    // syntactic operators
    {"\0", 0, 0, CLOSE, L, eof},
    {"(", 22, 0, OPENCALL, L, unmatched},
    {"(", 22, 0, OPEN, L, unmatched},
    {")", 0, 22, CLOSE, R, parentheses},
    {"[", 22, 0, OPENCALL, L, unmatched},
    {"[", 22, 0, OPEN, L, unmatched},
    {"]", 0, 22, CLOSE, R, square},
    {"{", 22, 0, OPEN, L, unmatched},
    {"}", 0, 22, CLOSE, R, curly},
    {"|", 1, 1, IN, N, pipe},
    {",", 2, 2, IN, L, comma},
    {"\n", 3, 3, IN, R, reduceNewline},
    {":=", 4, 4, IN, N, reduceDefine},
    {"::=", 4, 4, IN, N, reduceADT},
    {";", 5, 5, IN, L, semicolon},
    {"->", 6, 6, IN, R, newPatternLambda},

    // conditional operators
    {"||", 6, 6, IN, R, infix},
    {"?", 7, 7, IN, R, infix},
    {"?||", 7, 7, IN, R, infix},

    // monadic operators
    {">>=", 8, 8, IN, L, infix},

    // logical operators
    {"<=>", 9, 9, IN, N, infix},
    {"=>", 10, 10, IN, N, infix},
    {"\\/", 11, 11, IN, R, infix},
    {"/\\", 12, 12, IN, R, infix},

    // comparison/test operators
    {"=", 13, 13, IN, N, infix},
    {"!=", 13, 13, IN, N, infix},
    {"=*=", 13, 13, IN, N, infix},
    {"=?=", 13, 13, IN, N, infix},
    {"<", 13, 13, IN, N, infix},
    {">", 13, 13, IN, N, infix},
    {"<=", 13, 13, IN, N, infix},
    {">=", 13, 13, IN, N, infix},
    {"=<", 13, 13, IN, N, infix},
    {"~<", 13, 13, IN, N, infix},
    {"<:", 13, 13, IN, N, infix},
    {"<*=", 13, 13, IN, N, infix},
    {":", 13, 13, IN, N, infix},
    {"!:", 13, 13, IN, N, infix},
    {"~", 13, 13, IN, N, infix},

    // precedence 14: default
    // arithmetic/list operators
    {"..", 15, 15, IN, N, infix},
    {"::", 16, 16, IN, R, infix},
    {"&", 16, 16, IN, L, infix},
    {"+", 16, 16, IN, L, infix},
    {"\\./", 16, 16, IN, L, infix},
    {"++", 16, 16, IN, R, infix},
    {"-", 16, 16, IN, L, infix},
    {"\\", 16, 16, IN, L, infix},
    {"*", 17, 17, IN, L, infix},
    {"**", 17, 17, IN, R, infix},
    {"/", 17, 17, IN, L, infix},
    {"%", 17, 17, IN, L, infix},
    {"^", 18, 18, IN, R, infix},

    // functional operators
    {"<>", 19, 19, IN, R, infix},

    // precedence 20: space operator
    // prefix operators
    {"-", 21, 21, PRE, L, negate},
    {"--", 21, 21, PRE, L, prefix},
    {"!", 21, 21, PRE, L, prefix},
    {"#", 21, 21, PRE, L, prefix},
    {"*", 21, 21, PRE, L, asterisk},

    // precedence 22: parentheses/brackets
    {"^^", 23, 23, IN, L, infix},
    {".", 24, 24, IN, L, infix},
    {"`", 25, 25, PRE, L, prefix}
};

static Rules DEFAULT = {"", 14, 14, IN, L, infix};
static Rules SPACE = {" ", 20, 20, IN, L, apply};

static bool allowsOperatorBefore(Rules rules) {
    return rules.fixity == PRE || rules.fixity == OPEN || rules.fixity == CLOSE;
}

bool isSpace(Node* token) {
    return isLeaf(token) && isSpaceCharacter(getLexeme(token).start[0]);
}

static Rules* lookupRules(Node* token, bool isAfterOperator) {
    if (isSpace(token))
        return &SPACE;
    Rules* result = &DEFAULT;
    for (unsigned int i = 0; i < sizeof(RULES)/sizeof(Rules); i++)
        if (isThisToken(token, RULES[i].symbol)) {
            result = &(RULES[i]);
            if (!isAfterOperator || allowsOperatorBefore(RULES[i]))
                return result;
        }
    return result;
}

Node* setRules(Node* token, bool isAfterOperator) {
    return setValue(token, (long long)lookupRules(token, isAfterOperator));
}

Fixity getFixity(Node* operator) {
    return ((Rules*)getValue(operator))->fixity;
}

Node* applyOperator(Node* operator, Node* left, Node* right) {
    return ((Rules*)getValue(operator))->apply(operator, left, right);
}

bool isSpecialOperator(Node* operator) {
    Rules* rules = (Rules*)getValue(operator);
    return rules->apply != infix && rules->apply != prefix;
}

bool isHigherPrecedence(Node* left, Node* right) {
    Rules* leftRules = (Rules*)getValue(left);
    Rules* rightRules = (Rules*)getValue(right);
    if (leftRules->rightPrecedence == rightRules->leftPrecedence) {
        const char* message = "operator is non-associative";
        syntaxErrorIf(leftRules->associativity == N, message, left);
        syntaxErrorIf(rightRules->associativity == N, message, right);
    }

    if (rightRules->associativity == R)
        return leftRules->rightPrecedence > rightRules->leftPrecedence;
    else
        return leftRules->rightPrecedence >= rightRules->leftPrecedence;
}
