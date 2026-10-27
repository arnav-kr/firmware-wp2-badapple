#ifndef PRO2_PANEL_STOCK_DRIVER_H
#define PRO2_PANEL_STOCK_DRIVER_H
#include <stdint.h>
struct pro2_panel_diag {
 uint32_t power_on,power_off,te_resets,esd_resets,last_reason,starts,completes,errors,last_start_ms,last_complete_ms;
 uint32_t pm_state,sleeping,busy,brightness,pending,reset_raw,power_raw;
};
void pro2_panel_get_diag(struct pro2_panel_diag *out);
void pro2_panel_wait_ms(void *ctx,unsigned ms);
#endif
