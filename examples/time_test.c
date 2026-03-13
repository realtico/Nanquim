#include <stdio.h>
#include "../include/nanquim.h"
#include <math.h>

// Time Demo: Moving Circle
// Shows frame-rate independent movement using nq_delta_time()

int main() {
    nq_screen(800, 600, "Nanquim Time Demo (FPS Independent)", NQ_SCALE_FIXED);
    nq_setup_coords(-10.0, 10.0, -7.5, 7.5); // 4:3 Aspect

    float circle_x = -12.0; // Start off-screen
    float speed = 5.0;      // 5 units per second

    char fps_text[64];

    while(nq_running()) {
        nq_poll_events();
        
        // 1. Calculate Delta Time
        float dt = nq_delta_time();
        
        // 2. Clear Screen
        nq_background(40, 40, 50);
        
        // 3. Update Logic (FPS Independent)
        circle_x += speed * dt;
        
        // Wrap around
        if (circle_x > 12.0) circle_x = -12.0;

        // 4. Draw
        // Grid (1 unit spacing)
        nq_color(60, 60, 70);
        for(float x=-10; x<=10; x+=1.0) nq_line(x, -7.5, x, 7.5);
        for(float y=-6; y<=6; y+=1.0) nq_line(-10, y, 10, y);

        // Moving Circle
        nq_color(255, 100, 100);
        nq_circle(circle_x, 0.0, 0.5, true);
        
        // Label
        nq_color(255, 255, 255);
        snprintf(fps_text, 64, "Speed: %.1f units/sec (5 grid squares/s) | DT: %.4f s | FPS: %.0f", speed, dt, (dt > 0 ? 1.0/dt : 0));
        nq_put_text(-9.5, 7.0, fps_text);
        
        nq_put_text(-9.5, 6.5, "Grid spacing is 1.0 unit. Movement speed is constant regardless of FPS.");

        nq_sync_all();
        // Emulate heavy load? No, let it run as fast as VSync allows (or not).
    }
    
    nq_close();
    return 0;
}
