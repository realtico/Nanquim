#include "nanquim.h"
#include "nq_internal.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include <stdio.h>

// ==========================================
// API ASSETS & BUFFERS
// ==========================================

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
    
    nq_register_surface(surf);
    return surf;
}

void nq_free_surface(NQ_Surface* surf) {
    if (!surf) return;
    
    // Remove from global tracking
    nq_unregister_surface(surf);
    
    if (surf->soft_renderer) {
        SDL_DestroyRenderer(surf->soft_renderer);
    }
    
    if (surf->sdl_surf) {
        SDL_FreeSurface(surf->sdl_surf);
    }
    
    free(surf);
}

// Internal helper for copying regions (Atlas support)
NQ_Surface* nq_copy_rect(NQ_Surface* src, NQ_Rect r) {
    if (!src || !src->sdl_surf) return NULL;
    
    // Create destination surface of the region size
    NQ_Surface* dest = nq_create_surface(r.w, r.h);
    if (!dest) return NULL;
    
    SDL_Rect src_rect = {r.x, r.y, r.w, r.h};
    
    // Save original blend mode (we need NONE to copy alpha values correctly)
    SDL_BlendMode old_mode;
    SDL_GetSurfaceBlendMode(src->sdl_surf, &old_mode);
    SDL_SetSurfaceBlendMode(src->sdl_surf, SDL_BLENDMODE_NONE);
    
    // Copy pixels
    SDL_BlitSurface(src->sdl_surf, &src_rect, dest->sdl_surf, NULL);
    
    // Restore blend mode
    SDL_SetSurfaceBlendMode(src->sdl_surf, old_mode);
    
    // Copy colorkey settings if present
    if (src->has_colorkey) {
        nq_set_colorkey(dest, (Uint8)(src->colorkey >> 16), (Uint8)(src->colorkey >> 8), (Uint8)src->colorkey);
    }
    
    return dest;
}

NQ_Surface* nq_load_sprite(const char* path) {
    // Carregamento via SDL_image (suporta PNG, JPG, BMP)
    SDL_Surface* tmp = IMG_Load(path);
    if (!tmp) {
        fprintf(stderr, "NQ Error: Failed to load sprite %s: %s\n", path, IMG_GetError());
        return NULL;
    }
    
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
        nq_register_surface(surf);
        return surf;
    } else {
        free(surf);
        return NULL;
    }
}

void nq_set_colorkey(NQ_Surface* surf, Uint8 r, Uint8 g, Uint8 b) {
    if (!surf || !surf->sdl_surf) return;

    // 1. Ensure format is ARGB8888 for safe pixel manipulation
    if (surf->sdl_surf->format->format != SDL_PIXELFORMAT_ARGB8888) {
        SDL_Surface* converted = SDL_ConvertSurfaceFormat(surf->sdl_surf, SDL_PIXELFORMAT_ARGB8888, 0);
        if (converted) {
            SDL_FreeSurface(surf->sdl_surf);
            surf->sdl_surf = converted;
        } else {
            // Conversion failed, log error or return
             return; 
        }
    }
    
    // 2. Set SDL native colorkey for compatibility
    Uint32 key = SDL_MapRGB(surf->sdl_surf->format, r, g, b);
    SDL_SetColorKey(surf->sdl_surf, SDL_TRUE, key);
    
    // 3. Iterate pixels and set Alpha=0 for matching color (Baking transparency)
    SDL_Surface* s = surf->sdl_surf;
    
    if (SDL_MUSTLOCK(s)) SDL_LockSurface(s);
    
    int pixel_count = s->w * s->h;
    Uint32* pixels = (Uint32*)s->pixels; // Safe cast after conversion
    
    for (int i = 0; i < pixel_count; i++) {
        Uint8 pr, pg, pb, pa;
        SDL_GetRGBA(pixels[i], s->format, &pr, &pg, &pb, &pa);
        
        if (pr == r && pg == g && pb == b) {
            pixels[i] = SDL_MapRGBA(s->format, pr, pg, pb, 0);
        }
    }
    
    if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(s);

    surf->has_colorkey = true;
    surf->colorkey = key;
}
