# Manual do Desenvolvedor - Nanquim Framework (v0.2)

## Introdução

**Nanquim** é uma biblioteca gráfica minimalista projetada para tornar a programação visual em C tão simples quanto era no BASIC ou MATLAB. O foco é em **Simulação Científica** e **Prototipagem Rápida**.

Esta versão introduz a compilação como **Shared Library (.so)**, permitindo integração fácil em sistemas Linux.

## Compilação e Instalação

O projeto agora gera uma biblioteca dinâmica.

```bash
make
```

Isso gera:
- `lib/libnanquim.so`: O binário da biblioteca.
- `bin/`: Executáveis de exemplo linkados dinamicamente.

Para rodar os exemplos sem instalar a lib no `/usr/lib`, o Makefile já configura o `rpath`. Basta executar:

```bash
./bin/diffraction_sim
```

## API Reference

### Core

#### `int nq_screen(int w, int h, const char* title, NQ_ScaleMode mode)`
Cria uma janela de visualização. Inicializa a SDL se necessário.
- **mode:** `NQ_SCALE_FIXED` (default) mantém 1:1.

#### `void nq_figure(int id)`
Seleciona qual janela receberá os comandos.

#### `void nq_poll_events()`
Deve ser chamada no loop principal para processar inputs e manter a janela responsiva.

#### `void nq_sync_all()`
Atualiza todas as janelas. Transfere o buffer da RAM para a tela.

#### `float nq_delta_time()`
Retorna o tempo (em segundos) desde a última chamada desta função. Essencial para animações independentes de frame rate.

### Desenho (Drawing)

#### `void nq_setup_coords(float x_min, float x_max, float y_min, float y_max)`
Define as fronteiras do mundo. Ex: `nq_setup_coords(-1, 1, -1, 1)` cria um plano cartesiano centrado. As primitivas de desenho usarão essas coordenadas para mapear para os pixels da tela.

#### `void nq_pset(float x, float y)`
Plota um único pixel. Usa acesso direto à memória para performance.

#### `void nq_line(float x1, float y1, float x2, float y2)`
Desenha uma linha. Usa `lineRGBA` da SDL2_gfx com suporte a coordenadas lógicas do Nanquim.

#### `void nq_box(float x1, float y1, float x2, float y2, bool filled)`
Desenha um retângulo definido por dois cantos opostos.

#### `void nq_circle(float x, float y, float r, bool filled)`
Desenha um círculo. O raio `r` é interpretado na escala do eixo X (se em modo WORLD).

#### `void nq_arc(float x, float y, float r, float start, float end, bool filled)`
Desenha um arco (se `filled=false`) ou uma fatia de pizza (se `filled=true`).
- **start/end:** Ângulos em graus.
- **Nota:** O contorno (`filled=false`) é atualmente fixo em 1px.

#### `void nq_polygon(float *vx, float *vy, int n, bool filled)`
Desenha um polígono arbitrário definido por `n` vértices.
- **vx, vy:** Arrays de coordenadas lógicas.
- Se `filled=false`, desenha o contorno respeitando a espessura de linha atual.

#### `void nq_line_weight(float w)`
Define a espessura da linha para chamadas subsequentes. Default: 1.0f.
- Afeta: `nq_line`, `nq_polygon` (modo contorno).
- **Não** afeta: `nq_circle`, `nq_arc`, `nq_box` (estes permanecem com 1px devido a limitações da backend de software). *Dica: Para caixas grossas, desenhe um polígono de 4 vértices.*

#### `void nq_color(Uint8 r, Uint8 g, Uint8 b)`
Define a cor atual do "pincel" (linhas, pontos, contornos).

#### `void nq_background(Uint8 r, Uint8 g, Uint8 b)`
Limpa o buffer atual com a cor especificada.

### Assets e Buffers

#### `NQ_Surface* nq_create_surface(int w, int h)`
Cria um buffer offscreen para desenhar sprites ou camadas. Possui seu próprio renderizador de software.

#### `NQ_Surface* nq_load_sprite(const char* path)`
Carrega um arquivo BMP. Retorna uma `NQ_Surface` pronta para ser blitada ou usada como target.

#### `void nq_set_target(NQ_Surface* target)`
Redireciona todas as operações de desenho (pset, line, etc) para a Surface especificada em vez da tela.

#### `void nq_reset_target()`
Volta o desenho para a tela principal (Canvas da janela).

#### `void nq_blit(NQ_Surface* src, float x, float y, NQ_Anchor anchor)`

#### `void nq_blit_ex(NQ_Surface* src, float x, float y, float w, float h, float angle, NQ_Anchor anchor)`
Versão extendida do blit com suporte a escala e rotação.
- **angle:** Rotação em radianos. A rotação é aplicada em sentido horário visual (consistente com `nq_camera_rotate`).

### Câmera e Grid (New in v0.2.1)

#### `void nq_camera_set(float x, float y, float zoom)`
Define a posição central do mundo e o nível de zoom.

#### `void nq_camera_rotate(float angle)`
Rotaciona a view da câmera em radianos. A grade e o conteúdo giram de forma sincronizada.

#### `void nq_draw_grid_rect(float step_x, float step_y, bool labels)`
Desenha uma grade Cartesiana infinita. O passo (`step`) é adaptativo: se o zoom for alterado drasticamente, a grade recálcula as divisões automaticamente para evitar poluição visual.

#### `void nq_draw_grid_polar(float step_r, float step_theta, bool labels)`
Desenha uma grade Polar dinâmica, ajustando os anéis concêntricos conforme a câmera se move ou aproxima. Ideal para plotar funções polares como rosas ou espirais.

#### `void nq_grid_color(Uint8 r, Uint8 g, Uint8 b)`
Define a cor das linhas da grade.

#### `void nq_axis_color(Uint8 r, Uint8 g, Uint8 b)`
Define a cor dos eixos principais e dos rótulos numéricos. Aumente o contraste dessas cores para melhor legibilidade em fundos escuros.

### Input e Interação (Atualizado v0.2)

O Nanquim possui um sistema de input abstraído que normaliza a diferença entre coordenadas de tela (pixels) e coordenadas científicas (mundo).

#### `bool nq_key_down(NQ_Key key)`
Verifica se uma tecla específica está pressionada no momento.
- `NQ_KEY_UP`, `NQ_KEY_DOWN`, `NQ_KEY_SPACE`, etc. (Mapeamento direto SDL_SCANCODE)

#### `bool nq_mouse_button(NQ_MouseButton btn)`
Verifica o estado dos botões do mouse.
- `NQ_MOUSE_LEFT`, `NQ_MOUSE_RIGHT`, `NQ_MOUSE_MIDDLE`

#### `void nq_mouse_pos(float *x, float *y)`
Retorna a posição atual do mouse.
- **Importante:** Se `nq_setup_coords` estiver ativo, os valores retornados serão nas coordenadas do **Mundo** (ex: metros, graus), não em pixels. Isso permite interação direta com a simulação.

#### `int nq_mouse_scroll(void)`
Retorna o delta da roda do mouse acumulado desde a última leitura. Zera o contador após a leitura.

### Texto e Fontes

#### `void nq_put_text(float x, float y, const char* text)`
Escreve uma string na tela na posição (x,y) do mundo. Utiliza a cor atual definida por `nq_color`.

#### `void nq_set_font_size(int size)`
Define o tamanho base da fonte (atualmente controla o espaçamento/escala interno, implementação via SDL_gfx bitmap font).

