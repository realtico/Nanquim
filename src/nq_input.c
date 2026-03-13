#include "nanquim.h"
#include "nq_internal.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <math.h>

// Extern: definido no nq_core.c
extern NQ_Context* g_active_ctx;
extern NQ_Context* nq_get_active_context(void);

// ==========================================
// INPUT IMPLEMENTATION
// ==========================================

bool nq_key_down(NQ_Key key) {
    // SDL supports multiple keyboards but usually we just want the global state
    const Uint8* state = SDL_GetKeyboardState(NULL);
    // Cast key to int (scancode)
    SDL_Scancode sc = (SDL_Scancode)key;
    if (sc >= 0 && sc < SDL_NUM_SCANCODES) {
        return state[sc] != 0;
    }
    return false;
}

bool nq_mouse_button(NQ_MouseButton btn) {
    Uint32 mask = SDL_GetMouseState(NULL, NULL);
    switch (btn) {
        case NQ_MOUSE_LEFT:   return (mask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
        case NQ_MOUSE_MIDDLE: return (mask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
        case NQ_MOUSE_RIGHT:  return (mask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
        default: return false;
    }
}

int nq_mouse_scroll(void) {
    if (!g_active_ctx) return 0;
    NQ_Context* ctx = g_active_ctx;
    if (ctx) {
        int d = ctx->wheel_delta;
        ctx->wheel_delta = 0; // Consumir o delta
        return d;
    }
    return 0;
}

void nq_mouse_pos(float *x, float *y) {
    int win_mx, win_my;
    SDL_GetMouseState(&win_mx, &win_my);
    
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) {
        *x = (float)win_mx;
        *y = (float)win_my;
        return;
    }
    
    // 1. Transform Window Coords -> Canvas Coords (Reverse Scale/Offset)
    float canvas_x, canvas_y;
    
    if (ctx->scale_mode == NQ_SCALE_ASPECT) {
        // (Window - Offset) / Scale
        canvas_x = (win_mx - ctx->offset_x) / ctx->window_scale;
        canvas_y = (win_my - ctx->offset_y) / ctx->window_scale;
    } else {
        // Stretch mode (simple separate scaling if we implemented non-uniform)
        // For now using window_scale generic
        canvas_x = win_mx / ctx->window_scale;
        canvas_y = win_my / ctx->window_scale; // Wait, uniform scale might be wrong for stretch
        
        // Let's recompute dynamic stretch scale here to be safe if it wasn't perfect in sync
        int win_w, win_h;
        SDL_GetWindowSize(ctx->window, &win_w, &win_h);
        if (win_w > 0 && win_h > 0) {
            canvas_x = win_mx * ((float)ctx->canvas->w / win_w);
            canvas_y = win_my * ((float)ctx->canvas->h / win_h);
        } else {
            canvas_x = win_mx;
            canvas_y = win_my;
        }
    }
    
    // 2. Transform Canvas Pixels -> World (using nq_inv_map_point)
    // Note: nq_inv_map expects "pixels", which are effectively canvas coordinates
    nq_inv_map_point(ctx, (int)canvas_x, (int)canvas_y, x, y);
}

// ==========================================
// TEXT IMPLEMENTATION
// ==========================================

void nq_set_font_size(int size) {
    if (!g_active_ctx) return;
    NQ_Context* ctx = g_active_ctx;
    if (ctx) {
        ctx->font_size = size;
    }
}

// Internal: Map needed for text position
// static int map_x(NQ_Context* ctx, float x_world);
// static int map_y(NQ_Context* ctx, float y_world);

// We need to access the map functions from nq_draw.c or duplicate them? 
// Ideally they should be shared internally. 
// For now, let's implement a quick map here since we are in a separate compilation unit 

// ==========================================
// INPUT IMPLEMENTATION
// ==========================================

void nq_put_text(float x, float y, const char* text) {
    if (!g_active_ctx) return;
    NQ_Context* ctx = g_active_ctx;
    if (!ctx || !ctx->current_target) return;

    int px, py;
    nq_map_point(ctx, x, y, &px, &py);

    // Encode color to Uint32 0xAABBGGRR for SDL2_gfx
    Uint32 color = (ctx->pen_color.a << 24) | (ctx->pen_color.b << 16) | (ctx->pen_color.g << 8) | ctx->pen_color.r;
    
    // Use stringColor from SDL2_gfx
    stringColor(ctx->current_target->soft_renderer, px, py, text, color);
}