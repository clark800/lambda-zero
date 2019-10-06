#include <stdio.h>
#include <string.h>
#include "shared/lib/array.h"
#include "shared/lib/tree.h"
#include "shared/lib/util.h"
#include "shared/lib/stack.h"
#include "parse/shared/errors.h"
#include "parse/shared/ast.h"
#include "symbols.h"

void reduceLeft(Stack* stack, Node* operator);

static Array* RULES = NULL;     // note: this never gets free'd

typedef struct {
    String lexeme, alias, prior;
    Precedence leftPrecedence, rightPrecedence;
    Fixity fixity;
    Associativity associativity;
    bool special;
    Node* (*reduce)(Node* operator, Node* left, Node* right);
} Rules;

NodeStack* newNodeStack() { return (NodeStack*)newStack(); }
void deleteNodeStack(NodeStack* stack) { deleteStack((Stack*)stack); }

Node* getTop(NodeStack* stack) {
    assert(!isEmpty((Stack*)stack));
    return peek((Stack*)stack, 0);
}

static Rules* getRules(Node* node) {
    assert(isOperator(node));
    return (Rules*)getData(node);
}

Fixity getFixity(Node* op) {return getRules(op)->fixity;}
Associativity getAssociativity(Node* op) {return getRules(op)->associativity;}
String getPrior(Node* op) {return getRules(op)->prior;}

void eraseNode(Stack* stack, const char* lexeme) {
    if (!isEmpty(stack) && isThisOperator(peek(stack, 0), lexeme))
        release(pop(stack));
}

void erase(NodeStack* stack, const char* lexeme) {
    eraseNode((Stack*)stack, lexeme);
}

bool isSpecial(Node* node) {
    return isOperator(node) && getRules(node)->special;
}

bool isOpenOperator(Node* node) {
    return isOperator(node) && getFixity(node) == OPENFIX;
}

Node* reduceBracket(Node* open, Node* close, Node* before, Node* contents) {
    return getRules(close)->reduce(close, open,
        getRules(open)->reduce(open, before, contents));
}

static Node* propagateSection(Node* operator, SectionVariety side, Node* body) {
    if (isSpecial(operator) || !isJuxtaposition(body))
        syntaxError("operator does not support sections", operator);
    return Section(getTag(operator), side, body);
}

Node* reduce(Node* operator, Node* left, Node* right) {
    Node* leftOp = isSection(left) ? getSectionBody(left) : left;
    Node* rightOp = isSection(right) ? getSectionBody(right) : right;
    Node* result = getRules(operator)->reduce(operator, leftOp, rightOp);
    if (isSection(left))
        return propagateSection(operator, isSection(right) ?
            LEFTRIGHTSECTION : RIGHTSECTION, result);
    if (isSection(right))
        return propagateSection(operator, LEFTSECTION, result);
    return result;
}

void shiftPrefix(Stack* stack, Node* operator) {
    reduceLeft(stack, operator);
    push(stack, operator);
}

void shiftPostfix(Stack* stack, Node* operator) {
    eraseNode(stack, " "); // if we erase newlines it would break associativity
    reduceLeft(stack, operator);
    if (!isSpecial(operator) && isOpenOperator(peek(stack, 0)))
        push(stack, LeftPlaceholder(getTag(operator)));
    if (isOperator(peek(stack, 0)))
        syntaxError("missing left operand for", operator);
    Hold* operand = pop(stack);
    push(stack, reduce(operator, getNode(operand), VOID));
    release(operand);
}

void shiftInfix(Stack* stack, Node* operator) {
    eraseNode(stack, " "); // if we erase newlines it would break associativity
    reduceLeft(stack, operator);
    if (isOperator(peek(stack, 0))) {
        if (isThisOperator(operator, "+"))
            operator = parseSymbol(renameTag(getTag(operator), "(+)"), 0);
        else if (isThisOperator(operator, "-"))
            operator = parseSymbol(renameTag(getTag(operator), "(-)"), 0);
        else if (!isSpecial(operator) && isOpenOperator(peek(stack, 0)))
            push(stack, LeftPlaceholder(getTag(operator)));
        else syntaxError("missing left operand for", operator);
    }
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
    eraseNode(stack, "\n");
    eraseNode(stack, ";");

    Node* top = peek(stack, 0);
    if (isOperator(top) && !isSpecial(top) && !isEOF(close)) {
        if (isLeftPlaceholder(peek(stack, 1))) {
            // bracketed infix operator
            Hold* op = pop(stack);
            release(pop(stack));
            push(stack, Name(getTag(getNode(op))));
            release(op);
        } else if (isOpenOperator(peek(stack, 1))) {
            // bracketed prefix operator
            Hold* op = pop(stack);
            Tag tag = getTag(getNode(op));
            if (isThisOperator(getNode(op), "(+)"))
                push(stack, FixedName(tag, "+"));
            else if (isThisOperator(getNode(op), "(-)"))
                push(stack, FixedName(tag, "-"));
            else push(stack, Name(tag));
            release(op);
        } else if (getFixity(top) == INFIX || getFixity(top) == PREFIX)
            push(stack, RightPlaceholder(getTag(top)));
    }

    reduceLeft(stack, close);
    if (isEOF(peek(stack, 0)) && !isEOF(close))
        syntaxError("missing open for", close);
    Hold* contents = pop(stack);
    if (isOpenOperator(getNode(contents))) {
        pushBracket(stack, getNode(contents), close, NULL);
    } else {
        Hold* open = pop(stack);
        if (isEOF(getNode(open)) && !isEOF(close))
            syntaxError("missing open for", close);
        if (isOperator(getNode(contents)))
            syntaxError("missing right operand for", getNode(contents));
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
        static const char* message = "operator is non-associative";
        syntaxErrorIf(leftRules->associativity == N, message, left);
        syntaxErrorIf(rightRules->associativity == N, message, right);

        if (leftRules->associativity != rightRules->associativity)
            syntaxError("incompatible associativity", right);

        if (leftRules->associativity == RV)
            return getValue(left) > getValue(right);

        return leftRules->associativity == L;
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

Node* parseSymbol(Tag tag, long long value) {
    Rules* rules = findRules(tag.lexeme);
    if (rules == NULL && isThisString(tag.lexeme, " "))
        return Operator(tag, value, findRules(newString("( )", 3)));
    return rules == NULL ? Name(tag) :
        Operator(newTag(rules->alias, tag.location), value, rules);
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
    appendSyntax((Rules){tag.lexeme, tag.lexeme, prior, leftPrecedence,
        rightPrecedence, fixity, associativity, special, reducer});
}

void popSyntax(void) { unappend(RULES); }

void addCoreSyntax(const char* symbol, Precedence leftPrecedence,
        Precedence rightPrecedence, Fixity fixity, Associativity associativity,
        Node* (*reducer)(Node*, Node*, Node*)) {
    if (RULES == NULL)
        RULES = newArray(1024);
    bool special = strncmp(symbol, "(+)", 4) && strncmp(symbol, "(-)", 4);
    size_t length = symbol[0] == 0 && fixity == CLOSEFIX ? 1 : strlen(symbol);
    String lexeme = newString(symbol, (unsigned int)length);
    String empty = newString("", 0);
    appendSyntax((Rules){lexeme, lexeme, empty, leftPrecedence,
        rightPrecedence, fixity, associativity, special, reducer});
}

void addCoreAlias(const char* alias, const char* name) {
    Rules* rules = findRules(toString(name));
    appendSyntaxCopy(rules, toString(alias), rules->alias);
}

void addSyntaxCopy(String lexeme, Node* name, bool alias) {
    syntaxErrorIf(!isName(name), "expected operator name", name);
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
