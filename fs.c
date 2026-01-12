#include "fs.h"
#include "string.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

static file_t files[MAX_FILES];
static int current_dir_idx = 0;

static int find_free_slot() {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].in_use) return i;
    }
    return -1;
}

static int create_file_internal(const char* name, int type, int parent, const char* content) {
    int slot = find_free_slot();
    if (slot == -1) return -1;
    
    strcpy(files[slot].name, name);
    files[slot].type = type;
    files[slot].parent_idx = parent;
    files[slot].in_use = 1;
    files[slot].size = content ? strlen(content) : 0;
    if (content) strcpy(files[slot].content, content);
    else files[slot].content[0] = 0;
    
    return slot;
}

void fs_init() {
    // 1. Alles löschen
    for (int i = 0; i < MAX_FILES; i++) files[i].in_use = 0;
    current_dir_idx = 0;

    // 2. Root Verzeichnis (Index 0)
    files[0].in_use = 1;
    strcpy(files[0].name, "Root");
    files[0].type = FILE_TYPE_DIR;
    files[0].parent_idx = 0;

    // 3. Standard-Ordner erstellen
    int prog_dir = create_file_internal("Programs", FILE_TYPE_DIR, 0, NULL);
    int docs_dir = create_file_internal("Documents", FILE_TYPE_DIR, 0, NULL);
    int sys_dir  = create_file_internal("System", FILE_TYPE_DIR, 0, NULL);

    // 4. Apps in "Programs" erstellen
    // Der Content "app:ID" hilft uns später, das richtige Fenster zu öffnen
    create_file_internal("Calculator", FILE_TYPE_APP, prog_dir, "app:2");
    create_file_internal("Notepad",    FILE_TYPE_APP, prog_dir, "app:1");
    create_file_internal("Browser",    FILE_TYPE_APP, prog_dir, "app:4");
    create_file_internal("Terminal",   FILE_TYPE_APP, prog_dir, "app:6");
    create_file_internal("Python IDE", FILE_TYPE_APP, prog_dir, "app:7");
    create_file_internal("MemDoctor",  FILE_TYPE_APP, prog_dir, "app:5");
    create_file_internal("Files",      FILE_TYPE_APP, prog_dir, "app:3");
    create_file_internal("Text Editor", FILE_TYPE_APP, prog_dir, "app:8");

    // 5. Beispiel-Dateien in "Documents"
    create_file_internal("Readme.txt", FILE_TYPE_FILE, docs_dir, "Welcome to ZeroUX!\n\nThis is a structured filesystem.\nNavigate using the File Manager.");
    create_file_internal("Todo.txt",   FILE_TYPE_FILE, docs_dir, "- Fix bugs\n- Add more apps\n- Sleep");

    // 6. System Dateien
    create_file_internal("kernel.sys", FILE_TYPE_FILE, sys_dir, "BINARY_DATA_DO_NOT_TOUCH");

    // 7. Externe Binaries (Demo)
    create_file_internal("win_app.exe", FILE_TYPE_FILE, prog_dir, "MZ\x90\x00\x03\x00\x00\x00\x04\x00\x00\x00\xFF\xFF\x00\x00\xB8\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00\x00\x0E\x1F\xBA\x0E\x00\xB4\x09\xCD\x21\xB8\x01\x4C\xCD\x21\x54\x68\x69\x73\x20\x70\x72\x6F\x67\x72\x61\x6D\x20\x63\x61\x6E\x6E\x6F\x74\x20\x62\x65\x20\x72\x75\x6E\x20\x69\x6E\x20\x44\x4F\x53\x20\x6D\x6F\x64\x65\x2E\x0D\x0D\x0A\x24\x00\x00\x00\x00\x00\x00\x00");
    create_file_internal("linux_tool.elf", FILE_TYPE_FILE, prog_dir, "\x7F\x45\x4C\x46\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x03\x00\x01\x00\x00\x00\x54\x80\x04\x08\x34\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x34\x00\x20\x00\x01\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00");
}

file_t* fs_get_table() { return files; }
int fs_get_current_dir() { return current_dir_idx; }
void fs_set_current_dir(int dir) { current_dir_idx = dir; }

int fs_mkdir(const char* name) {
    return create_file_internal(name, FILE_TYPE_DIR, current_dir_idx, NULL);
}

int fs_touch(const char* name) {
    return create_file_internal(name, FILE_TYPE_FILE, current_dir_idx, "");
}

int fs_rm(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && files[i].parent_idx == current_dir_idx && strcmp(files[i].name, name) == 0) {
            // Einfache Löschung (rekursiv wäre besser, aber dies reicht erstmal)
            files[i].in_use = 0;
            return 0;
        }
    }
    return -1;
}

int fs_cd(const char* name) {
    if (strcmp(name, "..") == 0) {
        if (current_dir_idx != 0) {
            current_dir_idx = files[current_dir_idx].parent_idx;
        }
        return 0;
    }
    // Root check
    if (strcmp(name, "/") == 0 || strcmp(name, "\\") == 0) {
        current_dir_idx = 0;
        return 0;
    }
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && files[i].parent_idx == current_dir_idx && 
            files[i].type == FILE_TYPE_DIR && strcmp(files[i].name, name) == 0) {
            current_dir_idx = i;
            return 0;
        }
    }
    return -1;
}

void fs_pwd(char* buffer) {
    if (current_dir_idx == 0) {
        strcpy(buffer, "/");
        return;
    }
    
    // Rekursiv Pfad bauen (einfach: nur 1 Level Parent anzeigen für jetzt)
    // Richtig wäre ein Stack, aber wir machen es simpel:
    char temp[64];
    strcpy(buffer, "/");
    strcat(buffer, files[current_dir_idx].name);
}

// Neue Funktion zum Speichern von Daten (für Installer/Netzwerk)
int fs_save_file(const char* name, const char* data, int len) {
    // 1. Prüfen ob Datei existiert (Überschreiben)
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && files[i].parent_idx == current_dir_idx && strcmp(files[i].name, name) == 0) {
            files[i].size = len;
            for(int k=0; k<len; k++) files[i].content[k] = data[k];
            files[i].content[len] = 0; // Safety null
            return 0;
        }
    }
    // 2. Neue Datei erstellen
    int slot = find_free_slot();
    if (slot == -1) return -1;
    
    strcpy(files[slot].name, name);
    files[slot].type = FILE_TYPE_FILE;
    files[slot].parent_idx = current_dir_idx;
    files[slot].in_use = 1;
    files[slot].size = len;
    for(int k=0; k<len; k++) files[slot].content[k] = data[k];
    files[slot].content[len] = 0;
    return 0;
}