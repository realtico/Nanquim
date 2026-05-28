# Nanquim Backlog

Documento de backlog e priorizacao para o Nanquim, uma biblioteca C11 wrapper de SDL2 inspirada em BASIC, Processing, pygame e ferramentas cientificas como MATLAB/Octave.

Este arquivo resume o estado percebido do projeto e organiza possiveis proximos passos para evoluir o Nanquim como:

- ferramenta simples para desenho procedural e simulacoes;
- biblioteca de plotagem cientifica;
- engine grafica 2D leve para jogos e prototipos.

## Estado Atual

O Nanquim ja tem uma base madura para desenho procedural e simulacao visual:

- API procedural simples, baseada em estado global ativo.
- Criacao de janelas e suporte a multiplas figuras.
- Coordenadas de mundo em `float`, com mapeamento para pixels.
- Mouse retornando coordenadas de mundo.
- Primitivas: ponto, linha, caixa, circulo, arco e poligono.
- Preenchimento em formas basicas.
- Espessura de linha para linhas e poligonos em modo contorno.
- Texto simples via SDL2_gfx.
- Input basico de teclado, mouse e scroll.
- Delta time para animacoes independentes de frame rate.
- Camera com pan, zoom e rotacao.
- Grids cartesiano e polar adaptativos.
- Superficies offscreen.
- Carregamento de sprites via SDL_image.
- Color key para transparencia.
- Blit simples e blit com escala/rotacao.
- Desenho de regioes de atlas.
- Build via Makefile e CMake.
- Exemplos cobrindo plots, simulacao, input, camera, sprites e grids.

## Backlog Tecnico Existente

O arquivo `REVIEW.md` ja funciona como uma revisao tecnica inicial. Os pontos abaixo consolidam o que parece mais importante transformar em backlog.

### Alta Prioridade

- Padronizar includes dos exemplos para `#include "nanquim.h"`.
- Garantir que todos os exemplos chamem `nq_close()` antes de sair.
- Remover codigo morto em exemplos, especialmente superficies criadas e nao usadas.
- Corrigir/documentar a unidade de angulo de `nq_blit_ex`: header, manual e implementacao precisam concordar.
- Corrigir o Manual onde ele cita `nq_camera_set`, pois a API real usa `nq_camera_look_at`, `nq_camera_zoom` e `nq_camera_rotate`.
- Documentar claramente a convencao de eixo Y: modo cartesiano Y-up versus modo tela Y-down.
- Limpar comentarios deliberativos em funcoes centrais de mapeamento, mantendo apenas documentacao final objetiva.
- Adicionar uma API simples de pausa/limite de FPS, por exemplo `nq_delay(ms)` ou `nq_set_fps(fps)`, para evitar `SDL_Delay` nos exemplos.

### Media Prioridade

- Tornar `NQ_Context` opaco no header publico.
- Avaliar tambem tornar `NQ_Surface` parcialmente opaco ou documentar que ela e uma estrutura publica.
- Unificar o pipeline de mapeamento interno:
  - `nq_map_x`, `nq_map_y`, `nq_map_scalar` ignoram camera;
  - `nq_map_point`, `nq_map_scalar_cam` consideram camera.
- Remover ou marcar explicitamente como deprecated os helpers antigos de mapeamento.
- Remover campos aparentemente nao usados, como `base_ppu_x` e `base_ppu_y`.
- Adicionar logs ou mensagens de erro para falhas comuns:
  - desenhar sem contexto ativo;
  - carregar sprite inexistente;
  - exceder limite de janelas/superficies;
  - falha de alocacao.
- Melhorar documentacao sobre custo de rendering em software.

### Baixa Prioridade, Mas Importante Para Escala

- Otimizar `nq_draw_region`, que hoje cria e destroi uma surface temporaria a cada chamada.
- Criar cache de regioes de atlas.
- Revisar uso de `rotozoomSurfaceXY`, que aloca superficies temporarias por frame.
- Revisar helpers de flip para respeitar `pitch`/stride de `SDL_Surface`.
- Criar testes ou exemplos minimos de regressao para camera, input e assets.

## Backlog Para Ferramenta De Plotagem

Esta e a direcao mais natural do Nanquim hoje. A base de coordenadas de mundo, camera, grids e primitivas ja favorece muito plots cientificos.

### API De Plot

- `nq_plot(xs, ys, n)` para linhas a partir de arrays.
- `nq_plot_function(fn, xmin, xmax, samples)` para plotar funcoes matematicas.
- `nq_scatter(xs, ys, n)` para pontos.
- `nq_bar(...)` para barras.
- `nq_histogram(...)` para distribuicoes.
- `nq_polar_plot(...)` para curvas polares.
- `nq_parametric_plot(...)` para curvas parametrizadas.

### Eixos E Escalas

- Auto-scale a partir dos dados.
- Margem automatica em torno dos dados.
- Ticks adaptativos com numeros "bonitos".
- Precisao adaptativa dos labels.
- Labels dos eixos X/Y.
- Titulo do grafico.
- Legenda.
- Grade menor e maior.
- Controle explicito de escala linear/logaritmica.
- Controle explicito de Y-up/Y-down.

### Estilo Visual

- Cores por serie.
- Espessura por serie.
- Estilos de linha: solida, tracejada, pontilhada.
- Markers: circulo, quadrado, cruz, ponto.
- Alpha/transparencia.
- Paletas predefinidas.

### Layout

- Subplots em uma mesma janela.
- Viewports ou paineis independentes.
- Margens configuraveis.
- Sistema de coordenadas por painel.
- Export de imagem da janela ou de um plot especifico.

### Interacao

- Pan com mouse.
- Zoom com scroll.
- Reset de view.
- Tooltip ou leitura de coordenadas ao passar o mouse.
- Selecao/inspecao de ponto.
- Callback de clique em coordenadas do plot.

## Backlog Para Engine Grafica 2D

Para jogos, o Nanquim ja tem varios tijolos: janela, input, sprites, camera, delta time e desenho. Ainda faltam abstracoes mais especificas de jogo.

### Loop E Tempo

- `nq_run(update, draw)` opcional para usuarios que preferem callbacks.
- `nq_set_fps(fps)` ou `nq_frame_limit(fps)`.
- `nq_delay(ms)`.
- Medidor de FPS.
- Separacao opcional entre update fixo e draw variavel.

### Input De Jogo

- Eventos de tecla pressionada/solta:
  - `nq_key_pressed`;
  - `nq_key_released`.
- Eventos de mouse pressionado/solto.
- Posicao e delta do mouse.
- Mapeamento de acoes:
  - `"jump"` -> Space;
  - `"left"` -> A/Left.
- Gamepad basico, se o escopo crescer.

### Sprites E Animacao

- Cache de sprites por caminho.
- Cache de regioes de atlas.
- `NQ_Sprite` com posicao, escala, rotacao, origem e flip.
- `NQ_Animation` com frames, duracao e loop.
- Tint/cor multiplicativa.
- Alpha por sprite.
- Pivot/origin configuravel.
- Ordenacao por camada ou z-index.

### Cena E Organizacao

- Transform stack:
  - push;
  - pop;
  - translate;
  - rotate;
  - scale.
- Camadas de desenho.
- Camera por cena ou por viewport.
- Estados de jogo:
  - menu;
  - gameplay;
  - pausa;
  - game over.

### Colisao E Mundo

- AABB.
- Circulo versus circulo.
- Ponto versus retangulo/circulo.
- Retangulo versus retangulo.
- Helpers de distancia, normalizacao, lerp.
- Spatial grid simples para muitos objetos.
- Tilemap basico.
- Culling por camera.

### Audio

- Som curto.
- Musica.
- Volume global.
- Volume por canal.
- Pause/resume.

### Ferramentas De Jogo

- Particulas simples.
- Tween/easing.
- Timers.
- Random helpers.
- Screenshot.
- Save/load simples para dados de jogo.

## Backlog De Documentacao

- Atualizar `README.md` com todos os exemplos atuais.
- Atualizar `MANUAL.md` para refletir a API real.
- Criar pagina "Getting Started" com o menor exemplo possivel.
- Criar pagina "Plotting Guide".
- Criar pagina "Game Prototype Guide".
- Documentar convencoes:
  - coordenadas;
  - unidade de angulo;
  - ciclo de vida;
  - ownership de surfaces;
  - custo de performance do backend software.
- Adicionar tabela de funcoes publicas.
- Adicionar exemplos de uso por categoria.

## Roadmap Sugerido

### Marco 1: Hardening

Objetivo: deixar a base consistente, previsivel e documentada antes de adicionar muitas features.

- Corrigir inconsistencias de docs/API.
- Padronizar exemplos.
- Adicionar `nq_delay` ou `nq_set_fps`.
- Documentar Y-up/Y-down.
- Resolver graus versus radianos.
- Limpar comentarios internos ruidosos.
- Adicionar logs basicos de erro.

### Marco 2: Plotting Minimal

Objetivo: transformar o Nanquim em uma ferramenta confortavel para graficos simples.

- `nq_plot`.
- `nq_scatter`.
- `nq_plot_function`.
- Auto-scale.
- Eixos com ticks e labels melhores.
- Titulo, labels X/Y e legenda simples.
- Exemplo `plot_demo.c`.

### Marco 3: Interacao De Plot

Objetivo: plots exploraveis.

- Pan com mouse.
- Zoom com scroll.
- Reset de camera.
- Coordenada sob o cursor.
- Exemplo de inspecao de pontos.

### Marco 4: Game 2D Minimal

Objetivo: permitir um prototipo de jogo pequeno sem expor SDL diretamente.

- Input pressed/released.
- FPS control.
- Sprite/atlas cache.
- Animacao de sprites.
- Colisao AABB/circulo.
- Exemplo `platformer_demo.c` ou `topdown_demo.c`.

### Marco 5: Polimento E Distribuicao

Objetivo: tornar o projeto facil de usar em outro computador ou por outra pessoa.

- CMake install.
- Versao semantica.
- Release notes.
- CI basico.
- Testes ou smoke tests.
- Documentacao reorganizada.

## Perguntas De Produto

Estas perguntas ajudam a priorizar como PO:

- O Nanquim deve ser primeiro uma biblioteca de plotagem ou uma mini engine 2D?
- A API deve continuar 100% procedural ou pode ter objetos opcionais?
- O publico principal e educacional, cientifico, jogos pequenos ou todos em camadas?
- A prioridade e simplicidade absoluta ou performance para muitos sprites?
- Vale manter backend software como identidade do projeto ou preparar uma camada GPU no futuro?
- O projeto deve esconder completamente SDL do usuario final?
- A compatibilidade com Windows/macOS e meta de curto prazo ou apenas Linux por enquanto?

## Observacao Final

O Nanquim parece estar em um bom ponto de maturidade: a arquitetura central ja existe e os exemplos provam que a ideia funciona. O proximo salto nao precisa ser grande; provavelmente o melhor retorno vem de polir a API existente, corrigir as inconsistencias e criar uma camada de plotagem simples em cima do que ja esta pronto.
