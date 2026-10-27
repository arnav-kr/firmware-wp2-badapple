#include <stdint.h>
#include <vg_lite/vg_lite.h>
volatile uint32_t pro2_gpu_blits,pro2_gpu_video_blits,pro2_gpu_errors;
uintptr_t pro2_video_gpu_buffer;
extern vg_lite_error_t __real_vg_lite_blit(vg_lite_buffer_t *,vg_lite_buffer_t *,vg_lite_matrix_t *,vg_lite_blend_t,vg_lite_color_t,vg_lite_filter_t);
extern vg_lite_error_t __real_vg_lite_blit_rect(vg_lite_buffer_t *,vg_lite_buffer_t *,vg_lite_rectangle_t *,vg_lite_matrix_t *,vg_lite_blend_t,vg_lite_color_t,vg_lite_filter_t);
static void record(vg_lite_buffer_t *source,vg_lite_error_t result)
{
    pro2_gpu_blits++;
    if (source && ((uintptr_t)source->memory==pro2_video_gpu_buffer || (uintptr_t)source->address==pro2_video_gpu_buffer)) pro2_gpu_video_blits++;
    if (result!=VG_LITE_SUCCESS) pro2_gpu_errors++;
}
vg_lite_error_t __wrap_vg_lite_blit(vg_lite_buffer_t *target,vg_lite_buffer_t *source,vg_lite_matrix_t *matrix,vg_lite_blend_t blend,vg_lite_color_t color,vg_lite_filter_t filter)
{
    vg_lite_error_t rc=__real_vg_lite_blit(target,source,matrix,blend,color,filter);
    record(source,rc);return rc;
}
vg_lite_error_t __wrap_vg_lite_blit_rect(vg_lite_buffer_t *target,vg_lite_buffer_t *source,vg_lite_rectangle_t *rect,vg_lite_matrix_t *matrix,vg_lite_blend_t blend,vg_lite_color_t color,vg_lite_filter_t filter)
{
    vg_lite_error_t rc=__real_vg_lite_blit_rect(target,source,rect,matrix,blend,color,filter);
    record(source,rc);return rc;
}
