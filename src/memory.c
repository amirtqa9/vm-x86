#include "memory.h"
#include <stdlib.h>
#include <string.h>

bool mem_init(Memory *mem, size_t size) {
    if (size == 0) return false;
    mem->data = calloc(1, size);
    if (!mem->data) return false;
    mem->size = size;
    return true;
}

void mem_free(Memory *mem) {
    free(mem->data);
    mem->data = NULL;
    mem->size = 0;
}

bool mem_read8(const Memory *mem, uint32_t addr, uint8_t *out) {
    if ((size_t)addr >= mem->size) return false;
    *out = mem->data[addr];
    return true;
}

bool mem_read16(const Memory *mem, uint32_t addr, uint16_t *out) {
    if ((size_t)addr + 1 >= mem->size) return false;
    /* little-endian */
    *out = (uint16_t)(mem->data[addr]) | ((uint16_t)(mem->data[addr + 1]) << 8);
    return true;
}

bool mem_read32(const Memory *mem, uint32_t addr, uint32_t *out) {
    if ((size_t)addr + 3 >= mem->size) return false;
    *out = (uint32_t)(mem->data[addr])
         | ((uint32_t)(mem->data[addr + 1]) << 8)
         | ((uint32_t)(mem->data[addr + 2]) << 16)
         | ((uint32_t)(mem->data[addr + 3]) << 24);
    return true;
}

bool mem_write8(Memory *mem, uint32_t addr, uint8_t val) {
    if ((size_t)addr >= mem->size) return false;
    mem->data[addr] = val;
    return true;
}

bool mem_write16(Memory *mem, uint32_t addr, uint16_t val) {
    if ((size_t)addr + 1 >= mem->size) return false;
    mem->data[addr]     = (uint8_t)(val & 0xFF);
    mem->data[addr + 1] = (uint8_t)(val >> 8);
    return true;
}

bool mem_write32(Memory *mem, uint32_t addr, uint32_t val) {
    if ((size_t)addr + 3 >= mem->size) return false;
    mem->data[addr]     = (uint8_t)(val & 0xFF);
    mem->data[addr + 1] = (uint8_t)((val >> 8)  & 0xFF);
    mem->data[addr + 2] = (uint8_t)((val >> 16) & 0xFF);
    mem->data[addr + 3] = (uint8_t)((val >> 24) & 0xFF);
    return true;
}

bool mem_load(Memory *mem, uint32_t offset, const uint8_t *src, size_t len) {
    if ((size_t)offset + len > mem->size) return false;
    memcpy(mem->data + offset, src, len);
    return true;
}
