#include <assert.h>
#include <stdio.h>
#include "../drivers/display/panel_stock_sequence.h"
struct event {unsigned kind,value;};
#include "expected_stock_panel_trace.h"
static struct event events[32];static unsigned n;
static void drive(void *p,enum pro2_panel_line line,unsigned value){(void)p;assert(n<32);events[n++]=(struct event){line==PRO2_PANEL_RESET?0:1,value};}
static void wait(void *p,unsigned ms){(void)p;assert(n<32);events[n++]=(struct event){2,ms};}
static void cmd(void *p,unsigned value){(void)p;assert(n<32);events[n++]=(struct event){3,value};}
int main(void){
 for(unsigned cycle=0;cycle<1000;cycle++){
  n=0;pro2_panel_stock_on(NULL,drive,wait,20,20);
  assert(n==1+sizeof(expected_on)/sizeof(expected_on[0]));assert(events[0].kind==0 && events[0].value==0);
  for(unsigned i=0;i<sizeof(expected_on)/sizeof(expected_on[0]);i++){assert(events[i+1].kind==expected_on[i].kind);assert(events[i+1].value==expected_on[i].value);}
  n=0;pro2_panel_stock_sleep(NULL,cmd,wait);wait(NULL,100);pro2_panel_stock_off(NULL,drive,wait);
  const struct event expected_off[]={{3,0x28},{2,20},{3,0x10},{2,120},{2,100},{0,0},{2,20},{1,0},{2,20}};
  assert(n==sizeof(expected_off)/sizeof(expected_off[0]));unsigned total=0;
  for(unsigned i=0;i<n;i++){assert(events[i].kind==expected_off[i].kind && events[i].value==expected_off[i].value);if(events[i].kind==2)total+=events[i].value;}
  assert(total==280);
 }
 puts("1000 lifecycle cycles: power-on matches compiled stock waveform; shutdown ordering and280ms timing passed.");return 0;
}
