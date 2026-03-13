#include "nanquim.h"
#include <math.h>
#include <stdio.h>

// Global state for interaction
int rose_k = 3;

// Helper to draw the polar rose
void draw_rose() {
    nq_color(255, 100, 150); // Pinkish
    nq_line_weight(2.0f);

    float prev_x, prev_y;
    bool first = true;
    
    // Rose curve r = cos(k * theta)
    // Period depends on k. For integer k, 2*PI is usually enough (sometimes PI for odd k).
    // range 0 to 2PI is safe for integer k.

    int steps = 200;
    for (int i = 0; i <= steps; i++) {
        float theta = (float)i / steps * 2.0f * M_PI;
        
        // Rose formula: r = cos(k * theta)
        float r = cosf(rose_k * theta);
        
        // Convert polar to cartesian
        float x = r * cosf(theta);
        float y = r * sinf(theta);

        if (!first) {
            nq_line(prev_x, prev_y, x, y);
        }
        prev_x = x;
        prev_y = y;
        first = false;
    }
}

int main(void) {
    // 1. Initialize Window
    int win_id = nq_screen(800, 600, "Nanquim: Polar Rose Demo", NQ_SCALE_ASPECT);
    if (win_id < 0) return 1;

    // 2. Set coordinate system: Aspect ratio 4:3
    // We want Y to be -1.5 to 1.5 (height 3)
    // X should be (4/3)*3 = 4 wide -> -2 to 2
    nq_setup_coords(-2.0f, 2.0f, -1.5f, 1.5f);
    
    // Set colors for better visibility
    nq_grid_color(60, 70, 80);
    nq_axis_color(180, 180, 190);
    
    // 3. Create static background surface (buffer) - REMOVED
    // We use dynamic infinite grid now.

    NQ_Surface* bg = nq_create_surface(800, 600);
    
    // 4. Draw static elements once to the background
    nq_set_target(bg);
    nq_background(20, 30, 40); // Dark teal-ish background
    
    // Configure Grid Colors
    nq_grid_color(60, 70, 80);
    nq_axis_color(120, 130, 140);
    
    // Draw Polar Grid with labels
    // step_r = 0.5, step_theta = PI/12 (15 deg)
    nq_draw_grid_polar(0.5f, M_PI / 12.0f, true);
    
    // 5. Reset target to screen
    nq_reset_target();

    while (nq_running()) {
        nq_poll_events(); // Handle inputs
        
        if (nq_key_down(NQ_KEY_ESCAPE)) break;

        // Camera Controls
        float speed = 0.05f / nq_camera_get_zoom();
        if (nq_key_down(NQ_KEY_LEFT)) nq_camera_pan(-speed, 0.0f);
        if (nq_key_down(NQ_KEY_RIGHT)) nq_camera_pan(speed, 0.0f);
        if (nq_key_down(NQ_KEY_UP)) nq_camera_pan(0.0f, speed);
        if (nq_key_down(NQ_KEY_DOWN)) nq_camera_pan(0.0f, -speed);
        
        if (nq_key_down(NQ_KEY_Q)) nq_camera_rotate_rel(0.05f);
        if (nq_key_down(NQ_KEY_E)) nq_camera_rotate_rel(-0.05f);
        if (nq_key_down(NQ_KEY_Z)) nq_camera_zoom_rel(1.02f);
        if (nq_key_down(NQ_KEY_X)) nq_camera_zoom_rel(0.98f);
        
        if (nq_key_down(NQ_KEY_R)) {
            nq_camera_look_at(0,0);
            nq_camera_zoom(1.0f);
            nq_camera_rotate(0.0f);
        }
        
        // Interaction: 1-9 to set number of petals parameter k
        if (nq_key_down(NQ_KEY_1)) rose_k = 1;
        if (nq_key_down(NQ_KEY_2)) rose_k = 2;
        if (nq_key_down(NQ_KEY_3)) rose_k = 3;
        if (nq_key_down(NQ_KEY_4)) rose_k = 4;
        if (nq_key_down(NQ_KEY_5)) rose_k = 5;
        if (nq_key_down(NQ_KEY_6)) rose_k = 6;
        if (nq_key_down(NQ_KEY_7)) rose_k = 7;
        if (nq_key_down(NQ_KEY_8)) rose_k = 8;
        if (nq_key_down(NQ_KEY_9)) rose_k = 9;

        // Draw Frame
        // 1. Clear Screen
        nq_background(20, 30, 40);
        
        // 2. Draw Infinite Polar Grid
        nq_draw_grid_polar(0.5f, M_PI / 12.0f, true);
        
        // 3. Draw dynamic content
        draw_rose();
        
        // nq_render() alias for nq_sync_all() ?
        nq_sync_all();
        
        SDL_Delay(16);
    }
    
    // Cleanup
    // nq_free_surface(bg);
    return 0;
}
