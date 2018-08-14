#include "lib/tree.h"
#include "ast.h"
#include "scan.h"
#include "errors.h"
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

Node* apply(Node* operator, Node* left, Node* right) {
    return newApplication(getTag(operator), left, right);
}

Node* definition(Node* operator, Node* left, Node* right) {
    return newDefinition(getTag(operator), left, right);
}

Node* infix(Node* operator, Node* left, Node* right) {
    return apply(operator, apply(operator, operator, left), right);
}

Node* comma(Node* operator, Node* left, Node* right) {
    Tag tag = getTag(operator);
    return isCommaList(left) ? newCommaList(tag, left, right) :
        newCommaList(tag, newCommaList(tag, operator, left), right);
}

int getTupleSize(Node* tuple) {
    int i = 0;
    for (Node* n = getBody(tuple); isApplication(n); i++)
        n = getLeft(n);
    return i;
}

Node* newProjection(Tag tag, int size, int index) {
    Node* projection = newReference(tag,
        (unsigned long long)(size - index));
    for (int i = 0; i < size; i++)
        projection = newLambda(tag, newBlank(tag), projection);
    return projection;
}

Node* newPatternLambda(Node* operator, Node* left, Node* right) {
    Tag tag = getTag(operator);
    if (isSymbol(left))
        return newLambda(tag, left, right);
    if (!isTuple(left) || getTupleSize(left) == 0)
        syntaxError("invalid parameter", left);
    // (x, y) -> body ---> p -> (x -> y -> body) left(p) right(p)
    Node* body = right;
    for (Node* items = getBody(left); isApplication(items);
            items = getLeft(items))
        body = newPatternLambda(operator, getRight(items), body);
    for (int i = 0, size = getTupleSize(left); i < size; i++)
        body = newApplication(tag, body,
            newApplication(tag, newBlankReference(tag, 1),
                newProjection(tag, size, i)));
    return newLambda(tag, newBlank(tag), body);
}

Node* prefix(Node* operator, Node* left, Node* right) {
    (void)left;     // suppress unused parameter warning
    return apply(operator, operator, right);
}

Node* negate(Node* operator, Node* left, Node* right) {
    (void)left;     // suppress unused parameter warning
    return infix(operator, newInteger(getTag(operator), 0), right);
}

Node* unmatched(Node* operator, Node* left, Node* right) {
    syntaxError("missing close for", operator);
    return left == NULL ? right : left; // suppress unused parameter warning
}

Node* brackets(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "["), "missing open for", close);
    Tag tag = getTag(open);
    if (getFixity(open) == OPENCALL) {
        syntaxErrorIf(!isCommaList(contents), "missing argument to", open);
        syntaxErrorIf(!isComma(getLeft(getLeft(contents))),
            "too many arguments to", open);
        return newApplication(tag, newApplication(tag, open,
            getRight(getLeft(contents))), getRight(contents));
    }
    Node* list = newNil(getTag(open));
    if (contents == NULL)
        return list;
    if (!isCommaList(contents))
        return prepend(tag, contents, list);
    for(; isCommaList(getLeft(contents)); contents = getLeft(contents))
        list = prepend(tag, getRight(contents), list);
    return prepend(tag, getRight(contents), list);
}

Node* applyToCommaList(Tag tag, Node* base, Node* arguments) {
    if (!isCommaList(getLeft(arguments)))
        return base == NULL ? getRight(arguments) :
            newApplication(tag, base, getRight(arguments));
    return newApplication(tag, applyToCommaList(tag, base,
        getLeft(arguments)), getRight(arguments));
}

Node* parentheses(Node* close, Node* open, Node* contents) {
    syntaxErrorIf(!isThisToken(open, "("), "missing open for", close);
    Tag tag = getTag(open);
    if (getFixity(open) == OPENCALL)
        return isCommaList(contents) ? applyToCommaList(tag, NULL, contents) :
            newApplication(tag, contents, newUnit(tag)); // desugar f() to f(())

    if (contents == NULL)
        return newUnit(tag);
    if (isOperator(contents)) {
        // update rules to favor infix over prefix inside parenthesis
        setRules(contents, false);
        if (isSpecialOperator(contents) && !isComma(contents))
            syntaxError("operator cannot be parenthesized", contents);
        return newName(getTag(contents));
    }
    if (isCommaList(contents))
        return newLambda(tag, newTupleName(tag, 1),
            applyToCommaList(tag, newTupleName(tag, 1), contents));
    if (isApplication(contents))
        return setLocation(contents, getLocation(open));
    return contents;
}

// comma must be the lowest precedence operator above parentheses/brackets
// or else a commaList, which is an invalid operand, could get buried in the
// AST without being detected, then a surrounding parentheses could apply
// a tuple abstraction, which would bind across a bracket boundary.
// if a comma is not wrapped in parentheses or brackets, it will be at the
// very top level and thus won't be defined, so bind will catch this case.
Rules RULES[] = {
    // syntactic operators
    {"\0", 0, 0, CLOSE, L, NULL},
    {"(", 22, 0, OPENCALL, L, unmatched},
    {"(", 22, 0, OPEN, L, unmatched},
    {")", 0, 22, CLOSE, R, parentheses},
    {"[", 22, 0, OPENCALL, L, unmatched},
    {"[", 22, 0, OPEN, L, unmatched},
    {"]", 0, 22, CLOSE, R, brackets},
    {",", 1, 1, IN, L, comma},
    //{";", 1, 1, IN, N, semicolon},
    {"\n", 2, 2, IN, R, apply},
    {":=", 3, 3, IN, N, definition},
    {"|", 4, 4, IN, L, infix},
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

Rules DEFAULT = {"", 14, 14, IN, L, infix};
Rules SPACE = {" ", 20, 20, IN, L, apply};

bool allowsOperatorBefore(Rules rules) {
    return rules.fixity == PRE || rules.fixity == OPEN || rules.fixity == CLOSE;
}

bool isSpace(Node* token) {
    return isLeaf(token) && isSpaceCharacter(getLexeme(token).start[0]);
}

Rules* lookupRules(Node* token, bool isAfterOperator) {
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
