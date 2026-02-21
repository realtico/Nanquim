#include <stdio.h>
#include "../include/nanquim.h"

// Demo: Mouse & Keyboard Interaction
// Features: 
// - Mouse Click to place markers
// - Mouse Scroll to change marker size
// - Keyboard Arrows to move a cursor
// - Text showing coordinates

typedef struct {
    float x, y;
    int size;
    bool active;
} Marker;

#define MAX_MARKERS 100
Marker markers[MAX_MARKERS];
int marker_count = 0;

int cursor_size = 5;

int main(void) {
    nq_screen(800, 600, "Nanquim Input Demo: Mouse & Keyboard", NQ_SCALE_FIXED);
    
    // Setup logic coordinates: -10 to 10 on X, -7.5 to 7.5 on Y (Aspet Ratio 4:3)
    nq_setup_coords(-10.0f, 10.0f, 7.5f, -7.5f); // 7.5 Top, -7.5 Bottom
    
    // Initial clear
    for(int i=0; i<MAX_MARKERS; i++) markers[i].active = false;

    float user_x = 0.0f;
    float user_y = 0.0f;

    while(nq_running()) {
        nq_poll_events();
        
        if (nq_key_down(NQ_KEY_ESCAPE)) break;

        // Background
        nq_background(30, 30, 30);
        
        // --- KEYBOARD LOGIC ---
        float speed = 0.1f;
        if (nq_key_down(NQ_KEY_LSHIFT)) speed = 0.3f; // Turbo
        
        if (nq_key_down(NQ_KEY_LEFT))  user_x -= speed;
        if (nq_key_down(NQ_KEY_RIGHT)) user_x += speed;
        if (nq_key_down(NQ_KEY_UP))    user_y += speed;
        if (nq_key_down(NQ_KEY_DOWN))  user_y -= speed;
        
        // Clear markers with SPACE
        if (nq_key_down(NQ_KEY_SPACE)) marker_count = 0;

        // --- MOUSE LOGIC ---
        float mx, my;
        nq_mouse_pos(&mx, &my);
        
        // Scroll changes cursor size
        int scroll = nq_mouse_scroll();
        if (scroll != 0) {
            cursor_size += scroll;
            if (cursor_size < 1) cursor_size = 1;
            if (cursor_size > 50) cursor_size = 50;
        }

        // Click to add marker
        if (nq_mouse_button(NQ_MOUSE_LEFT)) {
            // Simple debounce/spam control could be added, but let's allow drawing lines
             if (marker_count < MAX_MARKERS) {
                 markers[marker_count++] = (Marker){mx, my, cursor_size, true};
             } else {
                 // Ring buffer style
                 int idx = marker_count % MAX_MARKERS;
                 markers[idx] = (Marker){mx, my, cursor_size, true};
                 if (marker_count < 20000) marker_count++; // Just increment to keep ring moving
             }
        }

        // --- DRAWING ---
        
        // Grid
        nq_color(50, 50, 50);
        nq_line(-10, 0, 10, 0);
        nq_line(0, -7.5, 0, 7.5);
        
        // Markers
        nq_color(255, 100, 100);
        for(int i=0; i<MAX_MARKERS; i++) {
             // Handle ring buffer logic for display
             // int idx = i; // If we just fill linearly up to MAX
             // Actually let's just draw what we have
             if (i < marker_count && i < MAX_MARKERS) {
                 nq_circle(markers[i].x, markers[i].y, markers[i].size * 0.05f, true);
             }
        }
        
        // User Cursor (Keyboard)
        nq_color(100, 255, 100);
        nq_box(user_x - 0.2, user_y - 0.2, user_x + 0.2, user_y + 0.2, false);
        
        // Mouse Cursor (Visual feedback)
        nq_color(100, 100, 255);
        nq_circle(mx, my, cursor_size * 0.05f, false);
        
        // Text Info
        char buffer[128];
        snprintf(buffer, 128, "Mouse: (%.2f, %.2f)", mx, my);
        nq_color(255, 255, 255);
        nq_put_text(mx + 0.5, my + 0.5, buffer); // Offset slightly
        
        snprintf(buffer, 128, "Markers: %d | Scroll Size: %d", (marker_count > MAX_MARKERS ? MAX_MARKERS : marker_count), cursor_size);
        // Put at fixed position (Top Left roughly)
        nq_put_text(-9.5, 7.0, buffer);
        
        nq_put_text(-9.5, 6.5, "[Arrows] Move Box | [Click] Draw | [Scroll] Size | [Space] Clear");

        nq_sync_all();
    }
    
    nq_close();
    return 0;
}
