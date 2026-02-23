#include "nanquim.h"
#include <math.h>

int main() {
    nq_screen(800, 600, "Nanquim Geometry & Thickness Test", NQ_SCALE_FIXED);
    nq_setup_coords(-10.0, 10.0, -10.0, 10.0);
    
    // Triangle vertices
    float tri_x[] = {-8, -5, -6};
    float tri_y[] = {-8, -8, -5};
    
    // Hexagon vertices
    float hex_x[6];
    float hex_y[6];
    for(int i=0; i<6; i++) {
        float angle = i * 60.0f * (M_PI / 180.0f);
        hex_x[i] = 7.0f + 2.0f * cosf(angle);
        hex_y[i] = -7.0f + 2.0f * sinf(angle);
    }

    while(nq_running()) {
        nq_poll_events();
        
        nq_background(30,30,30);
        
        // 1. Line Thickness
        nq_put_text(-9, -2, "Line Thickness:");
        for(int i=1; i<=5; i++) {
            nq_line_weight((float)i * 2.0f);
            nq_color(255, 100 + i*30, 0);
            nq_line(-9.0f, -1.0f + i, -5.0f, -1.0f + i);
        }
        nq_line_weight(1.0f); // Reset

        // 2. Arcs
        nq_put_text(2, -2, "Arcs & Pies:");
        nq_color(0, 255, 255);
        // Filled Pie
        nq_arc(4.0f, 2.0f, 2.0f, 45.0f, 315.0f, true);
        // Outline Arc
        nq_line_weight(3.0f); // Should affect if we decompose, but arcRGBA doesn't support it.
                              // Our implementation uses arcRGBA which is 1px.
        nq_color(255, 255, 0);
        nq_arc(4.0f, 2.0f, 2.5f, 45.0f, 315.0f, false);
        nq_line_weight(1.0f);

        // 3. Polygons
        nq_put_text(-9, 4, "Polygons:");
        
        // Filled Triangle
        nq_color(0, 255, 0);
        nq_polygon(tri_x, tri_y, 3, true);
        
        // Thick Outline Hexagon
        nq_color(255, 0, 255);
        nq_line_weight(4.0f);
        nq_polygon(hex_x, hex_y, 6, false);
        nq_line_weight(1.0f);
        
        // 4. Standard Primitives check
        nq_color(255, 255, 255);
        nq_box(-2, -2, 2, 2, false);
        nq_circle(0, 0, 1.0f, true);

        nq_sync_all();
    }
    nq_close();
    return 0;
}
