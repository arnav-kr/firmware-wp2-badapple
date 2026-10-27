#include <os_common_api.h>
#include <app_manager.h>
#include "app_ui.h"
#include "app_switch.h"
#include "app_defines.h"
#include "system_app.h"
extern int ota_app_init(void);

int system_app_launch_init(void)
{
    const char *ids[] = app_id_list;
    int ret = app_switch_init(ids, ARRAY_SIZE(ids));
    if (ret) return ret;
    return ota_app_init();
}
int system_app_launch(uint8_t mode)
{
    app_switch_add_app(APP_ID_OTA);
    return app_switch(APP_ID_OTA, APP_SWITCH_CURR, true);
}
