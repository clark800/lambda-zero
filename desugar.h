#ifndef DESUGAR_H
#define DESUGAR_H

#include "lib/tree.h"

Node* transformLambdaSugar(Node* operator, Node* left, Node* right);
void desugar(Node* node);

#endif
