#ifndef KITCHEN_WIRE_H
#define KITCHEN_WIRE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define WIRE_MAX_PAYLOAD 128u
#define WIRE_MAX_FRAME 138u
typedef struct { uint8_t type; uint16_t seq, length; uint8_t data[128]; } WireFrame;
typedef struct {
    uint8_t buffer[WIRE_MAX_FRAME];
    uint16_t used;
    uint32_t last_ms, bad_crc, bad_length, timeouts, frames;
} WireParser;
uint16_t wire_crc(const uint8_t *data, size_t size);
uint16_t wire_get16(const uint8_t *p);
uint32_t wire_get32(const uint8_t *p);
void wire_put16(uint8_t *p, uint16_t v);
void wire_put32(uint8_t *p, uint32_t v);
size_t wire_encode(const WireFrame *f, uint8_t *out, size_t capacity);
void wire_init(WireParser *p);
void wire_expire(WireParser *p, uint32_t now);
bool wire_feed(WireParser *p, uint8_t byte, uint32_t now, WireFrame *out);
#endif
