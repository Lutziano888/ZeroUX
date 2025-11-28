#ifndef FS_H
#define FS_H

#define MAX_FILENAME 32
#define MAX_FILES 64
#define MAX_FILE_SIZE 1024

typedef enum {
    FILE_TYPE_DIR,
    FILE_TYPE_FILE
} file_type_t;

typedef struct {
    char name[MAX_FILENAME];
    file_type_t type;
    char content[MAX_FILE_SIZE];
    int size;
    int parent_idx;  // Index des Elternverzeichnisses
    int in_use;      // 1 = belegt, 0 = frei
} file_t;

// Filesystem initialisieren
void fs_init();

// Verzeichnis wechseln
int fs_cd(const char* dirname);

// Aktuelles Verzeichnis anzeigen
void fs_pwd(char* out);

// Dateien im aktuellen Verzeichnis auflisten
void fs_ls();

// Datei erstellen
int fs_touch(const char* filename);

// Verzeichnis erstellen
int fs_mkdir(const char* dirname);

// Datei löschen
int fs_rm(const char* filename);

// Datei-Inhalt anzeigen
int fs_cat(const char* filename);

// In Datei schreiben
int fs_write(const char* filename, const char* content);

#endif