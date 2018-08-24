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

static Node* infix(Node* operator, Node* left, Node* right) {
    Node* symbol = setValue(newName(getTag(operator)), CONVERSION);
    return apply(operator, apply(operator, symbol, left), right);
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

static int getArgumentCount(Node* tuple) {
    int i = 0;
    for (Node* n = tuple; isApplication(n); i++)
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

Node* newLazyPatternLambda(Node* operator, Node* left, Node* right) {
    Tag tag = getTag(operator);
    if (isName(left))
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
    if (isName(left))
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

Node* newPatternLambda(Node* operator, Node* left, Node* right) {
    return newLazyPatternLambda(operator, left, right);
}

static Node* prefix(Node* operator, Node* left, Node* right) {
    (void)left;     // suppress unused parameter warning
    Node* symbol = setValue(newName(getTag(operator)), CONVERSION);
    return apply(operator, symbol, right);
}

static Node* negate(Node* operator, Node* left, Node* right) {
    (void)left;     // suppress unused parameter warning
    return infix(operator, newInteger(getTag(operator), 0), right);
}

static Node* unmatched(Node* operator, Node* left, Node* right) {
    syntaxError("missing close for", operator);
    return left == NULL ? right : left; // suppress unused parameter warning
}

static Node* brackets(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "["), "missing open for", close);
    Tag tag = getTag(open);
    if (contents == NULL)
        return newNil(tag);
    syntaxErrorIf(isDefinition(contents), "missing scope", contents);
    if (getFixity(open) == OPENCALL) {
        syntaxErrorIf(!isCommaList(contents), "missing argument to", open);
        syntaxErrorIf(isCommaList(getLeft(contents)),
            "too many arguments to", open);
        return newApplication(tag, newApplication(tag, newName(getTag(open)),
            getLeft(contents)), getRight(contents));
    }
    Node* list = newNil(tag);
    if (!isCommaList(contents))
        return prepend(tag, contents, list);
    for(; isCommaList(contents); contents = getLeft(contents))
        list = prepend(tag, getRight(contents), list);
    return prepend(tag, contents, list);
}

static Node* applyToCommaList(Tag tag, Node* base, Node* arguments) {
    if (!isCommaList(arguments))
        return base == NULL ? arguments : newApplication(tag, base, arguments);
    return newApplication(tag, applyToCommaList(tag, base,
        getLeft(arguments)), getRight(arguments));
}

static unsigned int getCommaListLength(Node* node) {
    return !isCommaList(node) ? 1 : 1 + getCommaListLength(getLeft(node));
}

static inline Node* newTuple(Tag tag, Node* commaList) {
    static const char commas[] = ",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,";
    unsigned int length = getCommaListLength(commaList) - 1;
    syntaxErrorIf(length > sizeof(commas), "tuple too big", commaList);
    Node* constructor = setValue(
        newName(newTag(newString(commas, length), tag.location)), CONVERSION);
    return applyToCommaList(tag, constructor, commaList);
}

static Node* parentheses(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "("), "missing open for", close);
    Tag tag = getTag(open);
    if (contents == NULL)
        return newName(renameTag(tag, "()"));
    syntaxErrorIf(isDefinition(contents), "missing scope", contents);
    if (getFixity(open) == OPENCALL)
        return isCommaList(contents) ? applyToCommaList(tag, NULL, contents) :
            // desugar f() to f(())
            newApplication(tag, contents, newName(renameTag(tag, "()")));

    if (isOperator(contents)) {
        // update rules to favor infix over prefix inside parenthesis
        setRules(contents, false);
        if (isSpecialOperator(contents) && !isComma(contents))
            syntaxError("operator cannot be parenthesized", contents);
        return newName(getTag(contents));
    }
    if (isCommaList(contents))
        return newTuple(tag, contents);
    if (isApplication(contents))
        return setLocation(contents, getLocation(open));
    return contents;
}

static Node* eof(Node* operator, Node* open, Node* contents) {
    (void)operator;
    syntaxErrorIf(isEOF(contents), "no input", open);
    syntaxErrorIf(!isEOF(open), "missing close for", open);
    syntaxErrorIf(isCommaList(contents), "comma not inside brackets", contents);
    return isDefinition(contents) ? transformDefinition(contents) : contents;
}

// comma must be the lowest precedence operator above parentheses/brackets
// or else a commaList, which is an invalid operand, could get buried in the
// AST without being detected, then a surrounding parentheses could apply
// a tuple abstraction, which would bind across a bracket boundary.
// if a comma is not wrapped in parentheses or brackets, it will be at the
// very top level and thus won't be defined, so bind will catch this case.
static Rules RULES[] = {
    // syntactic operators
    {"\0", 0, 0, CLOSE, L, eof},
    {"(", 22, 0, OPENCALL, L, unmatched},
    {"(", 22, 0, OPEN, L, unmatched},
    {")", 0, 22, CLOSE, R, parentheses},
    {"[", 22, 0, OPENCALL, L, unmatched},
    {"[", 22, 0, OPEN, L, unmatched},
    {"]", 0, 22, CLOSE, R, brackets},
    {",", 1, 1, IN, L, comma},
    {"\n", 2, 2, IN, R, reduceNewline},
    {":=", 3, 3, IN, N, reduceDefine},
    {"|", 4, 4, IN, N, infix},  // should be lower precedence than comma
    // todo: comma here: should be lower than semicolon
    {";", 4, 4, IN, L, semicolon},
    {"->", 5, 5, IN, R, newPatternLambda},

    // conditional operators
    {"||", 5, 5, IN, R, infix},
    {"?", 6, 6, IN, R, infix},
    {"?||", 6, 6, IN, R, infix},

    // monadic operators
    {">>=", 7, 7, IN, L, infix},

    // logical operators
    {"<=>", 8, 8, IN, N, infix},
    {"=>", 9, 9, IN, N, infix},
    {"\\/", 10, 10, IN, R, infix},
    {"/\\", 11, 11, IN, R, infix},

    // comparison/test operators
    {"=", 12, 12, IN, N, infix},
    {"!=", 12, 12, IN, N, infix},
    {"=*=", 12, 12, IN, N, infix},
    {"<", 12, 12, IN, N, infix},
    {">", 12, 12, IN, N, infix},
    {"<=", 12, 12, IN, N, infix},
    {">=", 12, 12, IN, N, infix},
    {"=<", 12, 12, IN, N, infix},
    {"~<", 12, 12, IN, N, infix},
    {"<:", 12, 12, IN, N, infix},
    {":", 12, 12, IN, N, infix},
    {"!:", 12, 12, IN, N, infix},
    {"~", 12, 12, IN, N, infix},

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
