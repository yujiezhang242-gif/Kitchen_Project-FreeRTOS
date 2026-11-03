#define _POSIX_C_SOURCE 200809L
#include "runtime.h"
#include "config_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef struct { uint8_t mem[2][1024]; const char *path; } SimFlash;
static uint32_t millis(void)
{
    struct timespec t; static uint64_t origin; uint64_t value;
    (void)clock_gettime(CLOCK_MONOTONIC,&t); value=(uint64_t)t.tv_sec*1000+(uint64_t)t.tv_nsec/1000000;
    if(!origin) origin=value; return (uint32_t)(value-origin);
}
static bool read_mem(void *v,unsigned slot,unsigned off,uint8_t *p,size_t n)
{
    SimFlash *f=v; if(slot>1 || off+n>1024) return false; memcpy(p,f->mem[slot]+off,n); return true;
}
static bool flush(SimFlash *f,unsigned slot,unsigned off,size_t n)
{
    FILE *out;
    if(!f->path) return true;
    out=fopen(f->path,"r+b"); if(!out) return false;
    if(fseek(out,(long)(slot*1024+off),SEEK_SET)!=0 || fwrite(f->mem[slot]+off,1,n,out)!=n) { fclose(out); return false; }
    return fclose(out)==0;
}
static bool erase_mem(void *v,unsigned slot)
{
    SimFlash *f=v; if(slot>1) return false; memset(f->mem[slot],255,1024); return flush(f,slot,0,1024);
}
static bool program_mem(void *v,unsigned slot,unsigned off,const uint8_t *p,size_t n)
{
    SimFlash *f=v; size_t i; if(slot>1 || off+n>1024) return false;
    for(i=0;i<n;i++) { if((f->mem[slot][off+i]&p[i])!=p[i]) return false; f->mem[slot][off+i]&=p[i]; }
    return flush(f,slot,off,n);
}
int main(int argc,char **argv)
{
    KitchenRuntime r; KitchenConfig c; KitchenSample sample={0,0,250,500,1000,true,true};
    ConfigStore store; SimFlash flash; ConfigBackend io={&flash,read_mem,erase_mem,program_mem};
    WireParser parser; WireFrame q,a; uint8_t encoded[138]; int b; size_t n;
    memset(&flash,255,sizeof flash); flash.path=argc>1?argv[1]:NULL;
    if(flash.path) {
        FILE *file=fopen(flash.path,"rb");
        if(file) { (void)fread(flash.mem,1,sizeof flash.mem,file); fclose(file); }
        else { file=fopen(flash.path,"wb"); if(!file) return 2; if(fwrite(flash.mem,1,sizeof flash.mem,file)!=sizeof flash.mem) return 2; fclose(file); }
    }
    bool loaded=config_load(&store,&io,&c); runtime_init(&r,&c); r.dirty=!loaded; wire_init(&parser);
    setvbuf(stdin,NULL,_IONBF,0); setvbuf(stdout,NULL,_IONBF,0);
    while((b=getchar())!=EOF) {
        uint32_t now=millis();
        if(!wire_feed(&parser,(uint8_t)b,now,&q)) continue;
        /* Synthetic sensor refreshes on requests; not an RTOS/hardware simulator. */
        sample.climate_ms=now; sample.light_ms=now; runtime_tick(&r,&sample,now);
        if(q.type==CMD_SAVE) {
            memset(&a,0,sizeof a); a.type=q.type|0x80; a.seq=q.seq; a.length=1;
            if(q.length) a.data[0]=RESULT_LENGTH;
            else if(!r.dirty || config_save(&store,&r.control.cfg)) r.dirty=false;
            else a.data[0]=RESULT_IO;
        } else runtime_command(&r,&q,&a,now);
        n=wire_encode(&a,encoded,sizeof encoded); if(fwrite(encoded,1,n,stdout)!=n) return 3;
    }
    return 0;
}
