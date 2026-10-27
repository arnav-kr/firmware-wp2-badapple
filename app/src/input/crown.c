/* MT3511 driver ported from stock, without calibration writes to NVRAM. */
#include <kernel.h>
#include <device.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <property_manager.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "media_controls.h"
#include "mt3511_init.h"
#define MT_ADDRESS 0x75
static const struct device *bus,*gpio;
static struct k_work_q knob_queue;
K_THREAD_STACK_DEFINE(knob_stack,2048);
static struct k_delayed_work knob_work;
static struct pro2_motion_state motion;
volatile uint32_t pro2_knob_reads,pro2_knob_errors,pro2_knob_steps;
volatile int32_t pro2_knob_ready,pro2_knob_last_delta,pro2_knob_last_rc;
volatile uint32_t pro2_knob_failed_reg,pro2_knob_init_phase;
volatile int32_t pro2_knob_reset_rc;
static unsigned init_attempts;
extern void pro2_audio_volume_steps(int steps);
static int read_reg(uint8_t reg,uint8_t *value)
{
    int rc=-EIO;
    for(unsigned attempt=0;attempt<3;attempt++) {
        rc=i2c_write_read(bus,MT_ADDRESS,&reg,1,value,1);
        if(!rc) break;
        k_msleep(10);
    }
    if(rc) pro2_knob_failed_reg=reg;
    pro2_knob_last_rc=rc;
    if (rc) pro2_knob_errors++;
    return rc;
}
static int write_reg(uint8_t reg,uint8_t value)
{
    uint8_t data[2]={reg,value};
    int rc=-EIO;
    for(unsigned attempt=0;attempt<3;attempt++) {
        rc=i2c_write(bus,data,sizeof(data),MT_ADDRESS);
        if(!rc) break;
        k_msleep(10);
    }
    if(rc) pro2_knob_failed_reg=reg;
    pro2_knob_last_rc=rc;
    if (rc) pro2_knob_errors++;
    return rc;
}
static int initialize_sensor(void)
{
    pro2_knob_init_phase=1;
    k_msleep(10);
    int rc=write_reg(6,0x80);
    pro2_knob_reset_rc=rc;
    /* Stock continues the unlock table after a reset transfer error. */
    if (rc && rc!=-EIO) return rc;
    k_msleep(10);
    pro2_knob_init_phase=2;
    for (unsigned i=0;i<ARRAY_SIZE(pro2_mt3511_init);i++) {
        rc=write_reg(pro2_mt3511_init[i][0],pro2_mt3511_init[i][1]);
        if (rc) return rc;
    }
    /* Stock 101c06e4(1) exits sensor sleep by clearing register6 bit3. */
    uint8_t power=0;
    if((rc=read_reg(6,&power))) return rc;
    if((rc=write_reg(6,power & ~8u))) return rc;
    if((rc=read_reg(6,&power))) return rc;
    if(power & 8u) return -EIO;
    pro2_knob_init_phase=3;
    char property[32]={0};
    int resolution=320;
    if (property_get("KNOB_FACTOR",property,sizeof(property)-1)>0) {
        int candidate=atoi(property);
        if (candidate>=101 && candidate<=4094) resolution=candidate;
    }
    uint8_t high=0,low=0,verify=0;
    if ((rc=read_reg(0x0f,&high))) return rc;
    if ((rc=write_reg(0x0d,(uint8_t)resolution))) return rc;
    if ((rc=write_reg(0x0f,(high&15)|((resolution>>8)<<4)))) return rc;
    if ((rc=read_reg(0x0d,&low))) return rc;
    if ((rc=read_reg(0x0f,&verify))) return rc;
    if ((((verify>>4)<<8)|low)!=resolution) return -EIO;
    return 0;
}
static void sample_motion(struct k_work *work)
{
    int delay=20;
    if (!pro2_knob_ready) {
        if (++init_attempts>5) return;
        int rc=initialize_sensor();
        pro2_knob_last_rc=rc;
        if (rc) {delay=1000;goto again;}
        pro2_knob_init_phase=4;
        pro2_knob_failed_reg=0;
        pro2_knob_ready=1;
    }
    /* The optical motion IRQ is active low. Keep I2C idle when no motion. */
    if (gpio_pin_get_raw(gpio,26)!=0) goto again;
    uint8_t status=0,x=0,y=0,high=0;
    if (read_reg(2,&status) || read_reg(3,&x) || read_reg(4,&y) || read_reg(0x12,&high)) {
        delay=100;goto again;
    }
    pro2_knob_reads++;
    int delta=pro2_knob_delta(x,high);
    pro2_knob_last_delta=delta;
    int steps=pro2_motion_volume(&motion,delta,high==0 && x==0 && y==0);
    if (steps) {
        pro2_knob_steps++;
        pro2_audio_volume_steps(steps);
    }
again:
    k_delayed_work_submit_to_queue(&knob_queue,&knob_work,K_MSEC(delay));
}
int pro2_knob_init(void)
{
    bus=device_get_binding("I2C_2");
    gpio=device_get_binding("GPIOB");
    if (!bus || !gpio) return pro2_knob_last_rc=-ENODEV;
    int rc=gpio_pin_configure(gpio,26,GPIO_INPUT|GPIO_PULL_UP);
    if (rc) return pro2_knob_last_rc=rc;
    k_work_queue_start(&knob_queue,knob_stack,K_THREAD_STACK_SIZEOF(knob_stack),8,NULL);
    k_delayed_work_init(&knob_work,sample_motion);
    k_delayed_work_submit_to_queue(&knob_queue,&knob_work,K_MSEC(3000));
    return 0;
}
