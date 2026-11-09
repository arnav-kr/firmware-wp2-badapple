/* Resource-free Pro 2 bring-up view. OTA transport remains in ota_app_foreground.c. */
#include "ota_app.h"
#include <app_ui.h>
#include <stdio.h>
#include <string.h>
#include <sys/atomic.h>
#include <shell/shell.h>
#include <power_manager.h>
#include <soc_pmu.h>
#include "media_controls.h"
#include "player.h"
#include "badapple_music.h"
#include "panel_stock_driver.h"
extern volatile uint32_t pro2_gpu_blits,pro2_gpu_video_blits,pro2_gpu_errors;
extern uintptr_t pro2_video_gpu_buffer;
static bool long_touch;
static int last_scale_x,last_scale_y;

extern volatile uint32_t bringup_key_events;
extern volatile uint32_t bringup_last_key;
extern volatile uint32_t bringup_rotations;
static volatile uint8_t upgrade_progress;
static volatile uint8_t upgrade_result;
static uint32_t touches;
static lv_obj_t *root;
static lv_obj_t *status_label;
static lv_obj_t *input_label;
static lv_obj_t *bar;
static lv_obj_t *title;
static atomic_t fullscreen_requested;
static bool fullscreen_visible;
static lv_timer_t *refresh;
static lv_obj_t *video_object;
static lv_timer_t *video_timer;
extern volatile bool bringup_transfer_active;
static bool video_failed;
static struct pro2_media_button crown_state;
static atomic_t media_requests;
static bool video_paused;
extern int pro2_audio_volume(void);
static volatile uint32_t crown_presses;
static void crown_poll(struct k_timer *timer)
{
    uint32_t now=k_uptime_get_32();
    bool pressed=soc_pmu_is_onoff_key_pressed();
    int action=pro2_media_button_sample(&crown_state,pressed,now);
    if (action && now>=1800) {
        crown_presses++;
        atomic_or(&media_requests,action);
    }
}
K_TIMER_DEFINE(crown_timer,crown_poll,NULL);
#ifdef PRO2_INPUT5
extern volatile uint32_t pro2_touch_irqs,pro2_touch_reads,pro2_touch_errors,pro2_touch_xy;
extern volatile int32_t pro2_touch_id;
extern volatile bool pro2_sound_active;
extern volatile int32_t pro2_sound_result;
extern volatile uint32_t pro2_sound_bytes,pro2_sound_tests,pro2_sound_samples;
extern volatile int32_t pro2_sound_prime_rc,pro2_sound_gain_rc,pro2_sound_pa_level,pro2_sound_pa_rc;
extern volatile int32_t pro2_board_rail_rc,pro2_board_rail_level,pro2_board_rail_idat;
extern volatile uint32_t pro2_board_rail_ctl;
extern volatile int32_t pro2_pa_gpio_idat;
extern volatile uint32_t pro2_sound_pcm_blocks,pro2_sound_pcm_nonzero,pro2_sound_pcm_peak,pro2_sound_pcm_sum;
extern volatile int32_t pro2_sound_pcm_min,pro2_sound_pcm_max;
extern volatile uint32_t pro2_sound_dac_digctl,pro2_sound_dac_fifoctl,pro2_sound_dac_stat;
extern volatile uint32_t pro2_sound_dac_pcm_buf_ctl,pro2_sound_dac_pcm_buf_stat,pro2_sound_dac_pcm_buf_cnt;
extern volatile uint32_t pro2_sound_dac_vol_lch,pro2_sound_dac_vol_rch;
extern volatile uint32_t pro2_sound_dac_anactl0,pro2_sound_dac_anactl1,pro2_sound_dac_anactl2,pro2_sound_dac_bias;
extern volatile uint32_t pro2_sound_audio_ldo,pro2_sound_cmu_dacclk;
extern volatile uint32_t pro2_sound_pa_odat,pro2_sound_rail_odat,pro2_sound_dac_capture_valid;
extern volatile uint32_t pro2_knob_reads,pro2_knob_errors,pro2_knob_steps,pro2_knob_failed_reg,pro2_knob_init_phase;
extern volatile int32_t pro2_knob_ready,pro2_knob_last_delta,pro2_knob_last_rc,pro2_knob_reset_rc;
extern volatile uint32_t pro2_dma_blocks,pro2_dma_nonzero,pro2_dma_peak,pro2_dma_bytes;
extern volatile unsigned pro2_i2c2_errors,pro2_i2c2_stat,pro2_i2c2_fifostat,pro2_i2c2_scl,pro2_i2c2_sda;
extern int pro2_play_test_tone(void);
static int shell_pro2_tone(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell,"Playing 1-second 500Hz speaker test at current volume");
    int rc=pro2_play_test_tone();
    shell_print(shell,"Tone result=%d PCM bytes=%lu",rc,(unsigned long)pro2_sound_bytes);
    return rc;
}
#endif
extern volatile uint32_t pro2_panel_checks;
extern volatile uint32_t pro2_panel_failures;
extern volatile uint32_t pro2_panel_status;

void bringup_toggle_fullscreen(void)
{
    atomic_xor(&fullscreen_requested, 1);
}

static int shell_pro2_status(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "PRO2 control13 uptime=%lu battery_mv=%d capacity=%d dc5v=%d charge=%d",
        (unsigned long)k_uptime_get_32(), power_manager_get_battery_vol()/1000,
        power_manager_get_battery_capacity(), power_manager_get_dc5v_status(),
        power_manager_get_charge_status());
    uint32_t pixel_sample=0;
    for(unsigned i=0;i<64;i++) pixel_sample=(pixel_sample<<5) ^ video_pixels[i*(VIDEO_WIDTH*VIDEO_HEIGHT-1)/63];
    shell_print(shell,"video source_sample=%08lx failed=%d paused=%d",(unsigned long)pixel_sample,video_failed,video_paused);
    struct pro2_panel_diag pd;pro2_panel_get_diag(&pd);
    shell_print(shell,"panel life on=%lu off=%lu TE=%lu ESD=%lu reason=%lu",
        (unsigned long)pd.power_on,(unsigned long)pd.power_off,(unsigned long)pd.te_resets,(unsigned long)pd.esd_resets,(unsigned long)pd.last_reason);
    shell_print(shell,"panel IO start=%lu done=%lu error=%lu last=%lu/%lu state=%lu sleep=%lu busy=%lu brightness=%lu/%lu reset_raw=%lu power_raw=%lu",
        (unsigned long)pd.starts,(unsigned long)pd.completes,(unsigned long)pd.errors,(unsigned long)pd.last_start_ms,(unsigned long)pd.last_complete_ms,
        (unsigned long)pd.pm_state,(unsigned long)pd.sleeping,(unsigned long)pd.busy,(unsigned long)pd.brightness,(unsigned long)pd.pending,(unsigned long)pd.reset_raw,(unsigned long)pd.power_raw);
    shell_print(shell, "panel checks=%lu failures=%lu regs_0D_66_0F_0A=%08lx frame=%lu fullscreen=%d",
        (unsigned long)pro2_panel_checks, (unsigned long)pro2_panel_failures,
        (unsigned long)pro2_panel_status, (unsigned long)video_frame,
        (int)atomic_get(&fullscreen_requested));
    shell_print(shell,"crown presses=%lu raw_events=%lu last_key=%08lx level=%d",
        (unsigned long)crown_presses,(unsigned long)bringup_key_events,
        (unsigned long)bringup_last_key,crown_state.debounce.stable);
#ifdef PRO2_INPUT5
    shell_print(shell,"touch chip=%ld irq=%lu reads=%lu errors=%lu xy=%08lx taps=%lu",
        (long)pro2_touch_id,(unsigned long)pro2_touch_irqs,(unsigned long)pro2_touch_reads,
        (unsigned long)pro2_touch_errors,(unsigned long)pro2_touch_xy,(unsigned long)touches);
    shell_print(shell,"sound tests=%lu result=%ld bytes=%lu active=%d",
        (unsigned long)pro2_sound_tests,(long)pro2_sound_result,
        (unsigned long)pro2_sound_bytes,pro2_sound_active);
    shell_print(shell,"dma blocks=%lu bytes=%lu nz=%lu peak=%lu reset_rc=%ld",(unsigned long)pro2_dma_blocks,(unsigned long)pro2_dma_bytes,(unsigned long)pro2_dma_nonzero,(unsigned long)pro2_dma_peak,(long)pro2_knob_reset_rc);
    shell_print(shell,"pcm blocks=%lu nz=%lu min=%ld max=%ld peak=%lu sum=%lu",
        (unsigned long)pro2_sound_pcm_blocks,(unsigned long)pro2_sound_pcm_nonzero,
        (long)pro2_sound_pcm_min,(long)pro2_sound_pcm_max,
        (unsigned long)pro2_sound_pcm_peak,(unsigned long)pro2_sound_pcm_sum);
    shell_print(shell,"dac dig=%08lx fifo=%08lx stat=%08lx volL=%08lx volR=%08lx",
        (unsigned long)pro2_sound_dac_digctl,(unsigned long)pro2_sound_dac_fifoctl,
        (unsigned long)pro2_sound_dac_stat,(unsigned long)pro2_sound_dac_vol_lch,
        (unsigned long)pro2_sound_dac_vol_rch);
    shell_print(shell,"dac ana0=%08lx ana1=%08lx ana2=%08lx bias=%08lx",
        (unsigned long)pro2_sound_dac_anactl0,(unsigned long)pro2_sound_dac_anactl1,
        (unsigned long)pro2_sound_dac_anactl2,(unsigned long)pro2_sound_dac_bias);
    shell_print(shell,"dac pcmctl=%08lx pcmstat=%08lx pcmcnt=%08lx ldo=%08lx clk=%08lx cap=%lu",
        (unsigned long)pro2_sound_dac_pcm_buf_ctl,(unsigned long)pro2_sound_dac_pcm_buf_stat,
        (unsigned long)pro2_sound_dac_pcm_buf_cnt,(unsigned long)pro2_sound_audio_ldo,
        (unsigned long)pro2_sound_cmu_dacclk,(unsigned long)pro2_sound_dac_capture_valid);
    shell_print(shell,"odat A21=%08lx A31=%08lx idat A21=%ld A31=%ld",
        (unsigned long)pro2_sound_pa_odat,(unsigned long)pro2_sound_rail_odat,
        (long)pro2_pa_gpio_idat,(long)pro2_board_rail_idat);
#endif
    shell_print(shell,"rail rc=%ld level=%ld ctl=%08lx PA_during_tone=%ld PA_rc=%ld",(long)pro2_board_rail_rc,(long)pro2_board_rail_level,(unsigned long)pro2_board_rail_ctl,(long)pro2_sound_pa_level,(long)pro2_sound_pa_rc);
    shell_print(shell,"audio samples=%lu prime=%ld gain=%ld volume=%d paused=%d",
        (unsigned long)pro2_sound_samples,(long)pro2_sound_prime_rc,(long)pro2_sound_gain_rc,pro2_audio_volume(),video_paused);
    shell_print(shell,"i2c2 errors=%u stat=%08x fifo=%08x scl=%08x sda=%08x",pro2_i2c2_errors,pro2_i2c2_stat,pro2_i2c2_fifostat,pro2_i2c2_scl,pro2_i2c2_sda);
    shell_print(shell,"knob phase=%lu failed_reg=%02lx",(unsigned long)pro2_knob_init_phase,(unsigned long)pro2_knob_failed_reg);
    shell_print(shell,"knob ready=%ld reads=%lu errors=%lu steps=%lu delta=%ld rc=%ld",
        (long)pro2_knob_ready,(unsigned long)pro2_knob_reads,(unsigned long)pro2_knob_errors,
        (unsigned long)pro2_knob_steps,(long)pro2_knob_last_delta,(long)pro2_knob_last_rc);
    shell_print(shell,"music buffer_ms=%lu prefills=%lu",(unsigned long)pro2_music_buffer_ms,(unsigned long)pro2_music_prefills);
    shell_print(shell,"music rc=%ld active=%d samples=%lu/%lu loops=%lu decode_us=%lu underruns=%lu GPU video=%lu total=%lu errors=%lu",(long)pro2_music_result,pro2_music_active,(unsigned long)pro2_music_samples,(unsigned long)pro2_music_total,(unsigned long)pro2_music_loops,(unsigned long)pro2_music_decode_us,(unsigned long)pro2_music_underruns,(unsigned long)pro2_gpu_video_blits,(unsigned long)pro2_gpu_blits,(unsigned long)pro2_gpu_errors);
    return 0;
}

static int shell_pro2_view(const struct shell *shell, size_t argc, char **argv)
{
    if (argc != 2) return -EINVAL;
    if (!strcmp(argv[1], "full")) atomic_set(&fullscreen_requested, 1);
    else if (!strcmp(argv[1], "normal")) atomic_set(&fullscreen_requested, 0);
    else return -EINVAL;
    shell_print(shell, "View queued: %s", argv[1]);
    return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(pro2_commands,
    SHELL_CMD(status, NULL, "Read battery and panel diagnostics", shell_pro2_status),
    SHELL_CMD(view, NULL, "view full|normal", shell_pro2_view),
#ifdef PRO2_INPUT5
    SHELL_CMD(tone, NULL, "One-second 500Hz speaker test", shell_pro2_tone),
#endif
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(pro2, &pro2_commands, "Pro 2 diagnostics and view", NULL);
static lv_image_dsc_t video_image = {
    .header = {.magic=LV_IMAGE_HEADER_MAGIC, .cf=LV_COLOR_FORMAT_RGB565,
               .w=VIDEO_WIDTH, .h=VIDEO_HEIGHT, .stride=VIDEO_WIDTH*2},
    .data_size=sizeof(video_pixels), .data=(const uint8_t *)video_pixels,
};

static void advance_video(lv_timer_t *timer)
{
    int action=atomic_set(&media_requests,0);
    if(action & PRO2_ACTION_RESTART) {video_paused=false;video_failed=false;badapple_music_restart();}
    else if(action & PRO2_ACTION_PLAY_PAUSE) {video_paused=!video_paused;badapple_music_pause(video_paused);}
    if (bringup_transfer_active || video_failed || video_paused || !badapple_music_video_ready()) return;
    uint32_t target=(uint64_t)badapple_music_clock()*15/16000;
    if(target>=badapple_frame_count) target=badapple_frame_count-1;
    if(video_frame==target+1) return;
    if(!video_decode_to(target)) {video_failed=true;badapple_music_pause(true);return;}
    lv_image_cache_drop(&video_image);
    lv_obj_invalidate(video_object);
}

static void touch_event(lv_event_t *event)
{
    lv_event_code_t code=lv_event_get_code(event);
    if(code==LV_EVENT_PRESSED) {touches++;long_touch=false;}
    if(code==LV_EVENT_LONG_PRESSED) long_touch=true;
    if(code==LV_EVENT_CLICKED && !long_touch) bringup_toggle_fullscreen();
}

static void refresh_screen(lv_timer_t *timer)
{
    char text[160];
    bool full=atomic_get(&fullscreen_requested) && !bringup_transfer_active;
    if(full!=fullscreen_visible) {
        fullscreen_visible=full;
        lv_obj_t *controls[]={title,status_label,input_label,bar};
        for(unsigned i=0;i<ARRAY_SIZE(controls);i++) {
            if(full) lv_obj_add_flag(controls[i],LV_OBJ_FLAG_HIDDEN);
            else lv_obj_remove_flag(controls[i],LV_OBJ_FLAG_HIDDEN);
        }
    }
    /* Uniform cover scale: fill the circle, preserve shape, crop sides. */
    int sx=full ? (DEF_UI_HEIGHT*256+VIDEO_HEIGHT-1)/VIDEO_HEIGHT : 320*256/VIDEO_WIDTH;
    int sy=sx;
    if(sx!=last_scale_x || sy!=last_scale_y) {
        lv_image_set_scale_x(video_object,sx);lv_image_set_scale_y(video_object,sy);
        last_scale_x=sx;last_scale_y=sy;
    }
    lv_obj_align(video_object,LV_ALIGN_CENTER,0,full?0:-8);
    if(full) return;
    uint32_t seconds=badapple_music_clock()/16000;
    snprintf(text,sizeof(text),"%s  %lu:%02lu / 3:39   Vol %d",video_paused?"PAUSED":"PLAYING",(unsigned long)(seconds/60),(unsigned long)(seconds%60),pro2_audio_volume());
    if(bringup_transfer_active) snprintf(text,sizeof(text),"OTA %u%%",upgrade_progress);
    else if(pro2_music_result && pro2_music_result!=-EAGAIN) snprintf(text,sizeof(text),"Audio error %ld",(long)pro2_music_result);
    lv_label_set_text(status_label,text);
    lv_label_set_text(input_label,"Tap: full screen\nCrown: pause / Hold: restart");
    lv_bar_set_value(bar,bringup_transfer_active?upgrade_progress:seconds*100/219,LV_ANIM_OFF);
}

void ota_view_init(void)
{
    view_stack_set_effect_type(UI_SWITCH_EFFECT_NONE);
    view_stack_push_view(OTA_VIEW, NULL);
}

void ota_view_deinit(void)
{
    view_stack_back_prev();
}

void ota_view_show_upgrade_result(uint8_t *text, bool failed)
{
    upgrade_result = failed ? 1 : 2;
}

void ota_view_show_upgrade_progress(uint8_t progress)
{
    upgrade_progress = progress > 100 ? 100 : progress;
    upgrade_result = 0;
}

static int bringup_layout(view_data_t *view_data)
{
    lv_obj_t *screen = lv_display_get_screen_active(view_data->display);
    root = lv_obj_create(screen);
    if (!root) return -ENOMEM;
    lv_obj_set_size(root, DEF_UI_WIDTH, DEF_UI_HEIGHT);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_style_text_color(root, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(root, &lv_font_montserrat_20, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(root, touch_event, LV_EVENT_ALL, NULL);
    title = lv_label_create(root);
    lv_label_set_text(title, "BAD APPLE v14");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 28);
    status_label = lv_label_create(root);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 58);
    input_label = lv_label_create(root);
    lv_obj_set_style_text_align(input_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(input_label, LV_ALIGN_BOTTOM_MID, 0, -14);
    bar = lv_bar_create(root);
    lv_obj_set_size(bar, 280, 12);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -78);
    lv_bar_set_range(bar, 0, 100);
    fullscreen_visible=false;
    atomic_set(&fullscreen_requested, 0);
    video_failed=false;
    video_reset();
    video_object=lv_image_create(root);
    lv_image_set_src(video_object,&video_image);
    lv_image_set_scale(video_object,320*256/VIDEO_WIDTH);
    lv_image_set_antialias(video_object,false);
    pro2_video_gpu_buffer=(uintptr_t)video_pixels;
    badapple_music_init();
    badapple_music_restart();
    lv_obj_align(video_object,LV_ALIGN_CENTER,0,-12);
    lv_obj_add_flag(video_object,LV_OBJ_FLAG_EVENT_BUBBLE);
    video_timer=lv_timer_create(advance_video,16,NULL);
    refresh_screen(NULL);
    refresh = lv_timer_create(refresh_screen, 250, NULL);
    if (!refresh) {
        lv_obj_delete(root);
        root = NULL;
        return -ENOMEM;
    }
    memset(&crown_state,0,sizeof(crown_state));
    k_timer_start(&crown_timer,K_MSEC(10),K_MSEC(10));
    return 0;
}

static int bringup_handler(uint16_t id, view_data_t *data, uint8_t msg, void *param)
{
    switch (msg) {
    case MSG_VIEW_PRELOAD: return ui_view_layout(id);
    case MSG_VIEW_LAYOUT: return bringup_layout(data);
    case MSG_VIEW_DELETE:
        badapple_music_pause(true);
        k_timer_stop(&crown_timer);
        if (video_timer) lv_timer_delete(video_timer);
        video_timer=NULL;
        if (refresh) lv_timer_delete(refresh);
        if (root) lv_obj_delete(root);
        refresh = NULL;
        root = NULL;
        return 0;
    default: return 0;
    }
}

VIEW_DEFINE2(ota_view, bringup_handler, NULL, NULL, OTA_VIEW, HIGHEST_ORDER,
             UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
