#include "shared/lib/tree.h"
#include "shared/lib/array.h"
#include "shared/lib/stack.h"
#include "parse/shared/token.h"
#include "parse/shared/debug.h"
#include "lex/lex.h"
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
