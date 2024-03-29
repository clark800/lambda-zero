#include "tree.h"
#include "stack.h"
#include "array.h"
#include "lex/token.h"
#include "lex/lex.h"
#include "opp/operator.h"
#include "opp/shift.h"
#include "ast.h"
#include "syntax.h"
#include "tokens.h"
#include "term.h"
#include "bind.h"
#include "parse.h"

static Node* getTop(Stack* stack) {
    return isEmpty(stack) ? NULL : peek(stack, 0);
}

static bool isThisOperator(Node* node, const char* lexeme) {
    return isOperator(node) && isThisTag(getTag(node), lexeme);
}

static void erase(Stack* stack, const char* lexeme) {
    if (!isEmpty(stack) && isThisOperator(peek(stack, 0), lexeme))
        release(pop(stack));
}

static bool isLeftSectionOperator(Node* op) {
    if (!isOperator(op) || isSpecialOperator(op))
        return false;
    Fixity fixity = getFixity(op);
    return fixity == INFIX || fixity == PREFIX;
}

static bool isRightSectionOperator(Node* op) {
    if (!isOperator(op) || isSpecialOperator(op))
        return false;
    Fixity fixity = getFixity(op);
    return fixity == INFIX || fixity == POSTFIX;
}

static void shiftNode(Stack* stack, Node* node) {
    Hold* nodeHold = hold(node);
    if (isOperator(node) && getFixity(node) == CLOSEFIX) {
        erase(stack, "\n");
        if (isThisOperator(node, ")"))
            erase(stack, ";");
    }
    if (isThisOperator(node, "\n")) {
        Node* top = getTop(stack);
        if (isOperator(top)) {
            if (getFixity(top) == PREFIX)
                syntaxErrorNode("missing operand after", top);
        } else {
            if (getSubprecedence(node) % 2 != 0)
                syntaxErrorNode("odd-width indent after", node);
            shift(stack, node);
        }
    } else if (isOperator(node)) {
        Node* top = getTop(stack);
        if (top == NULL) {
            shift(stack, node);
        } else if (isThisOperator(top, "(") && isRightSectionOperator(node)) {
            Location location = getLexeme(getTag(top)).location;
            erase(stack, "(");
            Hold* open = hold(parseSymbol(newLiteralLexeme("( ", location), 0));
            Hold* placeholder = hold(Name(newLiteralTag(".*", location, 0)));
            shift(stack, open);
            shift(stack, placeholder);
            shift(stack, node);
            release(open);
            release(placeholder);
        } else if (isThisOperator(node, ")") && isLeftSectionOperator(top)) {
            Location location = getLexeme(getTag(node)).location;
            erase(stack, "(");
            Hold* close = hold(parseSymbol(
                newLiteralLexeme(" )", location), 0));
            Hold* placeholder = hold(Name(newLiteralTag("*.", location, 0)));
            shift(stack, placeholder);
            shift(stack, close);
            release(placeholder);
            release(close);
        } else shift(stack, node);
    } else shift(stack, node);
    release(nodeHold);
}

static Hold* synthesize(Token (*lexer)(Token), Token start) {
    initSymbols();
    Stack* stack = newStack();
    Node* startNode = parseToken(start);
    shiftNode(stack, startNode);
    for (Token token = lexer(start); token.type != END; token = lexer(token))
        if (token.type != COMMENT && token.type != VSPACE
                && token.type != SPACE)
            shiftNode(stack, parseToken(token));
    Hold* ast = hold(getTop(stack));
    syntaxErrorNodeIf(ast == startNode, "no input", ast);
    deleteStack(stack);
    return ast;
}

Program parse(const char* input) {
    Hold* result = synthesize(lex, newStartToken(input));
    Array* globals = bind(result);
    Term* entry = elementAt(globals, length(globals) - 1);
    return (Program){result, entry, globals};
}

void deleteProgram(Program program) {
    release(program.root);
    deleteArray(program.globals);
}
