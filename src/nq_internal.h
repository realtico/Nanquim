#ifndef NQ_INTERNAL_H
#define NQ_INTERNAL_H

#include "nanquim.h"

// ==========================================
// Internal Global State Access
// ==========================================
// Defined in nq_core.c
extern NQ_Context* nq_get_active_context(void);
extern void nq_register_surface(NQ_Surface* surf);
extern void nq_unregister_surface(NQ_Surface* surf);

// Internal helper to copy a region from one surface to a new one
// Handles blend mode to ensure alpha is copied correctly
NQ_Surface* nq_copy_rect(NQ_Surface* src, NQ_Rect r);

// ==========================================
// Internal Math Helpers (Shared across modules)
// ==========================================
// Defined in nq_core.c

// Maps world X to screen pixel X
int nq_map_x(NQ_Context* ctx, float x_world);

// Maps world Y to screen pixel Y
int nq_map_y(NQ_Context* ctx, float y_world);

// Maps a scalar magnitude (like radius) from world to screen pixels
int nq_map_scalar(NQ_Context* ctx, float scalar_world);
int nq_map_scalar_cam(NQ_Context* ctx, float scalar_world);

// Inverse mapping: Screen Pixel X -> World X
float nq_inv_map_x(NQ_Context* ctx, int x_pixel);

// Inverse mapping: Screen Pixel Y -> World Y
float nq_inv_map_y(NQ_Context* ctx, int y_pixel);

// New global mapping function handling camera
void nq_map_point(NQ_Context* ctx, float wx, float wy, int* sx, int* sy);

// Inverse Map (Screen -> World) handling camera
void nq_inv_map_point(NQ_Context* ctx, int sx, int sy, float* wx, float* wy);

#endif // NQ_INTERNAL_H
