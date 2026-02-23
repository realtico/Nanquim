#include "../include/nanquim.h"
#include <math.h>

int main(void) {
    // 1. Cria Janela 800x600
    nq_screen(800, 600, "Nanquim Test: Coordinates & Primitives", NQ_SCALE_FIXED);
    
    // 2. Define Coordenadas Cartesianas
    // Vamos explicitamente definir um sistema onde (0,0) é centro
    // X: -400 a 400
    // Y: -300 a 300
    nq_setup_coords(-400.0f, 400.0f, -300.0f, 300.0f);

    float angle = 0.0f;

    while(nq_running()) {
        nq_poll_events();
        
        // Fundo Cinza Escuro
        nq_background(20, 20, 20);
        
        // Grid
        nq_color(50, 50, 50);
        nq_line(-400, 0, 400, 0); // Eixo X
        nq_line(0, -300, 0, 300); // Eixo Y
        
        // Desenhar Circulo Central (Raio 100)
        nq_color(255, 255, 0);
        nq_circle(0, 0, 100, false);
        
        // Desenhar Planeta orbitando
        float px = cosf(angle) * 200.0f;
        float py = sinf(angle) * 200.0f;
        
        // Linha do centro ao planeta
        nq_color(0, 100, 255);
        nq_line(0, 0, px, py);
        
        // Planeta
        nq_color(0, 255, 255);
        nq_circle(px, py, 20, true);
        
        // Box ao redor
        nq_color(255, 0, 0);
        nq_box(px - 30, py - 30, px + 30, py + 30, false);
        
        angle += 0.05f;
        
        nq_sync_all();
        SDL_Delay(16); // ~60fps
    }
    
    nq_close();
    return 0;
}
