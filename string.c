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

/* ----------- String Utilities ----------- */
const char* strstr(const char* haystack, const char* needle) {
    if (*needle == '\0')
        return haystack;
    
    while (*haystack) {
        const char* h = haystack;
        const char* n = needle;
        
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        
        if (*n == '\0')
            return haystack;
        
        haystack++;
    }
    
    return 0;
}

void memcpy(void* dest, const void* src, int n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while (n--) {
        *d++ = *s++;
    }
}

void* memset(void* ptr, int value, int num) {
    unsigned char* p = (unsigned char*)ptr;
    while (num--) *p++ = (unsigned char)value;
    return ptr;
}

int memcmp(const void* ptr1, const void* ptr2, int num) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    while (num--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

int strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
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
