#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "errors.h"
#include "symbols.h"
#include "patterns.h"
#include "define.h"
#include "brackets.h"

static Node* reduceApply(Node* operator, Node* left, Node* right) {
    return newApplication(getTag(operator), left, right);
}

static Node* reduceInfix(Node* operator, Node* left, Node* right) {
    return reduceApply(operator, reduceApply(operator,
        convertOperator(getTag(operator)), left), right);
}

static Node* reducePrefix(Node* operator, Node* left, Node* right) {
    (void)left;
    return reduceApply(operator, convertOperator(getTag(operator)), right);
}

static Node* reducePostfix(Node* operator, Node* left, Node* right) {
    (void)right;
    return reduceApply(operator, convertOperator(getTag(operator)), left);
}

static Node* reduceArrow(Node* operator, Node* left, Node* right) {
    if (isApplication(left) && isThisLexeme(left, "case"))
        return newCase(getTag(left), getRight(left), right);
    return newPatternLambda(getTag(operator), left, right);
}

static Node* reduceReserved(Node* operator, Node* left, Node* right) {
    (void)left, (void)right;
    syntaxError("reserved operator", operator);
    return NULL;
}

static void shiftSpace(Stack* stack, Node* operator) {
    reduceLeft(stack, operator);
    // ignore space after operators (note: close and postfix are never
    // pushed onto the stack, so the operator must be expecting a right operand)
    if (!isOperator(peek(stack, 0)))
        push(stack, operator);
}

static void shiftPrefix(Stack* stack, Node* operator) {
    reduceLeft(stack, operator);
    push(stack, operator);
}

static void shiftInfix(Stack* stack, Node* operator) {
    erase(stack, " ");      // if we erase newlines it would break associativity
    reduceLeft(stack, operator);
    if (isOperator(peek(stack, 0))) {
        if (isThisLeaf(operator, "+"))
            operator = parseSymbol(renameTag(getTag(operator), "(+)"), 0);
        else if (isThisLeaf(operator, "-"))
            operator = parseSymbol(renameTag(getTag(operator), "(-)"), 0);
        else if (!isSpecial(operator) && isOpenOperator(peek(stack, 0)))
            push(stack, newRename(getTag(operator), ".*"));
        else syntaxError("missing left operand for", operator);
    }
    push(stack, operator);
}

static void shiftPostfix(Stack* stack, Node* operator) {
    erase(stack, " ");      // if we erase newlines it would break associativity
    reduceLeft(stack, operator);
    if (!isSpecial(operator) && isOpenOperator(peek(stack, 0)))
        push(stack, newRename(getTag(operator), ".*"));
    if (isOperator(peek(stack, 0)))
        syntaxError("missing left operand for", operator);
    Hold* operand = pop(stack);
    push(stack, reduce(operator, getNode(operand), NULL));
    release(operand);
}

static Node* reduceError(Node* operator, Node* left, Node* right) {
    (void)left;
    Tag tag = getTag(operator);
    if (isThisLeaf(right, "[]"))
        return newBuiltin(tag, UNDEFINED);
    Node* exit = newBuiltin(renameTag(tag, "exit"), EXIT);
    return newApplication(tag, exit, newApplication(tag, newPrinter(tag),
        newApplication(tag, newBuiltin(tag, ERROR), right)));
}

static Node* reduceInvalid(Node* operator, Node* left, Node* right) {
    (void)left, (void)right;
    syntaxError("operator syntax undeclared", operator);
    return NULL;
}

static Node* defineSyntax(Node* definition, Node* left, Node* right) {
    syntaxErrorIf(!isApplication(left), "invalid left operand", left);
    Node* name = getRight(left);
    syntaxErrorIf(!isLeaf(name), "expected symbol operand to", getLeft(left));
    if (contains(getLexeme(name), '_'))
       syntaxError("invalid underscore in operator name", name);
    if (!isApplication(right))
        syntaxError("invalid syntax definition", definition);

    Tag tag = getTag(name);
    Node* fixity = getLeft(right);
    if (isThisLeaf(fixity, "mixfix")) {
        addMixfixSyntax(tag, getRight(right), shiftInfix);
        return newLambda(getTag(left), newName(tag), newName(tag));
    }

    if (!isNatural(getRight(right)))
        syntaxError("invalid syntax definition", definition);
    long long precedence = getValue(getRight(right));
    if (precedence < 0 || precedence > 99)
        syntaxError("invalid precedence", getRight(right));
    Precedence p = (Precedence)precedence;

    if (isThisLeaf(name, "()")) {
        tag = renameTag(tag, " ");
        if (!isThisLeaf(fixity, "infixL"))
            syntaxError("syntax must be infixL", fixity);
        addSyntax(tag, p, p, INFIX, L, shiftSpace, reduceInfix);
    }
    else if (isThisLeaf(fixity, "infix"))
        addSyntax(tag, p, p, INFIX, N, shiftInfix, reduceInfix);
    else if (isThisLeaf(fixity, "infixL"))
        addSyntax(tag, p, p, INFIX, L, shiftInfix, reduceInfix);
    else if (isThisLeaf(fixity, "infixR"))
        addSyntax(tag, p, p, INFIX, R, shiftInfix, reduceInfix);
    else if (isThisLeaf(fixity, "prefix"))
        addSyntax(tag, p, p, PREFIX, L, shiftPrefix, reducePrefix);
    else if (isThisLeaf(fixity, "postfix"))
        addSyntax(tag, p, p, POSTFIX, L, shiftPostfix, reducePostfix);
    else syntaxError("invalid fixity", fixity);

    // add special case prefix operators for "+" and "-"
    // done here to avoid hard-coding a precedence for them
    if (isThisLeaf(name, "+"))
        addBuiltinSyntax("(+)", p, p, PREFIX, L, shiftPrefix, reducePrefix);
    if (isThisLeaf(name, "-"))
        addBuiltinSyntax("(-)", p, p, PREFIX, L, shiftPrefix, reducePrefix);

    // reduce to an identity function with the operator symbol as parameter name
    // so that if the operator symbol is already defined it will be a syntax
    // error. this ensures that operator symbols can't be used before their
    // syntax is declared because use must follow definition and definition
    // must follow syntax declaration. builtin operators don't need to be
    // defined, but they can only be accessed as operators, which again means
    // that use must follow syntax declaration.
    return newLambda(getTag(left), newName(tag), newName(tag));
}

static Node* reduceNewline(Node* operator, Node* left, Node* right) {
    Tag tag = getTag(left);
    if (isDefinition(left) && isThisLexeme(left, ":="))
        return applyDefinition(tag, getLeft(left), getRight(left), right);
    if (isDefinition(left) && isThisLexeme(left, "::="))
        return applyADTDefinition(tag, getLeft(left), getRight(left), right);
    if (isDefinition(left) && isThisLexeme(left, "try"))
        return applyTryDefinition(tag, getLeft(left), getRight(left), right);
    if (isApplication(left) && isThisLexeme(left, "define"))
        return reduceDefine(getLeft(left), getRight(left), right);
    if (isCase(left) && isCase(right))
        return combineCases(tag, left, right);
    if (isApplication(left) && isThisLexeme(left, "case"))
        return reduceArrow(operator, left, right);
    return reduceApply(operator, left, right);
}

static void shiftNewline(Stack* stack, Node* operator) {
    erase(stack, " ");
    reduceLeft(stack, operator);
    Node* top = peek(stack, 0);
    if (isDefinition(top) && isThisLexeme(getLeft(top), "syntax")) {
        Hold* definition = pop(stack);
        push(stack, defineSyntax(top, getLeft(top), getRight(top)));
        release(definition);
    }
    // ignore newlines after operators for line continuations
    if (!isOperator(peek(stack, 0)))
        push(stack, operator);
}

void initSymbols(void) {
    addBuiltinSyntax("\0", 0, 0, CLOSEFIX, R, shiftClose, reduceEOF);
    addBuiltinSyntax("(", 90, 0, OPENFIX, R, shiftOpen, reduceUnmatched);
    addBuiltinSyntax(")", 0, 90, CLOSEFIX, R, shiftClose, reduceParentheses);
    addBuiltinSyntax("[", 90, 0, OPENFIX, R, shiftOpen, reduceUnmatched);
    addBuiltinSyntax("]", 0, 90, CLOSEFIX, R, shiftClose, reduceSquareBrackets);
    addBuiltinSyntax("{", 90, 0, OPENFIX, R, shiftOpenCurly, reduceUnmatched);
    addBuiltinSyntax("}", 0, 90, CLOSEFIX, R, shiftClose, reduceCurlyBrackets);
    addBuiltinSyntax("|", 1, 1, INFIX, N, shiftInfix, reduceReserved);
    addBuiltinSyntax(",", 2, 2, INFIX, L, shiftInfix, reduceApply);
    addBuiltinSyntax("\n", 3, 3, INFIX, RV, shiftNewline, reduceNewline);
    addBuiltinSyntax(";", 4, 4, INFIX, R, shiftInfix, reduceNewline);
    addBuiltinSyntax("define", 5, 5, PREFIX, N, shiftPrefix, reducePrefix);
    addBuiltinSyntax(":=", 5, 5, INFIX, N, shiftInfix, reduceDefine);
    addBuiltinSyntax("\u2254", 5, 5, INFIX, N, shiftInfix, reduceDefine);
    addBuiltinSyntax("::=", 5, 5, INFIX, N, shiftInfix, reduceADTDefinition);
    addBuiltinSyntax("\u2A74", 5, 5, INFIX, N, shiftInfix, reduceADTDefinition);
    // reserve precedence 6 for "try"
    addBuiltinSyntax("->", 7, 7, INFIX, R, shiftInfix, reduceArrow);
    addBuiltinSyntax("\u21A6", 7, 7, INFIX, R, shiftInfix, reduceArrow);
    addBuiltinSyntax("case", 8, 8, PREFIX, N, shiftPrefix, reducePrefix);
    addBuiltinSyntax("@", 9, 9, INFIX, N, shiftInfix, reduceApply);
    addBuiltinSyntax("syntax", 90, 90, PREFIX, L, shiftPrefix, reducePrefix);
    addBuiltinSyntax("error", 90, 90, PREFIX, L, shiftPrefix, reduceError);
    addBuiltinSyntax("( )", 99, 99, INFIX, L, shiftSpace, reduceInvalid);
    addBuiltinSyntax("$", 99, 99, PREFIX, L, shiftPrefix, reduceReserved);
}
