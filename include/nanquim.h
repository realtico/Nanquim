#ifndef NANQUIM_H
#define NANQUIM_H

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <stdbool.h>

/**
 * @file nanquim.h
 * @brief Biblioteca gráfica minimalista baseada em SDL2, inspirada em BASIC e MATLAB.
 * 
 * Filosofia:
 * - Simplicidade Procedural (API parecida com BASIC)
 * - Coordenadas Lógicas (Mundo float mapeado para Pixels int)
 * - Separação estrita entre Figuras (Janelas) e Superfícies (Buffers)
 */

/* --- CONFIGURAÇÕES DE TELA --- */
#define NQ_SCREEN_1 320, 240
#define NQ_SCREEN_2 640, 480
#define NQ_SCREEN_3 800, 600
#define NQ_SCREEN_4 1024, 768

/**
 * @brief Modos de escala para redimensionamento de janela.
 */
typedef enum {
    NQ_SCALE_FIXED,   // 1:1 Sem redimensionamento (Default)
    NQ_SCALE_STRETCH, // Anisotrópico (Estica para preencher a janela)
    NQ_SCALE_ASPECT   // Isotrópico (Mantém proporção com barras pretas)
} NQ_ScaleMode;

/**
 * @brief Modos de coordenadas (Pixels absolutos vs Mundo lógico).
 */
typedef enum {
    NQ_LITERAL,  // Pixels puros (0,0 no canto superior esquerdo)
    NQ_WORLD     // Coordenadas customizadas definidas por min/max
} NQ_CoordMode;

/**
 * @brief Ponto de ancoragem para operações de blit (desenho de sprites).
 */
typedef enum { 
    NQ_TOP_LEFT, 
    NQ_CENTERED 
} NQ_Anchor;

/* --- ESTRUTURAS --- */

/**
 * @brief Retângulo (x, y, w, h) para definição de regiões.
 * Substitui o uso de SDL_Rect na API pública.
 */
typedef struct {
    int x, y;
    int w, h;
} NQ_Rect;


/**
 * @brief Wrapper para superfícies de imagem (Buffer em RAM).
 */
typedef struct {
    SDL_Surface* sdl_surf;
    SDL_Renderer* soft_renderer; // Renderer de software para desenhar na surface
    int w, h;
    Uint32 colorkey;
    bool has_colorkey;
    Uint8 alpha;
} NQ_Surface;

/**
 * @brief Contexto de uma Janela/Figura (NQ_Figure).
 * 
 * Contém o estado da janela, renderer, buffer de desenho e configurações
 * de mapeamento de coordenadas.
 */
typedef struct {
    int id;                     // ID único da janela
    SDL_Window* window;         // Janela SDL
    SDL_Renderer* renderer;     // Renderer SDL (GPU)
    
    // Buffers
    NQ_Surface* canvas;         // Buffer principal de desenho (RAM) - Onde o nq_pset desenha
    NQ_Surface* current_target; // Alvo atual do "pincel" (pode ser o canvas ou outro surface)
    SDL_Texture* texture;       // Textura para upload do canvas para a GPU
    
    // Estado Gráfico
    SDL_Color pen_color;
    SDL_Color bg_color;
    SDL_Color axis_color; // Color for major axes (x=0, y=0)
    SDL_Color grid_color; // Color for grid lines
    float line_width;
    
    // Configurações de Input
    int wheel_delta;
    
    // Configurações de Texto
    int font_size;

    // Mapeamento Matemático
    NQ_CoordMode coord_mode;
    float x_min, x_max;
    float y_min, y_max;
    float inv_range_x, inv_range_y; // Precomputed 1.0 / (max - min)
    float base_ppu_x, base_ppu_y; // Base pixels per unit (at zoom 1.0)
    
    // Câmera Global
    float camera_x, camera_y;
    float camera_zoom;
    float camera_angle; // Radianos
    
    NQ_ScaleMode scale_mode;
    
    // Configuração de Visualização (Aspect Ratio)
    float window_scale;
    int offset_x, offset_y;
    
    bool running;
} NQ_Context;

/* --- API CORE --- */

/**
 * @brief Cria uma nova janela gráfica (Figura).
 * Define automaticamente esta janela como o contexto ativo.
 * 
 * @param w Largura em pixels.
 * @param h Altura em pixels.
 * @param title Título da janela.
 * @param mode Modo de escala (NQ_SCALE_FIXED, etc).
 * @return ID da janela criada (0 a N). Retorna -1 em erro.
 */
int  nq_screen(int w, int h, const char* title, NQ_ScaleMode mode);

/**
 * @brief Define qual janela (Figura) receberá os comandos de desenho subsequentes.
 * @param id ID da janela obtido via nq_screen().
 */
void nq_figure(int id);

/**
 * @brief Processa a fila de eventos do sistema (mouse, teclado, janela).
 * Deve ser chamado em loop. Atualiza o contexto ativo baseado no foco.
 */
void nq_poll_events();

/**
 * @brief Atualiza a tela de todas as janelas ativas.
 * Faz o upload dos buffers (canvas) para a GPU e apresenta na tela.
 */
void nq_sync_all();

/**
 * @brief Fecha todas as janelas e encerra o Nanquim.
 */
void nq_close();

/**
 * @brief Verifica se o sistema ainda deve continuar rodando.
 * @return true se a aplicação deve continuar, false se o usuário pediu para sair.
 */
bool nq_running();

/**
 * @brief Retorna o tempo passado (em segundos) desde a última chamada.
 * Útil para simulações independentes de frame rate.
 */
float nq_delta_time();

/* --- API DRAW --- */

/**
 * @brief Configura o sistema de coordenadas lógicas.
 * Ativa automaticamente o modo NQ_WORLD.
 * 
 * @param x_min Valor lógico da borda esquerda.
 * @param x_max Valor lógico da borda direita.
 * @param y_min Valor lógico da borda inferior (ou superior).
 * @param y_max Valor lógico da borda superior (ou inferior).
 */
void nq_setup_coords(float x_min, float x_max, float y_min, float y_max);

/**
 * @brief Define a cor atual do "pincel" (linhas, pontos, contornos).
 */
void nq_color(Uint8 r, Uint8 g, Uint8 b);

/**
 * @brief Define a cor de fundo e limpa a tela imediatamente com essa cor.
 */
void nq_background(Uint8 r, Uint8 g, Uint8 b);

/* Primitivas (Aceitam float para simulações precisas) */

/**
 * @brief Desenha um ponto na coordenada especificada.
 */
void nq_pset(float x, float y);

/**
 * @brief Desenha uma linha entre dois pontos.
 */
void nq_line(float x1, float y1, float x2, float y2);

/**
 * @brief Desenha um retângulo.
 * @param filled true para preenchido, false para apenas contorno.
 */
void nq_box(float x1, float y1, float x2, float y2, bool filled);

/**
 * @brief Desenha um círculo.
 * @param r Raio (nas unidades do eixo X se NQ_WORLD).
 */
void nq_circle(float x, float y, float r, bool filled);

/**
 * @brief Desenha um arco de circunferência ou setor preenchido.
 */
void nq_arc(float x, float y, float radius, float start_angle, float end_angle, bool filled);

/**
 * @brief Desenha um polígono.
 */
void nq_polygon(float *vx, float *vy, int n, bool filled);

/**
 * @brief Define a espessura da linha.
 */
void nq_line_weight(float w);

/* --- INPUT & INTERACTION --- */

typedef enum {
    NQ_KEY_SPACE = 44, // SDL_SCANCODE_SPACE matches standard often
    NQ_KEY_ESCAPE = 41,
    NQ_KEY_ENTER = 40,
    NQ_KEY_UP = 82,
    NQ_KEY_DOWN = 81,
    NQ_KEY_LEFT = 80,
    NQ_KEY_RIGHT = 79,
    NQ_KEY_A = 4, NQ_KEY_B = 5, NQ_KEY_C = 6, NQ_KEY_D = 7,
    NQ_KEY_E = 8, NQ_KEY_F = 9, NQ_KEY_G = 10, NQ_KEY_H = 11,
    NQ_KEY_I = 12, NQ_KEY_J = 13, NQ_KEY_K = 14, NQ_KEY_L = 15,
    NQ_KEY_M = 16, NQ_KEY_N = 17, NQ_KEY_O = 18, NQ_KEY_P = 19,
    NQ_KEY_Q = 20, NQ_KEY_R = 21, NQ_KEY_S = 22, NQ_KEY_T = 23,
    NQ_KEY_U = 24, NQ_KEY_V = 25, NQ_KEY_W = 26, NQ_KEY_X = 27,
    NQ_KEY_Y = 28, NQ_KEY_Z = 29,
    NQ_KEY_1 = 30, NQ_KEY_2 = 31, NQ_KEY_3 = 32, NQ_KEY_4 = 33,
    NQ_KEY_5 = 34, NQ_KEY_6 = 35, NQ_KEY_7 = 36, NQ_KEY_8 = 37,
    NQ_KEY_9 = 38, NQ_KEY_0 = 39,
    NQ_KEY_F1 = 58, NQ_KEY_F2 = 59, NQ_KEY_F3 = 60, NQ_KEY_F4 = 61,
    NQ_KEY_F5 = 62, NQ_KEY_F6 = 63, NQ_KEY_F7 = 64, NQ_KEY_F8 = 65,
    NQ_KEY_F9 = 66, NQ_KEY_F10 = 67, NQ_KEY_F11 = 68, NQ_KEY_F12 = 69,
    NQ_KEY_LCTRL = 224, NQ_KEY_LSHIFT = 225, NQ_KEY_LALT = 226
} NQ_Key;

typedef enum {
    NQ_MOUSE_LEFT = 1,
    NQ_MOUSE_MIDDLE = 2,
    NQ_MOUSE_RIGHT = 3
} NQ_MouseButton;

bool nq_key_down(NQ_Key key);
bool nq_mouse_button(NQ_MouseButton btn);
void nq_mouse_pos(float *x, float *y);
int nq_mouse_scroll(void);

/* --- TEXTO --- */
void nq_put_text(float x, float y, const char* text);
void nq_set_font_size(int size);

/* --- API ASSETS & BUFFERS --- */

/**
 * @brief Cria uma nova superfície (imagem/buffer) em memória.
 */
NQ_Surface* nq_create_surface(int w, int h);
void nq_free_surface(NQ_Surface* surf);

/**
 * @brief Carrega uma imagem de arquivo.
 * Suporta BMP nativamente. Outros formatos requerem SDL_Image.
 */
NQ_Surface* nq_load_sprite(const char* path);

/**
 * @brief Define a cor de transparência (Color Key).
 * Pixels com esta cor serão ignorados no blit.
 */
void nq_set_colorkey(NQ_Surface* surf, Uint8 r, Uint8 g, Uint8 b);

/**
 * @brief Desenha apenas uma região específica da superfície fonte (Atlas).
 * @param atlas Superfície contendo múltiplos frames (source).
 * @param region Retângulo definindo a região a ser copiada do atlas.
 * @param x Posição X no mundo.
 * @param y Posição Y no mundo.
 * @param w Largura no mundo de destino.
 * @param h Altura no mundo de destino.
 * @param angle Ângulo de rotação.
 * @param flip_h Se verdadeiro, espelha horizontalmente.
 */
void nq_draw_region(NQ_Surface* atlas, NQ_Rect region, float x, float y, float w, float h, float angle, bool flip_h);

/**
 * @brief Copia (brita) uma superfície para o alvo atual com transformações.
 * @param src Superfície a ser desenhada.
 * @param x Posição X no mundo.
 * @param y Posição Y no mundo.
 * @param w Largura no mundo (escala horizontal).
 * @param h Altura no mundo (escala vertical).
 * @param angle Ângulo em graus (horário).
 * @param anchor Ponto de ancoragem (NQ_TOP_LEFT ou NQ_CENTERED).
 */
void nq_blit_ex(NQ_Surface* src, float x, float y, float w, float h, float angle, NQ_Anchor anchor);

/**
 * @brief Define uma superfície específica como alvo de desenho.
 * Os comandos nq_pset, nq_line, etc. desenharão nesta superfície.
 */
void nq_set_target(NQ_Surface* target);

/**
 * @brief Restaura o alvo de desenho para a tela (canvas da janela ativa).
 */
void nq_reset_target();

/**
 * @brief Copia (brita) uma superfície para o alvo atual.
 */
void nq_blit(NQ_Surface* src, float x, float y, NQ_Anchor anchor);

/* --- GRID SYSTEM --- */
void nq_axis_color(Uint8 r, Uint8 g, Uint8 b);
void nq_grid_color(Uint8 r, Uint8 g, Uint8 b);
void nq_draw_grid_rect(float step_x, float step_y, bool show_labels);
void nq_draw_grid_polar(float step_r, float step_theta, bool show_labels);

/* --- CAMERA API --- */

/**
 * @brief Move a câmera global para olhar para um ponto específico (Ponto central da tela).
 */
void nq_camera_look_at(float x, float y);

/**
 * @brief Define o zoom da câmera. 1.0 é o padrão.
 */
void nq_camera_zoom(float z);
void nq_camera_zoom_rel(float factor);

/**
 * @brief Rotaciona a câmera.
 * @param angle Ângulo em radianos.
 */
void nq_camera_rotate(float angle);
void nq_camera_rotate_rel(float d_angle);

/**
 * @brief Move a câmera relativamente.
 */
void nq_camera_pan(float dx, float dy);

// Getters
float nq_camera_get_x(void);
float nq_camera_get_y(void);
float nq_camera_get_zoom(void);
float nq_camera_get_angle(void);

/**
 * @brief Retorna os limites visíveis do mundo atual com base na câmera.
 * Útil para otimizar desenho (culling) ou grades infinitas.
 */
void nq_get_camera_bounds(float* min_x, float* max_x, float* min_y, float* max_y);

#endif // NANQUIM_H
