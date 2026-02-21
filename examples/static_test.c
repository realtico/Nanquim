#include "../include/nanquim.h"
#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846

// Sinc function: sin(x)/x
// Handle x -> 0 limit
double sinc(double x) {
    if (fabs(x) < 1e-9) return 1.0;
    return sin(x) / x;
}

int main(void) {
    nq_screen(800, 400, "Nanquim: Static Precision Test - Sinc(x)", NQ_SCALE_FIXED);
    
    // Configura coordenadas
    // X: -20pi a 20pi
    // Y: -0.5 a 1.2
    nq_setup_coords(-60.0, 60.0, 1.2, -0.5);

    // Loop simples (estático, mas com refresh para manter a janela viva)
    while(nq_running()) {
        nq_poll_events();
        
        nq_background(255, 255, 255); // Fundo branco estilo paper
        
        // Eixos
        nq_color(0, 0, 0);
        nq_line(-60, 0, 60, 0); // X Axis
        nq_line(0, -0.5, 0, 1.2); // Y Axis
        
        // Desenhar a função
        nq_color(0, 0, 200);
        
        float prev_x = -60.0;
        float prev_y = sinc(prev_x);
        
        // Passo fino para testar a continuidade da linha pixel-a-pixel
        float step = 0.1f; 
        
        for (float x = -60.0; x <= 60.0; x += step) {
            float y = (float)sinc(x);
            nq_line(prev_x, prev_y, x, y);
            prev_x = x;
            prev_y = y;
        }
        
        nq_sync_all();
        SDL_Delay(30);
    }
    
    nq_close();
    return 0;
}
