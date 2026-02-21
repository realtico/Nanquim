#include "nanquim.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <math.h>

// Extern: definido no nq_core.c
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
    NQ_Context* ctx = nq_get_active_context();
    if (ctx) {
        int d = ctx->wheel_delta;
        ctx->wheel_delta = 0; // Consumir o delta
        return d;
    }
    return 0;
}

void nq_mouse_pos(float *x, float *y) {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) {
        *x = (float)mx;
        *y = (float)my;
        return;
    }

    // Inverse Mapping: Pixel -> World
    // x_world = x_min + (pixel / w) * (x_max - x_min)
    
    // Check for target dimension (usually canvas w/h, but let's use current target)
    int w = ctx->current_target ? ctx->current_target->w : 1;
    int h = ctx->current_target ? ctx->current_target->h : 1;
    
    if (ctx->coord_mode == NQ_LITERAL) {
        *x = (float)mx;
        *y = (float)my;
    } else {
        float norm_x = (float)mx / (float)w;
        // Inverse of map_y logic
        float norm_y = (float)my / (float)h;

        *x = ctx->x_min + norm_x * (ctx->x_max - ctx->x_min);
        *y = ctx->y_min + norm_y * (ctx->y_max - ctx->y_min);
    }
}

// ==========================================
// TEXT IMPLEMENTATION
// ==========================================

void nq_set_font_size(int size) {
    NQ_Context* ctx = nq_get_active_context();
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
// and map_x/map_y are static in nq_draw.c.
// BETTER ARCHITECTURE: move map functions to a shared internal header or expose them.
// But we can just replicate the math here for now to avoid refactoring nq_draw.c immediately.

static int _internal_map_x(NQ_Context* ctx, float x_world) {
    if (ctx->coord_mode == NQ_LITERAL) return (int)x_world;
    int w = ctx->current_target ? ctx->current_target->w : 0;
    float norm = (x_world - ctx->x_min) / (ctx->x_max - ctx->x_min);
    return (int)(norm * w);
}

static int _internal_map_y(NQ_Context* ctx, float y_world) {
    if (ctx->coord_mode == NQ_LITERAL) return (int)y_world;
    int h = ctx->current_target ? ctx->current_target->h : 0;
    float norm = (y_world - ctx->y_min) / (ctx->y_max - ctx->y_min);
    return (int)(norm * h);
}

void nq_put_text(float x, float y, const char* text) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx || !ctx->current_target || !ctx->current_target->soft_renderer) return;

    int px = _internal_map_x(ctx, x);
    int py = _internal_map_y(ctx, y);

    // SDL2_gfx uses 8x8 bitmap font fixed.
    // stringColor(renderer, x, y, text, color)
    // To support "font_size", we might not be able to easily scale SDL2_gfx bitmap font 
    // without creating a custom texture or using SDL_ttf.
    // Given the constraints, we will just use standard stringColor.
    
    // Future thought: Render to a temporary surface, scale it, then blit? Too slow.
    // For now: Industrial MVP -> Just text at position.
    
    stringRGBA(ctx->current_target->soft_renderer,
               px, py, text,
               ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
}
