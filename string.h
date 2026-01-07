#ifndef STRING_H
#define STRING_H

void strcpy(char* dest, const char* src);
void strcat(char* dest, const char* src);

int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, int n);

const char* strstr(const char* haystack, const char* needle);
void memcpy(void* dest, const void* src, int n);

void int_to_str(int num, char* out);
int str_to_int(const char* s);

#endif
