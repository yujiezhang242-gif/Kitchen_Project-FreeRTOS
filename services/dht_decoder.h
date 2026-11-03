#ifndef KITCHEN_DHT_DECODER_H
#define KITCHEN_DHT_DECODER_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    uint8_t state,bits,data[5];
    uint16_t edge_us;
    bool complete,failed;
} DhtDecoder;
void dht_decoder_start(volatile DhtDecoder *d);
/* Called on both GPIO edges with free-running 1MHz uint16 timer timestamp. */
void dht_decoder_edge(volatile DhtDecoder *d,bool high,uint16_t at_us);
bool dht_decoder_result(const volatile DhtDecoder *d,int16_t *temp,uint16_t *humid);
#endif
