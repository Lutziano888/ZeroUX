#include "python_interpreter.h"
#include "../string.h"

// Einfacher Speicher für Variablen (max 32 Integer-Variablen)
typedef struct {
    char name[32];
    int value;
} var_t;

static var_t variables[32];
static int var_count = 0;

static void reset_vars() {
    var_count = 0;
}

static void set_var(const char* name, int val) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            variables[i].value = val;
            return;
        }
    }
    if (var_count < 32) {
        strcpy(variables[var_count].name, name);
        variables[var_count].value = val;
        var_count++;
    }
}

static int get_var(const char* name) {
    // Ist es eine Zahl?
    if (name[0] >= '0' && name[0] <= '9') {
        int val = 0;
        const char* p = name;
        while (*p >= '0' && *p <= '9') {
            val = val * 10 + (*p - '0');
            p++;
        }
        return val;
    }
    // Variable suchen
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].value;
        }
    }
    return 0; // Undefined = 0
}

// Einfacher Parser für Ausdrücke: "a + b" oder "10"
static int eval_expr(const char* expr) {
    char buf[64];
    strcpy(buf, expr);
    
    // Suche nach Operator +
    char* plus = (char*)strstr(buf, "+");
    if (plus) {
        *plus = 0; // String teilen
        char* left = buf;
        char* right = plus + 1;
        // Trim spaces (sehr einfach)
        while (*left == ' ') left++;
        while (*right == ' ') right++;
        return get_var(left) + get_var(right);
    }
    
    // Suche nach Operator -
    char* minus = (char*)strstr(buf, "-");
    if (minus) {
        *minus = 0;
        char* left = buf;
        char* right = minus + 1;
        while (*left == ' ') left++;
        while (*right == ' ') right++;
        return get_var(left) - get_var(right);
    }

    // Kein Operator, nur Wert/Variable
    char* p = buf;
    while (*p == ' ') p++;
    return get_var(p);
}

void python_run_script(const char* script, char* out, int max_len) {
    reset_vars();
    out[0] = 0;
    
    char line[128];
    const char* p = script;
    int i = 0;
    
    while (*p) {
        if (*p == '\n' || i >= 127) {
            line[i] = 0;
            
            // --- Zeile verarbeiten ---
            // 1. Trim leading spaces
            char* cmd = line;
            while (*cmd == ' ' || *cmd == '\t') cmd++;
            
            if (cmd[0] == 0 || cmd[0] == '#') {
                // Leer oder Kommentar
            }
            else if (strncmp(cmd, "print(", 6) == 0) {
                char* content = cmd + 6;
                char* end = (char*)strstr(content, ")");
                if (end) {
                    *end = 0;
                    if (content[0] == '"') {
                        // String: print("Hello")
                        content++; // Skip "
                        char* quote = (char*)strstr(content, "\"");
                        if (quote) *quote = 0;
                        strcat(out, content);
                        strcat(out, "\n");
                    } else {
                        // Expression: print(x + 1)
                        int res = eval_expr(content);
                        char num[16];
                        int_to_str(res, num);
                        strcat(out, num);
                        strcat(out, "\n");
                    }
                }
            }
            else if (strstr(cmd, "=")) {
                // Assignment: x = 10
                char* eq = (char*)strstr(cmd, "=");
                *eq = 0;
                char* var_name = cmd;
                char* expr = eq + 1;
                
                // Trim var name
                while (*var_name == ' ') var_name++;
                int len = strlen(var_name);
                while (len > 0 && var_name[len-1] == ' ') var_name[--len] = 0;
                
                int val = eval_expr(expr);
                set_var(var_name, val);
            }
            // -------------------------
            
            i = 0;
        } else {
            line[i++] = *p;
        }
        p++;
    }
    // Letzte Zeile verarbeiten falls kein Newline am Ende
    if (i > 0) {
        line[i] = 0;
        char* cmd = line;
        while (*cmd == ' ' || *cmd == '\t') cmd++;
        if (strncmp(cmd, "print(", 6) == 0) {
             char* content = cmd + 6;
             char* end = (char*)strstr(content, ")");
             if (end) {
                 *end = 0;
                 if (content[0] == '"') {
                     content++; 
                     char* quote = (char*)strstr(content, "\"");
                     if (quote) *quote = 0;
                     strcat(out, content);
                     strcat(out, "\n");
                 } else {
                     int res = eval_expr(content);
                     char num[16];
                     int_to_str(res, num);
                     strcat(out, num);
                     strcat(out, "\n");
                 }
             }
        }
    }
}