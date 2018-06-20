bool isSpaceCharacter(char c);
bool isQuoteCharacter(char c);
bool isDelimiterCharacter(char c);
bool isOperandCharacter(char c);
bool isOperatorCharacter(char c);

String getFirstLexeme(const char* input);
String getNextLexeme(String lastLexeme);
