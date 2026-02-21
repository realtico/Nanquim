/*
 * Nanquim Framework v0.1 - Engine de Simulação Científica
 */

#include "nanquim.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <math.h>
#include <stdlib.h>

// Acesso externo ao contexto ativo (definido em nq_core.c)
extern NQ_Context* nq_get_active_context(void);

// ==========================================
// Utils Matemáticos
// ==========================================

static int map_x(NQ_Context* ctx, float x_world) {
    if (ctx->coord_mode == NQ_LITERAL) return (int)floor(x_world + 0.5f);
    if (fabs(ctx->x_max - ctx->x_min) < 0.000001) return 0; // Evita div zero
    
    // $$x_{pixel} = \frac{x_{world} - x_{min}}{x_{max} - x_{min}} \cdot \text{width}$$
    float norm = (x_world - ctx->x_min) / (ctx->x_max - ctx->x_min);
    int target_w = ctx->current_target ? ctx->current_target->w : 0;
    
    // Precisão refinada: floor(norm * dim + 0.5f) para arredondamento correto
    return (int)floor(norm * target_w + 0.5f);
}

static int map_y(NQ_Context* ctx, float y_world) {
    if (ctx->coord_mode == NQ_LITERAL) return (int)floor(y_world + 0.5f);
    if (fabs(ctx->y_max - ctx->y_min) < 0.000001) return 0;

    /*
     * Coordinate Mapping Logic:
     * We map [y_min, y_max] -> [0, target_h].
     * 
     * - Standard Sreen (Y-Down): y_min=0 (Top/0), y_max=H (Bottom/H)
     * - Cartesian (Y-Up): Pass y_min=TopValue, y_max=BottomValue.
     *   Since map_y maps y_min -> Pixel 0 (Top), setting y_min to the 
     *   positive world top coordinate effectively inverts the axis.
     */
    float norm = (y_world - ctx->y_min) / (ctx->y_max - ctx->y_min);
    
    int target_h = ctx->current_target ? ctx->current_target->h : 0;
    return (int)floor(norm * target_h + 0.5f);
}

static int map_scalar(NQ_Context* ctx, float scalar_world) {
    if (ctx->coord_mode == NQ_LITERAL) return (int)scalar_world;
    
    // Mapeia uma magnitude escalar (como raio) baseando-se na escala do eixo X (assumindo pixel quadrado)
    float world_width = fabsf(ctx->x_max - ctx->x_min);
    if (world_width < 0.000001) return 0;
    
    int target_w = ctx->current_target ? ctx->current_target->w : 0;
    return (int)floor((scalar_world / world_width) * target_w + 0.5f);
}

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
    if (ctx) {
        ctx->coord_mode = NQ_WORLD;
        ctx->x_min = x_min;
        ctx->x_max = x_max;
        ctx->y_min = y_min;
        ctx->y_max = y_max;
    }
}

void nq_color(Uint8 r, Uint8 g, Uint8 b) {
    NQ_Context* ctx = nq_get_active_context();
    if (ctx) {
        ctx->pen_color = (SDL_Color){r, g, b, 255};
        
        // Atualiza cor de desenho no soft renderer também
        if (ctx->current_target && ctx->current_target->soft_renderer) {
            SDL_SetRenderDrawColor(ctx->current_target->soft_renderer, r, g, b, 255);
        }
    }
}

void nq_background(Uint8 r, Uint8 g, Uint8 b) {
    NQ_Context* ctx = nq_get_active_context();
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
    if (!ctx || !ctx->current_target) return;

    int px = map_x(ctx, x);
    int py = map_y(ctx, y);
    
    // Caminho rápido: acesso direto à memória (sem overhead de SDL_RenderDrawPoint)
    SDL_Surface* surf = ctx->current_target->sdl_surf;
    Uint32 pixel = SDL_MapRGBA(surf->format, ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
    
    if (SDL_MUSTLOCK(surf)) SDL_LockSurface(surf);
    put_pixel_surface(surf, px, py, pixel);
    if (SDL_MUSTLOCK(surf)) SDL_UnlockSurface(surf);
}

void nq_line(float x1, float y1, float x2, float y2) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx || !ctx->current_target || !ctx->current_target->soft_renderer) return;

    int px1 = map_x(ctx, x1);
    int py1 = map_y(ctx, y1);
    int px2 = map_x(ctx, x2);
    int py2 = map_y(ctx, y2);

    // Usando Software Renderer para desenhar na Surface com SDL2_gfx ou Renderer padrão
    // Usaremos lineRGBA para consistência com o pedido de usar SDL2_gfx
    lineRGBA(ctx->current_target->soft_renderer, 
             px1, py1, px2, py2, 
             ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
}

void nq_box(float x1, float y1, float x2, float y2, bool filled) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx || !ctx->current_target || !ctx->current_target->soft_renderer) return;

    int px1 = map_x(ctx, x1);
    int py1 = map_y(ctx, y1);
    int px2 = map_x(ctx, x2);
    int py2 = map_y(ctx, y2);
    
    // Normalizar para garantir xmin < xmax (necessário para rectangleRGBA?) 
    // rectangleRGBA desenha entre x1,y1 e x2,y2. Ordem n importa.
    
    if (filled) {
        boxRGBA(ctx->current_target->soft_renderer, 
               px1, py1, px2, py2,
               ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
    } else {
        rectangleRGBA(ctx->current_target->soft_renderer, 
               px1, py1, px2, py2,
               ctx->pen_color.r, ctx->pen_color.g, ctx->pen_color.b, ctx->pen_color.a);
    }
}

void nq_circle(float x, float y, float r, bool filled) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx || !ctx->current_target || !ctx->current_target->soft_renderer) return;

    int px = map_x(ctx, x);
    int py = map_y(ctx, y);
    int pr = map_scalar(ctx, r);

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

/* --- API ASSETS & BUFFERS --- */

NQ_Surface* nq_create_surface(int w, int h) {
    NQ_Surface* surf = (NQ_Surface*)malloc(sizeof(NQ_Surface));
    surf->sdl_surf = SDL_CreateRGBSurface(0, w, h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    
    if (surf->sdl_surf) {
        surf->soft_renderer = SDL_CreateSoftwareRenderer(surf->sdl_surf);
    } else {
        surf->soft_renderer = NULL;
    }
    
    surf->w = w;
    surf->h = h;
    surf->has_colorkey = false;
    surf->alpha = 255;
    return surf;
}

NQ_Surface* nq_load_sprite(const char* path) {
    // Placeholder para carregamento de BMP
    // Requer SDL_LoadBMP ou similares.
    SDL_Surface* tmp = SDL_LoadBMP(path);
    if (!tmp) return NULL;
    
    NQ_Surface* surf = (NQ_Surface*)malloc(sizeof(NQ_Surface));
    // Converter para RGBA32 para compatibilidade
    surf->sdl_surf = SDL_ConvertSurfaceFormat(tmp, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(tmp);
    
    if (surf->sdl_surf) {
        surf->soft_renderer = SDL_CreateSoftwareRenderer(surf->sdl_surf);
        surf->w = surf->sdl_surf->w;
        surf->h = surf->sdl_surf->h;
        surf->has_colorkey = false;
        surf->alpha = 255;
        return surf;
    } else {
        free(surf);
        return NULL;
    }
}

void nq_set_target(NQ_Surface* target) {
    NQ_Context* ctx = nq_get_active_context();
    if (ctx && target) {
        ctx->current_target = target;
    }
}

void nq_reset_target() {
    NQ_Context* ctx = nq_get_active_context();
    if (ctx && ctx->canvas) {
        ctx->current_target = ctx->canvas;
    }
}

void nq_blit(NQ_Surface* src, float x, float y, NQ_Anchor anchor) {
    NQ_Context* ctx = nq_get_active_context();
    if (!ctx || !ctx->current_target || !src || !src->sdl_surf) return;
    
    int px = map_x(ctx, x);
    int py = map_y(ctx, y);
    
    SDL_Rect dst_rect;
    dst_rect.x = px;
    dst_rect.y = py;
    dst_rect.w = src->w;
    dst_rect.h = src->h;
    
    if (anchor == NQ_CENTERED) {
        dst_rect.x -= src->w / 2;
        dst_rect.y -= src->h / 2;
    }
    
    SDL_BlitSurface(src->sdl_surf, NULL, ctx->current_target->sdl_surf, &dst_rect);
}
