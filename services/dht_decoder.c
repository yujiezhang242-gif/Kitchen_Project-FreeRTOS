#include "dht_decoder.h"
void dht_decoder_start(volatile DhtDecoder *d)
{
    unsigned i; d->state=0; d->bits=0; d->edge_us=0; d->complete=false; d->failed=false;
    for(i=0;i<5;i++) d->data[i]=0;
}
void dht_decoder_edge(volatile DhtDecoder *d,bool high,uint16_t at)
{
    uint16_t dt=(uint16_t)(at-d->edge_us);
    if(d->complete || d->failed) return;
    switch(d->state) {
    case 0: /* Ignore host release rising edge; await sensor's response low. */
        if(!high) { d->edge_us=at; d->state=1; } break;
    case 1:
        if(!high || dt<50 || dt>120) { d->failed=true; break; }
        d->edge_us=at; d->state=2; break;
    case 2:
        if(high || dt<50 || dt>120) { d->failed=true; break; }
        d->edge_us=at; d->state=3; break;
    case 3: /* Data low period, then rising edge. */
        if(!high || dt<30 || dt>100) { d->failed=true; break; }
        d->edge_us=at; d->state=4; break;
    case 4:
        if(high || dt<15 || dt>110) { d->failed=true; break; }
        d->data[d->bits/8]=(uint8_t)((d->data[d->bits/8]<<1)|(dt>45?1:0));
        d->bits++; d->edge_us=at; d->state=3;
        if(d->bits==40) d->complete=true;
        break;
    default: d->failed=true; break;
    }
}
bool dht_decoder_result(const volatile DhtDecoder *d,int16_t *temp,uint16_t *humid)
{
    if(!d->complete || d->failed || d->data[0]>100 || d->data[2]>80 ||
       (uint8_t)(d->data[0]+d->data[1]+d->data[2]+d->data[3])!=d->data[4]) return false;
    *temp=(int16_t)(d->data[2]*10); *humid=(uint16_t)(d->data[0]*10); return true;
}
