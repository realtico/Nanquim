# Nanquim Framework — Revisão Técnica (Ombudsman)

**Data:** 13 de Março de 2026  
**Revisor:** Claudio (Claude, Ombudsman)  
**Escopo:** Header público, 5 módulos core, 11 exemplos, Makefile, README, Manual  

---

## O Bom (Pontos Fortes)

**1. Filosofia de API excelente.**  
A decisão de usar estado global implícito (como BASIC/Processing/p5.js) é acertadíssima para o público-alvo. Código como `nq_color(255,0,0); nq_circle(0,0,5,false);` é extremamente legível e convidativo. O Castor acertou em cheio na arquitetura procedural.

**2. Separação de módulos limpa.**  
A divisão em `nq_core`, `nq_draw`, `nq_camera`, `nq_input`, `nq_assets` com um `nq_internal.h` para comunicação entre eles é uma organização sólida. Cada arquivo tem responsabilidade clara.

**3. Sistema de coordenadas world-space.**  
O `nq_setup_coords` + `nq_map_point` elimina o trabalho mais tedioso em SDL (converter float para pixel). A função `nq_mouse_pos` retornando coordenadas do mundo é um diferencial importante para simulações interativas.

**4. Pipeline de câmera completo.**  
Translate-Rotate-Scale com `nq_map_point`/`nq_inv_map_point` e inversa correta. O `nq_get_camera_bounds` calculando AABB a partir dos 4 cantos invertidos é elegante e correto.

**5. Grade adaptativa.**  
O `nq_calc_adaptive_step` com nice-numbers (1, 2, 5 × 10^n) é exatamente o que ferramentas científicas profissionais (matplotlib, d3.js) fazem. Decisão de qualidade.

**6. Small-buffer optimization no polygon.**  
Stack-alloc para n≤64 vértices, heap para mais. Detalhe fino que mostra cuidado com performance.

**7. Exemplos diversificados.**  
De física real (Fraunhofer) a sprite animation (robot_parade), cada exemplo demonstra uma faceta diferente da lib. Isso é documentação viva.

**8. Makefile funcional com rpath.**  
Resolver o `LD_LIBRARY_PATH` via `-Wl,-rpath` torna a experiência de "build & run" imediata. Muito bom para onboarding.

---

## O Mau (Problemas Concretos para Corrigir)

**1. Inconsistência de includes nos exemplos.**  
Metade dos exemplos usa `#include "nanquim.h"` e a outra metade usa `#include "../include/nanquim.h"`. Funciona porque o Makefile passa `-Iinclude`, mas é confuso e sugere que os exemplos foram escritos em momentos diferentes.  
**Sugestão:** Padronizar todos para `#include "nanquim.h"`.

**2. Exemplos novos não chamam `nq_close()`.**  
`fourier_demo.c`, `polar_demo.c` e `tracking_demo.c` terminam com `return 0` sem chamar `nq_close()`. Os mais antigos (geometry_test, input_demo, time_test, etc.) chamam. Isso é um **vazamento de recursos** (SDL_Window, SDL_Renderer, SDL_Surface, SDL_Texture) e inconsistência no padrão dos exemplos.

**3. `polar_demo.c` ainda cria um `bg` surface que nunca é usado.**  
O código cria uma `NQ_Surface* bg`, desenha nela, mas no loop principal usa `nq_draw_grid_polar` dinâmico. A surface `bg` é alocada, desenhada e abandonada. Memória desperdiçada e código morto confuso para quem lê o exemplo.

**4. `diffraction_sim.c` chama `nq_setup_coords` dentro do loop.**  
`nq_setup_coords` é chamado a cada frame, o que reseta a câmera (zoom, posição, ângulo) toda iteração. Além de ser work desnecessário repetido 60×/segundo, impede qualquer futuro uso de câmera nesse demo.

**5. `nq_blit_ex` angle: header diz graus, código usa radianos.**  
Em `nanquim.h`, o doc diz `@param angle Ângulo em graus (horário)`. Mas a implementação real passa o ângulo direto ao `rotozoomSurfaceXY` que espera graus, enquanto o Manual diz "radianos". **Inconsistência perigosa** que vai gerar bugs silenciosos para usuários.

**6. `nq_map_point` tem ~35 linhas de comentários deliberativos.**  
Em `nq_core.c` (linhas ~400–470), há um bloco enorme de "pensamento em voz alta" ("Wait, if user set...", "Let's check...", "What if...") que parece output de LLM não limpo. Prejudica a legibilidade do código mais crítico da engine. Deve ser condensado em 2–3 linhas de documentação final.

**7. Dupla verificação de null em vários lugares.**  
Padrão repetido em `nq_draw.c`:
```c
if (!ctx) return;
if (!ctx || !ctx->current_target ...) return;  // ctx já foi checado!
```
Não é um bug, mas gera ruído. Ou faz um guard clause limpo, ou faz o check completo de uma vez.

**8. `SDL_Delay` exposto nos exemplos.**  
6 dos 11 exemplos chamam `SDL_Delay()` diretamente, quebrando a abstração. A lib deveria oferecer `nq_delay(ms)` ou `nq_set_fps(60)` para manter a promessa de "não precisar conhecer SDL".

**9. `NQ_Context` exposto no header público.**  
A struct completa com `SDL_Window*`, `SDL_Renderer*`, etc. está em `nanquim.h`. Isso acopla o usuário à implementação SDL. Se algum dia a engine migrar para Vulkan ou software puro, toda API quebra.  
**Sugestão:** Opaque pointer (`typedef struct NQ_Context NQ_Context;`) no header público, struct completa só no `nq_internal.h`.

**10. `nq_draw_region` aloca e desaloca uma surface temporária a cada frame.**  
Cada chamada faz `nq_copy_rect` → `nq_blit_ex` → `nq_free_surface`. No robot_parade, isso são 60 mallocs+blits+frees por segundo. Para um sprite animado, cache seria mais adequado.

---

## O Feio (Riscos Arquiteturais e Dívida Técnica)

**1. Dois pipelines de mapeamento coexistem.**  
O antigo (`nq_map_x`, `nq_map_y`, `nq_map_scalar`) ignora câmera. O novo (`nq_map_point`, `nq_map_scalar_cam`) considera câmera. Ambos estão expostos em `nq_internal.h`. Isso é uma **bomba-relógio**: qualquer novo código interno que use `nq_map_x` por engano vai gerar bugs de dessincronia com a câmera. O pipeline antigo deveria ser removido ou explicitamente marcado como deprecated.

**2. `base_ppu_x` / `base_ppu_y` nunca são usados.**  
Declarados no `NQ_Context`, mas nenhum código os inicializa ou lê. Campos fantasmas que poluem a struct.

**3. Rendering em software com rotozoom por frame é muito caro.**  
O `nq_blit_ex` chama `rotozoomSurfaceXY` (que aloca uma superfície nova) + `SDL_BlitSurface` + `SDL_FreeSurface` por blit. Para o tracking_demo com background 1600×1200, isso é uma rotação de imagem gigante em CPU a cada frame. Funciona para protótipos, mas é bom documentar esse trade-off.

**4. Convenção de Y-axis indefinida na documentação.**  
O código suporta Y-up (cartesiano) e Y-down (screen), decidindo pelo sinal de `y_max - y_min` em `nq_map_point`. Mas o Manual não explica essa dualidade. Exemplos usam ambas: `input_demo` usa `(7.5, -7.5)` (Y-down), `fourier_demo` usa `(-7.5, 7.5)` (Y-up). Usuários vão tropeçar aqui.

**5. Nenhum mecanismo de log/debug.**  
Quando algo dá errado (surface null, context missing), a lib simplesmente faz `return` silencioso. Sem stderr, sem código de erro, sem callback. Para uma ferramenta educacional, mensagens como `"nq_line: no active context, did you call nq_screen()?"` seriam extremamente valiosas.

**6. Limites hardcoded sem feedback.**  
`MAX_CONTEXTS=10`, `MAX_SURFACES=100`. Quando atingidos, `nq_register_surface` simplesmente para de registrar (sem aviso). Isso pode causar leaks invisíveis em programas mais complexos.

**7. Manual e README desatualizados em relação ao código.**
- README lista apenas 3 binários em "This will create" mas existem 11.
- Manual título diz "v0.2" mas a seção de câmera diz "v0.2.1". O cabeçalho de `nq_draw.c` diz "v0.1".
- `nq_blit` não tem descrição no Manual (linha cortada).
- Manual documenta `nq_camera_set(x, y, zoom)` que **não existe** no código. A API real é `nq_camera_look_at` + `nq_camera_zoom` separados.

**8. `create_flipped_surface` assume pitch == w×4.**  
O flip usa `y * w + x` para indexar, mas `SDL_Surface.pitch` pode ter padding. O código deveria usar `pitch/4` em vez de `w` para o stride. Na prática funciona para surfaces criadas pelo Nanquim (sem padding), mas é frágil para surfaces carregadas de arquivo.

---

## Resumo Executivo

| Categoria | Prioridade | Itens |
|-----------|-----------|-------|
| **Quick wins** | Alta | Padronizar includes, adicionar `nq_close()` nos exemplos novos, remover código morto do `polar_demo`, limpar comentários deliberativos do `nq_map_point` |
| **Correções de API** | Alta | Resolver ambiguidade graus/radianos no `nq_blit_ex`, documentar convenção de Y-axis, corrigir `nq_camera_set` no Manual |
| **Higiene técnica** | Média | Remover `base_ppu_x/y` não usados, unificar pipeline de mapeamento (deprecar `nq_map_x/y`), ocultar `NQ_Context` do header público |
| **Experiência do usuário** | Média | Adicionar `nq_delay()`, adicionar log de erros básico com `fprintf(stderr, ...)`, abstrair `SDL_Delay` |
| **Performance** | Baixa (mas documentar) | Cache de sprites recortados, nota sobre custo do rotozoom em software |

> **Conclusão:** A base é sólida e a filosofia está correta. O que precisa agora é uma passada de **polimento e consistência** antes de mais features. Um bom sprint de "hardening" resolveria 80% dos pontos acima.
