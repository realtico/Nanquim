#include "nanquim.h"
#include <stdio.h>
#include <stdlib.h>

// ==========================================
// Estado Global Interno
// ==========================================

#define MAX_CONTEXTS 10
static NQ_Context* g_contexts[MAX_CONTEXTS];
static int g_context_count = 0;
// O ponteiro global crucial
NQ_Context* g_active_ctx = NULL;
static bool g_is_running = true;

// Rastreamento de superfícies para evitar memory leaks
#define MAX_SURFACES 100
static NQ_Surface* g_surfaces[MAX_SURFACES];
static int g_surface_count = 0;

void nq_register_surface(NQ_Surface* surf) {
    if (g_surface_count < MAX_SURFACES) {
        g_surfaces[g_surface_count++] = surf;
    }
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
        
        ctx->wheel_delta = 0;
        ctx->font_size = 12; // Base size for text
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

    if (g_context_count >= MAX_CONTEXTS) {
        fprintf(stderr, "Erro: Limite máximo de janelas atingido.\n");
        return -1;
    }

    int new_id = g_context_count; // ID sequencial simples
    NQ_Context* ctx = create_context_struct(new_id);
    if (!ctx) return -1;

    // Criar Janela
    ctx->window = SDL_CreateWindow(title, 
                                   SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                   w, h, 
                                   SDL_WINDOW_SHOWN);
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
        if (ctx && ctx->window && ctx->renderer && ctx->canvas && ctx->texture) {
            // Upload pixels RAM -> VRAM
            SDL_UpdateTexture(ctx->texture, NULL, ctx->canvas->sdl_surf->pixels, ctx->canvas->sdl_surf->pitch);
            
            // Render on screen
            SDL_RenderClear(ctx->renderer);
            SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);
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
    SDL_Quit();
}

bool nq_running() {
    return g_is_running;
}
