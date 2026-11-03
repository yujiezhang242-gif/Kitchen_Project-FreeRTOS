#include "board.h"
#include "board_config.h"
#include "stm32f10x.h"
/* Small original 5x7 uppercase font. No frame buffer, one 128-byte line. */
static const uint8_t glyph[59][5]={
 ['!'-32]={0,0,95,0,0}, ['%'-32]={35,19,8,100,98},
 ['+'-32]={8,8,62,8,8}, ['-'-32]={8,8,8,8,8}, ['.'-32]={0,96,96,0,0}, ['/'-32]={32,16,8,4,2},
 ['0'-32]={62,81,73,69,62}, ['1'-32]={0,66,127,64,0}, ['2'-32]={66,97,81,73,70},
 ['3'-32]={33,65,69,75,49}, ['4'-32]={24,20,18,127,16}, ['5'-32]={39,69,69,69,57},
 ['6'-32]={60,74,73,73,48}, ['7'-32]={1,113,9,5,3}, ['8'-32]={54,73,73,73,54},
 ['9'-32]={6,73,73,41,30}, [':'-32]={0,54,54,0,0}, ['='-32]={20,20,20,20,20},
 ['?'-32]={2,1,81,9,6}, ['A'-32]={126,17,17,17,126}, ['B'-32]={127,73,73,73,54},
 ['C'-32]={62,65,65,65,34}, ['D'-32]={127,65,65,34,28}, ['E'-32]={127,73,73,73,65},
 ['F'-32]={127,9,9,9,1}, ['G'-32]={62,65,73,73,122}, ['H'-32]={127,8,8,8,127},
 ['I'-32]={0,65,127,65,0}, ['J'-32]={32,64,65,63,1}, ['K'-32]={127,8,20,34,65},
 ['L'-32]={127,64,64,64,64}, ['M'-32]={127,2,12,2,127}, ['N'-32]={127,4,8,16,127},
 ['O'-32]={62,65,65,65,62}, ['P'-32]={127,9,9,9,6}, ['Q'-32]={62,65,81,33,94},
 ['R'-32]={127,9,25,41,70}, ['S'-32]={70,73,73,73,49}, ['T'-32]={1,1,127,1,1},
 ['U'-32]={63,64,64,64,63}, ['V'-32]={31,32,64,32,31}, ['W'-32]={63,64,56,64,63},
 ['X'-32]={99,20,8,20,99}, ['Y'-32]={7,8,112,8,7}, ['Z'-32]={97,81,73,69,67}
};
#define SCL GPIO_Pin_6
#define SDA GPIO_Pin_7
static void set(uint16_t pin,bool high) { GPIO_WriteBit(GPIOB,pin,high?Bit_SET:Bit_RESET); board_delay_us(2); }
static bool high_clock(void)
{
    unsigned n=0; set(SCL,true);
    while(!GPIO_ReadInputDataBit(GPIOB,SCL)) { if(++n>100) return false; board_delay_us(1); }
    return true;
}
static void stop(void) { set(SCL,false); set(SDA,false); (void)high_clock(); set(SDA,true); }
static bool byte(uint8_t b)
{
    unsigned i; bool ack;
    for(i=0;i<8;i++) {
        set(SCL,false); set(SDA,(b&0x80)!=0); b<<=1;
        if(!high_clock()) return false;
    }
    set(SCL,false); set(SDA,true); if(!high_clock()) return false;
    ack=GPIO_ReadInputDataBit(GPIOB,SDA)==0; set(SCL,false); return ack;
}
static bool write(uint8_t control,const uint8_t *p,unsigned n)
{
    unsigned i; bool ok=true;
    set(SDA,true); if(!high_clock()) { stop(); return false; }
    set(SDA,false); set(SCL,false);
    if(!byte((uint8_t)(BOARD_OLED_ADDRESS<<1)) || !byte(control)) ok=false;
    for(i=0;i<n && ok;i++) ok=byte(p[i]);
    stop(); return ok;
}
bool board_display_line(unsigned row,const char *text)
{
    uint8_t command[3],line[128]; unsigned i,j,c; bool ended=false;
    if(row>7) return false;
    command[0]=(uint8_t)(0xb0+row); command[1]=0; command[2]=0x10;
    for(i=0;i<16;i++) {
        if(!ended && !*text) ended=true;
        c=ended?' ':(unsigned char)*text++;
        if(c<32 || c>90) c='?';
        for(j=0;j<8;j++) line[i*8+j]=j<5?glyph[c-32][j]:0;
    }
    return write(0,command,3) && write(0x40,line,sizeof line);
}
bool board_display_init(void)
{
    GPIO_InitTypeDef g; unsigned row;
    static const uint8_t init[]={0xae,0xd5,0x80,0xa8,0x3f,0xd3,0,0x40,0x8d,0x14,0x20,0x02,0xa1,0xc8,0xda,0x12,0x81,0x7f,0xd9,0xf1,0xdb,0x40,0xa4,0xa6,0xaf};
    g.GPIO_Pin=SCL|SDA; g.GPIO_Mode=GPIO_Mode_Out_OD; g.GPIO_Speed=GPIO_Speed_2MHz; GPIO_Init(GPIOB,&g);
    set(SDA,true); set(SCL,true);
    /* Clock a stuck slave out, then STOP. */
    for(row=0;row<9;row++) { set(SCL,false); (void)high_clock(); } stop();
    if(!write(0,init,sizeof init)) return false;
    for(row=0;row<8;row++) if(!board_display_line(row,"")) return false;
    return true;
}
