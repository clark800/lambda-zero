// operator precedence parser

#include <stdio.h>
#include <string.h>
#include "array.h"
#include "tree.h"
#include "util.h"
#include "stack.h"
#include "errors.h"
#include "opp.h"

void reduceLeft(Stack* stack, Node* operator);

static Array* RULES = NULL;     // note: this never gets free'd

typedef struct {
    String lexeme, alias, prior;
    char bracketType;
    Precedence leftPrecedence, rightPrecedence;
    Fixity fixity;
    Associativity associativity;
    bool special;
    Node* (*reduce)(Node* operator, Node* left, Node* right);
} Rules;

NodeStack* newNodeStack() { return (NodeStack*)newStack(); }
void deleteNodeStack(NodeStack* stack) { deleteStack((Stack*)stack); }

Node* getTop(NodeStack* stack) {
    return isEmpty((Stack*)stack) ? NULL : peek((Stack*)stack, 0);
}

static inline Node* Operator(Tag tag, long long subprecedence, void* rules) {
    return newLeaf(tag, 0, subprecedence, rules);
}

bool isOperator(Node* node) {
    return isLeaf(node) && getType(node) == 0;
}

bool isThisOperator(Node* node, const char* lexeme) {
    return isOperator(node) && isThisString(getLexeme(node), lexeme);
}

static Rules* getRules(Node* node) {
    assert(isOperator(node));
    return (Rules*)getData(node);
}

Fixity getFixity(Node* op) {return getRules(op)->fixity;}
Associativity getAssociativity(Node* op) {return getRules(op)->associativity;}
String getPrior(Node* op) {return getRules(op)->prior;}

bool isLeftSectionOperator(Node* op) {
    if (!isOperator(op)) return false;
    Rules* rules = getRules(op);
    Fixity fixity = rules->fixity;
    return !rules->special && (fixity == INFIX || fixity == PREFIX);
}

bool isRightSectionOperator(Node* op) {
    if (!isOperator(op)) return false;
    Rules* rules = getRules(op);
    Fixity fixity = rules->fixity;
    return !rules->special && (fixity == INFIX || fixity == POSTFIX);
}

void eraseNode(Stack* stack, const char* lexeme) {
    if (!isEmpty(stack) && isThisOperator(peek(stack, 0), lexeme))
        release(pop(stack));
}

void erase(NodeStack* stack, const char* lexeme) {
    eraseNode((Stack*)stack, lexeme);
}

bool isOpenOperator(Node* node) {
    return isOperator(node) && getFixity(node) == OPENFIX;
}

bool isCloseOperator(Node* node) {
    return isOperator(node) && getFixity(node) == CLOSEFIX;
}

Node* reduce(Node* operator, Node* left, Node* right) {
    return getRules(operator)->reduce(operator, left, right);
}

Node* reduceBracket(Node* open, Node* close, Node* before, Node* contents) {
    return reduce(close, before, reduce(open, before, contents));
}

void shiftPrefix(Stack* stack, Node* operator) {
    reduceLeft(stack, operator);
    push(stack, operator);
}

void shiftPostfix(Stack* stack, Node* operator) {
    eraseNode(stack, " "); // if we erase newlines it would break associativity
    reduceLeft(stack, operator);
    if (isOperator(peek(stack, 0)))
        syntaxError("missing left operand for", operator);
    Hold* operand = pop(stack);
    push(stack, reduce(operator, getNode(operand), VOID));
    release(operand);
}

void shiftInfix(Stack* stack, Node* operator) {
    eraseNode(stack, " "); // if we erase newlines it would break associativity
    reduceLeft(stack, operator);
    if (isOperator(peek(stack, 0)))
        syntaxError("missing left operand for", operator);
    push(stack, operator);
}

void shiftSpace(Stack* stack, Node* operator) {
    reduceLeft(stack, operator);
    // ignore space after operators (note: close and postfix are never
    // pushed onto the stack, so the operator must be expecting a right operand)
    if (!isOperator(peek(stack, 0)))
        push(stack, operator);
}

void shiftOpen(Stack* stack, Node* open) {
    reduceLeft(stack, open);
    push(stack, open);
}

void pushBracket(Stack* stack, Node* open, Node* close, Node* contents) {
    if (contents != NULL && isOperator(contents))
        syntaxError("missing right operand for", contents);

    if (getRules(open)->bracketType != getRules(close)->bracketType) {
        if (getRules(close)->bracketType == '\0')
            syntaxError("missing close for", open);
        else syntaxError("missing open for", close);
    }

    if (isEmpty(stack) || isOperator(peek(stack, 0))) {
        push(stack, reduceBracket(open, close, NULL, contents));
    } else {
        Hold* left = pop(stack);
        push(stack, reduceBracket(open, close, getNode(left), contents));
        release(left);
    }
}

void shiftClose(Stack* stack, Node* close) {
    eraseNode(stack, " ");
    reduceLeft(stack, close);

    Hold* contents = pop(stack);
    if (isOpenOperator(getNode(contents))) {
        pushBracket(stack, getNode(contents), close, NULL);
    } else {
        Hold* open = pop(stack);
        pushBracket(stack, getNode(open), close, getNode(contents));
        release(open);
    }
    release(contents);
}

static void shiftNode(Stack* stack, Node* node) {
    if (isOperator(node)) {
        switch(getRules(node)->fixity) {
            case INFIX: shiftInfix(stack, node); break;
            case PREFIX: shiftPrefix(stack, node); break;
            case POSTFIX: shiftPostfix(stack, node); break;
            case OPENFIX: shiftOpen(stack, node); break;
            case CLOSEFIX: shiftClose(stack, node); break;
            case SPACEFIX: shiftSpace(stack, node); break;
        }
    } else {
        if (!isEmpty(stack) && !isOperator(peek(stack, 0)))
            syntaxError("missing operator before", node);
        push(stack, node);
    }
}

void shift(NodeStack* stack, Node* node) { shiftNode((Stack*)stack, node); }

bool isHigherPrecedence(Node* left, Node* right) {
    assert(isOperator(left) && isOperator(right));

    Rules* leftRules = getRules(left);
    Rules* rightRules = getRules(right);

    if (leftRules->rightPrecedence == rightRules->leftPrecedence) {
        if (leftRules->associativity == N)
            syntaxError("operator is non-associative", left);
        if (rightRules->associativity == N)
            syntaxError("operator is non-associative", right);
        if (leftRules->associativity != rightRules->associativity)
            syntaxError("incompatible associativity", right);

        if (leftRules->associativity == R)
            return getValue(left) > getValue(right);
        return getValue(left) >= getValue(right);
    }

    return leftRules->rightPrecedence > rightRules->leftPrecedence;
}

static Rules* findRules(String lexeme) {
    for (unsigned int i = 0; i < length(RULES); ++i) {
        Rules* rules = elementAt(RULES, i);
        if (isSameString(lexeme, rules->lexeme))
            return rules;
    }
    return NULL;
}

Precedence findPrecedence(Node* node) {
    Rules* rules = findRules(getLexeme(node));
    syntaxErrorIf(rules == NULL, "syntax not defined", node);
    if (rules->leftPrecedence != rules->rightPrecedence)
        syntaxError("operator not supported", node);
    return rules->leftPrecedence;
}

Node* parseOperator(Tag tag, long long subprecedence) {
    Rules* rules = findRules(tag.lexeme);
    if (rules == NULL && isThisString(tag.lexeme, " "))
        return Operator(tag, subprecedence, findRules(newString("( )", 3)));
    return rules == NULL ? NULL :
        Operator(newTag(rules->alias, tag.location), subprecedence, rules);
}

static void reduceTop(Stack* stack) {
    Hold* right = pop(stack);
    Hold* operator = pop(stack);
    Fixity fixity = getFixity(getNode(operator));
    Hold* left = fixity == INFIX || fixity == SPACEFIX ?
        pop(stack) : hold(VOID);
    shiftNode(stack, reduce(getNode(operator), getNode(left), getNode(right)));
    release(right);
    release(operator);
    release(left);
}

void reduceLeft(Stack* stack, Node* operator) {
    while (!isEmpty(stack) && !isOperator(peek(stack, 0)) &&
            isHigherPrecedence(peek(stack, 1), operator))
        reduceTop(stack);
}

static void appendSyntaxCopy(Rules* rules, String lexeme, String alias) {
    Rules* newRules = (Rules*)smalloc(sizeof(Rules));
    *newRules = *rules;
    newRules->lexeme = lexeme;
    newRules->alias = alias;
    append(RULES, newRules);
}

static void appendSyntax(Rules rules) {
    Rules* newRules = (Rules*)smalloc(sizeof(Rules));
    *newRules = rules;
    append(RULES, newRules);
}

void addSyntax(Tag tag, String prior, Precedence leftPrecedence,
        Precedence rightPrecedence, Fixity fixity, Associativity associativity,
        Node* (*reducer)(Node*, Node*, Node*)) {
    bool special = prior.length != 0;
    if (special && associativity == N)
        tokenErrorIf(true, "expected numeric precedence", tag);
    tokenErrorIf(findRules(tag.lexeme) != NULL, "syntax already defined", tag);
    appendSyntax((Rules){tag.lexeme, tag.lexeme, prior, '_', leftPrecedence,
        rightPrecedence, fixity, associativity, special, reducer});
}

void popSyntax(void) { unappend(RULES); }

void initSyntax(void) { RULES = newArray(1024); }

void addCoreSyntax(const char* symbol, Precedence leftPrecedence,
        Precedence rightPrecedence, Fixity fixity, Associativity associativity,
        Node* (*reducer)(Node*, Node*, Node*)) {
    String lexeme = newString(symbol, (unsigned int)strlen(symbol));
    appendSyntax((Rules){lexeme, lexeme, newString("", 0), '_', leftPrecedence,
        rightPrecedence, fixity, associativity, true, reducer});
}

void addBracketSyntax(const char* symbol, char type, Precedence outerPrecedence,
    Fixity fixity, Node* (*reducer)(Node*, Node*, Node*)) {
    size_t length = symbol[0] == 0 && fixity == CLOSEFIX ? 1 : strlen(symbol);
    String lexeme = newString(symbol, (unsigned int)length);
    Precedence leftPrecedence = fixity == OPENFIX ? outerPrecedence : 0;
    Precedence rightPrecedence = fixity == OPENFIX ? 0 : outerPrecedence;
    appendSyntax((Rules){lexeme, lexeme, newString("", 0), type,
        leftPrecedence, rightPrecedence, fixity, R, true, reducer});
}

void addCoreAlias(const char* alias, const char* name) {
    Rules* rules = findRules(toString(name));
    appendSyntaxCopy(rules, toString(alias), rules->alias);
}

void addSyntaxCopy(String lexeme, Node* name, bool alias) {
    Rules* rules = findRules(getLexeme(name));
    syntaxErrorIf(rules == NULL, "syntax not defined", name);
    appendSyntaxCopy(rules, lexeme, alias ? rules->alias : lexeme);
}

void debugNodeStack(NodeStack* nodeStack, void debugNode(Node*)) {
    Stack* stack = (Stack*)nodeStack;
    Stack* reversed = newStack();
    for (Iterator* it = iterate(stack); !end(it); it = next(it))
        push(reversed, cursor(it));
    for (Iterator* it = iterate(reversed); !end(it); it = next(it)) {
        debugNode(cursor(it));
        fputs(" | ", stderr);
    }
    deleteStack(reversed);
}
