#include "shared/lib/tree.h"
#include "shared/lib/array.h"
#include "shared/token.h"
#include "lex/lex.h"
#include "synthesize/synthesize.h"
#include "synthesize/debug.h"
#include "bind/bind.h"
#include "parse.h"

static void debugParseStage(const char* label, Node* node, bool trace) {
    if (trace) {
        fputs("======================================", stderr);
        fputs("======================================\n", stderr);
        fputs(label, stderr);
        fputs(": ", stderr);
        debugAST(node);
        fputs("\n", stderr);
    }
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
