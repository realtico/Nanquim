#include "nanquim.h"
#include <math.h>
#include <stdio.h>

// Simple LERP helper
float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

int main(void) {
    // 1. Initialize Window
    int win_id = nq_screen(800, 600, "Nanquim: Camera Tracking Demo", NQ_SCALE_ASPECT);
    if (win_id < 0) return 1;

    // 2. Set coordinate system: Large world area
    nq_setup_coords(-20.0f, 20.0f, -15.0f, 15.0f);
    
    // 3. Create a large background surface with grid
    // Since our world is huge (-20 to 20 = width 40), let's make a texture that covers it.
    // Width 40 units. 800px screen handles 20 units width (zoom 1). So 40 units is 1600px.
    // Let's create a 1600x1200 surface to cover the world area.
    NQ_Surface* bg = nq_create_surface(1600, 1200);
    
    nq_set_target(bg);
    nq_background(30, 30, 30); // Dark grey
    
    nq_grid_color(60, 60, 60);
    nq_axis_color(100, 100, 100);
    
    // Draw grid covering the whole 40x30 area
    // Just pretend the surface IS the world view temporarily
    // We need to trick the drawing context to draw a larger grid onto this surface
    // OR: Manually draw lines.
    // OR: Just used standard grid, but the surface size matches world?
    // Let's just draw random "stars" or objects.
    
    nq_color(100, 100, 100);
    for (int i=0; i<100; i++) {
        float x = -20.0f + (rand() % 400) / 10.0f;
        float y = -15.0f + (rand() % 300) / 10.0f;
        float r = 0.2f + (rand() % 10) / 20.0f;
        nq_circle(x, y, r, true); // This draws to surface using current context mapping!
        // Current context mapping is set to -20..20 filling 800x600 screen?
        // No, we haven't changed setup_coords yet for the surface target.
        // Actually context uses same mapping for all targets usually.
        // If we want to burn a 40x30 world into a 1600x1200 texture, we need
        // 1600px / 40u = 40 ppu.
        // Our screen is 800px / 40u = 20 ppu.
        // So the surface is higher resolution.
        // If we draw to it, we rely on `map_point`.
        // `map_point` uses `current_target` size.
        // So if we switch target to 1600x1200 surface, `map_point` will re-calc PPU.
        // PPU_x = 1600 / 40 = 40.
        // Correct.
    }
    
    nq_reset_target();

    // Character State
    float char_x = 0.0f;
    float char_y = 0.0f;
    float char_angle = 0.0f;
    float speed = 0.2f;

    while (nq_running()) {
        nq_poll_events();
        
        if (nq_key_down(NQ_KEY_ESCAPE)) break;
        
        // Character Movement (WASD) relative to camera rotation?
        // Let's make character rotate with Left/Right and move Forward with Up.
        if (nq_key_down(NQ_KEY_LEFT)) char_angle += 0.05f;
        if (nq_key_down(NQ_KEY_RIGHT)) char_angle -= 0.05f;
        
        if (nq_key_down(NQ_KEY_UP)) {
            char_x += speed * cosf(char_angle);
            char_y += speed * sinf(char_angle);
        }
        if (nq_key_down(NQ_KEY_DOWN)) {
            char_x -= speed * cosf(char_angle);
            char_y -= speed * sinf(char_angle);
        }
        
        // Camera Follow Logic (Smooth)
        float cam_x = nq_camera_get_x();
        float cam_y = nq_camera_get_y();
        
        // Setup Tracking Mode
        // 1. Position Tracking
        float target_x = char_x;
        float target_y = char_y;
        
        // 2. Rotation Tracking (Optional: Camera rotates to look same way as char)
        // Let's keep camera angle fixed for now to see character rotation clearly.
        // Or smooth follow angle:
        // float target_a = -char_angle + M_PI/2; // Orient so "Up" is forward?
        
        nq_camera_look_at(lerp(cam_x, target_x, 0.1f), lerp(cam_y, target_y, 0.1f));
        
        // Use Z/X for Zoom
        if (nq_key_down(NQ_KEY_Z)) nq_camera_zoom_rel(1.02f);
        if (nq_key_down(NQ_KEY_X)) nq_camera_zoom_rel(0.98f);
        
        // Draw Frame
        nq_background(0, 0, 0); // Clear screen buffer
        
        // 1. Draw World Background (The static texture)
        // We drew it covering -20..20. So we blit it at center 0,0 with size 40x30.
        nq_blit_ex(bg, 0, 0, 40.0f, 30.0f, 0, NQ_CENTERED);
        
        // 2. Draw Character
        nq_color(255, 0, 0);
        
        // Draw a triangle for character to show orientation
        // Local shape vertices
        float r = 0.5f;
        float tip_x = char_x + r * cosf(char_angle);
        float tip_y = char_y + r * sinf(char_angle);
        
        float left_angle = char_angle + 2.5f; // ~140 deg
        float left_x = char_x + r * 0.7f * cosf(left_angle);
        float left_y = char_y + r * 0.7f * sinf(left_angle);
        
        float right_angle = char_angle - 2.5f;
        float right_x = char_x + r * 0.7f * cosf(right_angle);
        float right_y = char_y + r * 0.7f * sinf(right_angle);
        
        float vx[] = {tip_x, left_x, right_x};
        float vy[] = {tip_y, left_y, right_y};
        nq_polygon(vx, vy, 3, true);
        
        // Draw some debug text fixed on screen?
        // nq_setup_coords resets everything. 
        // For screen-space HUD, we'd need a separate context or switch modes temporarily.
        // Nanquim currently doesn't support easy "Pixel Mode" switch without breaking nq_camera setup.
        // But we can just use input.c's text at world coords near character?
        // nq_text(char_x, char_y + 1.0f, "Player");
        
        nq_sync_all();
        SDL_Delay(16);
    }
    
    nq_free_surface(bg);
    return 0;
}