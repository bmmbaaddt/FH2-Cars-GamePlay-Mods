#include "wheel_variants.h"
#include "features.h"
#include "support.h"

enum { MAX_SQL = 65536 };
typedef int (*Prepare)(void *, const char *, int, void **, const char **);
static unsigned char *image;
static unsigned char thunk[12];
static const unsigned char padding[12] = {0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
                                          0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc};
static const unsigned char *const accepted[] = {padding, thunk};
PatchDefinition wheel_variants_hook_patch = {
    0x108d414, 0,     12,    GROUP_RIMS, "stock_wheel_variant_query_thunk",
    padding,   thunk, thunk, accepted,   ARRAY_COUNT(accepted)};

static const char wheel_rows[] =
    "(/*FH2_WHEEL_VARIANTS*/SELECT * FROM List_Wheels UNION ALL SELECT " FF_WHEEL_ID_TEXT
    ", '" FF_WHEEL_MEDIA "', DisplayName, PartManufacturerID, Mass, Price, "
    "IsStock, Type, Lip, CenterCap, Logo, ActualColor, WheelColor, RGB, BrakeDrawOffset, IconPath "
    "FROM List_Wheels WHERE ID=91 AND EXISTS (SELECT 1 FROM Data_Car WHERE ID=" FF_CAR_ID_TEXT
    " AND MediaName='" FF_WHEEL_MEDIA "') AND NOT EXISTS "
    "(SELECT 1 FROM List_Wheels WHERE ID=" FF_WHEEL_ID_TEXT "))";

static int word_char(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
           ch == '_';
}

static int equals(const char *text, size_t length, const char *word) {
    if (length != text_length(word)) {
        return 0;
    }
    for (size_t i = 0; i < length; ++i) {
        char ch = text[i];
        if (ch >= 'a' && ch <= 'z') {
            ch -= 'a' - 'A';
        }
        if (ch != word[i]) {
            return 0;
        }
    }
    return 1;
}

static int implicit_alias(const char *sql, size_t size, size_t after) {
    while (after < size &&
           (sql[after] == ' ' || sql[after] == '\t' || sql[after] == '\r' || sql[after] == '\n')) {
        ++after;
    }
    size_t end = after;
    while (end < size && word_char(sql[end])) {
        ++end;
    }
    if (end == after) {
        return 1;
    }
    static const char *const clauses[] = {"WHERE", "ON",    "JOIN",  "LEFT",  "RIGHT", "INNER",
                                          "CROSS", "ORDER", "GROUP", "LIMIT", "UNION", "USING"};
    for (unsigned i = 0; i < ARRAY_COUNT(clauses); ++i) {
        if (equals(sql + after, end - after, clauses[i])) {
            return 1;
        }
    }
    return 0;
}

static size_t expand_query(const char *sql, size_t size, char *out, size_t capacity) {
    if (!sql || !out || !size || size >= MAX_SQL ||
        text_find(sql, size, "/*FH2_WHEEL_VARIANTS*/")) {
        return 0;
    }
    size_t first = 0;
    while (first < size &&
           (sql[first] == ' ' || sql[first] == '\n' || sql[first] == '\r' || sql[first] == '\t')) {
        ++first;
    }
    if (size - first < 6 || !equals(sql + first, 6, "SELECT") ||
        (size - first > 6 && word_char(sql[first + 6]))) {
        return 0;
    }
    size_t read = 0, written = 0;
    int source = 0, changed = 0;
    for (size_t i = first; i < size;) {
        char ch = sql[i];
        if (ch == '\'' || ch == '"' || ch == '`' || ch == '[') {
            char close = ch == '[' ? ']' : ch;
            ++i;
            while (i < size) {
                if (sql[i++] == close) {
                    if (close != ']' && i < size && sql[i] == close) {
                        ++i;
                    } else {
                        break;
                    }
                }
            }
            source = 0;
        } else if (i + 1 < size && ch == '-' && sql[i + 1] == '-') {
            while (i < size && sql[i] != '\n') {
                ++i;
            }
        } else if (i + 1 < size && ch == '/' && sql[i + 1] == '*') {
            i += 2;
            while (i + 1 < size && !(sql[i] == '*' && sql[i + 1] == '/')) {
                ++i;
            }
            i = i + 2 < size ? i + 2 : size;
        } else if (word_char(ch)) {
            size_t begin = i++;
            while (i < size && word_char(sql[i])) {
                ++i;
            }
            if (source && equals(sql + begin, i - begin, "LIST_WHEELS")) {
                size_t unchanged = begin - read;
                const char *alias = implicit_alias(sql, size, i) ? " AS List_Wheels" : "";
                size_t added = unchanged + sizeof(wheel_rows) - 1 + text_length(alias);
                if (added >= capacity - written) {
                    return 0;
                }
                memory_copy(out + written, sql + read, unchanged);
                written += unchanged;
                memory_copy(out + written, wheel_rows, sizeof(wheel_rows) - 1);
                written += sizeof(wheel_rows) - 1;
                memory_copy(out + written, alias, text_length(alias));
                written += text_length(alias);
                read = i;
                changed = 1;
            }
            source =
                equals(sql + begin, i - begin, "FROM") || equals(sql + begin, i - begin, "JOIN");
        } else {
            if (ch == ';') {
                for (size_t rest = i + 1; rest < size; ++rest) {
                    if (sql[rest] != ' ' && sql[rest] != '\t' && sql[rest] != '\n' &&
                        sql[rest] != '\r') {
                        return 0;
                    }
                }
            }
            if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
                source = 0;
            }
            ++i;
        }
    }
    if (!changed || size - read >= capacity - written) {
        return 0;
    }
    memory_copy(out + written, sql + read, size - read);
    written += size - read;
    out[written] = 0;
    return written;
}

static int prepare_wheel_query(void *database, const char *sql, int bytes, void **statement,
                               const char **tail) {
    Prepare original = (Prepare)(image + 0x3d710);
    size_t size = 0;
    if (sql) {
        while (size < MAX_SQL && (bytes < 0 || size < (unsigned)bytes) && sql[size]) {
            ++size;
        }
    }
    if (!size || size >= MAX_SQL || !text_find(sql, size, "List_Wheels") ||
        text_find(sql, size, "/*FH2_WHEEL_VARIANTS*/")) {
        return original(database, sql, bytes, statement, tail);
    }
    char *expanded = HeapAlloc(GetProcessHeap(), 0, MAX_SQL);
    size_t count = expanded ? expand_query(sql, size, expanded, MAX_SQL) : 0;
    if (!count) {
        if (expanded) {
            HeapFree(GetProcessHeap(), 0, expanded);
        }
        return original(database, sql, bytes, statement, tail);
    }
    const char *end = NULL;
    int result = original(database, expanded, (int)count, statement, &end);
    if (tail) {
        *tail = sql + size;
    }
    HeapFree(GetProcessHeap(), 0, expanded);
    return result;
}

void wheel_variants_prepare_hook(unsigned char *game_image) {
    image = game_image;
    const unsigned char jump[12] = {0x48, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xe0};
    uintptr_t callback = (uintptr_t)prepare_wheel_query;
    memory_copy(thunk, jump, sizeof(jump));
    memory_copy(thunk + 2, &callback, 8);
}

#ifdef FH2_TWEAKS_TEST
__declspec(dllexport) size_t Fh2TestWheelVariants(const char *sql, size_t size, char *output,
                                                  size_t capacity) {
    return expand_query(sql, size, output, capacity);
}
#endif
