#include "board.h"
#include "runtime.h"
#include "health.h"
#include "timing.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "text.h"
#include <string.h>
enum { TASK_SENSOR,TASK_CONTROL,TASK_UI,TASK_COMM,TASK_STORAGE,TASK_COUNT };
typedef struct { KitchenSample sample; KitchenOutput output; bool manual,dirty; } View;
typedef struct { KitchenConfig config; uint32_t revision; uint16_t seq; } SaveRequest;
typedef struct { uint32_t revision; uint16_t seq; bool ok; } SaveResult;
static QueueHandle_t samples,commands,replies,views,saves,saved,key_events;
static TaskHandle_t handles[TASK_COUNT];
static volatile uint32_t heartbeat[TASK_COUNT];
static volatile uint32_t sensor_errors,command_drops,reply_drops,tx_errors,display_errors;
static volatile uint32_t parser_crc_errors,parser_timeouts,parser_length_errors;
static TimingStats control_timing;
static StaticTask_t tcbs[TASK_COUNT+1],idle_tcb;
static StackType_t sensor_stack[256],control_stack[384],ui_stack[384];
static StackType_t comm_stack[448],storage_stack[256],health_stack[192],idle_stack[128];
static StaticQueue_t queue_blocks[7];
static uint8_t sample_bytes[sizeof(KitchenSample)],command_bytes[4*sizeof(WireFrame)];
static uint8_t reply_bytes[4*sizeof(WireFrame)],view_bytes[sizeof(View)];
static uint8_t save_bytes[sizeof(SaveRequest)],saved_bytes[sizeof(SaveResult)],key_bytes[4];
static volatile uint16_t stack_water[TASK_COUNT];
static KitchenRuntime runtime;
static ConfigStore store;
static bool valid_config;
static void count_drop(void) { taskENTER_CRITICAL(); command_drops++; taskEXIT_CRITICAL(); }
static void beat(unsigned id) { heartbeat[id]=board_millis(); }
static void send_reply(WireFrame *a)
{ if(xQueueSend(replies,a,0)!=pdPASS) reply_drops++; }
static void sensor_task(void *arg)
{
    TickType_t next=xTaskGetTickCount(); KitchenSample s={0}; uint32_t dht_at=0;
    bool dht_ok=false,attempted=false; (void)arg;
    for(;;) {
        uint32_t now=board_millis(); bool adc_ok;
        if(!attempted || (uint32_t)(now-dht_at)>=2000) {
            attempted=true;
            dht_ok=board_dht_read(&s.temp,&s.humid); dht_at=board_millis();
            if(dht_ok) s.climate_ms=dht_at;
            if(!dht_ok) sensor_errors++;
        }
        adc_ok=board_adc_read(&s.light); if(!adc_ok) sensor_errors++;
        if(adc_ok) s.light_ms=board_millis();
        s.climate_valid=dht_ok; s.light_valid=adc_ok;
        (void)xQueueOverwrite(samples,&s); beat(TASK_SENSOR);
        vTaskDelayUntil(&next,pdMS_TO_TICKS(100));
    }
}
static void control_task(void *arg)
{
    KitchenSample s; WireFrame q,a; SaveRequest sr; SaveResult result; View v;
    TickType_t wake=xTaskGetTickCount();
    bool saving=false; uint32_t last_save=0; bool ever_saved=false; unsigned n,j; uint8_t key;
    (void)arg;
    runtime_log(&runtime,board_millis(),5,valid_config?1:0);
    for(;;) {
        uint32_t started_cycles=board_cycles(); uint32_t now=board_millis();
        timing_start(&control_timing,started_cycles);
        if(xQueueReceive(samples,&s,0)==pdPASS) runtime_tick(&runtime,&s,now);
        else runtime_tick(&runtime,NULL,now);
        if(xQueueReceive(saved,&result,0)==pdPASS) {
            saving=false;
            if(result.ok && result.revision==runtime.revision) runtime.dirty=false;
            runtime_log(&runtime,now,6,result.ok?1:0);
            memset(&a,0,sizeof a); a.type=CMD_SAVE|0x80; a.seq=result.seq; a.length=1;
            a.data[0]=result.ok?RESULT_OK:RESULT_IO; send_reply(&a);
        }
        if(xQueueReceive(key_events,&key,0)==pdPASS && (key&1)) {
            kitchen_ack(&runtime.control,now); runtime_log(&runtime,now,4,0);
        }
        /* Bounded command batch prevents floods from starving local control. */
        for(n=0;n<4 && xQueueReceive(commands,&q,0)==pdPASS;n++) {
            if(q.type==CMD_SAVE) {
                memset(&a,0,sizeof a); a.type=CMD_SAVE|0x80; a.seq=q.seq; a.length=1;
                if(q.length) a.data[0]=RESULT_LENGTH;
                else if(saving) a.data[0]=RESULT_BUSY;
                else if(!runtime.dirty) a.data[0]=RESULT_OK;
                else if(ever_saved && (uint32_t)(now-last_save)<10000) a.data[0]=RESULT_BUSY;
                else {
                    sr.config=runtime.control.cfg; sr.revision=runtime.revision; sr.seq=q.seq;
                    if(xQueueSend(saves,&sr,0)==pdPASS) { saving=true; ever_saved=true; last_save=now; continue; }
                    a.data[0]=RESULT_BUSY;
                }
                send_reply(&a);
            } else if(q.type==CMD_DIAG) {
                memset(&a,0,sizeof a); a.type=CMD_DIAG|0x80; a.seq=q.seq; a.length=1;
                if(q.length) a.data[0]=RESULT_LENGTH;
                else {
                    a.length=59; wire_put32(a.data+1,board_reset_flags());
                    wire_put32(a.data+5,0);
                    wire_put32(a.data+9,0); wire_put32(a.data+13,sensor_errors);
                    wire_put32(a.data+17,board_uart_drops()); wire_put32(a.data+21,board_uart_errors());
                    wire_put32(a.data+25,command_drops); wire_put32(a.data+29,reply_drops);
                    wire_put32(a.data+33,tx_errors); wire_put32(a.data+37,display_errors);
                    wire_put32(a.data+41,parser_crc_errors); wire_put32(a.data+45,parser_timeouts);
                    for(j=0;j<TASK_COUNT;j++) wire_put16(a.data+49+j*2,stack_water[j]);
                }
                send_reply(&a);
            } else if(q.type==10 || q.type==11) {
                CrashRecord record;
                memset(&a,0,sizeof a); a.type=q.type|0x80; a.seq=q.seq; a.length=1;
                if(q.length) a.data[0]=RESULT_LENGTH;
                else if(q.type==11) board_crash_clear();
                else {
                    a.length=90; a.data[1]=board_crash_read(&record)?1:0;
                    if(a.data[1]) memcpy(a.data+2,&record,sizeof record);
                }
                send_reply(&a);
            } else if(q.type==9) {
                memset(&a,0,sizeof a); a.type=0x89; a.seq=q.seq; a.length=1;
                if(q.length) a.data[0]=RESULT_LENGTH;
                else {
                    uint32_t unit=board_cycles_per_ms()/1000u; a.length=25;
                    wire_put32(a.data+1,control_timing.cycles);
                    wire_put32(a.data+5,control_timing.max_interval/unit);
                    wire_put32(a.data+9,control_timing.max_execution/unit);
                    wire_put32(a.data+13,control_timing.late_intervals);
                    wire_put32(a.data+17,control_timing.overruns);
                    wire_put32(a.data+21,2); /* build ABI v2: static allocation, journal */
                }
                send_reply(&a);
            } else { runtime_command(&runtime,&q,&a,now); send_reply(&a); }
        }
        runtime_tick(&runtime,NULL,board_millis()); board_apply(runtime.output);
        v.sample=runtime.sample; v.output=runtime.output; v.manual=runtime.manual; v.dirty=runtime.dirty;
        (void)xQueueOverwrite(views,&v); beat(TASK_CONTROL);
        timing_finish(&control_timing,started_cycles,board_cycles());
        vTaskDelayUntil(&wake,pdMS_TO_TICKS(20));
    }
}
static void comm_task(void *arg)
{
    WireParser parser; WireFrame q,a; uint8_t b,encoded[138]; unsigned count;
    uint32_t old_drops=0,old_errors=0; (void)arg; wire_init(&parser);
    for(;;) {
        uint32_t now=board_millis();
        if(old_drops!=board_uart_drops() || old_errors!=board_uart_errors()) {
            parser.used=0; old_drops=board_uart_drops(); old_errors=board_uart_errors();
        }
        for(count=0;count<256 && board_uart_pop(&b);count++) {
            if(old_drops!=board_uart_drops() || old_errors!=board_uart_errors()) {
                parser.used=0; old_drops=board_uart_drops(); old_errors=board_uart_errors();
            }
            if(wire_feed(&parser,b,now,&q)) {
                if(xQueueSend(commands,&q,0)!=pdPASS) {
                    count_drop(); memset(&a,0,sizeof a); a.type=q.type|0x80; a.seq=q.seq;
                    a.length=1; a.data[0]=RESULT_BUSY;
                    if(!board_uart_send(encoded,wire_encode(&a,encoded,sizeof encoded))) tx_errors++;
                }
            }
        }
        wire_expire(&parser,now); parser_crc_errors=parser.bad_crc;
        parser_timeouts=parser.timeouts; parser_length_errors=parser.bad_length;
        if(xQueueReceive(replies,&a,0)==pdPASS)
            if(!board_uart_send(encoded,wire_encode(&a,encoded,sizeof encoded))) tx_errors++;
        beat(TASK_COMM); vTaskDelay(pdMS_TO_TICKS(5));
    }
}
static void ui_task(void *arg)
{
    TickType_t next=xTaskGetTickCount(); View v={0}; char text[24];
    uint8_t candidate=0,stable=0; unsigned equal=0,frame=0,page=0; bool oled;
    (void)arg; oled=board_display_init(); if(!oled) display_errors++;
    for(;;) {
        uint8_t keys=board_keys();
        if(keys!=candidate) { candidate=keys; equal=0; }
        else if(equal<2) equal++;
        if(equal==2 && stable!=candidate) {
            uint8_t pressed=(uint8_t)(candidate&~stable); stable=candidate;
            if(pressed&1) {
                uint8_t event=1;
                if(xQueueSend(key_events,&event,0)!=pdPASS) count_drop();
            }
            if(pressed&2) page=(page+1)%2;
        }
        (void)xQueueReceive(views,&v,0);
        if(++frame%10==0) {
            if(!oled && frame%100==0) oled=board_display_init();
            if(oled) {
                bool ok=true;
                ok=board_display_line(0,page?"SYSTEM DIAG":"KITCHEN RTOS") && ok;
                if(!page) {
                    (void)text_format(text,sizeof text,"T:%d H:%u",(int)v.sample.temp/10,(unsigned)v.sample.humid/10);
                    ok=board_display_line(2,text) && ok;
                    (void)text_format(text,sizeof text,"L:%u %s",(unsigned)v.sample.light,v.manual?"MAN":"AUTO");
                    ok=board_display_line(4,text) && ok;
                    (void)text_format(text,sizeof text,"%s V:%u B:%u",v.output.sensor_fault?"FAULT":v.output.hot?"HOT":"OK",v.output.ventilation?1u:0u,v.output.buzzer?1u:0u);
                } else {
                    (void)text_format(text,sizeof text,"STACK C:%u",(unsigned)stack_water[TASK_CONTROL]); ok=board_display_line(2,text) && ok;
                    (void)text_format(text,sizeof text,"RX DROP:%lu",(unsigned long)board_uart_drops()); ok=board_display_line(4,text) && ok;
                    (void)text_format(text,sizeof text,"%s E:%lu",v.dirty?"UNSAVED":"SAVED",(unsigned long)sensor_errors);
                }
                ok=board_display_line(6,text) && ok;
                if(!ok) { display_errors++; oled=false; }
            }
        }
        beat(TASK_UI); vTaskDelayUntil(&next,pdMS_TO_TICKS(20));
    }
}
static void storage_task(void *arg)
{
    SaveRequest q; SaveResult a; (void)arg;
    for(;;) {
        if(xQueueReceive(saves,&q,pdMS_TO_TICKS(100))==pdPASS) {
            a.revision=q.revision; a.seq=q.seq; a.ok=config_save(&store,&q.config);
            if(xQueueSend(saved,&a,0)!=pdPASS) board_panic(20);
        }
        beat(TASK_STORAGE);
    }
}
static void health_task(void *arg)
{
    KitchenHealth monitor; uint32_t last[5]; unsigned i; bool captured=false; (void)arg;
    health_init(&monitor,board_millis()); board_watchdog_start();
    for(;;) {
        uint32_t now=board_millis(); bool healthy=true;
        for(i=0;i<TASK_COUNT;i++) {
            last[i]=heartbeat[i];
            stack_water[i]=(uint16_t)uxTaskGetStackHighWaterMark(handles[i]);
            if(stack_water[i]<32) healthy=false;
        }
        { uint32_t stale=health_stale(&monitor,last,now);
          if(stale) { healthy=false; if(!captured) { board_capture_health(stale); captured=true; } } }
        if(healthy) captured=false;
        if(healthy) board_watchdog_feed();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
void app_assert(const char *file,unsigned line) { (void)file; board_panic(0x10000u|line); }
void vApplicationGetIdleTaskMemory(StaticTask_t **tcb,StackType_t **stack,uint32_t *words)
{ *tcb=&idle_tcb; *stack=idle_stack; *words=128; }
void vApplicationStackOverflowHook(TaskHandle_t task,char *name) { (void)task; (void)name; board_panic(31); }
int main(void)
{
    KitchenConfig config; ConfigBackend io; TaskHandle_t health;
    board_init(); io=board_config_backend(); valid_config=config_load(&store,&io,&config);
    runtime_init(&runtime,&config); runtime.dirty=!valid_config;
    samples=xQueueCreateStatic(1,sizeof(KitchenSample),sample_bytes,&queue_blocks[0]);
    commands=xQueueCreateStatic(4,sizeof(WireFrame),command_bytes,&queue_blocks[1]);
    replies=xQueueCreateStatic(4,sizeof(WireFrame),reply_bytes,&queue_blocks[2]);
    views=xQueueCreateStatic(1,sizeof(View),view_bytes,&queue_blocks[3]);
    saves=xQueueCreateStatic(1,sizeof(SaveRequest),save_bytes,&queue_blocks[4]);
    saved=xQueueCreateStatic(1,sizeof(SaveResult),saved_bytes,&queue_blocks[5]);
    key_events=xQueueCreateStatic(4,1,key_bytes,&queue_blocks[6]);
    configASSERT(samples && commands && replies && views && saves && saved && key_events);
    handles[TASK_SENSOR]=xTaskCreateStatic(sensor_task,"sensor",256,NULL,4,sensor_stack,&tcbs[TASK_SENSOR]);
    handles[TASK_CONTROL]=xTaskCreateStatic(control_task,"control",384,NULL,5,control_stack,&tcbs[TASK_CONTROL]);
    handles[TASK_UI]=xTaskCreateStatic(ui_task,"ui",384,NULL,2,ui_stack,&tcbs[TASK_UI]);
    handles[TASK_COMM]=xTaskCreateStatic(comm_task,"comm",448,NULL,3,comm_stack,&tcbs[TASK_COMM]);
    handles[TASK_STORAGE]=xTaskCreateStatic(storage_task,"storage",256,NULL,1,storage_stack,&tcbs[TASK_STORAGE]);
    health=xTaskCreateStatic(health_task,"health",192,NULL,6,health_stack,&tcbs[TASK_COUNT]);
    configASSERT(handles[0] && handles[1] && handles[2] && handles[3] && handles[4] && health);
    timing_init(&control_timing,5*board_cycles_per_ms(),25*board_cycles_per_ms());
    vTaskStartScheduler(); board_panic(32); return 0;
}
