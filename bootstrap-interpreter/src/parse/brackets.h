Node* Nil(Tag tag);
Node* prepend(Tag tag, Node* item, Node* list);

Node* reduceOpenParenthesis(Tag tag, Node* before, Node* contents);
Node* reduceOpenSquareBracket(Tag tag, Node* before, Node* contents);
Node* reduceOpenBrace(Tag tag, Node* before, Node* contents);
Node* reduceOpenFile(Tag tag, Node* before, Node* contents);
