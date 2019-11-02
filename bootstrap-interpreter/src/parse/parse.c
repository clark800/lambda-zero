#include "tree.h"
#include "stack.h"
#include "array.h"
#include "lex/token.h"
#include "lex/lex.h"
#include "opp/errors.h"
#include "opp/operator.h"
#include "opp/shift.h"
#include "ast.h"
#include "syntax.h"
#include "tokens.h"
#include "bind.h"
#include "debug.h"
#include "parse.h"

int DEBUG = 0;

Node* getTop(Stack* stack) {
    return isEmpty(stack) ? NULL : peek(stack, 0);
}

void erase(Stack* stack, const char* lexeme) {
    if (!isEmpty(stack) && isThisOperator(peek(stack, 0), lexeme))
        release(pop(stack));
}

static void shiftNode(Stack* stack, Node* node) {
    debugParseState(getTag(node), stack, DEBUG >= 2);
    if (isCloseOperator(node)) {
        erase(stack, "\n");
        if (isThisOperator(node, ")"))
            erase(stack, ";");
    }
    if (isThisOperator(node, "\n")) {
        if (!isOperator(getTop(stack))) {
            if (getValue(node) % 2 != 0)
                syntaxError("odd-width indent after", node);
            shift(stack, node);
        }
    } else if (isOperator(node)) {
        Node* top = getTop(stack);
        if (top == NULL) {
            shift(stack, node);
        } else if (isThisOperator(top, "(") && isRightSectionOperator(node)) {
            erase(stack, "(");
            Node* open = parseSymbol(renameTag(getTag(top), "( "), 0);
            Node* placeholder = Name(renameTag(getTag(top), ".*"));
            shift(stack, open);
            shift(stack, placeholder);
            shift(stack, node);
            release(hold(open));
            release(hold(placeholder));
        } else if (isThisOperator(node, ")") && isLeftSectionOperator(top)) {
            Node* close = parseSymbol(renameTag(getTag(node), " )"), 0);
            Node* placeholder = Name(renameTag(getTag(node), "*."));
            shift(stack, placeholder);
            shift(stack, close);
            release(hold(placeholder));
            release(hold(close));
        } else shift(stack, node);
    } else shift(stack, node);
    release(hold(node));
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
    syntaxErrorIf(getNode(ast) == startNode, "no input", getNode(ast));
    deleteStack(stack);
    return ast;
}

Program parse(const char* input) {
    Hold* result = synthesize(lex, newStartToken(input));
    debugParseStage("parse", getNode(result), DEBUG >= 2);
    Array* globals = bind(result);
    debugParseStage("bind", getNode(result), DEBUG >= 2);
    Node* entry = elementAt(globals, length(globals) - 1);
    debugParseStage("entry", entry, DEBUG >= 1);
    return (Program){result, entry, globals};
}

void deleteProgram(Program program) {
    release(program.root);
    deleteArray(program.globals);
}
