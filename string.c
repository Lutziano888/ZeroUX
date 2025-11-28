#include "string.h"

void strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = 0;
}

void strcat(char* dest, const char* src) {
    while (*dest)
        dest++;

    while (*src) {
        *dest++ = *src++;
    }
    *dest = 0;
}

int strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *(unsigned char*)a - *(unsigned char*)b;
}

int strncmp(const char* a, const char* b, int n) {
    while (n-- && *a && (*a == *b)) {
        a++;
        b++;
    }
    return (n < 0) ? 0 : (*(unsigned char*)a - *(unsigned char*)b);
}

/* ----------- Zahl → String ----------- */
void int_to_str(int num, char* out) {
    char tmp[12];
    int i = 0;
    int is_negative = 0;

    if (num == 0) {
        out[0] = '0';
        out[1] = 0;
        return;
    }

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    while (num > 0) {
        tmp[i++] = '0' + (num % 10);
        num /= 10;
    }

    int j = 0;

    if (is_negative)
        out[j++] = '-';

    while (i > 0)
        out[j++] = tmp[--i];

    out[j] = 0;
}

/* ----------- String → Zahl ----------- */
int str_to_int(const char* s) {
    int result = 0;
    int sign = 1;

    if (*s == '-') {
        sign = -1;
        s++;
    }

    while (*s) {
        result = result * 10 + (*s - '0');
        s++;
    }

    return result * sign;
}
