#include "text.h"
#include <stdarg.h>
#include <stdint.h>
static void put(char *dst,size_t cap,size_t *n,char c)
{ if(cap && *n<cap-1) dst[*n]=c; (*n)++; }
static void number(char *dst,size_t cap,size_t *n,unsigned long value)
{
    char digits[3*sizeof(unsigned long)+1]; unsigned i=0;
    do { digits[i++]=(char)('0'+value%10); value/=10; } while(value);
    while(i) put(dst,cap,n,digits[--i]);
}
size_t text_format(char *dst,size_t cap,const char *fmt,...)
{
    size_t n=0; va_list args; va_start(args,fmt);
    while(*fmt) {
        if(*fmt!='%') { put(dst,cap,&n,*fmt++); continue; }
        fmt++;
        if(*fmt=='s') { const char *s=va_arg(args,const char *); if(!s) s="?"; while(*s) put(dst,cap,&n,*s++); }
        else if(*fmt=='u') number(dst,cap,&n,va_arg(args,unsigned));
        else if(*fmt=='d') {
            int v=va_arg(args,int); unsigned magnitude=(unsigned)v;
            if(v<0) { put(dst,cap,&n,'-'); magnitude=0u-magnitude; }
            number(dst,cap,&n,magnitude);
        } else if(*fmt=='l' && fmt[1]=='u') { fmt++; number(dst,cap,&n,va_arg(args,unsigned long)); }
        else if(*fmt=='%') put(dst,cap,&n,'%');
        else if(!*fmt) break;
        else put(dst,cap,&n,'?');
        fmt++;
    }
    va_end(args); if(cap) dst[n<cap?n:cap-1]=0; return n;
}
