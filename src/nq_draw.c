/*
 * Nanquim Framework v0.1 - Engine de Simulação Científica
 */

#include "nanquim.h"
#include "nq_internal.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL2_rotozoom.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Acesso externo ao contexto ativo (definido em nq_core.c)
// extern NQ_Context* g_active_ctx; // DEPRECATED: Use nq_get_active_context()
extern NQ_Context* nq_get_active_context(void);

// ==========================================
// Software Rendering Helpers
// ==========================================

/*
 * put_pixel_surface:
 * Acessa diretamente a memória da superfície SDL (buffer) para máxima performance em psetting.
 * 
 * pitch: O número de bytes em uma linha da superfície (stride). 
 *        Necessário para calcular o endereço Y corretamente, pois pode haver padding.
 * current_target: A superfície onde estamos desenhando. Pode ser o canvas visível ou um buffer offscreen.
 */
static void put_pixel_surface(SDL_Surface *surface, int x, int y, Uint32 pixel) {
    if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) return;
    
    // Assumindo 32-bit (ARGB8888 ou RGBA8888) como criado no nq_core
    // Usamos Uint32* para escrever 4 bytes de uma vez.
    // O cast para Uint8* antes da aritmética de ponteiros serve para avançar em bytes (usando pitch).
    Uint32 *p = (Uint32 *)((Uint8 *)surface->pixels + y * surface->pitch + x * 4);
    *p = pixel;
}

// ==========================================
// API Draw Implementation
// ==========================================

void nq_setup_coords(float x_min, float x_max, float y_min, float y_max) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;

    ctx->coord_mode = NQ_WORLD;
    ctx->x_min = x_min;
    ctx->x_max = x_max;
    ctx->y_min = y_min;
    ctx->y_max = y_max;

    // Optimization: Precompute inverse range to use multiplication instead of division
    float range_x = x_max - x_min;
    float range_y = y_max - y_min;

    if (fabsf(range_x) < 0.0001f) ctx->inv_range_x = 1.0f;
    else ctx->inv_range_x = 1.0f / range_x;

    if (fabsf(range_y) < 0.0001f) ctx->inv_range_y = 1.0f;
    else ctx->inv_range_y = 1.0f / range_y;
    
    // Initialize Camera to view this entire range centered
    ctx->camera_x = (x_min + x_max) / 2.0f;
    // For Y, we need to respect the direction (min/max). Center is still average.
    ctx->camera_y = (y_min + y_max) / 2.0f;
    ctx->camera_zoom = 1.0f;
    ctx->camera_angle = 0.0f;
}

void nq_color(Uint8 r, Uint8 g, Uint8 b) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    
    ctx->pen_color = (SDL_Color){r, g, b, 255};
    
    // Atualiza cor de desenho no soft renderer também
    if (ctx->current_target && ctx->current_target->soft_renderer) {
        SDL_SetRenderDrawColor(ctx->current_target->soft_renderer, r, g, b, 255);
    }
}

void nq_background(Uint8 r, Uint8 g, Uint8 b) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    if (ctx) {
        ctx->bg_color = (SDL_Color){r, g, b, 255};
        
        NQ_Surface* target = ctx->current_target;
        if (target && target->sdl_surf) {
            Uint32 color = SDL_MapRGBA(target->sdl_surf->format, r, g, b, 255);
            SDL_FillRect(target->sdl_surf, NULL, color);
        }
    }
}

void nq_pset(float x, float y) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    if (!ctx || !ctx->current_target) return;

    int px, py;
    nq_map_point(ctx, x, y, &px, &py);
    
    // Caminho rápido: acesso direto à memória (sem overhead de SDL_RenderDrawPoint)
    SDL_Surface* surf = ctx->current_target->sdl_surf;
    Uint32 pixel = SDL_MapRGBA(surf->format, ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
    
    if (SDL_MUSTLOCK(surf)) SDL_LockSurface(surf);
    put_pixel_surface(surf, px, py, pixel);
    if (SDL_MUSTLOCK(surf)) SDL_UnlockSurface(surf);
}

void nq_line_weight(float w) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    if (ctx) {
        ctx->line_width = w;
    }
}

void nq_line(float x1, float y1, float x2, float y2) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    if (!ctx || !ctx->current_target || !ctx->current_target->soft_renderer) return;

    int px1, py1, px2, py2;
    nq_map_point(ctx, x1, y1, &px1, &py1);
    nq_map_point(ctx, x2, y2, &px2, &py2);

    // Usando Software Renderer para desenhar na Surface com SDL2_gfx ou Renderer padrão
    // Usaremos lineRGBA para consistência com o pedido de usar SDL2_gfx
    if (ctx->line_width > 1.0f) {
        thickLineRGBA(ctx->current_target->soft_renderer, 
             px1, py1, px2, py2, 
             (Uint8)ctx->line_width,
             ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
    } else {
        lineRGBA(ctx->current_target->soft_renderer, 
             px1, py1, px2, py2, 
             ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
    }
}

void nq_box(float x1, float y1, float x2, float y2, bool filled) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    if (!ctx || !ctx->current_target || !ctx->current_target->soft_renderer) return;

    // A box defined by (x1, y1) and (x2, y2) in world space must be transformed as a polygon 
    // because rotation might make it non-axis-aligned in screen space.
    
    float vx[4] = {x1, x2, x2, x1};
    float vy[4] = {y1, y1, y2, y2};
    
    // Use polygon to draw the rotated box
    nq_polygon(vx, vy, 4, filled);
}

void nq_circle(float x, float y, float r, bool filled) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    if (!ctx || !ctx->current_target || !ctx->current_target->soft_renderer) return;

    int px, py;
    nq_map_point(ctx, x, y, &px, &py);
    
    // Zoom affects radius
    int pr = nq_map_scalar_cam(ctx, r);

    // SDL2_gfx circle doesn't support thickness easily.
    // If we wanted thick circle outline, we would need to draw multiple circles or use arc approximation.
    // Following instructions strictly for nq_line priority.

    if (filled) {
        filledCircleRGBA(ctx->current_target->soft_renderer,
                         px, py, pr,
                         ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
    } else {
        circleRGBA(ctx->current_target->soft_renderer,
                   px, py, pr,
                   ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
    }
}

void nq_arc(float x, float y, float radius, float start_angle, float end_angle, bool filled) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    if (!ctx || !ctx->current_target || !ctx->current_target->soft_renderer) return;

    int px, py;
    nq_map_point(ctx, x, y, &px, &py);
    int pr = nq_map_scalar_cam(ctx, radius);
    
    // Adjust angles for camera rotation
    double cam_deg = ctx->camera_angle * 180.0 / M_PI;
    Sint16 s_angle = (Sint16)(start_angle - cam_deg);
    Sint16 e_angle = (Sint16)(end_angle - cam_deg);
    
    // SDL2_gfx angles are in degrees.
    if (filled) {
        filledPieRGBA(ctx->current_target->soft_renderer,
                      px, py, pr,
                      s_angle, e_angle,
                      ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
    } else {
        arcRGBA(ctx->current_target->soft_renderer,
                px, py, pr,
                s_angle, e_angle,
                ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
    }
}

void nq_polygon(float *vx, float *vy, int n, bool filled) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx || !ctx->current_target || !ctx->current_target->soft_renderer) return;
    
    if (n < 3) return;

    // Small Buffer Optimization (stack alloc for <= 64, heap otherwise)
    Sint16 stack_sx[64];
    Sint16 stack_sy[64];
    Sint16* sx = stack_sx;
    Sint16* sy = stack_sy;
    
    if (n > 64) {
        sx = (Sint16*)malloc(n * sizeof(Sint16));
        sy = (Sint16*)malloc(n * sizeof(Sint16));
        
        if (!sx || !sy) {
            if(sx && sx != stack_sx) free(sx);
            if(sy && sy != stack_sy) free(sy);
            return;
        }
    }

    // Map all vertices using camera transform
    for(int i=0; i<n; i++) {
        int px, py;
        nq_map_point(ctx, vx[i], vy[i], &px, &py);
        sx[i] = (Sint16)px;
        sy[i] = (Sint16)py;
    }
    
    if (filled) {
        filledPolygonRGBA(ctx->current_target->soft_renderer,
                          sx, sy, n,
                          ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
    } else {
        if (ctx->line_width > 1.0f) {
             // Decompose into thick lines for thickness support
             for(int i=0; i<n; i++) {
                 int next = (i+1)%n;
                 thickLineRGBA(ctx->current_target->soft_renderer,
                     sx[i], sy[i], sx[next], sy[next],
                     (Uint8)ctx->line_width,
                     ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
             }
        } else {
            polygonRGBA(ctx->current_target->soft_renderer,
                        sx, sy, n,
                        ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
        }
    }
    
    if (n > 64) {
        free(sx);
        free(sy);
    }
}

/* --- API ASSETS & BUFFERS --- */

void nq_set_target(NQ_Surface* target) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    if (ctx && target) {
        ctx->current_target = target;
    }
}

void nq_reset_target() {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    if (ctx && ctx->canvas) {
        ctx->current_target = ctx->canvas;
    }
}

void nq_blit(NQ_Surface* src, float x, float y, NQ_Anchor anchor) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    if (!ctx->current_target || !src || !src->sdl_surf) return;
    
    // Calculate World Size assuming 1px = 1px at Zoom 1.0
    int sw = ctx->current_target->w;
    float range_x = ctx->x_max - ctx->x_min;
    float ppu = (float)sw / fabsf(range_x);
    
    float w_world = src->w / ppu;
    float h_world = src->h / ppu;
    
    // Delegate to nq_blit_ex to handle zoom/rotation automatically
    // The angle is 0.0f (aligned with world axes)
    nq_blit_ex(src, x, y, w_world, h_world, 0.0f, anchor);
}

// Helper to flip surface content (simple pixel copy)
static SDL_Surface* create_flipped_surface(SDL_Surface* surface, bool flip_h, bool flip_v) {
    if (!surface) return NULL;
    
    SDL_Surface* flipped = SDL_CreateRGBSurface(0, surface->w, surface->h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!flipped) return NULL;
    
    // Lock both
    if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);
    if (SDL_MUSTLOCK(flipped)) SDL_LockSurface(flipped); // Should be in RAM
    
    Uint32* src_pixels = (Uint32*)surface->pixels;
    Uint32* dst_pixels = (Uint32*)flipped->pixels;
    
    int w = surface->w;
    int h = surface->h;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int src_idx = y * w + x;
            int dst_x = flip_h ? (w - 1 - x) : x;
            int dst_y = flip_v ? (h - 1 - y) : y;
            int dst_idx = dst_y * w + dst_x;
            
            dst_pixels[dst_idx] = src_pixels[src_idx];
        }
    }

    if (SDL_MUSTLOCK(flipped)) SDL_UnlockSurface(flipped);
    if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
    
    return flipped;
}

void nq_blit_ex(NQ_Surface* src, float x, float y, float w, float h, float angle, NQ_Anchor anchor) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    if (!ctx || !ctx->current_target || !src || !src->sdl_surf) return;

    // 1. Determine Flip and Absolute Size
    bool flip_h = (w < 0);
    bool flip_v = (h < 0);
    float abs_w = fabsf(w);
    float abs_h = fabsf(h);

    // 2. Calculate Target Pixels
    int px_w, px_h;
    
    if (ctx->coord_mode == NQ_LITERAL) {
        px_w = (int)abs_w;
        px_h = (int)abs_h;
    } else {
        // Camera Aware Size Calculation
        // Just use nq_map_scalar_cam for width logic?
        // Width in pixels = abs_w * ppu_x * zoom
        // But nq_map_scalar_cam returns int.
        
        // Let's reimplement PPU logic here for precision
        int sw = ctx->current_target->w;
        float range_x = ctx->x_max - ctx->x_min;
        float ppu = (float)sw / fabsf(range_x); // Assuming uniform PPU for sprite?
        
        px_w = (int)(abs_w * ppu * ctx->camera_zoom);
        px_h = (int)(abs_h * ppu * ctx->camera_zoom); 
        // Note: For non-uniform aspect ratio, we might want different PPU for H?
        // But usually sprites are 1:1 unless stretched.
    }
    
    // ... (Flip logic remains same) ...

    SDL_Surface* source_to_scale = src->sdl_surf;
    SDL_Surface* flipped_temp = NULL;
    
    if (flip_h || flip_v) {
        flipped_temp = create_flipped_surface(source_to_scale, flip_h, flip_v);
        if (flipped_temp) {
             // Handle color key explicitly if source has it
             if (src->has_colorkey) {
                 SDL_SetColorKey(flipped_temp, SDL_TRUE, src->colorkey);
             }
             source_to_scale = flipped_temp;
        }
    }
    
    // 4. Calculate Zoom Factors
    double zoom_x = (double)px_w / (double)source_to_scale->w;
    double zoom_y = (double)px_h / (double)source_to_scale->h;
    
    // 5. Rotozoom (Creates a NEW surface)
    if (zoom_x < 0.001 || zoom_y < 0.001) {
        if (flipped_temp) SDL_FreeSurface(flipped_temp);
        return;
    }
    
    // Calculate total rotation
    // Add Camera Rotation (in degrees) to Sprite Rotation
    double cam_deg = ctx->camera_angle * 180.0 / M_PI;
    // Sprite angle is CW in SDL? No, usually CCW.
    // If we want to Counter-Rotate the sprite by camera angle so it stays upright in world,
    // we subtract camera angle.
    // However, the VIEW is rotated.
    // If I rotate view +90 deg, world rotates -90 deg. Sprite rotates -90 deg.
    // So target angle = sprite_angle - cam_deg.
    // Wait, rotozoom uses degrees? Yes. Sign convention?
    // Usually + is CCW. SDL rotozoom: "angle: The angle of the rotation in degrees." 
    // It rotates CCW.
    
    double total_angle = angle - cam_deg;

    SDL_Surface* transformed = rotozoomSurfaceXY(source_to_scale, total_angle, zoom_x, zoom_y, 1);
    
    // Cleanup flipped temp immediately
    if (flipped_temp) SDL_FreeSurface(flipped_temp);
    
    if (!transformed) return; // Allocation failed

    // 6. Position and Anchor
    int px, py;
    nq_map_point(ctx, x, y, &px, &py);
    
    SDL_Rect dst_rect;
    dst_rect.x = px;
    dst_rect.y = py;
    dst_rect.w = transformed->w;
    dst_rect.h = transformed->h;
    
    if (anchor == NQ_CENTERED) {
        dst_rect.x -= transformed->w / 2;
        dst_rect.y -= transformed->h / 2;
    }
    
    SDL_SetSurfaceBlendMode(transformed, SDL_BLENDMODE_BLEND);
    SDL_BlitSurface(transformed, NULL, ctx->current_target->sdl_surf, &dst_rect);
    
    // 7. Cleanup Transformed
    SDL_FreeSurface(transformed);
}

void nq_draw_region(NQ_Surface* atlas, NQ_Rect region, float x, float y, float w, float h, float angle, bool flip_h) {
    if (!atlas) return;
    
    // Create temporary surface for the region
    NQ_Surface* reg_surf = nq_copy_rect(atlas, region);
    if (!reg_surf) return;
    
    // Draw using blit_ex logic
    // We pass negative width if flip_h is true (nq_blit_ex supports this convention based on prompt/implementation)
    float draw_w = flip_h ? -fabsf(w) : fabsf(w);
    
    // Default to CENTERED anchor as per typical sprite usage in games
    // The prompt doesn't specify anchor, but robot_parade used CENTRED.
    nq_blit_ex(reg_surf, x, y, draw_w, h, angle, NQ_CENTERED);
    
    // Free temporary surface
    nq_free_surface(reg_surf);
}

// ==========================================
// GRID SYSTEM
// ==========================================

void nq_axis_color(Uint8 r, Uint8 g, Uint8 b) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    ctx->axis_color = (SDL_Color){r, g, b, 255};
}

void nq_grid_color(Uint8 r, Uint8 g, Uint8 b) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    ctx->grid_color = (SDL_Color){r, g, b, 255};
}

// Adaptive Step Calculation for Infinite Grids
static float nq_calc_adaptive_step(NQ_Context* ctx, float base_step) {
    // Calculate Pixels Per Unit (PPU)
    // Using simple X range logic for scale estimation
    int sw = ctx->current_target ? ctx->current_target->w : 800;
    float range_x = fabsf(ctx->x_max - ctx->x_min);
    if (range_x < 0.0001f) return base_step;
    
    float ppu = (float)sw / range_x * ctx->camera_zoom;
    if (ppu < 0.0001f) return base_step;

    // Target pixel spacing ~80px to 100px
    float target_px = 80.0f;
    float ideal_step = target_px / ppu;
    
    // Find nearest nice number (1, 2, 5 * 10^n)
    float power_of_10 = powf(10.0f, floorf(log10f(ideal_step)));
    float normalized = ideal_step / power_of_10;
    
    float rounded_mantissa;
    if (normalized < 2.0f) rounded_mantissa = 1.0f;
    else if (normalized < 5.0f) rounded_mantissa = 2.0f;
    else rounded_mantissa = 5.0f;
    
    return rounded_mantissa * power_of_10;
}

void nq_draw_grid_rect(float step_x_hint, float step_y_hint, bool show_labels) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;
    
    // Use adaptive steps to prevent clutter and handle deep zoom
    float step_x = nq_calc_adaptive_step(ctx, step_x_hint);
    float step_y = nq_calc_adaptive_step(ctx, step_y_hint); // Usually same as X for square grid
    
    // Determine visible bounds based on camera
    float cam_min_x, cam_max_x, cam_min_y, cam_max_y;
    nq_get_camera_bounds(&cam_min_x, &cam_max_x, &cam_min_y, &cam_max_y);
    
    // Add some padding to ensure lines cover screen during rotation
    // nq_get_camera_bounds already returns the full covering rect of the rotated camera view.
    // So we just iterate from min to max.
    
    // Save current state
    SDL_Color old_pen = ctx->pen_color;
    float old_width = ctx->line_width;
    
    float x_start = ceilf(cam_min_x / step_x) * step_x;
    float y_start = ceilf(cam_min_y / step_y) * step_y;
    
    // 1. Draw Grid Lines
    nq_color(ctx->grid_color.r, ctx->grid_color.g, ctx->grid_color.b);
    nq_line_weight(1.0f);
    
    // Vertical lines
    for (float x = x_start; x <= cam_max_x; x += step_x) {
        if (fabsf(x) < 0.0001f) continue; // Skip main axis
        nq_line(x, cam_min_y, x, cam_max_y);
    }
    
    // Horizontal lines
    for (float y = y_start; y <= cam_max_y; y += step_y) {
        if (fabsf(y) < 0.0001f) continue; // Skip main axis
        nq_line(cam_min_x, y, cam_max_x, y);
    }
    
    // 2. Draw Main Axes
    nq_color(ctx->axis_color.r, ctx->axis_color.g, ctx->axis_color.b);
    nq_line_weight(2.0f);
    
    // X Axis (y=0) - Draw across the full view if y=0 is visible (or even if not, nq_line handles clipping)
    if (cam_min_y <= 0 && cam_max_y >= 0) {
        nq_line(cam_min_x, 0, cam_max_x, 0);
    }
    
    // Y Axis (x=0)
    if (cam_min_x <= 0 && cam_max_x >= 0) {
        nq_line(0, cam_min_y, 0, cam_max_y);
    }
    
    // 3. Labels
    if (show_labels) {
        char buf[32];
        Uint32 color = (ctx->axis_color.a << 24) | (ctx->axis_color.b << 16) | (ctx->axis_color.g << 8) | ctx->axis_color.r;
        
        // Origin (0,0) - Only if roughly visible
        if (cam_min_x <= 0 && cam_max_x >= 0 && cam_min_y <= 0 && cam_max_y >= 0) {
            int px, py;
            nq_map_point(ctx, 0.0f, 0.0f, &px, &py);
            // Draw "0" slightly offset (bottom-left quadrant style)
            stringColor(ctx->current_target->soft_renderer, px - 10, py + 4, "0", color);
        }
        
        // Loop X ticks (skip origin)
        for (float x = x_start; x <= cam_max_x; x += step_x) {
            if (fabsf(x) < 0.0001f) continue;
            
            snprintf(buf, sizeof(buf), "%.1f", x);
            int text_w = strlen(buf) * 8;
            
            int px, py;
            nq_map_point(ctx, x, 0.0f, &px, &py);
            
            // Draw slightly below X axis
            stringColor(ctx->current_target->soft_renderer, px - text_w/2, py + 4, buf, color);
        }
        
        // Loop Y ticks (skip origin)
        for (float y = y_start; y <= cam_max_y; y += step_y) {
            if (fabsf(y) < 0.0001f) continue;
            
            snprintf(buf, sizeof(buf), "%.1f", y);
            int text_w = strlen(buf) * 8;
            
            int px, py;
            nq_map_point(ctx, 0.0f, y, &px, &py);
            
            // Draw left of Y axis
            stringColor(ctx->current_target->soft_renderer, px - text_w - 4, py - 4, buf, color);
        }
    }
    
    // Restore state
    nq_color(old_pen.r, old_pen.g, old_pen.b);
    nq_line_weight(old_width);
}

void nq_draw_grid_polar(float step_r_hint, float step_theta, bool show_labels) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx) return;

    // Determine visible bounds based on camera
    float cam_min_x, cam_max_x, cam_min_y, cam_max_y;
    nq_get_camera_bounds(&cam_min_x, &cam_max_x, &cam_min_y, &cam_max_y);
    
    // Adaptive Step for R (Theta not adaptive for now unless clutter becomes huge)
    float step_r = nq_calc_adaptive_step(ctx, step_r_hint);

    // Save current state
    SDL_Color old_pen = ctx->pen_color;
    float old_width = ctx->line_width;

    
    // Calculate Grid Bounds
    // 1. Max Radius (furthest corner from origin)
    float max_r = 0.0f;
    float corners[4][2] = {
        {cam_min_x, cam_min_y}, {cam_max_x, cam_min_y},
        {cam_max_x, cam_max_y}, {cam_min_x, cam_max_y}
    };
    for(int i=0; i<4; i++) {
        float r = sqrtf(corners[i][0]*corners[i][0] + corners[i][1]*corners[i][1]);
        if (r > max_r) max_r = r;
    }

    // 2. Min Radius (closest point in View Rect to origin)
    // If origin is inside, min_r = 0
    float cx = (0.0f < cam_min_x) ? cam_min_x : (0.0f > cam_max_x) ? cam_max_x : 0.0f;
    float cy = (0.0f < cam_min_y) ? cam_min_y : (0.0f > cam_max_y) ? cam_max_y : 0.0f;
    float min_r = sqrtf(cx*cx + cy*cy);

    // Adjust start_r to next multiple step
    float start_r = ceilf(min_r / step_r) * step_r;
    if (start_r < step_r) start_r = step_r; // Always skip 0 radius (point)

    // 1. Concentric Circles
    nq_color(ctx->grid_color.r, ctx->grid_color.g, ctx->grid_color.b);
    nq_line_weight(1.0f);
    
    for (float r = start_r; r <= max_r; r += step_r) {
        nq_circle(0, 0, r, false);
    }
    
    // 2. Radial Lines
    for (float theta = 0; theta < 2 * M_PI; theta += step_theta) {
        // Skip lines that coincide with main axes
        if (fabsf(sinf(theta)) < 0.001f || fabsf(cosf(theta)) < 0.001f) continue;
        
        float x = max_r * cosf(theta);
        float y = max_r * sinf(theta);
        nq_line(0, 0, x, y);
    }
    
    // 3. Main Axes
    nq_color(ctx->axis_color.r, ctx->axis_color.g, ctx->axis_color.b);
    nq_line_weight(2.0f);
    nq_line(cam_min_x, 0, cam_max_x, 0); // Horizontal
    nq_line(0, cam_min_y, 0, cam_max_y); // Vertical

    // 4. Labels
    if (show_labels) {
        char buf[32];
        Uint32 color = (ctx->axis_color.a << 24) | (ctx->axis_color.b << 16) | (ctx->axis_color.g << 8) | ctx->axis_color.r;
        
        // Draw Radius labels along positive X axis (if visible)
        // Check if positive X axis is within View Y range and extends to View X range
        if (cam_min_y <= 0 && cam_max_y >= 0 && cam_max_x > 0) {
            float label_start_r = (start_r > cam_min_x) ? start_r : (ceilf(cam_min_x/step_r)*step_r);
            if (label_start_r < step_r) label_start_r = step_r;
            
            for (float r = label_start_r; r <= max_r; r += step_r) {
                if (r > cam_max_x) break; // Optimization: don't draw past screen right edge
                
                snprintf(buf, sizeof(buf), "%.1f", r);
                int px, py;
                nq_map_point(ctx, r, 0.0f, &px, &py);
                stringColor(ctx->current_target->soft_renderer, px + 2, py + 2, buf, color);
            }
        }
    }

    // Restore state
    nq_color(old_pen.r, old_pen.g, old_pen.b);
    nq_line_weight(old_width);
}

