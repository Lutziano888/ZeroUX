#ifndef FS_H
#define FS_H

#define MAX_FILES 64
#define MAX_FILENAME 32
#define MAX_CONTENT 1024

#define FILE_TYPE_EMPTY 0
#define FILE_TYPE_FILE  1
#define FILE_TYPE_DIR   2
#define FILE_TYPE_APP   3 // Neuer Typ für Apps

typedef struct {
    char name[MAX_FILENAME];
    int type;
    int size;
    int parent_idx;
    int in_use;
    char content[MAX_CONTENT];
} file_t;

void fs_init();
file_t* fs_get_table();
int fs_get_current_dir();
void fs_set_current_dir(int dir_idx);
int fs_mkdir(const char* name);
int fs_touch(const char* name);
int fs_rm(const char* name);
int fs_cd(const char* name);
void fs_pwd(char* buffer);

#endif