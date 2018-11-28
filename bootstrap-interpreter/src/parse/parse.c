#include "lib/tree.h"
#include "lib/array.h"
#include "lib/stack.h"
#include "lex/token.h"
#include "lex/lex.h"
#include "synthesize/debug.h"
#include "synthesize/synthesize.h"
#include "bind/bind.h"
#include "parse.h"

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
