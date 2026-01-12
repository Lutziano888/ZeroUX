#include "pmm.h"

#define BLOCK_SIZE 4096
#define BLOCKS_PER_BYTE 8

static uint32_t _max_blocks = 0;
static uint32_t _used_blocks = 0;
static uint8_t* _memory_map = 0;

// Hilfsfunktion: Setzt ein Bit in der Map (Block belegt)
static void mmap_set(int bit) {
    _memory_map[bit / 8] |= (1 << (bit % 8));
}

// Hilfsfunktion: Löscht ein Bit in der Map (Block frei)
static void mmap_unset(int bit) {
    _memory_map[bit / 8] &= ~(1 << (bit % 8));
}

// Hilfsfunktion: Findet das erste freie Bit (0)
static int mmap_first_free() {
    for (uint32_t i = 0; i < _max_blocks / 8; i++) {
        if (_memory_map[i] != 0xFF) { // Wenn das Byte nicht voll ist
            for (int j = 0; j < 8; j++) {
                int bit = 1 << j;
                if (!(_memory_map[i] & bit))
                    return i * 8 + j;
            }
        }
    }
    return -1;
}

void pmm_init(uint32_t mem_size_kb, uint32_t bitmap_base_addr) {
    _memory_map = (uint8_t*)bitmap_base_addr;
    _max_blocks = (mem_size_kb * 1024) / BLOCK_SIZE;
    _used_blocks = _max_blocks; // Erstmal alles als "belegt" annehmen zur Sicherheit

    // Alle Blöcke als "frei" (0) initialisieren
    // Achtung: In einem echten OS musst du hier Bereiche markieren, die der Kernel selbst belegt!
    for (uint32_t i = 0; i < _max_blocks / BLOCKS_PER_BYTE; i++) {
        _memory_map[i] = 0; 
    }
    _used_blocks = 0;
}

void* pmm_alloc_block() {
    if (_max_blocks - _used_blocks <= 0) return 0; // Kein RAM mehr!

    int frame = mmap_first_free();
    if (frame == -1) return 0;

    mmap_set(frame);
    uint32_t addr = frame * BLOCK_SIZE;
    _used_blocks++;

    return (void*)addr;
}

void pmm_free_block(void* p) {
    uint32_t addr = (uint32_t)p;
    int frame = addr / BLOCK_SIZE;
    
    mmap_unset(frame);
    _used_blocks--;
}

void pmm_mark_region_used(uint32_t base_addr, uint32_t size) {
    int start_block = base_addr / BLOCK_SIZE;
    int num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE; // Aufrunden

    for (int i = 0; i < num_blocks; i++) {
        int block = start_block + i;
        if (block < _max_blocks) {
            mmap_set(block);
            _used_blocks++;
        }
    }
}