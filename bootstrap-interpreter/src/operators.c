#include "lib/tree.h"
#include "ast.h"
#include "scan.h"
#include "errors.h"
#include "define.h"
#include "patterns.h"
#include "brackets.h"
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

static Node* reduceApply(Node* operator, Node* left, Node* right) {
    return newApplication(getTag(operator), left, right);
}

static Node* reduceInfix(Node* operator, Node* left, Node* right) {
    return reduceApply(operator, reduceApply(operator,
        convertOperator(operator), left), right);
}

Node* reduceAsterisk(Node* operator, Node* left, Node* right) {
    (void)left;
    syntaxErrorIf(!isValidPattern(right), "invalid operand of", operator);
    // rename operator so it will generate an error for being undefined if
    // a prefix asterisk appears outside an ADT definition
    return newName(renameTag(getTag(operator), "(*)"));
}

static Node* reducePipe(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(isDefinition(left), "missing scope", left);
    syntaxErrorIf(isDefinition(right), "missing scope", right);
    return newPipePair(getTag(operator), left, right);
}

static Node* reduceComma(Node* operator, Node* left, Node* right) {
    syntaxErrorIf(isDefinition(left), "missing scope", left);
    syntaxErrorIf(isDefinition(right), "missing scope", right);
    return newCommaList(getTag(operator), left, right);
}

static Node* reducePrefix(Node* operator, Node* left, Node* right) {
    (void)left;     // suppress unused parameter warning
    return reduceApply(operator, convertOperator(operator), right);
}

static Node* reduceNegate(Node* operator, Node* left, Node* right) {
    (void)left;     // suppress unused parameter warning
    return reduceInfix(operator, newInteger(getTag(operator), 0), right);
}

static Node* reduceEOF(Node* operator, Node* open, Node* contents) {
    (void)operator;
    syntaxErrorIf(!isEOF(open), "missing close for", open);
    syntaxErrorIf(isEOF(contents), "no input", open);
    syntaxErrorIf(isCommaList(contents), "comma not inside brackets", contents);
    syntaxErrorIf(isPipePair(contents), "pipe not inside brackets", contents);
    return isDefinition(contents) ? transformDefinition(contents) : contents;
}

static Node* reduceUnmatched(Node* operator, Node* left, Node* right) {
    syntaxError("missing close for", operator);
    return left == NULL ? right : left; // suppress unused parameter warning
}

// note: must check for pipe pairs, comma lists, and definitions in every
// reducer for operators of lower precedence to ensure that parser-specific
// node types don't reach the evaluator.
static Rules RULES[] = {
    // syntactic operators
    {"\0", 0, 0, CLOSE, L, reduceEOF},
    {"(", 22, 0, OPENCALL, L, reduceUnmatched},
    {"(", 22, 0, OPEN, L, reduceUnmatched},
    {")", 0, 22, CLOSE, R, reduceParentheses},
    {"[", 22, 0, OPENCALL, L, reduceUnmatched},
    {"[", 22, 0, OPEN, L, reduceUnmatched},
    {"]", 0, 22, CLOSE, R, reduceSquareBrackets},
    {"{", 22, 0, OPEN, L, reduceUnmatched},
    {"}", 0, 22, CLOSE, R, reduceCurlyBrackets},
    {"|", 1, 1, IN, N, reducePipe},
    {",", 2, 2, IN, L, reduceComma},
    {"\n", 3, 3, IN, R, reduceNewline},
    {":=", 4, 4, IN, N, reduceDefine},
    {"::=", 4, 4, IN, N, reduceADTDefinition},
    {";", 5, 5, IN, L, newPatternLambda},
    {"->", 6, 6, IN, R, newDestructuringLambda},

    // conditional operators
    {"||", 6, 6, IN, R, reduceInfix},
    {"?", 7, 7, IN, R, reduceInfix},
    {"?||", 7, 7, IN, R, reduceInfix},

    // monadic operators
    {">>=", 8, 8, IN, L, reduceInfix},

    // logical operators
    {"<=>", 9, 9, IN, N, reduceInfix},
    {"=>", 10, 10, IN, N, reduceInfix},
    {"\\/", 11, 11, IN, R, reduceInfix},
    {"/\\", 12, 12, IN, R, reduceInfix},

    // comparison/test operators
    {"=", 13, 13, IN, N, reduceInfix},
    {"!=", 13, 13, IN, N, reduceInfix},
    {"=*=", 13, 13, IN, N, reduceInfix},
    {"=?=", 13, 13, IN, N, reduceInfix},
    {"<", 13, 13, IN, N, reduceInfix},
    {">", 13, 13, IN, N, reduceInfix},
    {"<=", 13, 13, IN, N, reduceInfix},
    {">=", 13, 13, IN, N, reduceInfix},
    {"=<", 13, 13, IN, N, reduceInfix},
    {"~<", 13, 13, IN, N, reduceInfix},
    {"<:", 13, 13, IN, N, reduceInfix},
    {"<*=", 13, 13, IN, N, reduceInfix},
    {":", 13, 13, IN, N, reduceInfix},
    {"!:", 13, 13, IN, N, reduceInfix},
    {"~", 13, 13, IN, N, reduceInfix},

    // precedence 14: default
    // arithmetic/list operators
    {"..", 15, 15, IN, N, reduceInfix},
    {"::", 16, 16, IN, R, reduceInfix},
    {"&", 16, 16, IN, L, reduceInfix},
    {"+", 16, 16, IN, L, reduceInfix},
    {"\\./", 16, 16, IN, L, reduceInfix},
    {"++", 16, 16, IN, R, reduceInfix},
    {"-", 16, 16, IN, L, reduceInfix},
    {"\\", 16, 16, IN, L, reduceInfix},
    {"*", 17, 17, IN, L, reduceInfix},
    {"**", 17, 17, IN, R, reduceInfix},
    {"/", 17, 17, IN, L, reduceInfix},
    {"%", 17, 17, IN, L, reduceInfix},
    {"^", 18, 18, IN, R, reduceInfix},

    // functional operators
    {"<>", 19, 19, IN, R, reduceInfix},

    // precedence 20: space operator
    // prefix operators
    {"-", 21, 21, PRE, L, reduceNegate},
    {"--", 21, 21, PRE, L, reducePrefix},
    {"!", 21, 21, PRE, L, reducePrefix},
    {"#", 21, 21, PRE, L, reducePrefix},
    {"*", 21, 21, PRE, L, reduceAsterisk},

    // precedence 22: parentheses/brackets
    {"^^", 23, 23, IN, L, reduceInfix},
    {".", 24, 24, IN, L, reduceInfix},
    {"`", 25, 25, PRE, L, reducePrefix}
};

static Rules DEFAULT = {"", 14, 14, IN, L, reduceInfix};
static Rules SPACE = {" ", 20, 20, IN, L, reduceApply};

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
    return rules->apply != reduceInfix && rules->apply != reducePrefix;
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
