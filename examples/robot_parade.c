#include <stdio.h>
#include "../include/nanquim.h"
#include <math.h>

// Robot Parade Demo (Refactored)
// Validates: Atlas/Region Drawing, Animation Timer Fix, SDL Hiding

// Frame Constants
#define FRAME_W 256
#define FRAME_H 289
#define START_X 0
#define START_Y 363
#define X_STEP 256
#define NUM_FRAMES 4 

int main() {
    if (nq_screen(800, 600, "O Desfile do Robo (Atlas Version)", NQ_SCALE_ASPECT) < 0) {
        return -1;
    }
    
    // Setup World
    nq_setup_coords(-2.0, 2.0, -1.5, 1.5); 
    
    // Background Color
    nq_background(30, 30, 30);

    // 1. Load Sprite Sheet (Atlas)
    NQ_Surface* sheet = nq_load_sprite("assets/robot.png");
    if (!sheet) {
        printf("Error loading assets/robot.png\n");
        return -1;
    }
    
    // Set Color Key (MAGENTA) on the sheet
    // This allows nq_copy_rect/nq_draw_region to respect the transparent background
    nq_set_colorkey(sheet, 255, 0, 255);
    
    // 2. Define Frames using NQ_Rect (No SDL types)
    NQ_Rect frames[NUM_FRAMES];
    for(int i=0; i<NUM_FRAMES; i++) {
        frames[i].x = START_X + (i * X_STEP);
        frames[i].y = START_Y;
        frames[i].w = FRAME_W;
        frames[i].h = FRAME_H;
    }
    
    // 3. Animation State
    float timer = 0.0f;
    const float frame_duration = 0.2f; // Smooth animation
    int current_frame = 0;
    bool walking_right = true;
    
    float robot_x = -1.5f;
    float robot_y = 0.0f;
    float speed = 1.0f;
    
    // Robot Dimensions in World
    float world_h = 1.0f;
    float world_w = ((float)FRAME_W / (float)FRAME_H) * world_h;
    
    while(nq_running()) {
        nq_poll_events();

        if (nq_key_down(NQ_KEY_ESCAPE)) break;
        
        float dt = nq_delta_time();
        
        // Animation Logic (Fixed Timer: Subtract interval instead of reset)
        timer += dt;
        if (timer >= frame_duration) {
            timer -= frame_duration;
            current_frame = (current_frame + 1) % NUM_FRAMES;
        }
        
        // Movement Logic
        if (walking_right) {
            robot_x += speed * dt;
            if (robot_x > 1.5f) { 
                walking_right = false;
            }
        } else {
            robot_x -= speed * dt;
            if (robot_x < -1.5f) {
                walking_right = true;
            }
        }
        
        // Render Scene
        nq_background(40, 44, 52); 
        
        // Grid Lines
        nq_color(60, 60, 70);
        nq_line(-2, 0, 2, 0);
        nq_line(0, -1.5, 0, 1.5);
        
        // Draw Robot using Atlas Region
        // walking_right needs FLIP (nq_blit_ex convention: negative width flips)
        // But nq_draw_region takes explicit bool
        bool flip = walking_right;
        
        nq_draw_region(sheet, frames[current_frame], robot_x, robot_y, world_w, world_h, 0.0f, flip);
        
        // Text Info (if supported)
        nq_color(255, 255, 255);
        // Original nq_put_text call removed because definitions might be missing in header context
        // But logic is cleaner without dependency if nq_put_text isn't in core context I edited.
        // Assuming nq_put_text is available, uncomment:
        // char buffer[64];
        // snprintf(buffer, 64, "FPS: %.0f", (dt > 0 ? 1.0/dt : 0));
        // nq_put_text(-1.9, 1.4, buffer);
        
        nq_sync_all();
    }
    
    // Cleanup
    nq_free_surface(sheet);
    nq_close();
    return 0;
}
