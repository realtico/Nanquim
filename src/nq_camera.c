#include "nanquim.h"
#include "nq_internal.h"
#include <math.h>

void nq_camera_look_at(float x, float y) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    ctx->camera_x = x;
    ctx->camera_y = y;
}

void nq_camera_zoom(float z) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    if (z > 0.0001f) ctx->camera_zoom = z;
}

void nq_camera_zoom_rel(float factor) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    if (ctx->camera_zoom * factor > 0.0001f) {
        ctx->camera_zoom *= factor;
    }
}

void nq_camera_rotate(float angle) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    ctx->camera_angle = angle;
}

void nq_camera_rotate_rel(float d_angle) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    ctx->camera_angle += d_angle;
}

void nq_camera_pan(float dx, float dy) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    
    // Pan in WORLD coordinates
    ctx->camera_x += dx;
    ctx->camera_y += dy;
}

// Getters
float nq_camera_get_x(void) {
    NQ_Context* ctx = nq_get_active_context();
    return ctx ? ctx->camera_x : 0.0f;
}

float nq_camera_get_y(void) {
    NQ_Context* ctx = nq_get_active_context();
    return ctx ? ctx->camera_y : 0.0f;
}

float nq_camera_get_zoom(void) {
    NQ_Context* ctx = nq_get_active_context();
    return ctx ? ctx->camera_zoom : 1.0f;
}

float nq_camera_get_angle(void) {
    NQ_Context* ctx = nq_get_active_context();
    return ctx ? ctx->camera_angle : 0.0f;
}

// Conversions
void nq_camera_to_screen(float wx, float wy, int* sx, int* sy) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    nq_map_point(ctx, wx, wy, sx, sy);
}

void nq_screen_to_camera(int sx, int sy, float* wx, float* wy) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    nq_inv_map_point(ctx, sx, sy, wx, wy);
}

void nq_get_camera_bounds(float* min_x, float* max_x, float* min_y, float* max_y) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;

    int w = 0, h = 0;
    if (ctx->current_target) {
        w = ctx->current_target->w;
        h = ctx->current_target->h;
    } else {
        SDL_GetWindowSize(ctx->window, &w, &h);
    }
    
    // Corners of screen
    int screen_corners[4][2] = {
        {0, 0}, {w, 0}, {w, h}, {0, h}
    };
    
    float first_x, first_y;
    nq_inv_map_point(ctx, screen_corners[0][0], screen_corners[0][1], &first_x, &first_y);
    
    float min_lx = first_x, max_lx = first_x;
    float min_ly = first_y, max_ly = first_y;
    
    for(int i=1; i<4; i++) {
        float wx, wy;
        nq_inv_map_point(ctx, screen_corners[i][0], screen_corners[i][1], &wx, &wy);
        
        if (wx < min_lx) min_lx = wx;
        if (wx > max_lx) max_lx = wx;
        if (wy < min_ly) min_ly = wy;
        if (wy > max_ly) max_ly = wy;
    }
    
    if (min_x) *min_x = min_lx;
    if (max_x) *max_x = max_lx;
    if (min_y) *min_y = min_ly;
    if (max_y) *max_y = max_ly;
}
