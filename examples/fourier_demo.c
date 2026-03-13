#include "nanquim.h"
#include <math.h>
#include <stdio.h>

// Global state for interaction
int num_harmonics = 1;

// Helper to draw the waveform
void draw_wave(float time) {
    nq_color(255, 255, 0); // Yellow
    nq_line_weight(2.0f);

    float prev_x, prev_y;
    bool first = true;

    // Draw from x = -10 to 10
    for (float x = -10.0f; x <= 10.0f; x += 0.05f) {
        float y = 0.0f;
        // Square wave approximation: sum of odd harmonics
        for (int n = 1, k = 0; k < num_harmonics; n += 2, k++) {
            y += (4.0f / (n * M_PI)) * sinf(n * (x + time));
        }

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
    int win_id = nq_screen(800, 600, "Nanquim: Fourier Series Demo", NQ_SCALE_ASPECT);
    if (win_id < 0) return 1;

    // 2. Set coordinate system: X[-10, 10], Y[-7.5, 7.5]
    nq_setup_coords(-10.0f, 10.0f, -7.5f, 7.5f);
    
    // Configure Grid Colors
    nq_grid_color(50, 50, 70);
    nq_axis_color(100, 100, 120);
    
    // 5. Reset target to screen
    nq_reset_target();

    float time = 0.0f;

    while (nq_running()) {
        nq_poll_events(); // Handle inputs
        
        if (nq_key_down(NQ_KEY_ESCAPE)) break;

        // Camera Controls
        float speed = 0.1f / nq_camera_get_zoom(); // Adjust speed by zoom?
        if (nq_key_down(NQ_KEY_LEFT)) nq_camera_pan(-speed, 0.0f);
        if (nq_key_down(NQ_KEY_RIGHT)) nq_camera_pan(speed, 0.0f);
        if (nq_key_down(NQ_KEY_UP)) nq_camera_pan(0.0f, speed);
        if (nq_key_down(NQ_KEY_DOWN)) nq_camera_pan(0.0f, -speed);
        
        if (nq_key_down(NQ_KEY_Q)) nq_camera_rotate_rel(0.05f);
        if (nq_key_down(NQ_KEY_E)) nq_camera_rotate_rel(-0.05f);
        if (nq_key_down(NQ_KEY_Z)) nq_camera_zoom_rel(1.02f);
        if (nq_key_down(NQ_KEY_X)) nq_camera_zoom_rel(0.98f);
        
        // Reset Camera
        if (nq_key_down(NQ_KEY_R)) {
            nq_camera_look_at(0,0);
            nq_camera_zoom(1.0f);
            nq_camera_rotate(0.0f);
        }

        // Interaction: 1-9 to set number of harmonics
        // Simple check: if key is down, set harmonics. 
        if (nq_key_down(NQ_KEY_1)) num_harmonics = 1;
        if (nq_key_down(NQ_KEY_2)) num_harmonics = 2;
        if (nq_key_down(NQ_KEY_3)) num_harmonics = 3;
        if (nq_key_down(NQ_KEY_4)) num_harmonics = 4;
        if (nq_key_down(NQ_KEY_5)) num_harmonics = 5;
        if (nq_key_down(NQ_KEY_6)) num_harmonics = 6;
        if (nq_key_down(NQ_KEY_7)) num_harmonics = 7;
        if (nq_key_down(NQ_KEY_8)) num_harmonics = 8;
        if (nq_key_down(NQ_KEY_9)) num_harmonics = 9;

        // Draw Frame
        // 1. Clear Screen
        nq_background(20, 20, 30);
        
        // 2. Draw Infinite Grid
        nq_draw_grid_rect(1.0f, 1.0f, true);
        
        // 3. Draw dynamic content
        draw_wave(time);
        
        nq_sync_all();
        time += 0.05f;
        SDL_Delay(16);
    }
    
    // Cleanup
    // nq_free_surface(bg); // Not used anymore
    return 0;
}
