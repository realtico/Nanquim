# Nanquim

**Nanquim** is a lightweight Graphics Framework for C11, inspired by the simplicity of 8-bit BASIC graphics and scientific tools like MATLAB/Octave.

It wraps SDL2 to provide a high-level "Simulation Engine" API, focusing on procedural drawing, coordinate mapping, and easy pixel manipulation.

## Features

- **Procedural API:** Draw directly with `line`, `pset`, `circle` without passing context pointers everywhere.
- **Scientific Coordinates:** Built-in mapping from World Space (float) to Screen Space (int), including Y-axis inversion support.
- **Input System:** Integrated Keyboard and Mouse handling with logical coordinate mapping (click in world units!).
- **Text Support:** Simple overlay text rendering for scientific annotations.
- **Software Rendering Core:** Pixel-perfect manipulation in RAM buffers, blitted to GPU for display.
- **Multi-Window Support:** Manage multiple simulation figures simultaneously.
- **Unified Geometry:** Flexible `nq_polygon` and `nq_arc` primitives with filled/outline support.
- **Line Thickness:** Configurable line weight for robust diagrams.
- **Shared Library:** Can be compiled as a `.so` for dynamic linking.
- **Infinite Grids (New):** Dynamic, adaptive grid rendering that consistently covers the viewport regardless of zoom or pan.
- **Adaptive Zoom (New):** Automatic adjustment of grid density to keep scientific plots readable at any scale.

## Examples Included

- **`test_pattern`**: Basic shape and color test.
- **`geometry_test`**: Demonstration of polygons, arcs, and line thickness.
- **`static_test`**: Mathematical function plotting (Sinc function).
- **`diffraction_sim`**: Fraunhofer Diffraction physics simulation (Single/Double Slit).
- **`input_demo`**: Interactive demonstration of mouse, keyboard, and coordinate feedback.
- **`fourier_demo`**: Fourier Series visualization with infinite rectangular grid.
- **`polar_demo`**: Polar Rose curves on an adaptive polar grid.
- **`tracking_demo`**: Camera tracking and large world exploration.

## Dependencies

- SDL2 (`libsdl2-dev`)
- SDL2_gfx (`libsdl2-gfx-dev`)
- pkg-config

## Build

To build the library and examples:

```bash
make
```

This will create:
- `lib/libnanquim.so` (Shared Object)
- `bin/test_pattern` (Basic Test)
- `bin/diffraction_sim` (Scientific Simulation)
- `bin/static_test` (Precision Test)

## Usage Example

### Scientific Simulation (Fraunhofer Diffraction)

Run the interference simulator with custom physics parameters:

```bash
# Usage: ./diffraction_sim N lambda(nm) b(um) d(um)
./bin/diffraction_sim 5 532 5.0 20.0
```

### Code Example

```c
#include "nanquim.h"

int main() {
    nq_screen(800, 600, "My Sim", NQ_SCALE_FIXED);
    nq_setup_coords(-10.0, 10.0, -10.0, 10.0);
    
    while(nq_running()) {
        nq_poll_events();
        
        nq_background(30,30,30);
        nq_color(255, 0, 0);
        nq_circle(0, 0, 5.0, false); // Circle at (0,0) with radius 5
        
        nq_sync_all();
    }
    nq_close();
    return 0;
}
```
