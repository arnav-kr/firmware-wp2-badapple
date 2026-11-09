/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include "panel_icna3310b.h" /* Shared DCS and QSPI command encodings. */
#include "panel_device.h"
#include "panel_stock_sequence.h"
#include "panel_stock_driver.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *  FUNCTIONS
 **********************/
static void _panel_transmit(const struct device *dev, uint32_t cmd,
        const uint8_t *tx_data, size_t tx_count)
{
    struct lcd_panel_data *data = dev->data;

    assert(data->transfering == 0);

    display_controller_write_config(data->lcdc_dev, DDIC_QSPI_CMD_WR(cmd), tx_data, tx_count);
}

static void _panel_read(const struct device *dev, uint32_t cmd,
        uint8_t *rx_data, size_t rx_count)
{
    struct lcd_panel_data *data = dev->data;

    assert(data->transfering == 0);

    display_controller_read_config(data->lcdc_dev, DDIC_QSPI_CMD_RD(cmd), rx_data, rx_count);
}

static inline void _panel_transmit_cmd(const struct device *dev, uint32_t cmd)
{
    _panel_transmit(dev, cmd, NULL, 0);
}

static inline void _panel_transmit_p1(const struct device *dev, uint32_t cmd, uint8_t tx_data)
{
    _panel_transmit(dev, cmd, &tx_data, 1);
}

static void _panel_exit_sleep(const struct device *dev)
{
    struct lcd_panel_data *data = dev->data;

    _panel_transmit_cmd(dev, DDIC_CMD_SLPOUT);
    k_msleep(80);

    data->in_sleep = 0;
}

static int _panel_init(const struct device *dev)
{
    const struct lcd_panel_config *config = lcd_panel_get_config(dev);

    /* JX07003 CO5300 power/refresh settings (55 Hz) */
    _panel_transmit_p1(dev, 0xFE, 0x20);
    _panel_transmit_p1(dev, 0xF4, 0x5A);
    _panel_transmit_p1(dev, 0xF5, 0x59);

    _panel_transmit_p1(dev, 0xFE, 0x40);
    _panel_transmit_p1(dev, 0x39, 0x24);
    _panel_transmit_p1(dev, 0x3D, 0x08);
    _panel_transmit_p1(dev, 0x3F, 0x00);
    _panel_transmit_p1(dev, 0x40, 0x2B);

    _panel_transmit_p1(dev, 0xFE, 0x70);
    _panel_transmit_p1(dev, 0x65, 0x07);
    _panel_transmit_p1(dev, 0x66, 0x4E);
    _panel_transmit_p1(dev, 0x67, 0x10);

    _panel_transmit_p1(dev, 0xFE, 0x70);
    _panel_transmit_p1(dev, 0x8F, 0xF1);
    _panel_transmit_p1(dev, 0x90, 0x48);
    _panel_transmit_p1(dev, 0x91, 0x80);

    _panel_transmit_p1(dev, 0xFE, 0x20);
    _panel_transmit_p1(dev, 0xF4, 0x5A);
    _panel_transmit_p1(dev, 0xF5, 0x59);

    _panel_transmit_p1(dev, 0xFE, 0xD0);
    _panel_transmit_p1(dev, 0x21, 0x55);
    _panel_transmit_p1(dev, 0x3A, 0x07);
    _panel_transmit_p1(dev, 0x3B, 0x34);
    _panel_transmit_p1(dev, 0x1C, 0x10);

    _panel_transmit_p1(dev, 0xFE, 0x20);
    _panel_transmit_p1(dev, 0xF4, 0xA5);
    _panel_transmit_p1(dev, 0xF5, 0xA5);

    _panel_transmit_p1(dev, 0xFE, 0x00);
    _panel_transmit_p1(dev, 0xC4, 0x80);

    if (config->videomode.pixel_format == PIXEL_FORMAT_BGR_565) {
        _panel_transmit_p1(dev, DDIC_CMD_COLMOD, 0x55); /* rgb565 */
    } else {
        _panel_transmit_p1(dev, DDIC_CMD_COLMOD, 0x77); /* rgb888 */
    }

    /* TE on */
    _panel_transmit_p1(dev, DDIC_CMD_TEON, 0x00);

    _panel_transmit_p1(dev, DDIC_CMD_WRCTRLD, 0x20);
    const struct lcd_panel_data *data=dev->data;
    _panel_transmit_p1(dev, DDIC_CMD_WRDISBV, data->pending_brightness);
    _panel_transmit_p1(dev, DDIC_CMD_WRHBMDISBV, 0xFF);

    /* 466x466 panel, 8 px X offset (stock init: 2A 00 08 01 D9) */
    {
        const uint8_t caset[] = { 0x00, 0x08, 0x01, 0xD9 };
        const uint8_t raset[] = { 0x00, 0x00, 0x01, 0xD1 };

        _panel_transmit(dev, DDIC_CMD_CASET, caset, sizeof(caset));
        _panel_transmit(dev, DDIC_CMD_RASET, raset, sizeof(raset));
    }

    _panel_exit_sleep(dev);

    return 0;
}

static int _panel_set_brightness(const struct device *dev, uint8_t brightness)
{
    _panel_transmit_p1(dev, DDIC_CMD_WRDISBV, brightness);
    return 0;
}

static int _panel_set_mem_area(const struct device *dev, uint16_t x,
                 uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t cmd_data[2];

    x += CONFIG_PANEL_MEM_OFFSET_X;
    y += CONFIG_PANEL_MEM_OFFSET_Y;

    cmd_data[0] = sys_cpu_to_be16(x);
    cmd_data[1] = sys_cpu_to_be16(x + w - 1);
    _panel_transmit(dev, DDIC_CMD_CASET, (uint8_t *)&cmd_data[0], 4);

    cmd_data[0] = sys_cpu_to_be16(y);
    cmd_data[1] = sys_cpu_to_be16(y + h - 1);
    _panel_transmit(dev, DDIC_CMD_RASET, (uint8_t *)&cmd_data[0], 4);

    return 0;
}

static void pro2_panel_command(void *ctx,unsigned command)
{ _panel_transmit_cmd(ctx,command); }

static int _panel_blanking_on(const struct device *dev)
{
    pro2_panel_stock_sleep((void *)dev,pro2_panel_command,pro2_panel_wait_ms);
    return 0;
}

static int _panel_blanking_off(const struct device *dev)
{
    struct lcd_panel_data *data = dev->data;

    if (data->in_sleep)
        _panel_exit_sleep(dev);

    _panel_transmit_cmd(dev, DDIC_CMD_DISPON);
    return 0;
}

static int _panel_lowpower_enter(const struct device *dev)
{
    _panel_transmit_p1(dev, 0xFE, 0x00);
    _panel_transmit_cmd(dev, DDIC_CMD_IDMON);
    return 0;
}

static int _panel_lowpower_exit(const struct device *dev)
{
    _panel_transmit_p1(dev, 0xFE, 0x00);
    _panel_transmit_cmd(dev, DDIC_CMD_IDMOFF);
    return 0;
}

volatile uint32_t pro2_panel_checks;
volatile uint32_t pro2_panel_failures;
volatile uint32_t pro2_panel_status;
/* Stock 0x100f8800: register 0x0F == 0xC1 is a failure.
 * Read every status register before deciding, as stock does. */
static int _panel_check_esd(const struct device *dev)
{
    uint8_t pm=0, diagnostic=0, hbm=0, image=0;
    _panel_transmit_p1(dev, 0xFE, 0x00);
    _panel_read(dev, 0x0A, &pm, 1);
    _panel_read(dev, 0x0F, &diagnostic, 1);
    _panel_read(dev, 0x66, &hbm, 1);
    _panel_read(dev, 0x0D, &image, 1);
    pro2_panel_checks++;
    pro2_panel_status = pm | ((uint32_t)diagnostic << 8) |
                         ((uint32_t)hbm << 16) | ((uint32_t)image << 24);
    if ((pm & 0xBF) != 0x9C || diagnostic == 0xC1 || hbm || image) {
        pro2_panel_failures++;
        printk("PRO2 panel check failed: 0A=%02x 0F=%02x 66=%02x 0D=%02x\n",
               pm, diagnostic, hbm, image);
        return -EIO;
    }
    return 0;
}

static const struct lcd_panel_ops lcd_panel_ops = {
    .init = _panel_init,
    .blanking_on = _panel_blanking_on,
    .blanking_off = _panel_blanking_off,
    .lowpower_enter = _panel_lowpower_enter,
    .lowpower_exit = _panel_lowpower_exit,
    .check_esd = _panel_check_esd,
    .set_brightness = _panel_set_brightness,
    .write_prepare = _panel_set_mem_area,
};

const struct lcd_panel_config lcd_panel_co5300_jx07003_config = {
    .videoport = PANEL_VIDEO_PORT_INITIALIZER,
    .videomode = PANEL_VIDEO_MODE_INITIALIZER,
    .ops = &lcd_panel_ops,
    .cmd_ramwr = DDIC_QSPI_CMD_RAMWR(DDIC_CMD_RAMWR),
    .cmd_ramwc = DDIC_QSPI_CMD_RAMWR(DDIC_CMD_RAMWRC),
    .tw_reset = 20,
    .ts_reset = 20,
    .td_slpin = 100,
};
