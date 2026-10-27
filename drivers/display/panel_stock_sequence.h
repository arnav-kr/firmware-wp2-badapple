#ifndef PRO2_PANEL_STOCK_SEQUENCE_H
#define PRO2_PANEL_STOCK_SEQUENCE_H
/* Physical pin levels recovered from stock 100f811a..100f815e. */
enum pro2_panel_line { PRO2_PANEL_RESET, PRO2_PANEL_POWER };
typedef void (*pro2_panel_drive_fn)(void *, enum pro2_panel_line, unsigned);
typedef void (*pro2_panel_wait_fn)(void *, unsigned);
typedef void (*pro2_panel_cmd_fn)(void *, unsigned);
static inline void pro2_panel_stock_on(void *ctx, pro2_panel_drive_fn drive,
                                     pro2_panel_wait_fn wait, unsigned pulse, unsigned settle)
{
    drive(ctx, PRO2_PANEL_RESET, 0);
    drive(ctx, PRO2_PANEL_POWER, 1);
    wait(ctx, pulse);
    drive(ctx, PRO2_PANEL_RESET, 1);
    wait(ctx, pulse);
    drive(ctx, PRO2_PANEL_RESET, 0);
    wait(ctx, pulse);
    drive(ctx, PRO2_PANEL_RESET, 1);
    wait(ctx, settle);
}
static inline void pro2_panel_stock_sleep(void *ctx, pro2_panel_cmd_fn command,
                                        pro2_panel_wait_fn wait)
{
    command(ctx, 0x28);
    wait(ctx, 20);
    command(ctx, 0x10);
    wait(ctx, 120);
}
/* Called after the common td_slpin delay, as in stock 100f7c78..100f7c98. */
static inline void pro2_panel_stock_off(void *ctx, pro2_panel_drive_fn drive,
                                      pro2_panel_wait_fn wait)
{
    drive(ctx, PRO2_PANEL_RESET, 0);
    wait(ctx, 20);
    drive(ctx, PRO2_PANEL_POWER, 0);
    wait(ctx, 20);
}
#endif
