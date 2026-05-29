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

#define HITBOX_W 50
#define HITBOX_H 80
#define HITBOX_OFFSET_X 70
#define HITBOX_OFFSET_Y 40

#define VELOCIDADE 4.0f
#define GRAVIDADE 0.7f
#define FORCA_PULO -13.05f
#define MAX_QUEDA 18.0f

/* gravidade para zumbis */
#define GRAVIDADE_ZUMBI 0.55f
#define MAX_QUEDA_ZUMBI 14.0f

#define FRAMES_IDLE 6
#define FRAMES_RUN 8
#define FRAMES_JUMP 12
#define FRAMES_HURT 2
#define FRAMES_DEAD 3

/* zumbi */
#define FRAMES_ZUMBI_WALK 8
#define FRAMES_ZUMBI_RUN 7
#define FRAMES_ZUMBI_ATTACK1 5
#define FRAMES_ZUMBI_ATTACK2 4
#define FRAMES_ZUMBI_ATTACK3 5
#define FRAMES_ZUMBI_BITE 11
#define FRAMES_ZUMBI_HURT 3
#define FRAMES_ZUMBI_DEAD 5
#define FRAMES_ZUMBI_IDLE 8

#define MAX_VIDAS 5

#define ATTACK_DRAW_W DRAW_W
#define ATTACK_DRAW_H DRAW_H

/* =============================================================
   ZUMBI: sprite sheet usa frames de 96x96 px.
   ============================================================= */
#define ZUMBI_SRC_W 96
#define ZUMBI_SRC_H 96
#define ZUMBI_DRAW_W 120
#define ZUMBI_DRAW_H 120

#define ZUMBI_HBX_OFFSET_X 38
#define ZUMBI_HBX_OFFSET_Y 36
#define ZUMBI_HBX_W 44
#define ZUMBI_HBX_H 90

#define ZUMBI_OFFSET_X 0
#define ZUMBI_OFFSET_Y 0

/* estamina */
#define MAX_ESTAMINA 10.0f
#define CUSTO_PULO 1.0f
#define RECARGA_ESTAMINA 0.03f

/* ============================================================
   SANIDADE
   ============================================================ */
#define MAX_SANIDADE 100.0f
#define TOTAL_ZUMBIS_GAME_OVER 20
#define AVISO_ZUMBIS 15
#define QUEDA_POR_ZUMBI (MAX_SANIDADE / (float)TOTAL_ZUMBIS_GAME_OVER)
#define SANIDADE_OVERLAY_LIM 25.0f
#define TEMPO_INICIO_RECUPERACAO 5.0
#define TEMPO_RECUPERACAO_TOTAL 10.0

/* horda */
#define MAX_ZUMBIS_TELA 20
#define TOTAL_ZUMBIS_FASE 50
#define ZUMBIS_POR_ONDA 5
#define INTERVALO_ONDA 600

#define SPAWN_DIR_X_MIN 1700
#define SPAWN_DIR_X_MAX 1800
#define SPAWN_DIR_Y 540

#define SPAWN_TOP_LEFT_X 60
#define SPAWN_TOP_LEFT_Y 90

#define SPAWN_MIN_JOGADOR_Y 300
#define SPAWN_TOP_Y_THRESHOLD 300

/* área de patrulha */
#define PATROL_AREA_X_MIN 100
#define PATROL_AREA_X_MAX 1800

/* nível alto */
#define NIVEL_ALTO_Y 380

/* patrulha */
#define PATRULHA_VEL 0.4f
#define PATRULHA_DIST 80.0f

/* tempo de invulnerabilidade do jogador após tomar dano */
#define TEMPO_INVUL 1.0

/* Knockback */
#define KNOCKBACK_ZUMBI_DURACAO 1.3
#define KNOCKBACK_SAMURAI_DURACAO 0.5

#define KNOCKBACK_ZUMBI_X 15.0f
#define KNOCKBACK_SAMURAI_X 28.0f

/* Delay entre ataques consecutivos */
#define DELAY_ATAQUE_12 0.35
#define DELAY_ATAQUE_3 0.45

/* Dash do ataque 3 */
#define DASH_ATK3_DIST 84.0f
#define CUSTO_ATK3 4.0f
#define CUSTO_ATK1 1.0f
#define CUSTO_ATK2 2.0f

/* Hitbox de ataque do SAMURAI */
#define SAM_ATK_FRAME_INICIO 2
#define SAM_ATK_FRAME_FIM 4
#define SAM_ATK_W 25
#define SAM_ATK_H 50
#define SAM_ATK_REACH 18

/* Hitbox de ataque do ZUMBI */
#define ZUM_ATK_W 45
#define ZUM_ATK_H 60
#define ZUM_ATK_REACH 12

/* Margem vertical zumbi abaixo do jogador */
#define ZUMBI_ABAIXO_MARGEM 280

/* POÇÃO DE VIDA */
/* mMrgem esquerda proibida */
#define POCAO_SPAWN_X_MIN  415
#define POCAO_SPAWN_X_MAX  1870
#define POCAO_SPAWN_Y      100  /* cai do topo */
#define POCAO_LARGURA      48
#define POCAO_ALTURA       48
#define POCAO_GRAVIDADE    0.45f
#define POCAO_MAX_QUEDA    14.0f
#define POCAO_RECUPERA_VIDAS 3
#define POCAO_HITBOX_W     40
#define POCAO_HITBOX_H     40

unsigned char colisao[ALTURA][3000];
/* ponteiro global para o mapa de colisao ? usado pela pocao */
static ALLEGRO_BITMAP *jog_mapa_ptr = NULL;

/* ================================================================== */
typedef enum
{
    SAM_IDLE = 0,
    SAM_RUN,
    SAM_JUMP,
    SAM_ATTACK,
    SAM_HURT,
    SAM_DEAD
} EstadoSamurai;

typedef enum
{
    ZUM_WALK = 0,
    ZUM_RUN,
    ZUM_ATTACK,
    ZUM_BITE,
    ZUM_HURT,
    ZUM_DEAD,
    ZUM_IDLE
} EstadoZumbi;

typedef struct
{
    int ativa;
    char status[20];
} VidaStatus;

typedef struct
{
    float x, y, vel_y;
} Movimento;

typedef struct
{
    Movimento mov;
    float frame, frame_ataque;
    int no_chao, direcao, movendo, atacando, tipo_ataque;
    double ultimo_dano;
    double ultimo_ataque;
    float estamina;
    EstadoSamurai estado;
    float frame_hurt;
    double hurt_inicio;
    float frame_dead;
    int morto;
    int morte_animando;
    int dash_ativo;
    float dash_dist;
} Jogador;

typedef struct
{
    float x, y, x_inicial, y_inicial;
    float vel_y;
    float velocidade;
    int direcao, vivo;
    float frame;
    int vida;
    int tipo;
    EstadoZumbi estado;
    double tempo_hurt;
    int atacando_jogador;
    double tempo_ataque;
    float patrol_dir;
    float patrol_base;
    int spawn_tipo;
    int dano_aplicado;
    int atingido_no_ataque;
} Inimigo;

typedef struct
{
    double inicio, atual, fim;
    int ativo;
    float ranking[10];
    int quantidade_scores;
} Temporizador;

typedef enum
{
    MENU_JOGAR = 0,
    MENU_SAIR = 1
} OpcaoMenu;

typedef struct
{
    float valor;
    int zumbis_mortos;
    int ciclo_base;
    int game_over;
    double ultimo_abate;
    int regenerando;
} Sanidade;

typedef struct
{
    Inimigo pool[MAX_ZUMBIS_TELA];
    int total_spawned;
    int total_mortos;
    int timer_onda;
    int fase_concluida;
    int top_spawned;
    int top_ativo;
    int spawns_esq;
    int spawns_dir;
} Horda;

typedef struct
{
    float x, y;
    float vel_y;
    int   no_chao;
    int   ativa;
    int   coletada;
} Pocao;

/* ================================================================== */
/*  PROTÓTIPOS                                                          */
/* ================================================================== */
void perder_vida(VidaStatus *vidas);
int contar_vidas(VidaStatus *vidas);
void restaurar_vidas(VidaStatus *vidas, int quantidade);
void ordenar_ranking(float r[], int n);
int busca_binaria(float r[], int n, float v);
void carregar_ranking(Temporizador *t);
void salvar_ranking(Temporizador *t);
void atualizar_sanidade(Sanidade *san);
void gerar_mapa_colisao(ALLEGRO_BITMAP *mapa);
void pocao_tentar_spawn(Pocao *p, VidaStatus *vidas);
void pocao_atualizar(Pocao *p, Jogador *jog, VidaStatus *vidas);
void pocao_desenhar(Pocao *p, ALLEGRO_BITMAP *spr);

/* ================================================================== */
int **criar_matriz_decorativa(int linhas, int colunas)
{
    int **mat = (int **)malloc(linhas * sizeof(int *));
    for (int i = 0; i < linhas; i++)
    {
        mat[i] = (int *)malloc(colunas * sizeof(int));
        for (int j = 0; j < colunas; j++)
            mat[i][j] = rand() % 2;
    }
    return mat;
}

void desenhar_matriz_fundo(int **mat, int linhas, int colunas)
{
    for (int i = 0; i < linhas; i++)
        for (int j = 0; j < colunas; j++)
            if (mat[i][j] == 1)
                al_draw_filled_rectangle(j * 100, i * 100, j * 100 + 3, i * 100 + 3, al_map_rgba(255, 255, 255, 40));
}

void liberar_matriz(int **mat, int linhas)
{
    for (int i = 0; i < linhas; i++)
        free(mat[i]);
    free(mat);
}

OpcaoMenu executar_menu(ALLEGRO_EVENT_QUEUE *queue, ALLEGRO_TIMER *timer,
                        ALLEGRO_BITMAP *bg_menu, ALLEGRO_FONT *fonte)
{
    ALLEGRO_EVENT ev;
    OpcaoMenu opcao = MENU_JOGAR;
    while (1)
    {
        al_wait_for_event(queue, &ev);
        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            return MENU_SAIR;
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN)
        {
            if (ev.keyboard.keycode == ALLEGRO_KEY_A || ev.keyboard.keycode == ALLEGRO_KEY_LEFT)
                opcao = MENU_JOGAR;
            if (ev.keyboard.keycode == ALLEGRO_KEY_D || ev.keyboard.keycode == ALLEGRO_KEY_RIGHT)
                opcao = MENU_SAIR;
            if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER || ev.keyboard.keycode == ALLEGRO_KEY_SPACE)
                return opcao;
        }
        if (ev.type == ALLEGRO_EVENT_TIMER)
        {
            const char *jogar = (opcao == MENU_JOGAR) ? "> JOGAR <" : "JOGAR";
            const char *sair  = (opcao == MENU_SAIR)  ? "> SAIR <"  : "SAIR";
            al_clear_to_color(al_map_rgb(0, 0, 0));
            al_draw_scaled_bitmap(bg_menu, 0, 0,
                al_get_bitmap_width(bg_menu), al_get_bitmap_height(bg_menu),
                0, 0, LARGURA, ALTURA, 0);
            al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2.4, 990, ALLEGRO_ALIGN_CENTER, jogar);
            al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 1.6, 990, ALLEGRO_ALIGN_CENTER, sair);
            al_flip_display();
        }
    }
}

bool pixel_solido(ALLEGRO_BITMAP *mapa, int x, int y)
{
    (void)mapa;
    if (x < 0 || y < 0 || x >= 3000 || y >= ALTURA)
        return true;
    return colisao[y][x];
}

bool colide_mapa(ALLEGRO_BITMAP *mapa, float x, float y)
{
    int left = (int)x, right = (int)x + HITBOX_W - 1;
    int top  = (int)y, bottom = (int)y + HITBOX_H - 1;
    for (int px = left; px <= right; px += 4)
        if (pixel_solido(mapa, px, top))    return true;
    for (int px = left; px <= right; px += 4)
        if (pixel_solido(mapa, px, bottom)) return true;
    for (int py = top; py <= bottom; py += 4)
        if (pixel_solido(mapa, left, py))   return true;
    for (int py = top; py <= bottom; py += 4)
        if (pixel_solido(mapa, right, py))  return true;
    return false;
}

bool esta_no_chao(ALLEGRO_BITMAP *mapa, float x, float y)
{
    int left = (int)x + 4, right = (int)x + HITBOX_W - 4, foot = (int)y + HITBOX_H;
    return pixel_solido(mapa, left, foot) || pixel_solido(mapa, right, foot);
}

bool zumbi_colide_vertical(ALLEGRO_BITMAP *mapa, float x, float y, float vel_y)
{
    int hbx = (int)(x + ZUMBI_HBX_OFFSET_X);
    int hbw = ZUMBI_HBX_W;
    if (vel_y > 0)
    {
        int foot = (int)(y + ZUMBI_HBX_OFFSET_Y + ZUMBI_HBX_H);
        for (int px = hbx; px <= hbx + hbw; px += 6)
            if (pixel_solido(mapa, px, foot)) return true;
    }
    else
    {
        int top = (int)(y + ZUMBI_HBX_OFFSET_Y);
        for (int px = hbx; px <= hbx + hbw; px += 6)
            if (pixel_solido(mapa, px, top)) return true;
    }
    return false;
}

bool zumbi_no_chao(ALLEGRO_BITMAP *mapa, float x, float y)
{
    int hbx  = (int)(x + ZUMBI_HBX_OFFSET_X);
    int hbw  = ZUMBI_HBX_W;
    int foot = (int)(y + ZUMBI_HBX_OFFSET_Y + ZUMBI_HBX_H) + 2;
    for (int px = hbx; px <= hbx + hbw; px += 6)
        if (pixel_solido(mapa, px, foot)) return true;
    return false;
}

/* ------------------------------------------------------------------ */
int contar_vidas(VidaStatus *vidas)
{
    int c = 0;
    for (int i = 0; i < MAX_VIDAS; i++)
        if (vidas[i].ativa) c++;
    return c;
}

void restaurar_vidas(VidaStatus *vidas, int quantidade)
{
    int restauradas = 0;
    for (int i = 0; i < MAX_VIDAS && restauradas < quantidade; i++)
    {
        if (!vidas[i].ativa)
        {
            vidas[i].ativa = 1;
            strcpy(vidas[i].status, "Inteira");
            restauradas++;
        }
    }
}

void desenhar_vidas(VidaStatus *vidas, ALLEGRO_BITMAP *coracao)
{
    int vivas = contar_vidas(vidas);
    float pct = (float)vivas / MAX_VIDAS;
    ALLEGRO_COLOR cor = (vivas >= 4) ? al_map_rgb(0, 200, 0)
                      : (vivas >= 2) ? al_map_rgb(220, 200, 0)
                                     : al_map_rgb(200, 0, 0);
    float bx = 290, by = 940, bw = 320, bh = 63;
    al_draw_filled_rectangle(bx, by, bx + bw, by + bh, al_map_rgb(40, 40, 40));
    al_draw_filled_rectangle(bx, by, bx + bw * pct, by + bh, cor);
    al_draw_rectangle(bx, by, bx + bw, by + bh, al_map_rgb(255, 255, 255), 2);
    al_draw_scaled_bitmap(coracao, 0, 0,
        al_get_bitmap_width(coracao), al_get_bitmap_height(coracao),
        120, 813, 540, 340, 0);
}

void desenhar_estamina(float estamina, ALLEGRO_BITMAP *spr)
{
    float pct = estamina / MAX_ESTAMINA;
    ALLEGRO_COLOR cor = (estamina > 0) ? al_map_rgb(30, 144, 255) : al_map_rgb(80, 80, 80);
    float bx = 782, by = 940, bw = 325, bh = 63;
    al_draw_filled_rectangle(bx, by, bx + bw, by + bh, al_map_rgb(40, 40, 40));
    al_draw_filled_rectangle(bx, by, bx + bw * pct, by + bh, cor);
    al_draw_rectangle(bx, by, bx + bw, by + bh, al_map_rgb(255, 255, 255), 2);
    al_draw_scaled_bitmap(spr, 0, 0,
        al_get_bitmap_width(spr), al_get_bitmap_height(spr),
        625, 800, 540, 340, 0);
}

void desenhar_sanidade(Sanidade *san, ALLEGRO_BITMAP *spr)
{
    float pct = san->valor / MAX_SANIDADE;
    unsigned char g = (unsigned char)(20 * pct), b = g;
    ALLEGRO_COLOR cor = (san->valor > 0) ? al_map_rgb(180, g, b) : al_map_rgb(80, 80, 80);
    float bx = 90, by = 1050, bw = 30, bh = 230, fh = bh * pct, topo = by - bh;
    al_draw_filled_rectangle(bx, topo, bx + bw, by, al_map_rgb(40, 40, 40));
    al_draw_filled_rectangle(bx, by - fh, bx + bw, by, cor);
    al_draw_rectangle(bx, topo, bx + bw, by, al_map_rgb(255, 255, 255), 2);
    if (spr)
        al_draw_scaled_bitmap(spr, 0, 0,
            al_get_bitmap_width(spr), al_get_bitmap_height(spr),
            5, 780, 200, 300, 0);
}

static int zumbis_no_ciclo(const Sanidade *san)
{
    return san->zumbis_mortos - san->ciclo_base;
}

void desenhar_overlay_sanidade(Sanidade *san)
{
    if (zumbis_no_ciclo(san) < AVISO_ZUMBIS)
        return;

    double agora = al_get_time();
    double sem_matar = agora - san->ultimo_abate;
    float frac_overlay;

    if (sem_matar < TEMPO_INICIO_RECUPERACAO)
    {
        frac_overlay = 1.0f;
    }
    else if (sem_matar < TEMPO_RECUPERACAO_TOTAL)
    {
        float t_fade    = (float)(sem_matar - TEMPO_INICIO_RECUPERACAO);
        float dur_fade  = (float)(TEMPO_RECUPERACAO_TOTAL - TEMPO_INICIO_RECUPERACAO);
        frac_overlay = 1.0f - (t_fade / dur_fade);
        if (frac_overlay < 0.0f) frac_overlay = 0.0f;
    }
    else
    {
        return;
    }

    float pulso = 0.5f + 0.5f * sinf((float)agora * 5.0f);
    unsigned char alpha = (unsigned char)((60.0f + 120.0f * frac_overlay) * pulso * frac_overlay);
    if (alpha == 0) return;

    al_draw_filled_rectangle(0, 0, LARGURA, ALTURA, al_map_rgba(200, 0, 0, alpha));
}

void atualizar_sanidade(Sanidade *san)
{
    int no_ciclo = zumbis_no_ciclo(san);

    float alvo = MAX_SANIDADE - no_ciclo * QUEDA_POR_ZUMBI;
    if (alvo < 0.0f) alvo = 0.0f;

    double agora     = al_get_time();
    double sem_matar = agora - san->ultimo_abate;

    if (san->valor > alvo)
        san->valor = alvo;

    if (san->valor <= 0.0f)
    {
        san->valor    = 0.0f;
        san->game_over = 1;
        return;
    }

    if (sem_matar >= TEMPO_INICIO_RECUPERACAO && !san->game_over)
    {
        float dur_recup = (float)(TEMPO_RECUPERACAO_TOTAL - TEMPO_INICIO_RECUPERACAO);
        float t_recup   = (float)(sem_matar - TEMPO_INICIO_RECUPERACAO);
        float progresso = t_recup / dur_recup;
        if (progresso > 1.0f) progresso = 1.0f;

        float novo_valor = alvo + (MAX_SANIDADE - alvo) * progresso;
        if (novo_valor > san->valor)
            san->valor = novo_valor;
        if (san->valor > MAX_SANIDADE)
            san->valor = MAX_SANIDADE;

        san->regenerando = (progresso < 1.0f) ? 1 : 0;

        if (san->valor >= MAX_SANIDADE)
        {
            san->valor      = MAX_SANIDADE;
            san->ciclo_base = san->zumbis_mortos;
            san->regenerando = 0;
        }
    }
    else
    {
        san->regenerando = 0;
    }
}

void desenhar_aviso_sanidade(Sanidade *san, ALLEGRO_FONT *fonte, double t)
{
    if (zumbis_no_ciclo(san) < AVISO_ZUMBIS) return;
    if (san->game_over) return;

    double sem_matar = t - san->ultimo_abate;
    if (sem_matar >= TEMPO_RECUPERACAO_TOTAL) return;
    if ((int)(t / 0.6) % 2 == 0) return;

    float tx = LARGURA - 160.0f;
    float ty = ALTURA  - 110.0f;

    al_draw_text(fonte, al_map_rgba(0, 0, 0, 200), tx + 2, ty + 2, ALLEGRO_ALIGN_CENTER, "INSANIDADE!");
    al_draw_text(fonte, al_map_rgba(0, 0, 0, 200), tx + 2, ty + 38, ALLEGRO_ALIGN_CENTER, "Pare de matar!");
    al_draw_text(fonte, al_map_rgb(255, 60, 60),    tx, ty,      ALLEGRO_ALIGN_CENTER, "INSANIDADE!");
    al_draw_text(fonte, al_map_rgb(255, 180, 180),  tx, ty + 38, ALLEGRO_ALIGN_CENTER, "Pare de matar!");
}

void desenhar_mensagem_central_insanidade(Sanidade *san, ALLEGRO_FONT *fonte,
                                          ALLEGRO_FONT *fonte_hud, double t)
{
    if (zumbis_no_ciclo(san) < AVISO_ZUMBIS) return;
    if (san->game_over) return;

    double sem_matar = t - san->ultimo_abate;
    if (sem_matar >= TEMPO_RECUPERACAO_TOTAL) return;

    float frac_overlay;
    if (sem_matar < TEMPO_INICIO_RECUPERACAO)
    {
        frac_overlay = 1.0f;
    }
    else
    {
        float t_fade   = (float)(sem_matar - TEMPO_INICIO_RECUPERACAO);
        float dur_fade = (float)(TEMPO_RECUPERACAO_TOTAL - TEMPO_INICIO_RECUPERACAO);
        frac_overlay = 1.0f - (t_fade / dur_fade);
        if (frac_overlay < 0.0f) frac_overlay = 0.0f;
    }
    if (frac_overlay < 0.05f) return;

    float pulso    = 0.7f + 0.3f * sinf((float)t * 3.0f);
    unsigned char alpha_txt = (unsigned char)(255.0f * frac_overlay * pulso);

    float cx = LARGURA / 2.0f;
    float cy = ALTURA  / 2.0f - 40.0f;

    float box_w = 780.0f, box_h = 110.0f;
    al_draw_filled_rounded_rectangle(
        cx - box_w / 2.0f, cy - 20.0f,
        cx + box_w / 2.0f, cy + box_h,
        12, 12, al_map_rgba(0, 0, 0, (unsigned char)(160.0f * frac_overlay)));

    al_draw_text(fonte,
        al_map_rgba(255, 40, 40, alpha_txt),
        cx, cy, ALLEGRO_ALIGN_CENTER, "VOCE ESTA INSANO!");

    al_draw_text(fonte_hud,
        al_map_rgba(255, 200, 200, alpha_txt),
        cx, cy + 52.0f, ALLEGRO_ALIGN_CENTER,
        "Pare de eliminar inimigos ou perca sua mente!");
}

void desenhar_hud_tempo(ALLEGRO_FONT *f, const char *txt)
{
    al_draw_filled_rounded_rectangle(14, 14, 314, 72, 8, 8, al_map_rgba(0, 0, 0, 120));
    al_draw_filled_rounded_rectangle(10, 10, 310, 68, 8, 8, al_map_rgb(101, 60, 20));
    al_draw_rounded_rectangle(10, 10, 310, 68, 8, 8, al_map_rgb(218, 165, 32), 3);
    al_draw_rounded_rectangle(14, 14, 306, 64, 6, 6, al_map_rgba(255, 210, 80, 80), 1);
    al_draw_text(f, al_map_rgba(0, 0, 0, 180), 22, 22, 0, txt);
    al_draw_text(f, al_map_rgb(255, 215, 0),   20, 20, 0, txt);
}

void desenhar_hud_horda(ALLEGRO_FONT *f, Horda *h)
{
    char buf[64];
    sprintf(buf, "Zumbis: %d / %d", h->total_mortos, TOTAL_ZUMBIS_FASE);
    float cx = LARGURA / 2.0f, by_ = 10, bh_ = 58, bw_ = 280;
    al_draw_filled_rounded_rectangle(cx - bw_/2 + 4, by_ + 4, cx + bw_/2 + 4, by_ + bh_ + 4, 8, 8, al_map_rgba(0,0,0,120));
    al_draw_filled_rounded_rectangle(cx - bw_/2, by_, cx + bw_/2, by_ + bh_, 8, 8, al_map_rgb(101,60,20));
    al_draw_rounded_rectangle(cx - bw_/2, by_, cx + bw_/2, by_ + bh_, 8, 8, al_map_rgb(218,165,32), 3);
    al_draw_text(f, al_map_rgba(0,0,0,180), cx+2, by_+12+2, ALLEGRO_ALIGN_CENTER, buf);
    al_draw_text(f, al_map_rgb(255,215,0),  cx,   by_+12,   ALLEGRO_ALIGN_CENTER, buf);
}

void desenhar_hud_ataque(ALLEGRO_BITMAP *at1, ALLEGRO_BITMAP *at2, ALLEGRO_BITMAP *at3,
                         ALLEGRO_BITMAP *hab1, ALLEGRO_BITMAP *hab2, ALLEGRO_BITMAP *hab3,
                         int selecao, ALLEGRO_FONT *fonte_hud)
{
    float bx = 40.0f, by = 90.0f, tam = 62.0f, pad = 12.0f;
    ALLEGRO_BITMAP *sprites[3] = {hab1, hab2, hab3};
    const char *nomes[3] = {"", "", ""};
    for (int i = 0; i < 3; i++)
    {
        float sx = bx + i * (tam + pad), sy = by;
        int sel = (i + 1 == selecao);
        al_draw_filled_rounded_rectangle(sx+3, sy+3, sx+tam+3, sy+tam+3, 6, 6, al_map_rgba(0,0,0,120));
        al_draw_filled_rounded_rectangle(sx, sy, sx+tam, sy+tam, 6, 6,
            sel ? al_map_rgb(218,165,32) : al_map_rgb(60,35,10));
        al_draw_rounded_rectangle(sx, sy, sx+tam, sy+tam, 6, 6, al_map_rgb(218,165,32), sel ? 3 : 1);
        float ip = sel ? 1.0f : 5.0f;
        al_draw_scaled_bitmap(sprites[i], 0, 0,
            al_get_bitmap_width(sprites[i]), al_get_bitmap_height(sprites[i]),
            sx+ip, sy+ip, tam-ip*3, tam-ip*3, 0);
        al_draw_text(fonte_hud,
            sel ? al_map_rgb(255,215,0) : al_map_rgb(180,180,180),
            sx + tam/2.0f, sy + tam + 4, ALLEGRO_ALIGN_CENTER, nomes[i]);
        if (sel)
        {
            al_draw_filled_circle(sx+tam-10, sy+10, 9, al_map_rgb(0,0,0));
            al_draw_circle(sx+tam-10, sy+10, 9, al_map_rgb(218,165,32), 1);
            al_draw_text(fonte_hud, al_map_rgb(255,235,0), sx+tam-10, sy+2, ALLEGRO_ALIGN_CENTER, "K");
        }
    }
    (void)at1; (void)at2; (void)at3;
}

void desenhar_roda_habilidade(ALLEGRO_BITMAP *at1, ALLEGRO_BITMAP *at2, ALLEGRO_BITMAP *at3,
                              ALLEGRO_BITMAP *hab1, ALLEGRO_BITMAP *hab2, ALLEGRO_BITMAP *hab3,
                              int selecao, ALLEGRO_FONT *fonte_hud)
{
    float cx = LARGURA/2.0f, cy = ALTURA/2.0f, r_ext = 175.0f, r_int = 58.0f;
    al_draw_filled_rectangle(0, 0, LARGURA, ALTURA, al_map_rgba(0,0,0,140));
    al_draw_text(fonte_hud, al_map_rgba(0,0,0,200), cx+2, cy-r_ext-52+2, ALLEGRO_ALIGN_CENTER, "Selecionar Ataque");
    al_draw_text(fonte_hud, al_map_rgb(255,215,0),   cx,   cy-r_ext-52,   ALLEGRO_ALIGN_CENTER, "Selecionar Ataque");
    const char *nomes[3] = {"", "", ""};
    ALLEGRO_BITMAP *sprites[3] = {hab1, hab2, hab3};
    float base = -(float)ALLEGRO_PI / 2.0f, step = 2.0f * (float)ALLEGRO_PI / 3.0f;
    for (int i = 0; i < 3; i++)
    {
        float a0 = base + i * step, amid = a0 + step / 2.0f;
        int sel = (i + 1 == selecao);
        ALLEGRO_COLOR cf = sel ? al_map_rgba(218,165,32,210) : al_map_rgba(60,35,10,185);
        al_draw_filled_pieslice(cx, cy, r_ext, a0, step, cf);
        al_draw_pieslice(cx, cy, r_ext, a0, step, al_map_rgb(218,165,32), sel ? 4 : 2);
        float dist = r_int + (r_ext - r_int) * 0.55f;
        float ix = cx + cosf(amid)*dist, iy = cy + sinf(amid)*dist;
        float itam = sel ? 150.0f : 130.0f;
        al_draw_scaled_bitmap(sprites[i], 0, 0,
            al_get_bitmap_width(sprites[i]), al_get_bitmap_height(sprites[i]),
            ix - itam/2.0f, iy - itam/2.0f, itam, itam, 0);
        float tx = cx + cosf(amid)*(r_ext+32.0f), ty = cy + sinf(amid)*(r_ext+32.0f);
        al_draw_text(fonte_hud,
            sel ? al_map_rgb(255,215,0) : al_map_rgb(200,200,200),
            tx, ty-14, ALLEGRO_ALIGN_CENTER, nomes[i]);
    }
    al_draw_filled_circle(cx, cy, r_int, al_map_rgb(28,14,4));
    al_draw_circle(cx, cy, r_int, al_map_rgb(218,165,32), 2);
    al_draw_text(fonte_hud, al_map_rgba(0,0,0,180), cx+2, cy+r_ext+20, ALLEGRO_ALIGN_CENTER,
        "Q / E para navegar   |   Solte K para confirmar");
    al_draw_text(fonte_hud, al_map_rgb(220,220,220), cx,   cy+r_ext+18, ALLEGRO_ALIGN_CENTER,
        "Q / E para navegar   |   Solte K para confirmar");
    (void)at1; (void)at2; (void)at3;
}

typedef struct
{
    ALLEGRO_BITMAP *walk, *run, *attack1, *attack2, *attack3, *bite, *hurt, *dead, *idle;
} ZumbiSprites;

typedef struct
{
    ALLEGRO_BITMAP *idle, *run, *jump, *attack1, *attack2, *attack3, *hurt, *dead;
} SamuraiSprites;

/* ================================================================== */
/*  POÇÃO                                                               */
/* ================================================================== */
void pocao_tentar_spawn(Pocao *p, VidaStatus *vidas)
{
    if (p->ativa) return;
    if (contar_vidas(vidas) != 1) return;

    p->x        = (float)(POCAO_SPAWN_X_MIN + rand() % (POCAO_SPAWN_X_MAX - POCAO_SPAWN_X_MIN));
    p->y        = (float)POCAO_SPAWN_Y;   /* começa acima da tela */
    p->vel_y    = 0.0f;
    p->no_chao  = 0;
    p->ativa    = 1;
    p->coletada = 0;
}

/* Verifica colisão vertical da poção com o mapa de colisão.
   Usa a hitbox da poção (POCAO_HITBOX_W x POCAO_HITBOX_H) centrada na sprite. */
static bool pocao_piso_abaixo(ALLEGRO_BITMAP *mapa, float x, float y)
{
    int foot = (int)(y + POCAO_ALTURA) + 1;
    int left  = (int)(x + (POCAO_LARGURA - POCAO_HITBOX_W) / 2.0f);
    int right = left + POCAO_HITBOX_W - 1;
    for (int px2 = left; px2 <= right; px2 += 4)
        if (pixel_solido(mapa, px2, foot)) return true;
    return false;
}

void pocao_atualizar(Pocao *p, Jogador *jog, VidaStatus *vidas)
{
    if (!p->ativa) return;

    /* gravidade ? só cai se não está no chão */
    if (!p->no_chao)
    {
        p->vel_y += POCAO_GRAVIDADE;
        if (p->vel_y > POCAO_MAX_QUEDA) p->vel_y = POCAO_MAX_QUEDA;

        float ny = p->y + p->vel_y;

        if (pocao_piso_abaixo(jog_mapa_ptr, p->x, ny))
        {
            /* desce pixel a pixel até encostar */
            while (!pocao_piso_abaixo(jog_mapa_ptr, p->x, p->y + 1.0f))
                p->y += 1.0f;
            p->vel_y   = 0.0f;
            p->no_chao = 1;
        }
        else
        {
            p->y = ny;
        }

        /* caiu fora da tela ? desativa */
        if (p->y > ALTURA + 100) { p->ativa = 0; return; }
    }

    /* colisão com o jogador */
    float jx = jog->mov.x;
    float jy = jog->mov.y;
    if (p->x < jx + HITBOX_W && p->x + POCAO_HITBOX_W > jx &&
        p->y < jy + HITBOX_H && p->y + POCAO_HITBOX_H > jy)
    {
        restaurar_vidas(vidas, POCAO_RECUPERA_VIDAS);
        p->ativa    = 0;
        p->coletada = 1;
    }
}

void pocao_desenhar(Pocao *p, ALLEGRO_BITMAP *spr)
{
    if (!p->ativa || !spr) return;

    /* brilho pulsante simples baseado no tempo */
    double t = al_get_time();
    float brilho = 0.5f + 0.5f * sinf((float)t * 4.0f);
    unsigned char ga = (unsigned char)(60.0f + 80.0f * brilho);
    al_draw_filled_circle(p->x + POCAO_LARGURA / 2.0f,
                          p->y + POCAO_ALTURA  / 2.0f,
                          POCAO_LARGURA * 0.7f,
                          al_map_rgba(100, 220, 100, ga));

    al_draw_scaled_bitmap(spr, 0, 0,
        al_get_bitmap_width(spr), al_get_bitmap_height(spr),
        p->x, p->y, POCAO_LARGURA, POCAO_ALTURA, 0);
}

/* ================================================================== */
/*  HORDA                                                               */
/* ================================================================== */
void horda_init(Horda *h)
{
    for (int i = 0; i < MAX_ZUMBIS_TELA; i++)
        h->pool[i].vivo = 0;
    h->total_spawned  = 0;
    h->total_mortos   = 0;
    h->timer_onda     = 180;
    h->fase_concluida = 0;
    h->top_spawned    = 0;
    h->top_ativo      = 0;
    h->spawns_esq     = 0;
    h->spawns_dir     = 0;
}

static int zumbis_vivos_na_tela(Horda *h)
{
    int c = 0;
    for (int i = 0; i < MAX_ZUMBIS_TELA; i++)
        if (h->pool[i].vivo) c++;
    return c;
}

static int slot_livre(Horda *h)
{
    for (int i = 0; i < MAX_ZUMBIS_TELA; i++)
        if (!h->pool[i].vivo) return i;
    return -1;
}

static void spawnar_zumbi(Horda *h, int s, float sx_, float sy_, int spawn_tipo)
{
    Inimigo *z = &h->pool[s];
    z->x           = sx_;
    z->y           = sy_;
    z->vel_y       = 0.0f;
    z->x_inicial   = sx_;
    z->y_inicial   = sy_;
    z->patrol_base = sx_;
    z->patrol_dir  = (rand() % 2 == 0) ? 1.0f : -1.0f;
    z->direcao     = ALLEGRO_FLIP_HORIZONTAL;
    z->vivo        = 1;
    z->estado      = ZUM_WALK;
    z->frame       = (float)(rand() % 8);
    z->tempo_hurt  = 0.0;
    z->atacando_jogador = 0;
    z->tempo_ataque     = 0.0;
    z->spawn_tipo       = spawn_tipo;
    z->dano_aplicado    = 0;
    z->atingido_no_ataque = 0;

    int rapido = (rand() % 100) < 25;
    if (rapido)
    {
        z->tipo       = 1;
        z->velocidade = 1.8f + (float)(rand() % 50) / 100.0f;
        z->vida       = 5;
    }
    else
    {
        z->tipo       = 0;
        z->velocidade = 0.6f + (float)(rand() % 40) / 100.0f;
        z->vida       = 5;
    }
}

void horda_atualizar_spawn(Horda *h, float jogador_y)
{
    if (h->fase_concluida || h->total_spawned >= TOTAL_ZUMBIS_FASE)
        return;

    /* só começa quando jogador chega na área */
    if (jogador_y < SPAWN_MIN_JOGADOR_Y)
        return;

    int vivos = zumbis_vivos_na_tela(h);

    /* nova onda quando sobra apenas 2 vivos
       (ou seja, 3 já morreram) */
    if (vivos > 2)
        return;

    /* spawn mais rápido */
    if (--h->timer_onda <= 0)
    {
        int restam = TOTAL_ZUMBIS_FASE - h->total_spawned;

        /* máximo de 5 por onda */
        int para_spawn = (restam < 5)
            ? restam
            : 5;

        for (int n = 0; n < para_spawn; n++)
        {
            int s = slot_livre(h);
            if (s < 0) break;

            float sx_;
            float sy_;
            int spawn_tipo = 0;

            /* mistura esquerda e direita */
            int lado = rand() % 2;

            /* limita 25 por lado */
            if (h->spawns_esq >= 25)
                lado = 1;

            if (h->spawns_dir >= 25)
                lado = 0;

            if (lado == 0)
            {
                /* TOPO ESQUERDO */
                sx_ = SPAWN_TOP_LEFT_X +
                      (float)(rand() % 180);

                sy_ = SPAWN_TOP_LEFT_Y;

                h->spawns_esq++;
            }
            else
            {
                /* DIREITA */
                sx_ = SPAWN_DIR_X_MIN +
                      (float)(rand() %
                      (SPAWN_DIR_X_MAX - SPAWN_DIR_X_MIN));

                sy_ = SPAWN_DIR_Y;

                h->spawns_dir++;
            }

            spawnar_zumbi(h, s, sx_, sy_, spawn_tipo);

            h->total_spawned++;
        }

        /* delay menor entre ondas */
        h->timer_onda = 120;
    }
}

void horda_atualizar_fisica(Horda *h, ALLEGRO_BITMAP *mapa)
{
    for (int i = 0; i < MAX_ZUMBIS_TELA; i++)
    {
        Inimigo *z = &h->pool[i];
        if (!z->vivo || z->estado == ZUM_DEAD) continue;

        z->vel_y += GRAVIDADE_ZUMBI;
        if (z->vel_y > MAX_QUEDA_ZUMBI) z->vel_y = MAX_QUEDA_ZUMBI;

        float ny = z->y + z->vel_y;

        if (z->vel_y > 0.0f)
        {
            if (zumbi_colide_vertical(mapa, z->x, ny, z->vel_y))
            {
                while (!zumbi_colide_vertical(mapa, z->x, z->y + 1.0f, 1.0f))
                    z->y += 1.0f;
                z->vel_y = 0.0f;
            }
            else
            {
                z->y = ny;
            }
        }
        else if (z->vel_y < 0.0f)
        {
            if (zumbi_colide_vertical(mapa, z->x, ny, z->vel_y))
                z->vel_y = 0.0f;
            else
                z->y = ny;
        }

        if (z->y < 0.0f) z->y = 0.0f;
        if (z->y > ALTURA + 300) z->vivo = 0;
    }
}

void horda_atualizar_movimento(Horda *h, Jogador *j)
{
    double agora = al_get_time();
    for (int i = 0; i < MAX_ZUMBIS_TELA; i++)
    {
        Inimigo *z = &h->pool[i];
        if (!z->vivo) continue;

        if (z->estado == ZUM_DEAD)
        {
            z->frame += 0.08f;
            if (z->frame >= FRAMES_ZUMBI_DEAD) z->vivo = 0;
            continue;
        }

        if (z->estado == ZUM_HURT)
        {
            double elapsed = agora - z->tempo_hurt;
            z->frame = (float)(elapsed / KNOCKBACK_ZUMBI_DURACAO) * FRAMES_ZUMBI_HURT;
            if (elapsed >= KNOCKBACK_ZUMBI_DURACAO)
            {
                z->frame  = 0;
                z->estado = ZUM_WALK;
            }
            continue;
        }

        if (z->estado == ZUM_ATTACK || z->estado == ZUM_BITE)
        {
            int max_f    = (z->estado == ZUM_ATTACK) ? FRAMES_ZUMBI_ATTACK1 : FRAMES_ZUMBI_BITE;
            float vel_an = (z->tipo == 1) ? 0.15f : 0.10f;
            z->frame += vel_an;
            if (z->frame >= max_f)
            {
                z->frame  = 0;
                z->estado = ZUM_WALK;
                z->atacando_jogador = 0;
                z->dano_aplicado    = 0;
            }
            continue;
        }

        float jx = j->mov.x;
        float jy = j->mov.y;

        int jogador_morto  = (j->estado == SAM_DEAD);
        int zumbi_abaixo   = (z->y > jy + ZUMBI_ABAIXO_MARGEM);
        int jogador_alto   = (jy < NIVEL_ALTO_Y);

        if (!jogador_morto && zumbi_abaixo)
        {
            z->estado = ZUM_IDLE;
            z->frame += 0.05f;
            if (z->frame >= FRAMES_ZUMBI_IDLE) z->frame = 0.0f;
            continue;
        }

        if (jogador_morto || jogador_alto)
        {
            float dist_base = z->x - z->patrol_base;
            if (fabsf(dist_base) > PATRULHA_DIST)
            {
                if (dist_base > 0)
                {
                    z->x -= PATRULHA_VEL;
                    z->direcao = ALLEGRO_FLIP_HORIZONTAL;
                }
                else
                {
                    z->x += PATRULHA_VEL;
                    z->direcao = 0;
                }
                z->estado = ZUM_WALK;
                z->frame += 0.05f;
                if (z->frame >= FRAMES_ZUMBI_WALK) z->frame = 0.0f;
            }
            else
            {
                z->estado = ZUM_IDLE;
                z->x += PATRULHA_VEL * z->patrol_dir;
                if (z->x > z->patrol_base + PATRULHA_DIST)
                {
                    z->patrol_dir = -1.0f;
                    z->direcao    = ALLEGRO_FLIP_HORIZONTAL;
                }
                else if (z->x < z->patrol_base - PATRULHA_DIST)
                {
                    z->patrol_dir = 1.0f;
                    z->direcao    = 0;
                }
                z->frame += 0.04f;
                if (z->frame >= FRAMES_ZUMBI_IDLE) z->frame = 0.0f;
            }
            continue;
        }

        float dx     = fabsf(jx - z->x);
        float dy     = fabsf(jy - z->y);
        float alcance = 55.0f;

        if (dx < alcance && dy < 60.0f)
        {
            if (!z->atacando_jogador || (agora - z->tempo_ataque > 1.4))
            {
                z->atacando_jogador = 1;
                z->tempo_ataque     = agora;
                z->frame            = 0;
                z->dano_aplicado    = 0;
                z->estado = (z->tipo == 1) ? ZUM_BITE : ZUM_ATTACK;
            }
        }
        else
        {
            if (jx < z->x)
            {
                z->x -= z->velocidade;
                z->direcao = ALLEGRO_FLIP_HORIZONTAL;
            }
            else
            {
                z->x += z->velocidade;
                z->direcao = 0;
            }
            z->estado = (z->tipo == 1) ? ZUM_RUN : ZUM_WALK;
            float va  = (z->tipo == 1) ? 0.14f : 0.07f;
            z->frame += va;
            int mf    = (z->tipo == 1) ? FRAMES_ZUMBI_RUN : FRAMES_ZUMBI_WALK;
            if (z->frame >= mf) z->frame = 0.0f;
        }
    }
}

void horda_verificar_ataque(Horda *h, Jogador *jog, Sanidade *san)
{
    if (!jog->atacando) return;

    int frame_atual = (int)jog->frame_ataque;
    if (frame_atual < SAM_ATK_FRAME_INICIO || frame_atual >= SAM_ATK_FRAME_FIM)
        return;

    float atk_h = SAM_ATK_H;
    float atk_w = SAM_ATK_W;
    float atk_y = jog->mov.y + (HITBOX_H - atk_h) / 2.0f;
    float atk_x;

    if (jog->direcao == 0)
        atk_x = jog->mov.x + HITBOX_W + SAM_ATK_REACH;
    else
        atk_x = jog->mov.x - SAM_ATK_W - SAM_ATK_REACH;

    for (int i = 0; i < MAX_ZUMBIS_TELA; i++)
    {
        Inimigo *z = &h->pool[i];
        if (!z->vivo)                continue;
        if (z->estado == ZUM_DEAD)   continue;
        if (z->atingido_no_ataque)   continue;

        float hzx = z->x + ZUMBI_HBX_OFFSET_X;
        float hzy = z->y + ZUMBI_HBX_OFFSET_Y;
        float hzw = ZUMBI_HBX_W;
        float hzh = ZUMBI_HBX_H;

        if (atk_x < hzx + hzw &&
            atk_x + atk_w > hzx &&
            atk_y < hzy + hzh &&
            atk_y + atk_h > hzy)
        {
            int dano = (jog->tipo_ataque == 3) ? 2 : 1;
            z->vida -= dano;
            if (z->vida < 0) z->vida = 0;

            z->atingido_no_ataque = 1;

            float kb_dir = (jog->direcao == 0) ? 1.0f : -1.0f;
            z->x += kb_dir * KNOCKBACK_ZUMBI_X;

            if (z->vida <= 0)
            {
                z->estado = ZUM_DEAD;
                z->frame  = 0;

                h->total_mortos++;
                san->zumbis_mortos++;
                san->ultimo_abate = al_get_time();
                san->regenerando  = 0;

                atualizar_sanidade(san);

                if (h->total_mortos >= TOTAL_ZUMBIS_FASE)
                    h->fase_concluida = 1;
            }
            else
            {
                if (z->estado != ZUM_HURT)
                {
                    z->estado     = ZUM_HURT;
                    z->frame      = 0;
                    z->tempo_hurt = al_get_time();
                }
            }
        }
    }
}

void horda_verificar_dano_jogador(Horda *h, Jogador *jog, VidaStatus *vidas)
{
    double agora = al_get_time();
    if (agora - jog->ultimo_dano <= TEMPO_INVUL) return;

    for (int i = 0; i < MAX_ZUMBIS_TELA; i++)
    {
        Inimigo *z = &h->pool[i];
        if (!z->vivo || z->estado == ZUM_DEAD)        continue;
        if (z->estado != ZUM_ATTACK && z->estado != ZUM_BITE) continue;
        if (z->dano_aplicado) continue;

        int frame_ativo  = (int)z->frame;
        int inicio_dano  = (z->estado == ZUM_BITE) ? 4 : 2;
        if (frame_ativo < inicio_dano) continue;

        float body_x = z->x + ZUMBI_HBX_OFFSET_X;
        float body_y = z->y + ZUMBI_HBX_OFFSET_Y;
        float atk_y  = body_y;
        float atk_x;

        if (z->direcao == 0)
            atk_x = body_x + ZUMBI_HBX_W + ZUM_ATK_REACH;
        else
            atk_x = body_x - ZUM_ATK_W - ZUM_ATK_REACH;

        float jx = jog->mov.x, jy = jog->mov.y;
        float jw = HITBOX_W,   jh = HITBOX_H;

        if (atk_x < jx + jw && atk_x + ZUM_ATK_W > jx &&
            atk_y < jy + jh && atk_y + ZUM_ATK_H > jy)
        {
            perder_vida(vidas);
            jog->ultimo_dano = agora;

            float kb_dir = (z->direcao == 0) ? -1.0f : 1.0f;
            jog->mov.x    += kb_dir * KNOCKBACK_SAMURAI_X;
            jog->mov.vel_y = -6.0f;
            jog->direcao   = (z->direcao == 0) ? ALLEGRO_FLIP_HORIZONTAL : 0;

            z->dano_aplicado = 1;

            if (jog->estado != SAM_DEAD)
            {
                jog->estado     = SAM_HURT;
                jog->frame_hurt = 0;
                jog->hurt_inicio = agora;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Desenho dos zumbis  (sem hitbox visual)                            */
/* ------------------------------------------------------------------ */
void horda_desenhar(Horda *h, ZumbiSprites *spr)
{
    for (int i = 0; i < MAX_ZUMBIS_TELA; i++)
    {
        Inimigo *z = &h->pool[i];
        if (!z->vivo) continue;

        float dx = z->x;
        float dy = z->y;

        ALLEGRO_BITMAP *sheet = NULL;
        int max_f = FRAMES_ZUMBI_WALK;

        switch (z->estado)
        {
        case ZUM_WALK:   sheet = spr->walk;    max_f = FRAMES_ZUMBI_WALK;    break;
        case ZUM_RUN:    sheet = spr->run;     max_f = FRAMES_ZUMBI_RUN;     break;
        case ZUM_ATTACK: sheet = spr->attack1; max_f = FRAMES_ZUMBI_ATTACK1; break;
        case ZUM_BITE:   sheet = spr->bite;    max_f = FRAMES_ZUMBI_BITE;    break;
        case ZUM_HURT:   sheet = spr->hurt;    max_f = FRAMES_ZUMBI_HURT;    break;
        case ZUM_DEAD:   sheet = spr->dead;    max_f = FRAMES_ZUMBI_DEAD;    break;
        default:         sheet = spr->idle;    max_f = FRAMES_ZUMBI_IDLE;    break;
        }

        if (!sheet) sheet = spr->walk;

        int fz = (int)z->frame % max_f;

        ALLEGRO_COLOR tint = (z->tipo == 1)
            ? al_map_rgb(255, 120, 120)
            : al_map_rgb(255, 255, 255);

        al_draw_tinted_scaled_bitmap(sheet, tint,
            fz * ZUMBI_SRC_W, 0, ZUMBI_SRC_W, ZUMBI_SRC_H,
            dx, dy, ZUMBI_DRAW_W, ZUMBI_DRAW_H,
            z->direcao);

        /* barra de vida */
        if (z->estado != ZUM_DEAD)
        {
            float bw2 = 58.0f;
            float bx2 = dx + ZUMBI_DRAW_W / 2.0f - bw2 / 2.0f;
            float by2 = dy + 4.0f;
            float vida_pct = (float)z->vida / 5.0f;

            al_draw_filled_rectangle(bx2, by2, bx2 + bw2, by2 + 6, al_map_rgb(80, 0, 0));
            al_draw_filled_rectangle(bx2, by2, bx2 + bw2 * vida_pct, by2 + 6, al_map_rgb(255, 0, 0));
        }
        /* hitbox visual REMOVIDA */
    }
}

/* ================================================================== */
/*  RANKING / GAME OVER                                                 */
/* ================================================================== */
void tela_ranking(ALLEGRO_EVENT_QUEUE *queue, ALLEGRO_TIMER *timer,
                  ALLEGRO_FONT *fonte, ALLEGRO_FONT *fonte_hud,
                  Temporizador *tempo, double tempo_final)
{
    int pos = busca_binaria(tempo->ranking, tempo->quantidade_scores, (float)tempo_final);
    if (tempo->quantidade_scores < 10)
    {
        for (int i = tempo->quantidade_scores; i > pos; i--)
            tempo->ranking[i] = tempo->ranking[i - 1];
        tempo->ranking[pos] = (float)tempo_final;
        tempo->quantidade_scores++;
    }
    else if (pos < 10)
    {
        for (int i = 9; i > pos; i--)
            tempo->ranking[i] = tempo->ranking[i - 1];
        tempo->ranking[pos] = (float)tempo_final;
    }
    salvar_ranking(tempo);

    ALLEGRO_EVENT ev;
    while (1)
    {
        al_wait_for_event(queue, &ev);
        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) return;
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN)
            if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER ||
                ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                return;
        if (ev.type != ALLEGRO_EVENT_TIMER) continue;

        al_clear_to_color(al_map_rgb(15, 10, 5));
        float pw = 700, ph = 680,
              px_ = LARGURA/2.0f - pw/2.0f, py_ = ALTURA/2.0f - ph/2.0f;
        al_draw_filled_rounded_rectangle(px_+6, py_+6, px_+pw+6, py_+ph+6, 14, 14, al_map_rgba(0,0,0,160));
        al_draw_filled_rounded_rectangle(px_, py_, px_+pw, py_+ph, 14, 14, al_map_rgb(50,28,8));
        al_draw_rounded_rectangle(px_, py_, px_+pw, py_+ph, 14, 14, al_map_rgb(218,165,32), 4);
        al_draw_text(fonte, al_map_rgb(255,215,0), LARGURA/2.0f, py_+20, ALLEGRO_ALIGN_CENTER, "FASE CONCLUIDA!");
        char buf[80];
        sprintf(buf, "Seu tempo: %.2f s", tempo_final);
        al_draw_text(fonte_hud, al_map_rgb(255,255,180), LARGURA/2.0f, py_+80, ALLEGRO_ALIGN_CENTER, buf);
        al_draw_text(fonte_hud, al_map_rgb(218,165,32),  LARGURA/2.0f, py_+148, ALLEGRO_ALIGN_CENTER, "TOP 10");
        for (int i = 0; i < tempo->quantidade_scores; i++)
        {
            char linha[64];
            sprintf(linha, "%d.   %.2f s", i + 1, tempo->ranking[i]);
            ALLEGRO_COLOR cor = (fabsf(tempo->ranking[i] - (float)tempo_final) < 0.005f)
                ? al_map_rgb(255,215,0) : al_map_rgb(220,220,220);
            al_draw_text(fonte_hud, cor, LARGURA/2.0f, py_+195+i*44, ALLEGRO_ALIGN_CENTER, linha);
        }
        al_draw_text(fonte_hud, al_map_rgb(160,160,160),
            LARGURA/2.0f, py_+ph-52, ALLEGRO_ALIGN_CENTER,
            "Pressione ENTER para sair");
        al_flip_display();
    }
}

int tela_game_over(ALLEGRO_EVENT_QUEUE *queue, ALLEGRO_TIMER *timer,
                   ALLEGRO_FONT *fonte, ALLEGRO_FONT *fonte_hud, const char *motivo)
{
    al_flush_event_queue(queue);
    ALLEGRO_EVENT ev;
    int opcao = 0;
    while (1)
    {
        al_wait_for_event(queue, &ev);
        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) return 0;
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN)
        {
            if (ev.keyboard.keycode == ALLEGRO_KEY_LEFT || ev.keyboard.keycode == ALLEGRO_KEY_A)
                opcao = 0;
            if (ev.keyboard.keycode == ALLEGRO_KEY_RIGHT || ev.keyboard.keycode == ALLEGRO_KEY_D)
                opcao = 1;
            if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER || ev.keyboard.keycode == ALLEGRO_KEY_SPACE)
                return (opcao == 0) ? 1 : 0;
            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                return 0;
        }
        if (ev.type != ALLEGRO_EVENT_TIMER) continue;

        al_clear_to_color(al_map_rgb(10, 5, 5));
        float pw = 740, ph = 340,
              px_ = LARGURA/2.0f - pw/2.0f, py_ = ALTURA/2.0f - ph/2.0f;
        al_draw_filled_rounded_rectangle(px_+6, py_+6, px_+pw+6, py_+ph+6, 14, 14, al_map_rgba(0,0,0,180));
        al_draw_filled_rounded_rectangle(px_, py_, px_+pw, py_+ph, 14, 14, al_map_rgb(60,10,10));
        al_draw_rounded_rectangle(px_, py_, px_+pw, py_+ph, 14, 14, al_map_rgb(200,30,30), 4);
        al_draw_text(fonte, al_map_rgb(255,60,60), LARGURA/2.0f, py_+28, ALLEGRO_ALIGN_CENTER, "GAME OVER");
        al_draw_line(px_+40, py_+88, px_+pw-40, py_+88, al_map_rgb(200,30,30), 2);
        al_draw_text(fonte_hud, al_map_rgb(255,200,200), LARGURA/2.0f, py_+102, ALLEGRO_ALIGN_CENTER, motivo);
        al_draw_line(px_+40, py_+158, px_+pw-40, py_+158, al_map_rgb(120,20,20), 1);
        const char *tr = (opcao == 0) ? "> REINICIAR <" : "REINICIAR";
        const char *ts = (opcao == 1) ? "> SAIR <"      : "SAIR";
        al_draw_text(fonte, (opcao==0)?al_map_rgb(255,215,0):al_map_rgb(180,180,180),
            LARGURA/2.0f-160, py_+186, ALLEGRO_ALIGN_CENTER, tr);
        al_draw_text(fonte, (opcao==1)?al_map_rgb(255,215,0):al_map_rgb(180,180,180),
            LARGURA/2.0f+160, py_+186, ALLEGRO_ALIGN_CENTER, ts);
        al_draw_text(fonte_hud, al_map_rgb(130,130,130), LARGURA/2.0f, py_+ph-46,
            ALLEGRO_ALIGN_CENTER,
            "A/D ou Setas para navegar  |  ENTER para confirmar");
        al_flip_display();
    }
}

/* ================================================================== */
void perder_vida(VidaStatus *vidas)
{
    for (int i = MAX_VIDAS - 1; i >= 0; i--)
        if (vidas[i].ativa)
        {
            vidas[i].ativa = 0;
            strcpy(vidas[i].status, "Perdida");
            break;
        }
}

void ordenar_ranking(float r[], int n)
{
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - i - 1; j++)
            if (r[j] > r[j + 1])
            {
                float t = r[j]; r[j] = r[j + 1]; r[j + 1] = t;
            }
}

int busca_binaria(float r[], int n, float v)
{
    int lo = 0, hi = n - 1, mid;
    while (lo <= hi)
    {
        mid = (lo + hi) / 2;
        if (v < r[mid]) hi = mid - 1;
        else            lo = mid + 1;
    }
    return lo;
}

void carregar_ranking(Temporizador *t)
{
    FILE *f = fopen("ranking.txt", "r");
    if (!f) return;
    while (fscanf(f, "%f", &t->ranking[t->quantidade_scores]) == 1)
    {
        t->quantidade_scores++;
        if (t->quantidade_scores >= 10) break;
    }
    fclose(f);
    ordenar_ranking(t->ranking, t->quantidade_scores);
}

void salvar_ranking(Temporizador *t)
{
    FILE *f = fopen("ranking.txt", "w");
    if (!f) return;
    for (int i = 0; i < t->quantidade_scores; i++)
        fprintf(f, "%.2f\n", t->ranking[i]);
    fclose(f);
}

void gerar_mapa_colisao(ALLEGRO_BITMAP *mapa)
{
    al_lock_bitmap(mapa, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);
    for (int y = 0; y < al_get_bitmap_height(mapa); y++)
        for (int x = 0; x < al_get_bitmap_width(mapa); x++)
        {
            ALLEGRO_COLOR c = al_get_pixel(mapa, x, y);
            unsigned char r, g, b;
            al_unmap_rgb(c, &r, &g, &b);
            colisao[y][x] = ((r + g + b) / 3 < 120);
        }
    al_unlock_bitmap(mapa);
}

/* ================================================================== */
/*  Desenho do samurai  (sem hitbox visual)                            */
/* ================================================================== */
void desenhar_samurai(Jogador *jog, SamuraiSprites *spr)
{
    float draw_x = jog->mov.x - HITBOX_OFFSET_X;
    float draw_y = jog->mov.y - HITBOX_OFFSET_Y;

    switch (jog->estado)
    {
    case SAM_HURT:
    {
        double elapsed = al_get_time() - jog->hurt_inicio;
        int mf = FRAMES_HURT;
        int f  = (int)((elapsed / KNOCKBACK_SAMURAI_DURACAO) * mf);
        if (f >= mf) f = mf - 1;
        al_draw_scaled_bitmap(spr->hurt,
            128 * f, 0, 128, 128,
            draw_x, draw_y, DRAW_W, DRAW_H,
            jog->direcao);
        if (elapsed >= KNOCKBACK_SAMURAI_DURACAO)
        {
            jog->estado     = SAM_IDLE;
            jog->frame_hurt = 0;
        }
        break;
    }
    case SAM_DEAD:
    {
        int mf = FRAMES_DEAD;
        int f  = (int)jog->frame_dead;
        if (f >= mf) { f = mf - 1; jog->morte_animando = 0; }
        else           jog->morte_animando = 1;
        al_draw_scaled_bitmap(spr->dead,
            128 * f, 0, 128, 128,
            draw_x, draw_y, DRAW_W, DRAW_H,
            jog->direcao);
        jog->frame_dead += 0.06f;
        break;
    }
    case SAM_ATTACK:
    {
        int mf = (jog->tipo_ataque == 1) ? 6 :
                 (jog->tipo_ataque == 2) ? 4 : 3;
        int f  = (int)jog->frame_ataque;
        if (f >= mf) f = mf - 1;

        ALLEGRO_BITMAP *atk =
            (jog->tipo_ataque == 1) ? spr->attack1 :
            (jog->tipo_ataque == 2) ? spr->attack2 : spr->attack3;

        al_draw_scaled_bitmap(atk,
            128 * f, 0, 128, 128,
            draw_x - 10, draw_y - 1, ATTACK_DRAW_W, ATTACK_DRAW_H,
            jog->direcao);

        jog->frame_ataque += 0.30f;
        if (jog->frame_ataque >= mf)
        {
            jog->atacando     = 0;
            jog->frame_ataque = 0;
            jog->estado       = SAM_IDLE;
        }
        break;
    }
    case SAM_JUMP:
    {
        int f = (int)jog->frame % FRAMES_JUMP;
        al_draw_scaled_bitmap(spr->jump,
            128 * f, 0, 128, 128,
            draw_x, draw_y, DRAW_W, DRAW_H,
            jog->direcao);
        break;
    }
    case SAM_RUN:
    {
        int f = (int)jog->frame % FRAMES_RUN;
        al_draw_scaled_bitmap(spr->run,
            128 * f, 0, 128, 128,
            draw_x, draw_y, DRAW_W, DRAW_H,
            jog->direcao);
        break;
    }
    default:
    {
        int f = (int)jog->frame % FRAMES_IDLE;
        al_draw_scaled_bitmap(spr->idle,
            128 * f, 0, 128, 128,
            draw_x, draw_y, DRAW_W, DRAW_H,
            jog->direcao);
        break;
    }
    }
    /* hitbox visual REMOVIDA */
}

void atualizar_estado_samurai(Jogador *jog)
{
    if (jog->estado == SAM_DEAD)  return;
    if (jog->estado == SAM_HURT)  return;
    if (jog->atacando)            { jog->estado = SAM_ATTACK; return; }
    if (!jog->no_chao)            jog->estado = SAM_JUMP;
    else if (jog->movendo)        jog->estado = SAM_RUN;
    else                          jog->estado = SAM_IDLE;
}

/* ================================================================== */
/*  MAIN                                                                */
/* ================================================================== */
int main(void)
{
    srand((unsigned)time(NULL));

    if (!al_init())              { puts("Erro al_init");      return 1; }
    if (!al_init_primitives_addon()) { puts("Erro primitives"); return 1; }
    if (!al_install_keyboard())  { puts("Erro keyboard");     return 1; }
    if (!al_init_image_addon())  { puts("Erro image");        return 1; }
    al_init_font_addon();
    al_init_ttf_addon();

    ALLEGRO_DISPLAY     *display = al_create_display(LARGURA, ALTURA);
    ALLEGRO_EVENT_QUEUE *queue   = al_create_event_queue();
    ALLEGRO_TIMER       *timer   = al_create_timer(1.0 / FPS);
    if (!display || !queue || !timer) { puts("Erro init"); return 1; }

    ALLEGRO_FONT *fonte     = al_load_ttf_font("assets/PublicPixel.ttf", 30, 0);
    ALLEGRO_FONT *fonte_hud = al_load_ttf_font("assets/PublicPixel.ttf", 18, 0);
    if (!fonte)     fonte     = al_create_builtin_font();
    if (!fonte_hud) fonte_hud = al_create_builtin_font();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());

    ALLEGRO_BITMAP *bg_menu = al_load_bitmap("assets/cenarios/inicio.png");
    if (!bg_menu) { puts("Erro bg_menu"); return 1; }

    al_start_timer(timer);

    OpcaoMenu escolha = executar_menu(queue, timer, bg_menu, fonte);
    if (escolha == MENU_SAIR)
    {
        al_destroy_bitmap(bg_menu);
        al_destroy_font(fonte_hud);
        al_destroy_font(fonte);
        al_destroy_timer(timer);
        al_destroy_event_queue(queue);
        al_destroy_display(display);
        return 0;
    }
    al_flush_event_queue(queue);

    ALLEGRO_BITMAP *bg   = al_load_bitmap("assets/cenarios/background2.png");
    ALLEGRO_BITMAP *mapa = al_load_bitmap("assets/cenarios/colisao2.png");

    SamuraiSprites sam_spr;
    sam_spr.idle    = al_load_bitmap("assets/sprites/Samurai/Idle.png");
    sam_spr.run     = al_load_bitmap("assets/sprites/Samurai/Run.png");
    sam_spr.jump    = al_load_bitmap("assets/sprites/Samurai/Jump.png");
    sam_spr.attack1 = al_load_bitmap("assets/sprites/Samurai/Attack_1.png");
    sam_spr.attack2 = al_load_bitmap("assets/sprites/Samurai/Attack_2.png");
    sam_spr.attack3 = al_load_bitmap("assets/sprites/Samurai/Attack_3.png");
    sam_spr.hurt    = al_load_bitmap("assets/sprites/Samurai/Hurt.png");
    sam_spr.dead    = al_load_bitmap("assets/sprites/Samurai/Dead.png");

    ZumbiSprites zum_spr;
    zum_spr.walk    = al_load_bitmap("assets/sprites/Zombies/Walk.png");
    zum_spr.run     = al_load_bitmap("assets/sprites/Zombies/Run.png");
    zum_spr.attack1 = al_load_bitmap("assets/sprites/Zombies/Attack_1.png");
    zum_spr.attack2 = al_load_bitmap("assets/sprites/Zombies/Attack_2.png");
    zum_spr.attack3 = al_load_bitmap("assets/sprites/Zombies/Attack_3.png");
    zum_spr.bite    = al_load_bitmap("assets/sprites/Zombies/Bite.png");
    zum_spr.hurt    = al_load_bitmap("assets/sprites/Zombies/Hurt.png");
    zum_spr.dead    = al_load_bitmap("assets/sprites/Zombies/Dead.png");
    zum_spr.idle    = al_load_bitmap("assets/sprites/Zombies/Idle.png");

    ALLEGRO_BITMAP *coracao      = al_load_bitmap("assets/itens/vida.png");
    ALLEGRO_BITMAP *spr_estamina = al_load_bitmap("assets/itens/estamina.png");
    ALLEGRO_BITMAP *spr_sanidade = al_load_bitmap("assets/itens/sanidade.png");
    ALLEGRO_BITMAP *habilidade1  = al_load_bitmap("assets/itens/Habilidade1.png");
    ALLEGRO_BITMAP *habilidade2  = al_load_bitmap("assets/itens/Habilidade2.png");
    ALLEGRO_BITMAP *habilidade3  = al_load_bitmap("assets/itens/Habilidade3.png");
    ALLEGRO_BITMAP *spr_pocao    = al_load_bitmap("assets/itens/pocao.png");

    if (!bg || !mapa ||
        !sam_spr.idle || !sam_spr.run || !sam_spr.jump ||
        !sam_spr.attack1 || !sam_spr.attack2 || !sam_spr.attack3 ||
        !sam_spr.hurt || !sam_spr.dead ||
        !zum_spr.walk || !zum_spr.run  ||
        !zum_spr.attack1 || !zum_spr.attack2 || !zum_spr.attack3 ||
        !zum_spr.bite || !zum_spr.hurt || !zum_spr.dead || !zum_spr.idle ||
        !coracao || !spr_estamina || !habilidade1 || !habilidade2 || !habilidade3)
    {
        puts("Erro ao carregar bitmaps");
        return 1;
    }
    if (!spr_pocao) puts("Aviso: assets/itens/pocao.png nao encontrado");

    gerar_mapa_colisao(mapa);
    jog_mapa_ptr = mapa; /* expõe para pocao_atualizar */

    /* ---- estado inicial ---- */
    Jogador jogador  = {0};
    jogador.mov.x    = 60;
    jogador.mov.y    = 253;
    jogador.tipo_ataque  = 1;
    jogador.estamina     = MAX_ESTAMINA;
    jogador.estado       = SAM_IDLE;
    jogador.hurt_inicio  = -999.0;
    jogador.ultimo_ataque = -999.0;
    jogador.dash_ativo   = 0;
    jogador.dash_dist    = 0.0f;

    Sanidade sanidade = {MAX_SANIDADE, 0, 0, 0, 0.0, 0};

    Horda horda;
    horda_init(&horda);

    Pocao  pocao = {0};
    pocao.ativa  = 0;
    double pocao_ultimo_check = 0.0;

    Temporizador tempo = {0};
    tempo.inicio = al_get_time();
    tempo.ativo  = 1;
    carregar_ranking(&tempo);

    VidaStatus *vetor_vidas = malloc(MAX_VIDAS * sizeof(VidaStatus));
    for (int i = 0; i < MAX_VIDAS; i++)
    {
        vetor_vidas[i].ativa = 1;
        strcpy(vetor_vidas[i].status, "Inteira");
    }

    int lm = ALTURA  / 100 + 1;
    int cm = LARGURA / 100 + 1;
    int **matriz_dec = criar_matriz_decorativa(lm, cm);

    int pausado     = 0;
    double pausa_inicio = 0;
    int roda_aberta = 0, roda_selecao = 1;
    sanidade.ultimo_abate = al_get_time();

    /* teclas "anteriores" ? declaradas fora do loop */
    int esc_ant = 0;
    int k_ant   = 0;
    int j_ant   = 0;
    int q_ant   = 0;
    int e_ant   = 0;

    int rodando = 1;
    ALLEGRO_EVENT ev;
    ALLEGRO_KEYBOARD_STATE state;

    while (rodando)
    {
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
        {
            rodando = 0;
            break;
        }
        if (ev.type != ALLEGRO_EVENT_TIMER)
            continue;

        /* ?? leitura do teclado ?? */
        al_get_keyboard_state(&state);

        /* ESC ? pausa */
        int esc_now = al_key_down(&state, ALLEGRO_KEY_ESCAPE);
        if (esc_now && !esc_ant &&
            !sanidade.game_over && !horda.fase_concluida &&
            jogador.estado != SAM_DEAD)
        {
            pausado = !pausado;
            if (pausado)
            {
                pausa_inicio = al_get_time();
            }
            else
            {
                double dt = al_get_time() - pausa_inicio;
                tempo.inicio          += dt;
                jogador.ultimo_dano   += dt;
                sanidade.ultimo_abate += dt;
            }
        }
        esc_ant = esc_now;

        int morte_pronta  = (jogador.estado == SAM_DEAD && !jogador.morte_animando);
        int em_game_over  = sanidade.game_over || morte_pronta;

        if (!pausado && !em_game_over && !horda.fase_concluida)
        {
            /* ?? lógica de jogo ?? */
            atualizar_sanidade(&sanidade);
            jogador.frame  += 0.15f;
            jogador.movendo = 0;
            if (tempo.ativo)
                tempo.atual = al_get_time() - tempo.inicio;

            /* ?? roda de habilidades ?? */
            int k_now = al_key_down(&state, ALLEGRO_KEY_K);
            if (k_now && !k_ant)
            {
                roda_aberta  = 1;
                roda_selecao = jogador.tipo_ataque;
            }
            if (!k_now && k_ant)
            {
                jogador.tipo_ataque = roda_selecao;
                roda_aberta = 0;
            }
            if (roda_aberta)
            {
                int q_now2 = al_key_down(&state, ALLEGRO_KEY_Q);
                int e_now2 = al_key_down(&state, ALLEGRO_KEY_E);
                if (q_now2 && !q_ant) { roda_selecao--; if (roda_selecao < 1) roda_selecao = 3; }
                if (e_now2 && !e_ant) { roda_selecao++; if (roda_selecao > 3) roda_selecao = 1; }
                q_ant = q_now2;
                e_ant = e_now2;
            }
            k_ant = k_now;

            /* ?? ataque ?? */
            int j_now = al_key_down(&state, ALLEGRO_KEY_J);
            double agora_atk   = al_get_time();
            double delay_atual = (jogador.tipo_ataque == 3) ? DELAY_ATAQUE_3 : DELAY_ATAQUE_12;

            if (j_now && !j_ant && !jogador.atacando && !roda_aberta &&
                jogador.estado != SAM_HURT && jogador.estado != SAM_DEAD &&
                (agora_atk - jogador.ultimo_ataque) >= delay_atual)
            {
                int custo_est = (jogador.tipo_ataque == 3) ? 4 : 1;

                if (jogador.estamina >= custo_est)
                {
                    jogador.atacando      = 1;
                    jogador.frame_ataque  = 0;

                    /* reseta flag de hit em todos os zumbis */
                    for (int i = 0; i < MAX_ZUMBIS_TELA; i++)
                        horda.pool[i].atingido_no_ataque = 0;

                    jogador.estamina -= custo_est;
                    if (jogador.estamina < 0) jogador.estamina = 0;

                    jogador.estado       = SAM_ATTACK;
                    jogador.ultimo_ataque = agora_atk;

                    if (jogador.tipo_ataque == 3)
                    {
                        jogador.dash_ativo = 1;
                        jogador.dash_dist  = DASH_ATK3_DIST;
                    }
                }
            }
            j_ant = j_now;

            /* ?? dash do ataque 3 ?? */
            if (jogador.dash_ativo && jogador.atacando)
            {
                float passo = 8.0f;
                if (jogador.dash_dist > 0.0f)
                {
                    float move    = (jogador.dash_dist < passo) ? jogador.dash_dist : passo;
                    float dx_dash = (jogador.direcao == 0) ? move : -move;
                    float nx_d    = jogador.mov.x + dx_dash;
                    if (!colide_mapa(mapa, nx_d, jogador.mov.y))
                        jogador.mov.x = nx_d;
                    jogador.dash_dist -= move;
                }
                if (jogador.dash_dist <= 0.0f || !jogador.atacando)
                    jogador.dash_ativo = 0;
            }

            /* ?? movimento horizontal e pulo ?? */
            if (jogador.estado != SAM_HURT && jogador.estado != SAM_DEAD)
            {
                float nx = jogador.mov.x;
                if (al_key_down(&state, ALLEGRO_KEY_D) && !jogador.atacando && !roda_aberta)
                {
                    nx += VELOCIDADE;
                    jogador.direcao = 0;
                    jogador.movendo = 1;
                }
                if (al_key_down(&state, ALLEGRO_KEY_A) && !jogador.atacando && !roda_aberta)
                {
                    nx -= VELOCIDADE;
                    jogador.direcao = ALLEGRO_FLIP_HORIZONTAL;
                    jogador.movendo = 1;
                }
                if (!colide_mapa(mapa, nx, jogador.mov.y))
                    jogador.mov.x = nx;

                if (al_key_down(&state, ALLEGRO_KEY_W) &&
                    jogador.no_chao && !jogador.atacando && !roda_aberta &&
                    jogador.estamina >= CUSTO_PULO)
                {
                    jogador.mov.vel_y = FORCA_PULO;
                    jogador.estamina -= CUSTO_PULO;
                    if (jogador.estamina < 0) jogador.estamina = 0;
                }
            }

            /* ?? gravidade e colisão vertical ?? */
            jogador.mov.vel_y += GRAVIDADE;
            if (jogador.mov.vel_y > MAX_QUEDA) jogador.mov.vel_y = MAX_QUEDA;
            float ny = jogador.mov.y + jogador.mov.vel_y;
            if (!colide_mapa(mapa, jogador.mov.x, ny))
                jogador.mov.y = ny;
            else
                jogador.mov.vel_y = 0;
            jogador.no_chao = esta_no_chao(mapa, jogador.mov.x, jogador.mov.y);

            /* recarga de estamina no chão */
            if (jogador.no_chao)
            {
                jogador.estamina += RECARGA_ESTAMINA;
                if (jogador.estamina > MAX_ESTAMINA) jogador.estamina = MAX_ESTAMINA;
            }

            /* caiu fora do mapa */
            if (jogador.mov.y > ALTURA + 200)
            {
                perder_vida(vetor_vidas);
                jogador.mov.x     = 60;
                jogador.mov.y     = 253;
                jogador.mov.vel_y = 0;
            }

            atualizar_estado_samurai(&jogador);

            /* ?? horda ?? */
            horda_atualizar_spawn(&horda, jogador.mov.y);
            horda_atualizar_fisica(&horda, mapa);
            horda_atualizar_movimento(&horda, &jogador);
            horda_verificar_ataque(&horda, &jogador, &sanidade);
            horda_verificar_dano_jogador(&horda, &jogador, vetor_vidas);

            /* ?? poção ?? */
            {
                double agora_poc = al_get_time();
                if (!pocao.ativa && (agora_poc - pocao_ultimo_check) >= 3.0)
                {
                    pocao_ultimo_check = agora_poc;
                    pocao_tentar_spawn(&pocao, vetor_vidas);
                }
                pocao_atualizar(&pocao, &jogador, vetor_vidas);
            }

            /* ?? morte do jogador ?? */
            {
                int vivas = 0;
                for (int i = 0; i < MAX_VIDAS; i++)
                    if (vetor_vidas[i].ativa) vivas++;
                if (vivas == 0 && jogador.estado != SAM_DEAD)
                {
                    jogador.estado       = SAM_DEAD;
                    jogador.frame_dead   = 0;
                    jogador.morte_animando = 1;
                    jogador.mov.vel_y    = -3.0f;
                }
            }

            if (horda.fase_concluida && tempo.ativo)
            {
                tempo.fim   = al_get_time();
                tempo.atual = tempo.fim - tempo.inicio;
                tempo.ativo = 0;
            }
        } /* fim !pausado && !em_game_over && !fase_concluida */

        /* ?? física durante animação de morte ?? */
        if (!pausado && jogador.estado == SAM_DEAD &&
            jogador.morte_animando && !sanidade.game_over)
        {
            jogador.mov.vel_y += GRAVIDADE * 0.30f;
            if (jogador.mov.vel_y > 3.5f) jogador.mov.vel_y = 3.5f;
            float ny2 = jogador.mov.y + jogador.mov.vel_y;
            if (!colide_mapa(mapa, jogador.mov.x, ny2))
                jogador.mov.y = ny2;
            else
                jogador.mov.vel_y = 0;
            morte_pronta = (jogador.estado == SAM_DEAD && !jogador.morte_animando);
        }

        /* ===== DESENHO ===== */
        double t_agora = al_get_time();
        al_clear_to_color(al_map_rgb(0, 0, 0));
        al_draw_bitmap(bg, 0, 0, 0);
        desenhar_matriz_fundo(matriz_dec, lm, cm);

        pocao_desenhar(&pocao, spr_pocao);
        horda_desenhar(&horda, &zum_spr);
        desenhar_samurai(&jogador, &sam_spr);
        desenhar_overlay_sanidade(&sanidade);

        char txt[60];
        sprintf(txt, "Tempo: %.2f s", tempo.atual);
        desenhar_hud_tempo(fonte_hud, txt);
        desenhar_hud_horda(fonte_hud, &horda);
        desenhar_vidas(vetor_vidas, coracao);
        desenhar_estamina(jogador.estamina, spr_estamina);
        desenhar_sanidade(&sanidade, spr_sanidade);
        desenhar_aviso_sanidade(&sanidade, fonte_hud, t_agora);
        desenhar_mensagem_central_insanidade(&sanidade, fonte, fonte_hud, t_agora);
        desenhar_hud_ataque(sam_spr.attack1, sam_spr.attack2, sam_spr.attack3,
                            habilidade1, habilidade2, habilidade3,
                            jogador.tipo_ataque, fonte_hud);

        if (pausado)
        {
            al_draw_filled_rectangle(0, 0, LARGURA, ALTURA, al_map_rgba(0,0,0,160));
            al_draw_filled_rounded_rectangle(
                LARGURA/2.0-310, ALTURA/2.0-90,
                LARGURA/2.0+310, ALTURA/2.0+70,
                12, 12, al_map_rgb(101,60,20));
            al_draw_rounded_rectangle(
                LARGURA/2.0-310, ALTURA/2.0-90,
                LARGURA/2.0+310, ALTURA/2.0+70,
                12, 12, al_map_rgb(218,165,32), 3);
            al_draw_text(fonte, al_map_rgb(255,215,0),
                LARGURA/2.0, ALTURA/2.0-75, ALLEGRO_ALIGN_CENTER, "PAUSADO");
            al_draw_text(fonte_hud, al_map_rgb(255,255,255),
                LARGURA/2.0, ALTURA/2.0, ALLEGRO_ALIGN_CENTER,
                "Pressione ESC para continuar");
        }

        if (roda_aberta)
            desenhar_roda_habilidade(sam_spr.attack1, sam_spr.attack2, sam_spr.attack3,
                                     habilidade1, habilidade2, habilidade3,
                                     roda_selecao, fonte_hud);

        al_flip_display();

        /* ?? game over / vitória (após flip) ?? */
        morte_pronta = (jogador.estado == SAM_DEAD && !jogador.morte_animando);
        if (morte_pronta || sanidade.game_over)
        {
            al_rest(0.8);
            const char *motivo = sanidade.game_over
                ? "Sua sanidade chegou ao limite..."
                : "Voce foi derrotado pelos zumbis!";
            int reiniciar = tela_game_over(queue, timer, fonte, fonte_hud, motivo);
            if (reiniciar)
            {
                /* ?? reinicia todo o estado do jogo ?? */
                jogador = (Jogador){0};
                jogador.mov.x        = 60;
                jogador.mov.y        = 253;
                jogador.tipo_ataque  = 1;
                jogador.estamina     = MAX_ESTAMINA;
                jogador.estado       = SAM_IDLE;
                jogador.hurt_inicio  = -999.0;
                jogador.ultimo_ataque = -999.0;
                jogador.dash_ativo   = 0;
                jogador.dash_dist    = 0.0f;

                sanidade = (Sanidade){MAX_SANIDADE, 0, 0, 0, 0.0, 0};
                sanidade.ultimo_abate = al_get_time();

                horda_init(&horda);

                pocao = (Pocao){0};
                pocao_ultimo_check = 0.0;

                for (int i = 0; i < MAX_VIDAS; i++)
                {
                    vetor_vidas[i].ativa = 1;
                    strcpy(vetor_vidas[i].status, "Inteira");
                }

                tempo = (Temporizador){0};
                tempo.inicio = al_get_time();
                tempo.ativo  = 1;
                carregar_ranking(&tempo);

                pausado     = 0;
                roda_aberta = 0;
                roda_selecao = 1;
                esc_ant = 0; k_ant = 0; j_ant = 0; q_ant = 0; e_ant = 0;

                al_flush_event_queue(queue);
                continue;
            }
            rodando = 0;
            continue;
        }

        if (horda.fase_concluida && !sanidade.game_over)
        {
            al_draw_filled_rectangle(0, 0, LARGURA, ALTURA, al_map_rgba(255,215,0,40));
            al_draw_text(fonte, al_map_rgb(255,215,0),
                LARGURA/2.0f, ALTURA/2.0f-60, ALLEGRO_ALIGN_CENTER,
                "50 ZUMBIS ELIMINADOS!");
            al_draw_text(fonte_hud, al_map_rgb(255,255,255),
                LARGURA/2.0f, ALTURA/2.0f, ALLEGRO_ALIGN_CENTER,
                "Abrindo ranking...");
            al_flip_display();
            al_rest(2.0);
            tela_ranking(queue, timer, fonte, fonte_hud, &tempo, tempo.atual);
            rodando = 0;
            continue;
        }

    } /* fim while(rodando) */

    /* ?? limpeza ?? */
    free(vetor_vidas);
    liberar_matriz(matriz_dec, lm);
    al_destroy_bitmap(bg_menu);
    al_destroy_bitmap(bg);
    al_destroy_bitmap(mapa);
    al_destroy_bitmap(sam_spr.idle);
    al_destroy_bitmap(sam_spr.run);
    al_destroy_bitmap(sam_spr.jump);
    al_destroy_bitmap(sam_spr.attack1);
    al_destroy_bitmap(sam_spr.attack2);
    al_destroy_bitmap(sam_spr.attack3);
    al_destroy_bitmap(sam_spr.hurt);
    al_destroy_bitmap(sam_spr.dead);
    al_destroy_bitmap(zum_spr.walk);
    al_destroy_bitmap(zum_spr.run);
    al_destroy_bitmap(zum_spr.attack1);
    al_destroy_bitmap(zum_spr.attack2);
    al_destroy_bitmap(zum_spr.attack3);
    al_destroy_bitmap(zum_spr.bite);
    al_destroy_bitmap(zum_spr.hurt);
    al_destroy_bitmap(zum_spr.dead);
    al_destroy_bitmap(zum_spr.idle);
    al_destroy_bitmap(coracao);
    al_destroy_bitmap(spr_estamina);
    if (spr_sanidade) al_destroy_bitmap(spr_sanidade);
    if (spr_pocao)    al_destroy_bitmap(spr_pocao);
    al_destroy_bitmap(habilidade1);
    al_destroy_bitmap(habilidade2);
    al_destroy_bitmap(habilidade3);
    al_destroy_font(fonte_hud);
    al_destroy_font(fonte);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);
    return 0;
}