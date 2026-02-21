#include "../include/nanquim.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define PI 3.14159265358979323846

/*
 * Physics Note:
 * I(theta) = I0 * (sinc(beta))^2 * (sin(N*alpha)/sin(alpha))^2
 * alpha = (pi * d * sin(theta)) / lambda
 * beta  = (pi * b * sin(theta)) / lambda
 * 
 * Units need to be consistent. We use nanometers for lambda, micrometers for slits.
 * 1 um = 1000 nm.
 */

typedef struct {
    int N;          // Number of slits
    double lambda;  // Wavelength (nm)
    double b;       // Slit width (um)
    double d;       // Slit spacing (um) -- center to center
} SimulationParams;

double intensity(double theta_rad, SimulationParams p) {
    // Convert all to nanometers
    double b_nm = p.b * 1000.0;
    double d_nm = p.d * 1000.0;
    
    double sin_theta = sin(theta_rad);
    
    // Single slit diffraction factor (Diffraction envelope)
    // beta = (k * b * sin(theta)) / 2  = (pi * b * sin(theta)) / lambda
    double beta = (PI * b_nm * sin_theta) / p.lambda;
    
    double diff_factor;
    if (fabs(beta) < 1e-9) diff_factor = 1.0;
    else diff_factor = pow(sin(beta) / beta, 2.0);
    
    // Multi-slit interference factor
    // alpha = (k * d * sin(theta)) / 2 = (pi * d * sin(theta)) / lambda
    double alpha = (PI * d_nm * sin_theta) / p.lambda;
    
    double interf_factor;
    if (fabs(sin(alpha)) < 1e-9) {
        // Limit alpha -> m*pi is N^2 usually, but we normalize I0 later.
        // Actually, lim (sin(Na)/sin(a))^2 = N^2
        interf_factor = (double)p.N * (double)p.N;
    } else {
        interf_factor = pow(sin(p.N * alpha) / sin(alpha), 2.0);
    }
    
    // Normalize so max peak is 1.0 (approximately)
    // The max intensity is N^2.
    return diff_factor * (interf_factor / ((double)p.N * (double)p.N));
}

// Approximate Wavelength to RGB mapping (Dan Bruton's Algorithm adapted)
void spectral_color(double lambda_nm, int* r, int* g, int* b) {
    double red = 0.0, green = 0.0, blue = 0.0;

    if (lambda_nm >= 380.0 && lambda_nm < 440.0) {
        red = -(lambda_nm - 440.0) / (440.0 - 380.0);
        blue = 1.0;
    } else if (lambda_nm >= 440.0 && lambda_nm < 490.0) {
        green = (lambda_nm - 440.0) / (490.0 - 440.0);
        blue = 1.0;
    } else if (lambda_nm >= 490.0 && lambda_nm < 510.0) {
        green = 1.0;
        blue = -(lambda_nm - 510.0) / (510.0 - 490.0);
    } else if (lambda_nm >= 510.0 && lambda_nm < 580.0) {
        red = (lambda_nm - 510.0) / (580.0 - 510.0);
        green = 1.0;
    } else if (lambda_nm >= 580.0 && lambda_nm < 645.0) {
        red = 1.0;
        green = -(lambda_nm - 645.0) / (645.0 - 580.0);
    } else if (lambda_nm >= 645.0 && lambda_nm <= 780.0) {
        red = 1.0;
    }

    // Intensity fall-off near vision limits
    double factor = 0.0;
    if (lambda_nm >= 380.0 && lambda_nm < 420.0) {
        factor = 0.3 + 0.7 * (lambda_nm - 380.0) / (420.0 - 380.0);
    } else if (lambda_nm >= 420.0 && lambda_nm < 700.0) {
        factor = 1.0;
    } else if (lambda_nm >= 700.0 && lambda_nm <= 780.0) {
        factor = 0.3 + 0.7 * (780.0 - lambda_nm) / (780.0 - 700.0);
    }

    *r = (int)(red * factor * 255.0);
    *g = (int)(green * factor * 255.0);
    *b = (int)(blue * factor * 255.0);
}

int main(int argc, char* argv[]) {
    // Defaults
    SimulationParams params = {
        .N = 2,
        .lambda = 500.0, // Green light (nm)
        .b = 2.0,        // 2 um width
        .d = 10.0        // 10 um spacing
    };

    if (argc >= 5) {
        params.N = atoi(argv[1]);
        params.lambda = atof(argv[2]);
        params.b = atof(argv[3]);
        params.d = atof(argv[4]);
    } else {
        printf("Usage: %s N lambda(nm) b(um) d(um)\n", argv[0]);
        printf("Using defaults: N=%d, L=%.1f, b=%.1f, d=%.1f\n", params.N, params.lambda, params.b, params.d);
    }

    int width = 1024;
    int height = 768;
    
    nq_screen(width, height, "Fraunhofer Diffraction Simulation", NQ_SCALE_FIXED);

    // Define simulation range (Theta in degrees)
    // The visual field is -5 to +5 degrees usually sufficient for small slits
    double max_theta_deg = 5.0;
    
    // We need two coordinate systems. We can switch them on the fly or translate Y.
    // Let's manually manage Y offsets to draw on Top/Bottom halves.
    
    // Map X for theta: -5 to 5
    // Map Y for intensity: 0 to 1
    nq_setup_coords(-max_theta_deg, max_theta_deg, 0.0, 1.0);

    while(nq_running()) {
        nq_poll_events();
        
        nq_background(10, 10, 15);
        
        // --- TOP HALF: Visual Pattern ---
        // We simulate a "wall" by drawing vertical lines
        // Y range for top half in pixels: 0 to half_h
        // We iterate pixels (screen space) to cover gaps
        
        for (int px = 0; px < width; px++) {
            // Map pixel X back to Theta
            double norm_x = (double)px / (double)width; // 0 to 1
            double theta_deg = -max_theta_deg + norm_x * (2.0 * max_theta_deg);
            double theta_rad = theta_deg * (PI / 180.0);
            
            double I = intensity(theta_rad, params);
            
            // Map Intensity to Grayscale (or Green scale since lambda=500 usually)
            // Let's use Color based on approximate Wavelength eventually? 
            // For now, pure Green monochrome like a laser.
            int brightness = (int)(I * 255.0);
            if (brightness > 255) brightness = 255;
            if (brightness < 0) brightness = 0;
            
            // Use nq_line with literal pixels for vertical drawing (faster/easier here to mix modes)
            // But we want to use the API. 
            // The API maps World coordinates.
            // Let's use nq_coord_mode to draw the top part in LITERAL mode?
            // Or just map the logic.
            
            // Let's draw plotting the intensity as brightness lines.
            // We need to draw a vertical line at this theta.
            // Since nq_line maps from World X, we use theta_deg.
            
            // Color mapping
            nq_color(0, brightness, 0); // Green laser
            
            // Problem: nq_line maps Y. Our coordinate system is 0..1 (Intensity).
            // We want to draw from Screen Y=0 to Screen Y=half_h.
            // In World Y (0..1), this corresponds to bottom-half logic usually?
            // Actually, let's use a trick.
            // We can draw a line at theta_deg from Y=2.0 to Y=3.0 (outside 0..1)?
            // No, nq_setup_coords defined 0..1.
            
            // Better approach: Use nq_pset loop or just switch coords? 
            // Let's draw the graph first (Bottom), then the pattern (Top using Literal).
        }

        // --- DRAWING STRATEGY ---
        
        // Setup Split Screen Coordinate System
        // We map Y from [2.1, -0.1] to cover the full window height.
        // - Top region (Y > 1.0): Display diffraction pattern
        // - Bottom region (Y: 0.0 to 1.0): Display intensity graph
        nq_setup_coords(-max_theta_deg, max_theta_deg, 2.1, -0.1);
        
        // Draw Visual Pattern (Top: Y 1.1 to 2.1)
        double step = (2.0 * max_theta_deg) / (double)width; 
        
        // Otimização: Desenhar linhas verticais
        
        // Compute base color from wavelength once
        int base_r, base_g, base_b;
        spectral_color(params.lambda, &base_r, &base_g, &base_b);
        
        for (double theta = -max_theta_deg; theta < max_theta_deg; theta += step) {
            double rad = theta * (PI / 180.0);
            double I = intensity(rad, params);
            
            // Modulate brightness by Intensity I
            int r = (int)(base_r * I);
            int g = (int)(base_g * I);
            int b = (int)(base_b * I);

            // Draw Top "Wall" Strip
            nq_color(r, g, b); 
            nq_line(theta, 1.1, theta, 2.1); 
        }

        // Draw Graph (Bottom: Y 0.0 to 1.0)
        nq_color(100, 100, 100);
        nq_line(-max_theta_deg, 0, max_theta_deg, 0); // Axis
        nq_line(0, 0, 0, 1.0); // Center

        nq_color(255, 255, 0); // Yellow graph (keeps contrast)
        double prev_theta = -max_theta_deg;
        double prev_I = intensity(prev_theta * (PI/180.0), params);
        
        for (double theta = -max_theta_deg; theta < max_theta_deg; theta += step) {
            double rad = theta * (PI / 180.0);
            double I = intensity(rad, params);
            
            nq_line(prev_theta, prev_I, theta, I);
            
            prev_theta = theta;
            prev_I = I;
        }

        // Text Info (Simulation param display would require text support... future feature)
        // For now, console log is enough
        
        nq_sync_all();
        // Wait longer (static sim)
        SDL_Delay(50);
    }

    nq_close();
    return 0;
}
