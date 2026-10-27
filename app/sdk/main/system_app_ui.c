#include <app_ui.h>
#include <lvgl/lvgl_view.h>
#include <view_stack.h>
#include <ui_service.h>
int system_app_ui_init(void)
{
    lvgl_view_system_init();
    ui_service_register_gesture_default_callback();
    view_stack_init();
    return 0;
}
