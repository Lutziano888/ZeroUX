#include "fs.h"
#include "string.h"
#include "vga.h"

static file_t files[MAX_FILES];
static int current_dir = 0;  // Index des aktuellen Verzeichnisses

// Hilfsfunktion: Freien Slot finden
static int find_free_slot() {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].in_use) return i;
    }
    return -1;
}

// Hilfsfunktion: Datei/Verzeichnis im aktuellen Verzeichnis finden
static int find_file(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && 
            files[i].parent_idx == current_dir &&
            strcmp(files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void fs_init() {
    // Alle Slots als frei markieren
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].in_use = 0;
    }
    
    // Root-Verzeichnis erstellen
    files[0].in_use = 1;
    strcpy(files[0].name, "/");
    files[0].type = FILE_TYPE_DIR;
    files[0].parent_idx = 0;  // Root zeigt auf sich selbst
    files[0].size = 0;
    
    current_dir = 0;
    
    vga_println("Filesystem initialized.");
}

int fs_cd(const char* dirname) {
    // ".." = Elternverzeichnis
    if (strcmp(dirname, "..") == 0) {
        if (current_dir != 0) {
            current_dir = files[current_dir].parent_idx;
            return 0;
        }
        return 0;  // Bereits in root
    }
    
    // "/" = Root
    if (strcmp(dirname, "/") == 0) {
        current_dir = 0;
        return 0;
    }
    
    // Verzeichnis suchen
    int idx = find_file(dirname);
    if (idx == -1) {
        vga_print("cd: directory not found: ");
        vga_println(dirname);
        return -1;
    }
    
    if (files[idx].type != FILE_TYPE_DIR) {
        vga_print("cd: not a directory: ");
        vga_println(dirname);
        return -1;
    }
    
    current_dir = idx;
    return 0;
}

void fs_pwd(char* out) {
    if (current_dir == 0) {
        strcpy(out, "/");
        return;
    }
    
    // Pfad rekursiv aufbauen
    char temp[256];
    temp[0] = 0;
    
    int idx = current_dir;
    while (idx != 0) {
        char part[MAX_FILENAME + 2];
        strcpy(part, "/");
        strcat(part, files[idx].name);
        strcat(part, temp);
        strcpy(temp, part);
        idx = files[idx].parent_idx;
    }
    
    strcpy(out, temp);
}

void fs_ls() {
    int found = 0;
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && files[i].parent_idx == current_dir) {
            found = 1;
            
            if (files[i].type == FILE_TYPE_DIR) {
                vga_print("[DIR]  ");
            } else {
                vga_print("[FILE] ");
            }
            
            vga_print(files[i].name);
            
            if (files[i].type == FILE_TYPE_FILE) {
                vga_print(" (");
                char size_str[12];
                int_to_str(files[i].size, size_str);
                vga_print(size_str);
                vga_print(" bytes)");
            }
            
            vga_print("\n");
        }
    }
    
    if (!found) {
        vga_println("(empty directory)");
    }
}

int fs_mkdir(const char* dirname) {
    // Prüfen ob Name bereits existiert
    if (find_file(dirname) != -1) {
        vga_print("mkdir: directory already exists: ");
        vga_println(dirname);
        return -1;
    }
    
    // Freien Slot finden
    int idx = find_free_slot();
    if (idx == -1) {
        vga_println("mkdir: no free slots");
        return -1;
    }
    
    // Verzeichnis erstellen
    files[idx].in_use = 1;
    strcpy(files[idx].name, dirname);
    files[idx].type = FILE_TYPE_DIR;
    files[idx].parent_idx = current_dir;
    files[idx].size = 0;
    
    return 0;
}

int fs_touch(const char* filename) {
    // Prüfen ob Name bereits existiert
    if (find_file(filename) != -1) {
        vga_print("touch: file already exists: ");
        vga_println(filename);
        return -1;
    }
    
    // Freien Slot finden
    int idx = find_free_slot();
    if (idx == -1) {
        vga_println("touch: no free slots");
        return -1;
    }
    
    // Datei erstellen
    files[idx].in_use = 1;
    strcpy(files[idx].name, filename);
    files[idx].type = FILE_TYPE_FILE;
    files[idx].parent_idx = current_dir;
    files[idx].size = 0;
    files[idx].content[0] = 0;
    
    return 0;
}

int fs_rm(const char* filename) {
    int idx = find_file(filename);
    if (idx == -1) {
        vga_print("rm: file not found: ");
        vga_println(filename);
        return -1;
    }
    
    // Verzeichnis nur löschen wenn leer
    if (files[idx].type == FILE_TYPE_DIR) {
        for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].in_use && files[i].parent_idx == idx) {
                vga_println("rm: directory not empty");
                return -1;
            }
        }
    }
    
    files[idx].in_use = 0;
    return 0;
}

int fs_cat(const char* filename) {
    int idx = find_file(filename);
    if (idx == -1) {
        vga_print("cat: file not found: ");
        vga_println(filename);
        return -1;
    }
    
    if (files[idx].type != FILE_TYPE_FILE) {
        vga_print("cat: not a file: ");
        vga_println(filename);
        return -1;
    }
    
    if (files[idx].size == 0) {
        vga_println("(empty file)");
    } else {
        vga_println(files[idx].content);
    }
    
    return 0;
}

int fs_write(const char* filename, const char* content) {
    int idx = find_file(filename);
    if (idx == -1) {
        vga_print("write: file not found: ");
        vga_println(filename);
        return -1;
    }
    
    if (files[idx].type != FILE_TYPE_FILE) {
        vga_print("write: not a file: ");
        vga_println(filename);
        return -1;
    }
    
    // Content kopieren (mit Größenbeschränkung)
    int len = 0;
    while (content[len] && len < MAX_FILE_SIZE - 1) {
        files[idx].content[len] = content[len];
        len++;
    }
    files[idx].content[len] = 0;
    files[idx].size = len;
    
    return 0;
}