
typedef struct {
    const char* start;
    unsigned int length;
} String;

typedef struct {
    unsigned int line, column;
} Location;

typedef struct {
    String lexeme;
    Location location;
} Tag;

extern const String EMPTY;
String newString(const char* start, unsigned int length);
Location newLocation(unsigned int line, unsigned int column);
Tag newTag(String lexeme, Location location);
Tag renameTag(Tag tag, const char* name);
bool isThisString(String a, const char* b);
bool isSameString(String a, String b);
