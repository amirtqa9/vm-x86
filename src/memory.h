#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    uint8_t *data;
    size_t   size;
} Memory;

/* Returns false if size == 0 or allocation fails */
bool mem_init(Memory *mem, size_t size);
void mem_free(Memory *mem);

/* All reads/writes return false on out-of-range access */
bool mem_read8 (const Memory *mem, uint32_t addr, uint8_t  *out);
bool mem_read16(const Memory *mem, uint32_t addr, uint16_t *out);
bool mem_read32(const Memory *mem, uint32_t addr, uint32_t *out);

bool mem_write8 (Memory *mem, uint32_t addr, uint8_t  val);
bool mem_write16(Memory *mem, uint32_t addr, uint16_t val);
bool mem_write32(Memory *mem, uint32_t addr, uint32_t val);

/* Load raw bytes into memory at offset; returns false if it doesn't fit */
bool mem_load(Memory *mem, uint32_t offset, const uint8_t *src, size_t len);

#endif /* MEMORY_H */
