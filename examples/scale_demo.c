#include <stdio.h>
#include "../include/nanquim.h"
#include <math.h>

// Dynamic Scale Demo
// Features: Aspect Ratio Preservation, Mouse Tracking, Grid, ESC to exit

int main(void) {
    // 320x240 internal canvas (Low res to make scaling obvious)
    if (nq_screen(320, 240, "Dynamic Scaling Demo - Try Resizing!", NQ_SCALE_ASPECT) < 0) {
        return -1;
    }
    
    // World coordinates: -10 to 10
    nq_setup_coords(-10.0, 10.0, -7.5, 7.5);
    
    while(nq_running()) {
        nq_poll_events();
        
        // ESC to exit
        if (nq_key_down(NQ_KEY_ESCAPE)) {
            break;
        }
        
        // Update Logic
        float mx, my;
        nq_mouse_pos(&mx, &my);
        
        // Render
        nq_background(20, 20, 25);
        
        // Grid
        nq_line_weight(1.0f);
        nq_color(50, 50, 60);
        for(float x = -10; x <= 10; x += 2.0f) nq_line(x, -7.5, x, 7.5);
        for(float y = -7.5; y <= 7.5; y += 2.0f) nq_line(-10, y, 10, y);
        
        // Axes
        nq_color(100, 100, 120);
        nq_line(-10, 0, 10, 0);
        nq_line(0, -7.5, 0, 7.5);
        
        // Mouse Cursor (Crosshair)
        nq_color(255, 200, 0);
        float cursor_size = 0.5f;
        nq_line(mx - cursor_size, my, mx + cursor_size, my);
        nq_line(mx, my - cursor_size, mx, my + cursor_size);
        
        // Box at mouse
        nq_color(0, 255, 100);
        nq_box(mx - 1.0f, my - 1.0f, mx + 1.0f, my + 1.0f, false);
        
        // Canvas Border (to see aspect ratio limits)
        nq_color(255, 0, 0);
        nq_line_weight(2.0f);
        // Draw frame around world edges (approx)
        nq_box(-9.9f, -7.4f, 9.9f, 7.4f, false);

        nq_sync_all();
    }
    
    nq_close();
    return 0;
}
