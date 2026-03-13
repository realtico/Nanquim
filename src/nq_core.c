#include "nanquim.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL_image.h>

// ==========================================
// Estado Global Interno
// ==========================================

#define MAX_CONTEXTS 10
static NQ_Context* g_contexts[MAX_CONTEXTS];
static int g_context_count = 0;
// O ponteiro global crucial
NQ_Context* g_active_ctx = NULL;
static bool g_is_running = true;

// Timing
static Uint32 g_last_time = 0;

// Rastreamento de superfícies para evitar memory leaks
#define MAX_SURFACES 100
static NQ_Surface* g_surfaces[MAX_SURFACES];
static int g_surface_count = 0;

void nq_register_surface(NQ_Surface* surf) {
    if (g_surface_count < MAX_SURFACES) {
        g_surfaces[g_surface_count++] = surf;
    }
}

void nq_unregister_surface(NQ_Surface* surf) {
    for (int i = 0; i < g_surface_count; i++) {
        if (g_surfaces[i] == surf) {
            // Swap with last element to remove (order doesn't matter)
            g_surfaces[i] = g_surfaces[--g_surface_count];
            return;
        }
    }
}

// ==========================================
// Math Unification
// ==========================================

int nq_map_x(NQ_Context* ctx, float x_world) {
    if (!ctx) return 0;
    if (ctx->coord_mode == NQ_LITERAL) return (int)floor(x_world + 0.5f);
    
    // Optimization: Use precomputed inverse range
    float norm = (x_world - ctx->x_min) * ctx->inv_range_x;
    int target_w = ctx->current_target ? ctx->current_target->w : 0;
    return (int)floor(norm * target_w + 0.5f);
}

int nq_map_y(NQ_Context* ctx, float y_world) {
    if (!ctx) return 0;
    if (ctx->coord_mode == NQ_LITERAL) return (int)floor(y_world + 0.5f);

    // Optimization: Use precomputed inverse range
    float norm = (y_world - ctx->y_min) * ctx->inv_range_y;
    int target_h = ctx->current_target ? ctx->current_target->h : 0;
    return (int)floor(norm * target_h + 0.5f);
}

int nq_map_scalar(NQ_Context* ctx, float scalar_world) {
    if (!ctx) return 0;
    if (ctx->coord_mode == NQ_LITERAL) return (int)scalar_world;
    
    // Use absolute value of inverse range for scalar mapping (magnitudes)
    float inv_width = fabsf(ctx->inv_range_x);
    int target_w = ctx->current_target ? ctx->current_target->w : 0;
    return (int)floor((scalar_world * inv_width) * target_w + 0.5f);
}

float nq_inv_map_x(NQ_Context* ctx, int x_pixel) {
    if (!ctx) return 0.0f;
    if (ctx->coord_mode == NQ_LITERAL) return (float)x_pixel;
    
    int target_w = ctx->current_target ? ctx->current_target->w : 1;
    if (target_w == 0) return 0.0f;
    
    float norm = (float)x_pixel / (float)target_w;
    return ctx->x_min + norm * (ctx->x_max - ctx->x_min);
}

float nq_inv_map_y(NQ_Context* ctx, int y_pixel) {
    if (!ctx) return 0.0f;
    if (ctx->coord_mode == NQ_LITERAL) return (float)y_pixel;
    
    int target_h = ctx->current_target ? ctx->current_target->h : 1;
    if (target_h == 0) return 0.0f;
    
    float norm = (float)y_pixel / (float)target_h;
    return ctx->y_min + norm * (ctx->y_max - ctx->y_min);
}

float nq_delta_time(void) {
    Uint32 now = SDL_GetTicks();
    if (g_last_time == 0) {
        g_last_time = now;
        return 0.0f;
    }
    float dt = (now - g_last_time) / 1000.0f;
    g_last_time = now;
    return dt;
}

// ==========================================
// Funções Auxiliares Internas
// ==========================================

static NQ_Context* create_context_struct(int id) {
    NQ_Context* ctx = (NQ_Context*)malloc(sizeof(NQ_Context));
    if (ctx) {
        ctx->id = id;
        ctx->window = NULL;
        ctx->renderer = NULL;
        ctx->texture = NULL;
        ctx->canvas = NULL;
        ctx->current_target = NULL;
        ctx->coord_mode = NQ_LITERAL;
        ctx->scale_mode = NQ_SCALE_FIXED;
        ctx->running = true;
        
        ctx->x_min = 0.0f; ctx->x_max = 1.0f;
        ctx->y_min = 0.0f; ctx->y_max = 1.0f;
        
        ctx->pen_color = (SDL_Color){255, 255, 255, 255};
        ctx->bg_color = (SDL_Color){0, 0, 0, 255};
        ctx->axis_color = (SDL_Color){100, 100, 100, 255}; // Default gray for axis
        ctx->grid_color = (SDL_Color){40, 40, 40, 255};    // Darker gray for grid
        ctx->line_width = 1.0f;
        
        ctx->camera_x = 0.5f;
        ctx->camera_y = 0.5f;
        ctx->camera_angle = 0.0f;
        ctx->camera_zoom = 1.0f;
        
        ctx->wheel_delta = 0;
        ctx->font_size = 12; // Base size for text
        
        // Scale defaults
        ctx->window_scale = 1.0f;
        ctx->offset_x = 0;
        ctx->offset_y = 0;
    }
    return ctx;
}

static void set_active_context(NQ_Context* ctx) {
    if (g_active_ctx != ctx) {
        g_active_ctx = ctx;
    }
}

// Acessor interno para nq_draw
NQ_Context* nq_get_active_context(void) {
    return g_active_ctx;
}

static NQ_Context* find_context_by_window_id(Uint32 id) {
    for (int i = 0; i < g_context_count; i++) {
        if (g_contexts[i] && g_contexts[i]->window && SDL_GetWindowID(g_contexts[i]->window) == id) {
            return g_contexts[i];
        }
    }
    return NULL;
}

static NQ_Context* find_context_by_id(int id) {
    for (int i = 0; i < g_context_count; i++) {
        if (g_contexts[i] && g_contexts[i]->id == id) {
            return g_contexts[i];
        }
    }
    return NULL;
}

// ==========================================
// API Implementation
// ==========================================

int nq_screen(int w, int h, const char* title, NQ_ScaleMode mode) {
    // Inicializa SDL se for a primeira janela
    if (g_context_count == 0 && SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Erro ao inicializar SDL: %s\n", SDL_GetError());
        return -1;
    }

    if (g_context_count == 0) {
        // Inicializar SDL_image
        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            fprintf(stderr, "SDL_image could not initialize! IMG_Error: %s\n", IMG_GetError());
            // Warning only? No, maybe critical if they check for PNG.
        }
    }

    if (g_context_count >= MAX_CONTEXTS) {
        fprintf(stderr, "Erro: Limite máximo de janelas atingido.\n");
        return -1;
    }

    int new_id = g_context_count; // ID sequencial simples
    NQ_Context* ctx = create_context_struct(new_id);
    if (!ctx) return -1;

    // Criar Janela
    int window_flags = SDL_WINDOW_SHOWN;
    if (mode == NQ_SCALE_ASPECT || mode == NQ_SCALE_STRETCH) {
        window_flags |= SDL_WINDOW_RESIZABLE;
    }

    ctx->window = SDL_CreateWindow(title, 
                                   SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                   w, h, 
                                   window_flags);
    if (!ctx->window) {
        free(ctx);
        return -1;
    }

    // Criar Renderer (Hardware)
    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx->renderer) {
        SDL_DestroyWindow(ctx->window);
        free(ctx);
        return -1;
    }

    // Criar Canvas (Surface/RAM)
    ctx->canvas = (NQ_Surface*)malloc(sizeof(NQ_Surface));
    // Cria surface RGBA32
    ctx->canvas->sdl_surf = SDL_CreateRGBSurface(0, w, h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    
    // Criar Software Renderer para permitir desenhar primitivas SDL na surface
    ctx->canvas->soft_renderer = SDL_CreateSoftwareRenderer(ctx->canvas->sdl_surf);

    ctx->canvas->w = w;
    ctx->canvas->h = h;
    ctx->canvas->has_colorkey = false;
    ctx->canvas->alpha = 255;
    
    // Textura de streaming (Canvas -> GPU)
    ctx->texture = SDL_CreateTexture(ctx->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);

    // Initial state
    ctx->current_target = ctx->canvas;
    ctx->scale_mode = mode;

    g_contexts[g_context_count++] = ctx;
    set_active_context(ctx);
    
    // Default config
    nq_background(0, 0, 0);

    return new_id;
}

void nq_figure(int id) {
    NQ_Context* ctx = find_context_by_id(id);
    if (ctx) {
        set_active_context(ctx);
        SDL_RaiseWindow(ctx->window);
    }
}

void nq_poll_events() {
    // Zero out wheel delta for all contexts (simple approach for single-threaded loop)
    for(int i=0; i<g_context_count; i++) {
        if (g_contexts[i]) g_contexts[i]->wheel_delta = 0;
    }

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            g_is_running = false;
        }
        else if (e.type == SDL_MOUSEWHEEL) {
             NQ_Context* target = find_context_by_window_id(e.wheel.windowID);
             if (target) {
                 // Invert Y depending on platform? Standard is +1 up usually.
                 target->wheel_delta += e.wheel.y;
             }
        }
        
        // Auto-focus logic
        if (e.type == SDL_WINDOWEVENT) {
             if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED || 
                 e.window.event == SDL_WINDOWEVENT_ENTER) {
                NQ_Context* target = find_context_by_window_id(e.window.windowID);
                if (target) set_active_context(target);
            }
        }
    }
}

void nq_sync_all() {
    for (int i = 0; i < g_context_count; i++) {
        NQ_Context* ctx = g_contexts[i];
        if (ctx && ctx->window && ctx->renderer && ctx->canvas && ctx->texture && ctx->canvas->sdl_surf) {
            // Upload pixels RAM -> VRAM
            SDL_UpdateTexture(ctx->texture, NULL, ctx->canvas->sdl_surf->pixels, ctx->canvas->sdl_surf->pitch);
            
            // Render on screen
            
            // Clear with black for potential letterboxing
            SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255);
            SDL_RenderClear(ctx->renderer);

            if (ctx->scale_mode == NQ_SCALE_ASPECT) {
                int win_w, win_h;
                SDL_GetWindowSize(ctx->window, &win_w, &win_h);
                
                float scale_w = (float)win_w / (float)ctx->canvas->w;
                float scale_h = (float)win_h / (float)ctx->canvas->h;
                
                // Use smallest scale to fit
                float scale = (scale_w < scale_h) ? scale_w : scale_h;
                
                int final_w = (int)(ctx->canvas->w * scale);
                int final_h = (int)(ctx->canvas->h * scale);
                
                // Center
                int offset_x = (win_w - final_w) / 2;
                int offset_y = (win_h - final_h) / 2;
                
                // Store layout for input transformation
                ctx->window_scale = scale;
                ctx->offset_x = offset_x;
                ctx->offset_y = offset_y;
                
                SDL_Rect dst_rect = {offset_x, offset_y, final_w, final_h};
                SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, &dst_rect);
            } else {
                // STRETCH or FIXED (if handled by OS window manager)
                // For simplicity, default SDL behavior stretches to window size
                
                // Just update scale for input (approximate if stretch)
                int win_w, win_h;
                SDL_GetWindowSize(ctx->window, &win_w, &win_h);
                ctx->window_scale = (float)win_w / (float)ctx->canvas->w; // Non-uniform scale not fully supported in input map logic yet
                ctx->offset_x = 0;
                ctx->offset_y = 0;

                SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);
            }

            SDL_RenderPresent(ctx->renderer);
        }
    }
}

void nq_close() {
    // Limpar superfícies criadas pelo usuário
    for (int i = 0; i < g_surface_count; i++) {
        NQ_Surface* surf = g_surfaces[i];
        if (surf) {
            if (surf->soft_renderer) SDL_DestroyRenderer(surf->soft_renderer);
            if (surf->sdl_surf) SDL_FreeSurface(surf->sdl_surf);
            free(surf);
            g_surfaces[i] = NULL;
        }
    }
    g_surface_count = 0;

    for (int i = 0; i < g_context_count; i++) {
        NQ_Context* ctx = g_contexts[i];
        if (ctx) {
            if (ctx->texture) SDL_DestroyTexture(ctx->texture);
            if (ctx->renderer) SDL_DestroyRenderer(ctx->renderer);
            if (ctx->window) SDL_DestroyWindow(ctx->window);
            
            if (ctx->canvas) {
                if (ctx->canvas->soft_renderer) SDL_DestroyRenderer(ctx->canvas->soft_renderer);
                if (ctx->canvas->sdl_surf) SDL_FreeSurface(ctx->canvas->sdl_surf);
                free(ctx->canvas);
            }
            
            free(ctx);
            g_contexts[i] = NULL;
        }
    }
    g_context_count = 0;
    g_active_ctx = NULL;

    IMG_Quit();
    SDL_Quit();
}

bool nq_running() {
    return g_is_running;
}

// ==========================================
// CAMERA MAPPING
// ==========================================

// Maps World Point (wx, wy) -> Screen Point (sx, sy) considering Camera (x, y, zoom, angle)
void nq_map_point(NQ_Context* ctx, float wx, float wy, int* sx, int* sy) {
    if (!ctx) return;
    
    // 0. Base Screen Dimensions
    int sw = 0, sh = 0;
    if (ctx->current_target) {
        sw = ctx->current_target->w;
        sh = ctx->current_target->h;
    } else {
        SDL_GetWindowSize(ctx->window, &sw, &sh);
    }
    
    // 1. Calculate Pixels Per Unit (PPU) at Zoom 1.0 (View Reference)
    // Based on the user defined x_min/x_max filling the screen initially
    float range_x = ctx->x_max - ctx->x_min;
    float range_y = ctx->y_max - ctx->y_min; // Can be negative for flipped Y
    
    float ppu_x = (float)sw / fabsf(range_x);
    // Usually we want aspect ratio correct PPU if in Aspect mode.
    // Simplifying: Use X range to define scale if mostly square, or just use separate.
    float ppu_y = (float)sh / fabsf(range_y); 
    
    if (ctx->scale_mode == NQ_SCALE_ASPECT) {
        // Use the smaller PPU to fit both within screen? 
        // Or strictly follow X? Standard is usually fitting the defined box.
        // Let's use the X PPU for both to keep circles circular.
        ppu_y = ppu_x; 
        // Wait, if Y range is flipped (-10 to 10 vs 10 to -10), sign matters?
        // Inverse math logic: Y usually goes UP in world, DOWN in screen.
        // If y_max > y_min (e.g. -7.5 to 7.5), and we want 7.5 at Top (0) and -7.5 at Bottom (h).
        // Then we need to flip.
    }
    
    // 2. Translate world point relative to camera
    float dx = wx - ctx->camera_x;
    float dy = wy - ctx->camera_y;
    
    // 3. Rotate (Camera Angle)
    // If camera rotates +Theta, world seems to rotate -Theta.
    float c = cosf(-ctx->camera_angle);
    float s = sinf(-ctx->camera_angle);
    float rx = dx * c - dy * s;
    float ry = dx * s + dy * c;
    
    // 4. Scale by Camera Zoom
    rx *= ctx->camera_zoom;
    ry *= ctx->camera_zoom;
    
    // 5. Map to Screen Coordinates (Center relative)
    float center_x = sw / 2.0f;
    float center_y = sh / 2.0f;
    
    *sx = (int)(center_x + rx * ppu_x);
    
    // Y-Axis Flip Logic
    // If we want standard cartesian (Up is +), and screen is (Down is +).
    // If y_min < y_max (standard math), we want +Y to go UP (lower screen Y).
    // So we subtract ry.
    // However, user might have defined y_min=0 (top), y_max=100 (bottom).
    // Let's trust the sign of range_y? 
    // If y_max > y_min, it's standard Cartesian usually.
    // If default setup (-7.5 to 7.5) -> y_max=7.5 > y_min=-7.5.
    // We want +dy to result in -sy.
    // So: center_y - ry * ppu_y.
    
    // What if user set y_min=0, y_max=100 (Screen like)?
    // y_max > y_min. range is +, map expects 0->min, 100->max?
    // If min is top, then range is positive 100?
    // Actually nq_setup_coords usually takes (0, 800, 0, 600).
    // Then (400,300) is center. Camera at 400,300.
    // dy = +10. We want +10 screen pixels.
    // So we should ADD if coords are "screen like".
    
    // Let's check nq_setup_coords definition in nq_draw
    // Standard setup: nq_setup_coords(-10, 10, -7.5, 7.5). Max Y is Top (for math).
    // Wait, in previous demos: nq_setup_coords(-10.0f, 10.0f, -7.5f, 7.5f);
    // And drawing function: (4.0 / n*PI) * sin.
    // Positive result implies Up?
    // Let's replicate  logic?
    
    // Current :
    // (y - y_min) * (sh / (y_max - y_min)). NO?
    // Usually it includes offset and flip.
    
    // Let's look at  generic map function again.
    // nq_map_y implementation is NOT visible in context?
    // I need to read nq_map_y implementation to be consistent.
    // But for GLOBAL CAMERA, we replace the logic essentially.
    
    // Assumption: Standard Cartesian: Y increases UP. Screen Y increases DOWN.
    // So we subtract.
    
    if (ctx->y_max > ctx->y_min) {
        // Cartesian-ish (or user wants Y to grow opposite to screen?)
        // Let's assume standard Math mode for now as default for camera.
        *sy = (int)(center_y - ry * ppu_y);
    } else {
        // Screen-ish (Y grows down)
        *sy = (int)(center_y + ry * ppu_y);
    }
}

// Inverse Map (Screen -> World)
void nq_inv_map_point(NQ_Context* ctx, int sx, int sy, float* wx, float* wy) {
    if (!ctx) return;
    
    int sw = 0, sh = 0;
    if (ctx->current_target) {
        sw = ctx->current_target->w;
        sh = ctx->current_target->h;
    } else {
        SDL_GetWindowSize(ctx->window, &sw, &sh);
    }
    
    // 1. Calculate Pixels Per Unit
    float range_x = ctx->x_max - ctx->x_min;
    float range_y = ctx->y_max - ctx->y_min;
    
    float ppu_x = (float)sw / fabsf(range_x);
    float ppu_y = (float)sh / fabsf(range_y);
    
    if (ctx->scale_mode == NQ_SCALE_ASPECT) {
        ppu_y = ppu_x; 
    }
    
    float center_x = sw / 2.0f;
    float center_y = sh / 2.0f;
    
    // 2. Relative from Center (Screen Space)
    float rx = (sx - center_x) / ppu_x;
    float ry = 0.0f;
    
    // Handle Flip logic - same as forward mapping check
    if (ctx->y_max > ctx->y_min) {
        // Cartesian (Up is +), Screen (Down is +). So ScreenY = Center - WorldY.
        // Therefore WorldY = Center - ScreenY.
        ry = (center_y - sy) / ppu_y;
    } else {
        // Screen-like (Down is +). ScreenY = Center + WorldY.
        // WorldY = ScreenY - Center.
        ry = (sy - center_y) / ppu_y;
    }
    
    // 3. Un-Zoom
    rx /= ctx->camera_zoom;
    ry /= ctx->camera_zoom;
    
    // 4. Un-Rotate (Inverse of Rotation by -Angle is Rotation by +Angle)
    // Forward was: dx*cos(-A) - dy*sin(-A)
    // Inverse is: rx*cos(A) - ry*sin(A) (Wait? No)
    // Let Forward Mapping be M. M = R(-theta) * S * T(-cam).
    // Inverse: T(+cam) * S^(-1) * R(+theta).
    // So we apply R(+theta) then T(+cam).
    // Rotating (rx, ry) by +theta:
    float c = cosf(ctx->camera_angle);
    float s = sinf(ctx->camera_angle);
    
    float dx = rx * c - ry * s;
    float dy = rx * s + ry * c;
    
    // 5. Un-Translate (Add Camera Pos)
    *wx = dx + ctx->camera_x;
    *wy = dy + ctx->camera_y;
}

int nq_map_scalar_cam(NQ_Context* ctx, float s) {
    if (!ctx) return 0;
    int sw = 0; //, sh = 0;
    if (ctx->current_target) {
        sw = ctx->current_target->w;
        // sh = ctx->current_target->h;
    } else {
        SDL_GetWindowSize(ctx->window, &sw, NULL); //, &sh);
    }
    
    float range_x = ctx->x_max - ctx->x_min;
    float ppu_x = (float)sw / fabsf(range_x);
    
    // Zoom affects scalar size
    return (int)( s * ppu_x * ctx->camera_zoom );
}
