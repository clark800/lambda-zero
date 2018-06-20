
typedef struct {
    const char* start;
    unsigned int length;
} String;

String newString(const char* start, unsigned int length);
bool isThisString(String a, const char* b);
bool isSameString(String a, String b);
