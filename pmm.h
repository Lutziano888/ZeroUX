#ifndef PMM_H
#define PMM_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

// Initialisiert den Speicher-Manager mit der RAM-Größe (in KB) und einer Adresse für die Bitmap
void pmm_init(uint32_t mem_size_kb, uint32_t bitmap_base_addr);

void* pmm_alloc_block();       // Reserviert einen 4KB Block
void pmm_free_block(void* p);  // Gibt einen Block wieder frei

// Markiert einen Speicherbereich als belegt (z.B. Kernel-Code, Video-RAM)
void pmm_mark_region_used(uint32_t base_addr, uint32_t size);

#endif