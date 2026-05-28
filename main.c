#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LARGURA 1920
#define ALTURA 1080
#define FPS 60

#define SPRITE_SRC_W 96
#define SPRITE_SRC_H 60

#define SPRITE_SCALE 1.95f
#define DRAW_W (SPRITE_SRC_W * SPRITE_SCALE)
#define DRAW_H (SPRITE_SRC_H * SPRITE_SCALE)

#define HITBOX_W 40
#define HITBOX_H 45
#define HITBOX_OFFSET_X 66
#define HITBOX_OFFSET_Y 76

#define VELOCIDADE 4.0f
#define GRAVIDADE 0.7f
#define FORCA_PULO -13.05f
#define MAX_QUEDA 18.0f

#define FRAMES_IDLE 6
#define FRAMES_RUN 8
#define FRAMES_JUMP 12

#define MAX_VIDAS 5
#define FRAMES_ZUMBI 8

#define ATTACK_DRAW_W DRAW_W
#define ATTACK_DRAW_H DRAW_H

#define ZUMBI_DRAW_W 120
#define ZUMBI_DRAW_H 120

#define ZUMBI_OFFSET_X 50
#define ZUMBI_OFFSET_Y 110

/* estamina */
#define MAX_ESTAMINA 10.0f
#define CUSTO_PULO 1.0f
#define RECARGA_ESTAMINA 0.03f

/* sanidade */
#define MAX_SANIDADE 100.0f
#define ZUMBIS_POR_QUEDA 15
#define QUEDA_SANIDADE 20.0f
#define RECUPERA_SANIDADE 0.03f
#define TEMPO_RECUPERACAO 5.0

/* horda */
#define MAX_ZUMBIS_TELA 10
#define TOTAL_ZUMBIS_FASE 50
#define ZUMBIS_POR_ONDA 5
#define INTERVALO_ONDA 600 /* ~10s entre ondas a 60fps */

/* posicoes de spawn */
#define SPAWN_X_MIN 2550
#define SPAWN_X_MAX 2900
#define SPAWN_Y 760

unsigned char colisao[ALTURA][3000];

/* ================================================================== */
/*  TIPOS                                                               */
/* ================================================================== */

typedef struct {
  int ativa;
  char status[20];
} VidaStatus;

typedef struct {
  float x, y, vel_y;
} Movimento;

typedef struct {
  Movimento mov;
  float frame, frame_ataque;
  int no_chao, direcao, movendo, atacando, tipo_ataque;
  double ultimo_dano;
  float estamina;
} Jogador;

typedef struct {
  float x, y, x_inicial, y_inicial;
  float velocidade;
  int direcao, vivo;
  float frame;
  int vida;
  int tipo; /* 0 = normal, 1 = rapido */
} Inimigo;

typedef struct {
  double inicio, atual, fim;
  int ativo;
  float ranking[10];
  int quantidade_scores;
} Temporizador;

typedef enum { MENU_JOGAR = 0, MENU_SAIR = 1 } OpcaoMenu;

typedef struct {
  float valor;
  int zumbis_mortos;
  int ultimo_gatilho;
  int game_over;
  double ultimo_abate;
  int regenerando;
} Sanidade;

typedef struct {
  Inimigo pool[MAX_ZUMBIS_TELA];
  int total_spawned;
  int total_mortos;
  int timer_onda;
  int fase_concluida;
} Horda;

/* ================================================================== */
/*  PROTOTIPOS (forward declarations)                                   */
/* ================================================================== */

void perder_vida(VidaStatus *vidas);
void ordenar_ranking(float r[], int n);
int busca_binaria(float r[], int n, float v);
void carregar_ranking(Temporizador *t);
void salvar_ranking(Temporizador *t);
void atualizar_sanidade(Sanidade *san);
void gerar_mapa_colisao(ALLEGRO_BITMAP *mapa);

/* ================================================================== */
/*  FUNCOES AUXILIARES                                                  */
/* ================================================================== */

int **criar_matriz_decorativa(int linhas, int colunas) {
  int **mat = (int **)malloc(linhas * sizeof(int *));
  for (int i = 0; i < linhas; i++) {
    mat[i] = (int *)malloc(colunas * sizeof(int));
    for (int j = 0; j < colunas; j++)
      mat[i][j] = rand() % 2;
  }
  return mat;
}

void desenhar_matriz_fundo(int **mat, int linhas, int colunas) {
  for (int i = 0; i < linhas; i++)
    for (int j = 0; j < colunas; j++)
      if (mat[i][j] == 1)
        al_draw_filled_rectangle(j * 100, i * 100, j * 100 + 3, i * 100 + 3,
                                 al_map_rgba(255, 255, 255, 40));
}

void liberar_matriz(int **mat, int linhas) {
  for (int i = 0; i < linhas; i++)
    free(mat[i]);
  free(mat);
}

OpcaoMenu executar_menu(ALLEGRO_EVENT_QUEUE *queue, ALLEGRO_TIMER *timer,
                        ALLEGRO_BITMAP *bg_menu, ALLEGRO_FONT *fonte) {
  ALLEGRO_EVENT ev;
  OpcaoMenu opcao = MENU_JOGAR;
  while (1) {
    al_wait_for_event(queue, &ev);
    if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
      return MENU_SAIR;
    if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
      if (ev.keyboard.keycode == ALLEGRO_KEY_A ||
          ev.keyboard.keycode == ALLEGRO_KEY_LEFT)
        opcao = MENU_JOGAR;
      if (ev.keyboard.keycode == ALLEGRO_KEY_D ||
          ev.keyboard.keycode == ALLEGRO_KEY_RIGHT)
        opcao = MENU_SAIR;
      if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER ||
          ev.keyboard.keycode == ALLEGRO_KEY_SPACE)
        return opcao;
    }
    if (ev.type == ALLEGRO_EVENT_TIMER) {
      const char *jogar = (opcao == MENU_JOGAR) ? "> JOGAR <" : "JOGAR";
      const char *sair = (opcao == MENU_SAIR) ? "> SAIR <" : "SAIR";
      al_clear_to_color(al_map_rgb(0, 0, 0));
      al_draw_scaled_bitmap(bg_menu, 0, 0, al_get_bitmap_width(bg_menu),
                            al_get_bitmap_height(bg_menu), 0, 0, LARGURA,
                            ALTURA, 0);
      al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2.4, 990,
                   ALLEGRO_ALIGN_CENTER, jogar);
      al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 1.6, 990,
                   ALLEGRO_ALIGN_CENTER, sair);
      al_flip_display();
    }
  }
}

bool pixel_solido(ALLEGRO_BITMAP *mapa, int x, int y) {
  if (x < 0 || y < 0 || x >= 3000 || y >= ALTURA)
    return true;
  return colisao[y][x];
}

bool colide_mapa(ALLEGRO_BITMAP *mapa, float x, float y) {
  int left = (int)x, right = (int)x + HITBOX_W - 1;
  int top = (int)y, bottom = (int)y + HITBOX_H - 1;
  for (int px = left; px <= right; px += 4)
    if (pixel_solido(mapa, px, top))
      return true;
  for (int px = left; px <= right; px += 4)
    if (pixel_solido(mapa, px, bottom))
      return true;
  for (int py = top; py <= bottom; py += 4)
    if (pixel_solido(mapa, left, py))
      return true;
  for (int py = top; py <= bottom; py += 4)
    if (pixel_solido(mapa, right, py))
      return true;
  return false;
}

bool esta_no_chao(ALLEGRO_BITMAP *mapa, float x, float y) {
  int left = (int)x + 4;
  int right = (int)x + HITBOX_W - 4;
  int foot = (int)y + HITBOX_H;
  return pixel_solido(mapa, left, foot) || pixel_solido(mapa, right, foot);
}

/* ------------------------------------------------------------------ */
/*  HUD: VIDAS                                                          */
/* ------------------------------------------------------------------ */
void desenhar_vidas(VidaStatus *vidas, ALLEGRO_BITMAP *coracao) {
  int vivas = 0;
  for (int i = 0; i < MAX_VIDAS; i++)
    if (vidas[i].ativa)
      vivas++;
  float pct = (float)vivas / MAX_VIDAS;
  ALLEGRO_COLOR cor = (vivas >= 4)   ? al_map_rgb(0, 200, 0)
                      : (vivas >= 2) ? al_map_rgb(220, 200, 0)
                                     : al_map_rgb(200, 0, 0);
  float bx = 290, by = 940, bw = 320, bh = 63;
  al_draw_filled_rectangle(bx, by, bx + bw, by + bh, al_map_rgb(40, 40, 40));
  al_draw_filled_rectangle(bx, by, bx + bw * pct, by + bh, cor);
  al_draw_rectangle(bx, by, bx + bw, by + bh, al_map_rgb(255, 255, 255), 2);
  al_draw_scaled_bitmap(coracao, 0, 0, al_get_bitmap_width(coracao),
                        al_get_bitmap_height(coracao), 120, 813, 540, 340, 0);
}

/* ------------------------------------------------------------------ */
/*  HUD: ESTAMINA                                                       */
/* ------------------------------------------------------------------ */
void desenhar_estamina(float estamina, ALLEGRO_BITMAP *spr) {
  float pct = estamina / MAX_ESTAMINA;
  ALLEGRO_COLOR cor =
      (estamina > 0) ? al_map_rgb(30, 144, 255) : al_map_rgb(80, 80, 80);
  float bx = 782, by = 940, bw = 325, bh = 63;
  al_draw_filled_rectangle(bx, by, bx + bw, by + bh, al_map_rgb(40, 40, 40));
  al_draw_filled_rectangle(bx, by, bx + bw * pct, by + bh, cor);
  al_draw_rectangle(bx, by, bx + bw, by + bh, al_map_rgb(255, 255, 255), 2);
  al_draw_scaled_bitmap(spr, 0, 0, al_get_bitmap_width(spr),
                        al_get_bitmap_height(spr), 625, 800, 540, 340, 0);
}

/* ------------------------------------------------------------------ */
/*  HUD: SANIDADE (vertical, esquerda)                                  */
/* ------------------------------------------------------------------ */
void desenhar_sanidade(Sanidade *san, ALLEGRO_BITMAP *spr) {
  float pct = san->valor / MAX_SANIDADE;
  unsigned char g = (unsigned char)(20 * pct);
  unsigned char b = g;
  ALLEGRO_COLOR cor =
      (san->valor > 0) ? al_map_rgb(180, g, b) : al_map_rgb(80, 80, 80);

  float barra_x = 90;
  float barra_base_y = 1050;
  float barra_largura = 30;
  float barra_altura = 230;
  float fill_h = barra_altura * pct;
  float topo = barra_base_y - barra_altura;

  al_draw_filled_rectangle(barra_x, topo, barra_x + barra_largura, barra_base_y,
                           al_map_rgb(40, 40, 40));
  al_draw_filled_rectangle(barra_x, barra_base_y - fill_h,
                           barra_x + barra_largura, barra_base_y, cor);
  al_draw_rectangle(barra_x, topo, barra_x + barra_largura, barra_base_y,
                    al_map_rgb(255, 255, 255), 2);

  if (spr)
    al_draw_scaled_bitmap(spr, 0, 0, al_get_bitmap_width(spr),
                          al_get_bitmap_height(spr), 5, 780, 200, 300, 0);
}

void desenhar_overlay_sanidade(Sanidade *san) {
  float perda = 1.0f - (san->valor / MAX_SANIDADE);
  unsigned char alpha = (unsigned char)(perda * 220.0f);
  if (alpha == 0)
    return;
  al_draw_filled_rectangle(0, 0, LARGURA, ALTURA,
                           al_map_rgba(180, 0, 0, alpha));
}

void atualizar_sanidade(Sanidade *san) {
  int gatilho = san->zumbis_mortos / ZUMBIS_POR_QUEDA;
  if (gatilho > san->ultimo_gatilho) {
    san->ultimo_gatilho = gatilho;
    san->valor -= QUEDA_SANIDADE;
    if (san->valor < 0)
      san->valor = 0;
    if (san->valor <= 0)
      san->game_over = 1;
  }
  if (al_get_time() - san->ultimo_abate >= TEMPO_RECUPERACAO) {
    san->regenerando = 1;
    san->valor += RECUPERA_SANIDADE;
    if (san->valor > MAX_SANIDADE)
      san->valor = MAX_SANIDADE;
  }
}

void desenhar_aviso_sanidade(Sanidade *san, ALLEGRO_FONT *fonte, double t) {
  if (san->valor >= MAX_SANIDADE || san->game_over)
    return;
  if ((int)(t / 0.6) % 2 == 0)
    return;
  float perda = 1.0f - (san->valor / MAX_SANIDADE);
  unsigned char g = (unsigned char)(200 * (1.0f - perda)), b = g;
  al_draw_text(fonte, al_map_rgba(0, 0, 0, 180), LARGURA / 2.0f + 3, 123,
               ALLEGRO_ALIGN_CENTER, "Sua sanidade esta diminuindo!");
  al_draw_text(fonte, al_map_rgb(255, g, b), LARGURA / 2.0f, 120,
               ALLEGRO_ALIGN_CENTER, "Sua sanidade esta diminuindo!");
}

/* ------------------------------------------------------------------ */
/*  HUD: TEMPO                                                          */
/* ------------------------------------------------------------------ */
void desenhar_hud_tempo(ALLEGRO_FONT *f, const char *txt) {
  al_draw_filled_rounded_rectangle(14, 14, 314, 72, 8, 8,
                                   al_map_rgba(0, 0, 0, 120));
  al_draw_filled_rounded_rectangle(10, 10, 310, 68, 8, 8,
                                   al_map_rgb(101, 60, 20));
  al_draw_rounded_rectangle(10, 10, 310, 68, 8, 8, al_map_rgb(218, 165, 32), 3);
  al_draw_rounded_rectangle(14, 14, 306, 64, 6, 6,
                            al_map_rgba(255, 210, 80, 80), 1);
  al_draw_text(f, al_map_rgba(0, 0, 0, 180), 22, 22, 0, txt);
  al_draw_text(f, al_map_rgb(255, 215, 0), 20, 20, 0, txt);
}

/* ------------------------------------------------------------------ */
/*  HUD: CONTADOR DE ZUMBIS (ondas)                                     */
/* ------------------------------------------------------------------ */
void desenhar_hud_horda(ALLEGRO_FONT *f, Horda *h) {
  char buf[64];
  sprintf(buf, "Zumbis: %d / %d", h->total_mortos, TOTAL_ZUMBIS_FASE);
  float cx = LARGURA / 2.0f;
  float by_ = 10, bh_ = 58, bw_ = 280;
  al_draw_filled_rounded_rectangle(cx - bw_ / 2 + 4, by_ + 4, cx + bw_ / 2 + 4,
                                   by_ + bh_ + 4, 8, 8,
                                   al_map_rgba(0, 0, 0, 120));
  al_draw_filled_rounded_rectangle(cx - bw_ / 2, by_, cx + bw_ / 2, by_ + bh_,
                                   8, 8, al_map_rgb(101, 60, 20));
  al_draw_rounded_rectangle(cx - bw_ / 2, by_, cx + bw_ / 2, by_ + bh_, 8, 8,
                            al_map_rgb(218, 165, 32), 3);
  al_draw_text(f, al_map_rgba(0, 0, 0, 180), cx + 2, by_ + 12 + 2,
               ALLEGRO_ALIGN_CENTER, buf);
  al_draw_text(f, al_map_rgb(255, 215, 0), cx, by_ + 12, ALLEGRO_ALIGN_CENTER,
               buf);
}

/* ================================================================== */
/*  HUD: ATAQUE SELECIONADO — canto inferior direito                    */
/* ================================================================== */
void desenhar_hud_ataque(ALLEGRO_BITMAP *at1, ALLEGRO_BITMAP *at2,
                         ALLEGRO_BITMAP *at3, ALLEGRO_BITMAP *hab1,
                         ALLEGRO_BITMAP *hab2, ALLEGRO_BITMAP *hab3,
                         int selecao, ALLEGRO_FONT *fonte_hud) {
  float bx = 40.0f;
  float by = 90.0f;
  float tam_slot = 62.0f;
  float pad = 12.0f;

  ALLEGRO_BITMAP *sprites[3] = {hab1, hab2, hab3};
  const char *nomes[3] = {"", "", ""};

  /* titulo */
  al_draw_text(fonte_hud, al_map_rgba(0, 0, 0, 180), bx + 102, by - 34,
               ALLEGRO_ALIGN_CENTER, "");
  al_draw_text(fonte_hud, al_map_rgb(218, 165, 32), bx + 100, by - 36,
               ALLEGRO_ALIGN_CENTER, "");

  for (int i = 0; i < 3; i++) {
    float sx = bx + i * (tam_slot + pad);
    float sy = by;
    int sel = (i + 1 == selecao);

    /* sombra */
    al_draw_filled_rounded_rectangle(sx + 3, sy + 3, sx + tam_slot + 3,
                                     sy + tam_slot + 3, 6, 6,
                                     al_map_rgba(0, 0, 0, 120));

    /* fundo do slot */
    al_draw_filled_rounded_rectangle(sx, sy, sx + tam_slot, sy + tam_slot, 6, 6,
                                     sel ? al_map_rgb(218, 165, 32)
                                         : al_map_rgb(60, 35, 10));

    /* borda */
    al_draw_rounded_rectangle(sx, sy, sx + tam_slot, sy + tam_slot, 6, 6,
                              al_map_rgb(218, 165, 32), sel ? 3 : 1);

    /* icone */
    float ipad = sel ? 1.0f : 5.0f;
    al_draw_scaled_bitmap(sprites[i], 0, 0, al_get_bitmap_width(sprites[i]),
                          al_get_bitmap_height(sprites[i]), sx + ipad,
                          sy + ipad, tam_slot - ipad * 3, tam_slot - ipad * 3,
                          0);

    /* nome abaixo */
    al_draw_text(fonte_hud,
                 sel ? al_map_rgb(255, 215, 0) : al_map_rgb(180, 180, 180),
                 sx + tam_slot / 2.0f, sy + tam_slot + 4, ALLEGRO_ALIGN_CENTER,
                 nomes[i]);

    /* indicador "K" no slot ativo */
    if (sel) {
      al_draw_filled_circle(sx + tam_slot - 10, sy + 10, 9,
                            al_map_rgb(0, 0, 0));
      al_draw_circle(sx + tam_slot - 10, sy + 10, 9, al_map_rgb(218, 165, 32),
                     1);
      al_draw_text(fonte_hud, al_map_rgb(255, 235, 0), sx + tam_slot - 10,
                   sy + 2, ALLEGRO_ALIGN_CENTER, "K");
    }
  }
  (void)at1;
  (void)at2;
  (void)at3; /* evita warning de parametros nao usados */
}

/* ================================================================== */
/*  RODA DE HABILIDADE — abre ao segurar K, navega com Q/E             */
/* ================================================================== */
void desenhar_roda_habilidade(ALLEGRO_BITMAP *at1, ALLEGRO_BITMAP *at2,
                              ALLEGRO_BITMAP *at3, ALLEGRO_BITMAP *hab1,
                              ALLEGRO_BITMAP *hab2, ALLEGRO_BITMAP *hab3,
                              int selecao, ALLEGRO_FONT *fonte_hud) {
  float cx = LARGURA / 2.0f;
  float cy = ALTURA / 2.0f;
  float r_ext = 175.0f;
  float r_int = 58.0f;

  /* overlay escuro */
  al_draw_filled_rectangle(0, 0, LARGURA, ALTURA, al_map_rgba(0, 0, 0, 140));

  /* titulo */
  al_draw_text(fonte_hud, al_map_rgba(0, 0, 0, 200), cx + 2,
               cy - r_ext - 52 + 2, ALLEGRO_ALIGN_CENTER, "Selecionar Ataque");
  al_draw_text(fonte_hud, al_map_rgb(255, 215, 0), cx, cy - r_ext - 52,
               ALLEGRO_ALIGN_CENTER, "Selecionar Ataque");

  const char *nomes[3] = {"", "", ""};
  ALLEGRO_BITMAP *sprites[3] = {hab1, hab2, hab3};

  /* 1a fatia comeca no topo (-90 graus), 3 fatias de 120 graus */
  float base = -(float)ALLEGRO_PI / 2.0f;
  float step = 2.0f * (float)ALLEGRO_PI / 3.0f;

  for (int i = 0; i < 3; i++) {
    float a0 = base + i * step;
    float amid = a0 + step / 2.0f;
    int sel = (i + 1 == selecao);

    /* fatia */
    ALLEGRO_COLOR cor_fatia =
        sel ? al_map_rgba(218, 165, 32, 210) : al_map_rgba(60, 35, 10, 185);
    al_draw_filled_pieslice(cx, cy, r_ext, a0, step, cor_fatia);

    /* borda dourada */
    al_draw_pieslice(cx, cy, r_ext, a0, step, al_map_rgb(218, 165, 32),
                     sel ? 4 : 2);

    /* icone */
    float dist = r_int + (r_ext - r_int) * 0.55f;
    float ix = cx + cosf(amid) * dist;
    float iy = cy + sinf(amid) * dist;
    float itam = sel ? 150.0f : 130.0f;

    al_draw_scaled_bitmap(sprites[i], 0, 0, al_get_bitmap_width(sprites[i]),
                          al_get_bitmap_height(sprites[i]), ix - itam / 2.0f,
                          iy - itam / 2.0f, itam, itam, 0);

    /* nome fora do circulo */
    float tx = cx + cosf(amid) * (r_ext + 32.0f);
    float ty = cy + sinf(amid) * (r_ext + 32.0f);
    al_draw_text(fonte_hud,
                 sel ? al_map_rgb(255, 215, 0) : al_map_rgb(200, 200, 200), tx,
                 ty - 14, ALLEGRO_ALIGN_CENTER, nomes[i]);
  }

  /* buraco central */
  al_draw_filled_circle(cx, cy, r_int, al_map_rgb(28, 14, 4));
  al_draw_circle(cx, cy, r_int, al_map_rgb(218, 165, 32), 2);

  /* instrucao */
  al_draw_text(fonte_hud, al_map_rgba(0, 0, 0, 180), cx + 2, cy + r_ext + 20,
               ALLEGRO_ALIGN_CENTER,
               "Q / E para navegar   |   Solte K para confirmar");
  al_draw_text(fonte_hud, al_map_rgb(220, 220, 220), cx, cy + r_ext + 18,
               ALLEGRO_ALIGN_CENTER,
               "Q / E para navegar   |   Solte K para confirmar");

  (void)at1;
  (void)at2;
  (void)at3;
}

/* ================================================================== */
/*  HORDA: spawn / logica                                               */
/* ================================================================== */

void horda_init(Horda *h) {
  for (int i = 0; i < MAX_ZUMBIS_TELA; i++)
    h->pool[i].vivo = 0;
  h->total_spawned = 0;
  h->total_mortos = 0;
  h->timer_onda = 180;
  h->fase_concluida = 0;
}

static int zumbis_vivos_na_tela(Horda *h) {
  int c = 0;
  for (int i = 0; i < MAX_ZUMBIS_TELA; i++)
    if (h->pool[i].vivo)
      c++;
  return c;
}

static int slot_livre(Horda *h) {
  for (int i = 0; i < MAX_ZUMBIS_TELA; i++)
    if (!h->pool[i].vivo)
      return i;
  return -1;
}

void horda_atualizar_spawn(Horda *h) {
  if (h->fase_concluida || h->total_spawned >= TOTAL_ZUMBIS_FASE)
    return;

  h->timer_onda--;
  if (h->timer_onda > 0)
    return;

  int restam = TOTAL_ZUMBIS_FASE - h->total_spawned;
  int para_spawn = (restam < ZUMBIS_POR_ONDA) ? restam : ZUMBIS_POR_ONDA;

  for (int n = 0; n < para_spawn; n++) {
    int s = slot_livre(h);
    if (s < 0)
      break;

    float sx = SPAWN_X_MIN + (float)(rand() % (SPAWN_X_MAX - SPAWN_X_MIN));
    sx += n * 60;

    h->pool[s].x = sx;
    h->pool[s].y = SPAWN_Y;
    h->pool[s].x_inicial = sx;
    h->pool[s].y_inicial = SPAWN_Y;
    h->pool[s].direcao = ALLEGRO_FLIP_HORIZONTAL;
    h->pool[s].vivo = 1;
    h->pool[s].frame = (float)(rand() % 8);

    int rapido = (rand() % 100) < 25;
    if (rapido) {
      h->pool[s].tipo = 1;
      h->pool[s].velocidade = 1.8f + (float)(rand() % 50) / 100.0f;
      h->pool[s].vida = 70;
    } else {
      h->pool[s].tipo = 0;
      h->pool[s].velocidade = 0.6f + (float)(rand() % 40) / 100.0f;
      h->pool[s].vida = 100;
    }
    h->total_spawned++;
  }
  h->timer_onda = INTERVALO_ONDA;
}

void horda_atualizar_movimento(Horda *h, Jogador *j) {
  for (int i = 0; i < MAX_ZUMBIS_TELA; i++) {
    Inimigo *z = &h->pool[i];
    if (!z->vivo)
      continue;

    float vel_anim = (z->tipo == 1) ? 0.12f : 0.05f;
    z->frame += vel_anim;

    float dy = fabsf(j->mov.y - z->y);
    if (dy > 180) {
      if (z->x < z->x_inicial) {
        z->x += z->velocidade;
        z->direcao = 0;
      } else if (z->x > z->x_inicial) {
        z->x -= z->velocidade;
        z->direcao = ALLEGRO_FLIP_HORIZONTAL;
      }
    } else {
      if (j->mov.x < z->x) {
        z->x -= z->velocidade;
        z->direcao = ALLEGRO_FLIP_HORIZONTAL;
      } else {
        z->x += z->velocidade;
        z->direcao = 0;
      }
    }
  }
}

void horda_verificar_ataque(Horda *h, Jogador *jog, Sanidade *san) {
  if (!jog->atacando)
    return;
  if (jog->frame_ataque < 2.0f || jog->frame_ataque > 2.5f)
    return;

  float atk_x = (jog->direcao == 0) ? jog->mov.x + 40 : jog->mov.x - 60;
  float atk_y = jog->mov.y;
  float haw = 55, hah = 50;

  for (int i = 0; i < MAX_ZUMBIS_TELA; i++) {
    Inimigo *z = &h->pool[i];
    if (!z->vivo)
      continue;

    float hzx = z->x + 10, hzy = z->y + 10, hzw = 45, hzh = 75;
    if (atk_x < hzx + hzw && atk_x + haw > hzx && atk_y < hzy + hzh &&
        atk_y + hah > hzy) {
      int dano = (jog->tipo_ataque == 1)   ? 15
                 : (jog->tipo_ataque == 2) ? 30
                                           : 50;
      z->vida -= dano;
      if (z->vida < 0)
        z->vida = 0;
      if (z->vida <= 0) {
        z->vivo = 0;
        h->total_mortos++;
        san->zumbis_mortos++;
        san->ultimo_abate = al_get_time();
        san->regenerando = 0;
        atualizar_sanidade(san);
        if (h->total_mortos >= TOTAL_ZUMBIS_FASE)
          h->fase_concluida = 1;
      }
    }
  }
}

void horda_verificar_dano_jogador(Horda *h, Jogador *jog, VidaStatus *vidas) {
  for (int i = 0; i < MAX_ZUMBIS_TELA; i++) {
    Inimigo *z = &h->pool[i];
    if (!z->vivo)
      continue;

    if (jog->mov.x < z->x + 60 && jog->mov.x + HITBOX_W > z->x &&
        jog->mov.y < z->y + 80 && jog->mov.y + HITBOX_H > z->y) {
      if (al_get_time() - jog->ultimo_dano > 1.0) {
        perder_vida(vidas);
        jog->ultimo_dano = al_get_time();
        jog->mov.x += (jog->direcao == 0) ? -120 : 120;
        jog->mov.vel_y = -8;
      }
    }
  }
}

void horda_desenhar(Horda *h, ALLEGRO_BITMAP *spr) {
  for (int i = 0; i < MAX_ZUMBIS_TELA; i++) {
    Inimigo *z = &h->pool[i];
    if (!z->vivo)
      continue;

    float dx = z->x - ZUMBI_OFFSET_X;
    float dy = z->y - ZUMBI_OFFSET_Y;
    int fz = (int)z->frame % FRAMES_ZUMBI;

    ALLEGRO_COLOR tint =
        (z->tipo == 1) ? al_map_rgb(255, 120, 120) : al_map_rgb(255, 255, 255);
    al_draw_tinted_scaled_bitmap(spr, tint, fz * 96, 0, 96, 96, dx, dy,
                                 ZUMBI_DRAW_W, ZUMBI_DRAW_H, z->direcao);

    float bw2 = 58;
    float bx2 = dx + ZUMBI_DRAW_W / 2.0f - bw2 / 2.0f - 2;
    float by2 = dy + 26;
    al_draw_filled_rectangle(bx2, by2, bx2 + bw2, by2 + 6,
                             al_map_rgb(80, 0, 0));
    al_draw_filled_rectangle(bx2, by2, bx2 + (z->vida * 0.6f), by2 + 6,
                             al_map_rgb(255, 0, 0));
  }
}

/* ================================================================== */
/*  RANKING: tela de fim de fase                                        */
/* ================================================================== */
void tela_ranking(ALLEGRO_EVENT_QUEUE *queue, ALLEGRO_TIMER *timer,
                  ALLEGRO_FONT *fonte, ALLEGRO_FONT *fonte_hud,
                  Temporizador *tempo, double tempo_final) {

  int pos = busca_binaria(tempo->ranking, tempo->quantidade_scores,
                          (float)tempo_final);

  if (tempo->quantidade_scores < 10) {
    for (int i = tempo->quantidade_scores; i > pos; i--)
      tempo->ranking[i] = tempo->ranking[i - 1];

    tempo->ranking[pos] = (float)tempo_final;
    tempo->quantidade_scores++;
  } else if (pos < 10) {
    for (int i = 9; i > pos; i--)
      tempo->ranking[i] = tempo->ranking[i - 1];

    tempo->ranking[pos] = (float)tempo_final;
  }
  salvar_ranking(tempo);

  ALLEGRO_EVENT ev;
  while (1) {
    al_wait_for_event(queue, &ev);
    if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
      return;
    if (ev.type == ALLEGRO_EVENT_KEY_DOWN)
      if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER ||
          ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
        return;
    if (ev.type != ALLEGRO_EVENT_TIMER)
      continue;

    al_clear_to_color(al_map_rgb(15, 10, 5));

    float pw = 700, ph = 680;
    float px_ = LARGURA / 2.0f - pw / 2.0f;
    float py_ = ALTURA / 2.0f - ph / 2.0f;

    al_draw_filled_rounded_rectangle(px_ + 6, py_ + 6, px_ + pw + 6,
                                     py_ + ph + 6, 14, 14,
                                     al_map_rgba(0, 0, 0, 160));
    al_draw_filled_rounded_rectangle(px_, py_, px_ + pw, py_ + ph, 14, 14,
                                     al_map_rgb(50, 28, 8));
    al_draw_rounded_rectangle(px_, py_, px_ + pw, py_ + ph, 14, 14,
                              al_map_rgb(218, 165, 32), 4);
    al_draw_rounded_rectangle(px_ + 6, py_ + 6, px_ + pw - 6, py_ + ph - 6, 10,
                              10, al_map_rgba(255, 210, 80, 60), 1);

    al_draw_text(fonte, al_map_rgba(0, 0, 0, 180), LARGURA / 2.0f + 3,
                 py_ + 20 + 3, ALLEGRO_ALIGN_CENTER, "FASE CONCLUIDA!");
    al_draw_text(fonte, al_map_rgb(255, 215, 0), LARGURA / 2.0f, py_ + 20,
                 ALLEGRO_ALIGN_CENTER, "FASE CONCLUIDA!");

    char buf[80];
    sprintf(buf, "Seu tempo: %.2f s", tempo_final);
    al_draw_text(fonte_hud, al_map_rgba(0, 0, 0, 180), LARGURA / 2.0f + 2,
                 py_ + 80 + 2, ALLEGRO_ALIGN_CENTER, buf);
    al_draw_text(fonte_hud, al_map_rgb(255, 255, 180), LARGURA / 2.0f, py_ + 80,
                 ALLEGRO_ALIGN_CENTER, buf);

    al_draw_line(px_ + 40, py_ + 135, px_ + pw - 40, py_ + 135,
                 al_map_rgb(218, 165, 32), 2);
    al_draw_text(fonte_hud, al_map_rgb(218, 165, 32), LARGURA / 2.0f, py_ + 148,
                 ALLEGRO_ALIGN_CENTER, "TOP 10");

    for (int i = 0; i < tempo->quantidade_scores; i++) {
      char linha[64];
      sprintf(linha, "%d.   %.2f s", i + 1, tempo->ranking[i]);
      ALLEGRO_COLOR cor = al_map_rgb(220, 220, 220);
      if (fabsf(tempo->ranking[i] - (float)tempo_final) < 0.005f)
        cor = al_map_rgb(255, 215, 0);
      al_draw_text(fonte_hud, al_map_rgba(0, 0, 0, 140), LARGURA / 2.0f + 2,
                   py_ + 195 + i * 44 + 2, ALLEGRO_ALIGN_CENTER, linha);
      al_draw_text(fonte_hud, cor, LARGURA / 2.0f, py_ + 195 + i * 44,
                   ALLEGRO_ALIGN_CENTER, linha);
    }

    al_draw_text(fonte_hud, al_map_rgba(0, 0, 0, 160), LARGURA / 2.0f + 2,
                 py_ + ph - 52 + 2, ALLEGRO_ALIGN_CENTER,
                 "Pressione ENTER para sair");
    al_draw_text(fonte_hud, al_map_rgb(160, 160, 160), LARGURA / 2.0f,
                 py_ + ph - 52, ALLEGRO_ALIGN_CENTER,
                 "Pressione ENTER para sair");

    al_flip_display();
  }
}

/* ================================================================== */
/*  IMPLEMENTACOES das funcoes declaradas como prototipos               */
/* ================================================================== */

void perder_vida(VidaStatus *vidas) {
  for (int i = MAX_VIDAS - 1; i >= 0; i--)
    if (vidas[i].ativa) {
      vidas[i].ativa = 0;
      strcpy(vidas[i].status, "Perdida");
      break;
    }
}

void ordenar_ranking(float r[], int n) {
  for (int i = 0; i < n - 1; i++)
    for (int j = 0; j < n - i - 1; j++)
      if (r[j] > r[j + 1]) {
        float t = r[j];
        r[j] = r[j + 1];
        r[j + 1] = t;
      }
}

int busca_binaria(float r[], int n, float v) {
  int lo = 0, hi = n - 1, mid;
  while (lo <= hi) {
    mid = (lo + hi) / 2;
    if (v < r[mid])
      hi = mid - 1;
    else
      lo = mid + 1;
  }
  return lo;
}

void carregar_ranking(Temporizador *t) {
  FILE *f = fopen("ranking.txt", "r");
  if (!f)
    return;
  while (fscanf(f, "%f", &t->ranking[t->quantidade_scores]) == 1) {
    t->quantidade_scores++;
    if (t->quantidade_scores >= 10)
      break;
  }
  fclose(f);
  ordenar_ranking(t->ranking, t->quantidade_scores);
}

void salvar_ranking(Temporizador *t) {
  FILE *f = fopen("ranking.txt", "w");
  if (!f)
    return;
  for (int i = 0; i < t->quantidade_scores; i++)
    fprintf(f, "%.2f\n", t->ranking[i]);
  fclose(f);
}

void gerar_mapa_colisao(ALLEGRO_BITMAP *mapa) {
  al_lock_bitmap(mapa, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);
  for (int y = 0; y < al_get_bitmap_height(mapa); y++)
    for (int x = 0; x < al_get_bitmap_width(mapa); x++) {
      ALLEGRO_COLOR c = al_get_pixel(mapa, x, y);
      unsigned char r, g, b;
      al_unmap_rgb(c, &r, &g, &b);
      colisao[y][x] = ((r + g + b) / 3 < 120);
    }
  al_unlock_bitmap(mapa);
}

/* ================================================================== */
int main(void)
/* ================================================================== */
{
  srand((unsigned)time(NULL));

  if (!al_init()) {
    puts("Erro al_init");
    return 1;
  }
  if (!al_init_primitives_addon()) {
    puts("Erro primitives");
    return 1;
  }
  if (!al_install_keyboard()) {
    puts("Erro keyboard");
    return 1;
  }
  if (!al_init_image_addon()) {
    puts("Erro image");
    return 1;
  }
  al_init_font_addon();
  al_init_ttf_addon();

  ALLEGRO_DISPLAY *display = al_create_display(LARGURA, ALTURA);
  ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
  ALLEGRO_TIMER *timer = al_create_timer(1.0 / FPS);
  if (!display || !queue || !timer) {
    puts("Erro init display/queue/timer");
    return 1;
  }

  ALLEGRO_FONT *fonte = al_load_ttf_font("assets/PublicPixel.ttf", 30, 0);
  ALLEGRO_FONT *fonte_hud = al_load_ttf_font("assets/PublicPixel.ttf", 18, 0);
  if (!fonte)
    fonte = al_create_builtin_font();
  if (!fonte_hud)
    fonte_hud = al_create_builtin_font();

  al_register_event_source(queue, al_get_display_event_source(display));
  al_register_event_source(queue, al_get_timer_event_source(timer));
  al_register_event_source(queue, al_get_keyboard_event_source());

  ALLEGRO_BITMAP *bg_menu = al_load_bitmap("assets/cenarios/inicio.png");
  if (!bg_menu) {
    puts("Erro bg_menu");
    return 1;
  }

  al_start_timer(timer);

  OpcaoMenu escolha = executar_menu(queue, timer, bg_menu, fonte);
  if (escolha == MENU_SAIR) {
    al_destroy_bitmap(bg_menu);
    al_destroy_font(fonte_hud);
    al_destroy_font(fonte);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);
    return 0;
  }
  al_flush_event_queue(queue);

  /* --- bitmaps do jogo --- */
  ALLEGRO_BITMAP *bg = al_load_bitmap("assets/cenarios/background2.png");
  ALLEGRO_BITMAP *mapa = al_load_bitmap("assets/cenarios/colisao2.png");
  ALLEGRO_BITMAP *idle = al_load_bitmap("assets/sprites/Samurai/Idle.png");
  ALLEGRO_BITMAP *run = al_load_bitmap("assets/sprites/Samurai/Run.png");
  ALLEGRO_BITMAP *jump = al_load_bitmap("assets/sprites/Samurai/Jump.png");
  ALLEGRO_BITMAP *coracao = al_load_bitmap("assets/itens/vida.png");
  ALLEGRO_BITMAP *spr_estamina = al_load_bitmap("assets/itens/estamina.png");
  ALLEGRO_BITMAP *zumbi_sprite =
      al_load_bitmap("assets/sprites/Zombies/Walk.png");
  ALLEGRO_BITMAP *ataque1 =
      al_load_bitmap("assets/sprites/Samurai/Attack_1.png");
  ALLEGRO_BITMAP *ataque2 =
      al_load_bitmap("assets/sprites/Samurai/Attack_2.png");
  ALLEGRO_BITMAP *ataque3 =
      al_load_bitmap("assets/sprites/Samurai/Attack_3.png");
  ALLEGRO_BITMAP *spr_sanidade = al_load_bitmap("assets/itens/sanidade.png");

  /* --- icones exclusivos da roda/HUD de habilidade --- */
  ALLEGRO_BITMAP *habilidade1 = al_load_bitmap("assets/itens/Habilidade1.png");
  ALLEGRO_BITMAP *habilidade2 = al_load_bitmap("assets/itens/Habilidade2.png");
  ALLEGRO_BITMAP *habilidade3 = al_load_bitmap("assets/itens/Habilidade3.png");

  if (!bg || !mapa || !idle || !run || !jump || !coracao || !spr_estamina ||
      !zumbi_sprite || !ataque1 || !ataque2 || !ataque3 || !habilidade1 ||
      !habilidade2 || !habilidade3) {
    puts("Erro bitmaps");
    return 1;
  }

  gerar_mapa_colisao(mapa);

  /* --- jogador --- */
  Jogador jogador = {0};
  jogador.mov.x = 60;
  jogador.mov.y = 253;
  jogador.tipo_ataque = 1;
  jogador.estamina = MAX_ESTAMINA;

  /* --- sanidade --- */
  Sanidade sanidade = {MAX_SANIDADE, 0, 0, 0, 0.0, 0};

  /* --- horda --- */
  Horda horda;
  horda_init(&horda);

  /* --- temporizador --- */
  Temporizador tempo = {0};
  tempo.inicio = al_get_time();
  tempo.ativo = 1;
  carregar_ranking(&tempo);

  /* --- vidas --- */
  VidaStatus *vetor_vidas = malloc(MAX_VIDAS * sizeof(VidaStatus));
  for (int i = 0; i < MAX_VIDAS; i++) {
    vetor_vidas[i].ativa = 1;
    strcpy(vetor_vidas[i].status, "Inteira");
  }

  /* --- decoracao --- */
  int lm = ALTURA / 100 + 1, cm = LARGURA / 100 + 1;
  int **matriz_dec = criar_matriz_decorativa(lm, cm);

  /* --- controle de pausa --- */
  int pausado = 0;
  double pausa_inicio = 0;

  /* --- roda de habilidade --- */
  int roda_aberta = 0;
  int roda_selecao = 1; /* espelha tipo_ataque enquanto a roda esta aberta */

  sanidade.ultimo_abate = al_get_time();

  int rodando = 1;
  ALLEGRO_EVENT ev;
  ALLEGRO_KEYBOARD_STATE state;

  while (rodando) {
    al_wait_for_event(queue, &ev);
    if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
      rodando = 0;
      break;
    }
    if (ev.type != ALLEGRO_EVENT_TIMER)
      continue;

    al_get_keyboard_state(&state);

    /* ---------- ESC: pausa ---------- */
    static int esc_ant = 0;
    int esc_now = al_key_down(&state, ALLEGRO_KEY_ESCAPE);
    if (esc_now && !esc_ant && !sanidade.game_over && !horda.fase_concluida) {
      pausado = !pausado;
      if (pausado)
        pausa_inicio = al_get_time();
      else {
        double dt = al_get_time() - pausa_inicio;
        tempo.inicio += dt;
        jogador.ultimo_dano += dt;
        sanidade.ultimo_abate += dt;
      }
    }
    esc_ant = esc_now;

    /* ============================================================ */
    if (!pausado && !sanidade.game_over && !horda.fase_concluida) {
      atualizar_sanidade(&sanidade);
      jogador.frame += 0.15f;
      jogador.movendo = 0;

      if (tempo.ativo)
        tempo.atual = al_get_time() - tempo.inicio;

      /* --------------------------------------------------------
         RODA DE HABILIDADE — tecla K
         - Pressionar K : abre a roda e salva selecao corrente
         - Q / E        : navega enquanto a roda esta aberta
         - Soltar K     : confirma e fecha a roda
         -------------------------------------------------------- */
      static int k_ant = 0;
      int k_now = al_key_down(&state, ALLEGRO_KEY_K);

      if (k_now && !k_ant) /* K pressionado: abre roda */
      {
        roda_aberta = 1;
        roda_selecao = jogador.tipo_ataque;
      }
      if (!k_now && k_ant) /* K solto: confirma e fecha */
      {
        jogador.tipo_ataque = roda_selecao;
        roda_aberta = 0;
      }

      /* navegacao Q / E so quando a roda esta aberta */
      if (roda_aberta) {
        static int q_ant = 0, e_ant = 0;
        int q_now = al_key_down(&state, ALLEGRO_KEY_Q);
        int e_now = al_key_down(&state, ALLEGRO_KEY_E);

        if (q_now && !q_ant) {
          roda_selecao--;
          if (roda_selecao < 1)
            roda_selecao = 3;
        }
        if (e_now && !e_ant) {
          roda_selecao++;
          if (roda_selecao > 3)
            roda_selecao = 1;
        }

        q_ant = q_now;
        e_ant = e_now;
      }

      k_ant = k_now;
      /* -------------------------------------------------------- */

      /* J: atacar (bloqueado enquanto roda esta aberta) */
      static int j_ant = 0;
      int j_now = al_key_down(&state, ALLEGRO_KEY_J);
      if (j_now && !j_ant && !jogador.atacando && !roda_aberta) {
        if (jogador.estamina >= 1.5f) {
          jogador.atacando = 1;
          jogador.frame_ataque = 0;
          jogador.estamina -= 1.5f;
        }
      }
      j_ant = j_now;

      /* chao */
      jogador.no_chao = esta_no_chao(mapa, jogador.mov.x, jogador.mov.y);

      /* recarga estamina no chao */
      if (jogador.no_chao) {
        jogador.estamina += RECARGA_ESTAMINA;
        if (jogador.estamina > MAX_ESTAMINA)
          jogador.estamina = MAX_ESTAMINA;
      }

      /* movimento horizontal (bloqueado pela roda) */
      float nx = jogador.mov.x;
      if (al_key_down(&state, ALLEGRO_KEY_D) && !jogador.atacando &&
          !roda_aberta) {
        nx += VELOCIDADE;
        jogador.direcao = 0;
        jogador.movendo = 1;
      }
      if (al_key_down(&state, ALLEGRO_KEY_A) && !jogador.atacando &&
          !roda_aberta) {
        nx -= VELOCIDADE;
        jogador.direcao = ALLEGRO_FLIP_HORIZONTAL;
        jogador.movendo = 1;
      }
      if (!colide_mapa(mapa, nx, jogador.mov.y))
        jogador.mov.x = nx;

      /* pulo (bloqueado pela roda) */
      if (al_key_down(&state, ALLEGRO_KEY_W) && jogador.no_chao &&
          !jogador.atacando && !roda_aberta && jogador.estamina >= CUSTO_PULO) {
        jogador.mov.vel_y = FORCA_PULO;
        jogador.estamina -= CUSTO_PULO;
        if (jogador.estamina < 0)
          jogador.estamina = 0;
      }

      /* gravidade */
      jogador.mov.vel_y += GRAVIDADE;
      if (jogador.mov.vel_y > MAX_QUEDA)
        jogador.mov.vel_y = MAX_QUEDA;
      float ny = jogador.mov.y + jogador.mov.vel_y;
      if (!colide_mapa(mapa, jogador.mov.x, ny))
        jogador.mov.y = ny;
      else
        jogador.mov.vel_y = 0;

      jogador.no_chao = esta_no_chao(mapa, jogador.mov.x, jogador.mov.y);

      /* caiu no buraco */
      if (jogador.mov.y > ALTURA + 200) {
        perder_vida(vetor_vidas);
        jogador.mov.x = 60;
        jogador.mov.y = 253;
        jogador.mov.vel_y = 0;
      }

      /* horda */
      horda_atualizar_spawn(&horda);
      horda_atualizar_movimento(&horda, &jogador);
      horda_verificar_ataque(&horda, &jogador, &sanidade);
      horda_verificar_dano_jogador(&horda, &jogador, vetor_vidas);

      /* fase concluida: para o timer */
      if (horda.fase_concluida && tempo.ativo) {
        tempo.fim = al_get_time();
        tempo.atual = tempo.fim - tempo.inicio;
        tempo.ativo = 0;
      }
    }
    /* ============================================================ */

    /* ---------- DESENHO ---------- */
    float draw_x = jogador.mov.x - HITBOX_OFFSET_X;
    float draw_y = jogador.mov.y - HITBOX_OFFSET_Y;

    al_clear_to_color(al_map_rgb(0, 0, 0));
    al_draw_bitmap(bg, 0, 0, 0);
    desenhar_matriz_fundo(matriz_dec, lm, cm);

    horda_desenhar(&horda, zumbi_sprite);

    /* jogador */
    if (jogador.atacando) {
      int mf = (jogador.tipo_ataque == 1)   ? 6
               : (jogador.tipo_ataque == 2) ? 4
                                            : 3;
      int f = (int)jogador.frame_ataque;
      if (f >= mf)
        f = mf - 1;
      ALLEGRO_BITMAP *atk = (jogador.tipo_ataque == 1)   ? ataque1
                            : (jogador.tipo_ataque == 2) ? ataque2
                                                         : ataque3;
      al_draw_scaled_bitmap(atk, 128 * f, 0, 128, 128, draw_x - 10, draw_y - 1,
                            ATTACK_DRAW_W, ATTACK_DRAW_H, jogador.direcao);
      jogador.frame_ataque += 0.30f;
      if (jogador.frame_ataque >= mf) {
        jogador.atacando = 0;
        jogador.frame_ataque = 0;
      }
    } else if (!jogador.no_chao) {
      int f = (int)jogador.frame % FRAMES_JUMP;
      al_draw_scaled_bitmap(jump, 128 * f, 0, 128, 128, draw_x, draw_y, DRAW_W,
                            DRAW_H, jogador.direcao);
    } else if (jogador.movendo) {
      int f = (int)jogador.frame % FRAMES_RUN;
      al_draw_scaled_bitmap(run, 128 * f, 0, 128, 128, draw_x, draw_y, DRAW_W,
                            DRAW_H, jogador.direcao);
    } else {
      int f = (int)jogador.frame % FRAMES_IDLE;
      al_draw_scaled_bitmap(idle, 128 * f, 0, 128, 128, draw_x, draw_y, DRAW_W,
                            DRAW_H, jogador.direcao);
    }

    /* overlay vermelho de sanidade */
    desenhar_overlay_sanidade(&sanidade);

    /* HUD */
    char txt[60];
    sprintf(txt, "Tempo: %.2f s", tempo.atual);
    desenhar_hud_tempo(fonte_hud, txt);
    desenhar_hud_horda(fonte_hud, &horda);
    desenhar_vidas(vetor_vidas, coracao);
    desenhar_estamina(jogador.estamina, spr_estamina);
    desenhar_sanidade(&sanidade, spr_sanidade);
    desenhar_aviso_sanidade(&sanidade, fonte, al_get_time());

    /* HUD ataque — canto inferior direito (sempre visivel) */
    desenhar_hud_ataque(ataque1, ataque2, ataque3, habilidade1, habilidade2,
                        habilidade3, jogador.tipo_ataque, fonte_hud);

    /* game over por sanidade */
    if (sanidade.game_over) {
      al_draw_text(fonte, al_map_rgba(0, 0, 0, 200), LARGURA / 2.0f + 4,
                   ALTURA / 2.0f - 50 + 4, ALLEGRO_ALIGN_CENTER, "GAME OVER");
      al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2.0f,
                   ALTURA / 2.0f - 50, ALLEGRO_ALIGN_CENTER, "GAME OVER");
      al_draw_text(fonte_hud, al_map_rgba(0, 0, 0, 180), LARGURA / 2.0f + 2,
                   ALTURA / 2.0f + 20 + 2, ALLEGRO_ALIGN_CENTER,
                   "Sua sanidade chegou ao limite...");
      al_draw_text(fonte_hud, al_map_rgb(255, 200, 200), LARGURA / 2.0f,
                   ALTURA / 2.0f + 20, ALLEGRO_ALIGN_CENTER,
                   "Sua sanidade chegou ao limite...");
    }

    /* fase concluida */
    if (horda.fase_concluida && !sanidade.game_over) {
      al_draw_filled_rectangle(0, 0, LARGURA, ALTURA,
                               al_map_rgba(255, 215, 0, 40));
      al_draw_text(fonte, al_map_rgba(0, 0, 0, 200), LARGURA / 2.0f + 4,
                   ALTURA / 2.0f - 60 + 4, ALLEGRO_ALIGN_CENTER,
                   "50 ZUMBIS ELIMINADOS!");
      al_draw_text(fonte, al_map_rgb(255, 215, 0), LARGURA / 2.0f,
                   ALTURA / 2.0f - 60, ALLEGRO_ALIGN_CENTER,
                   "50 ZUMBIS ELIMINADOS!");
      al_draw_text(fonte_hud, al_map_rgb(255, 255, 255), LARGURA / 2.0f,
                   ALTURA / 2.0f, ALLEGRO_ALIGN_CENTER, "Abrindo ranking...");
      al_flip_display();
      al_rest(2.0);
      tela_ranking(queue, timer, fonte, fonte_hud, &tempo, tempo.atual);
      rodando = 0;
      continue;
    }

    /* pausa */
    if (pausado) {
      al_draw_filled_rectangle(0, 0, LARGURA, ALTURA,
                               al_map_rgba(0, 0, 0, 160));
      al_draw_filled_rounded_rectangle(LARGURA / 2.0 - 310, ALTURA / 2.0 - 90,
                                       LARGURA / 2.0 + 310, ALTURA / 2.0 + 70,
                                       12, 12, al_map_rgb(101, 60, 20));
      al_draw_rounded_rectangle(LARGURA / 2.0 - 310, ALTURA / 2.0 - 90,
                                LARGURA / 2.0 + 310, ALTURA / 2.0 + 70, 12, 12,
                                al_map_rgb(218, 165, 32), 3);
      al_draw_text(fonte, al_map_rgba(0, 0, 0, 180), LARGURA / 2.0 + 3,
                   ALTURA / 2.0 - 75 + 3, ALLEGRO_ALIGN_CENTER, "PAUSADO");
      al_draw_text(fonte, al_map_rgb(255, 215, 0), LARGURA / 2.0,
                   ALTURA / 2.0 - 75, ALLEGRO_ALIGN_CENTER, "PAUSADO");
      al_draw_text(fonte_hud, al_map_rgba(0, 0, 0, 180), LARGURA / 2.0 + 2,
                   ALTURA / 2.0 + 2, ALLEGRO_ALIGN_CENTER,
                   "Pressione ESC para continuar");
      al_draw_text(fonte_hud, al_map_rgb(255, 255, 255), LARGURA / 2.0,
                   ALTURA / 2.0, ALLEGRO_ALIGN_CENTER,
                   "Pressione ESC para continuar");
    }

    /* roda de habilidade — por cima de tudo */
    if (roda_aberta)
      desenhar_roda_habilidade(ataque1, ataque2, ataque3, habilidade1,
                               habilidade2, habilidade3, roda_selecao,
                               fonte_hud);

    al_flip_display();
  }

  /* limpeza */
  free(vetor_vidas);
  liberar_matriz(matriz_dec, lm);
  al_destroy_bitmap(bg_menu);
  al_destroy_bitmap(bg);
  al_destroy_bitmap(mapa);
  al_destroy_bitmap(idle);
  al_destroy_bitmap(run);
  al_destroy_bitmap(jump);
  al_destroy_bitmap(coracao);
  al_destroy_bitmap(spr_estamina);
  al_destroy_bitmap(zumbi_sprite);
  al_destroy_bitmap(ataque1);
  al_destroy_bitmap(ataque2);
  al_destroy_bitmap(ataque3);
  al_destroy_bitmap(habilidade1);
  al_destroy_bitmap(habilidade2);
  al_destroy_bitmap(habilidade3);
  if (spr_sanidade)
    al_destroy_bitmap(spr_sanidade);
  al_destroy_font(fonte_hud);
  al_destroy_font(fonte);
  al_destroy_timer(timer);
  al_destroy_event_queue(queue);
  al_destroy_display(display);
  return 0;
}
