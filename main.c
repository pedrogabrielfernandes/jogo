#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define LARGURA 1920
#define ALTURA 1080
#define FPS 60

// tamanho original do sprite
#define SPRITE_SRC_W 96
#define SPRITE_SRC_H 84

// ESCALA
#define SPRITE_SCALE 1.95f

#define DRAW_W (SPRITE_SRC_W * SPRITE_SCALE)
#define DRAW_H (SPRITE_SRC_H * SPRITE_SCALE)

// hitbox proporcional 
#define HITBOX_W 35
#define HITBOX_H 80

#define HITBOX_OFFSET_X 66
#define HITBOX_OFFSET_Y 42

#define VELOCIDADE 5.0f
#define GRAVIDADE 1.0f
#define FORCA_PULO -15.05f
#define MAX_QUEDA 18.0f

typedef struct {
    float x;
    float y;
    float vel_y;
} Movimento;

typedef struct {
    Movimento mov;

    float frame;
    int no_chao;
    int direcao;
    int movendo;
} Jogador;

// PRETO = solido
bool pixel_solido(ALLEGRO_BITMAP *mapa, int x, int y) {
    if (x < 0 || y < 0 || x >= al_get_bitmap_width(mapa) || y >= al_get_bitmap_height(mapa))
        return true;

    ALLEGRO_COLOR cor = al_get_pixel(mapa, x, y);
    unsigned char r, g, b;
    al_unmap_rgb(cor, &r, &g, &b);

    return (r < 50 && g < 50 && b < 50);
}

bool colide_mapa(ALLEGRO_BITMAP *mapa, float x, float y) {
    int left   = (int)x;
    int right  = (int)x + HITBOX_W - 1;
    int top    = (int)y;
    int bottom = (int)y + HITBOX_H - 1;

    return pixel_solido(mapa, left, top) ||
           pixel_solido(mapa, right, top) ||
           pixel_solido(mapa, left, bottom) ||
           pixel_solido(mapa, right, bottom);
}

bool esta_no_chao(ALLEGRO_BITMAP *mapa, float x, float y) {
    int left  = (int)x + 4;
    int right = (int)x + HITBOX_W - 4;
    int foot  = (int)y + HITBOX_H;

    return pixel_solido(mapa, left, foot) ||
           pixel_solido(mapa, right, foot);
}

int main() {
    if (!al_init()) return 1;
    if (!al_install_keyboard()) return 1;
    if (!al_init_image_addon()) return 1;

    al_init_font_addon();
    al_init_ttf_addon();

    ALLEGRO_DISPLAY *display = al_create_display(LARGURA, ALTURA);
    if (!display) return 1;

    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    ALLEGRO_TIMER *timer = al_create_timer(1.0 / FPS);

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());

    ALLEGRO_BITMAP *bg   = al_load_bitmap("assets/cenarios/background2.png");
    ALLEGRO_BITMAP *mapa = al_load_bitmap("assets/cenarios/colisao2.png");

    ALLEGRO_BITMAP *idle = al_load_bitmap("assets/sprites/IDLE.png");
    ALLEGRO_BITMAP *run  = al_load_bitmap("assets/sprites/RUN.png");
    ALLEGRO_BITMAP *jump = al_load_bitmap("assets/sprites/JUMP.png");

    if (!bg || !mapa || !idle || !run || !jump) {
        printf("Erro ao carregar arquivos\n");
        return 1;
    }

Jogador *jogador = malloc(sizeof(Jogador));

if (jogador == NULL) {
    printf("Erro ao alocar memoria\n");
    return 1;
}

jogador->mov.x = 60;
jogador->mov.y = 253;
jogador->mov.vel_y = 0;
jogador->frame = 0;

jogador->no_chao = 0;
jogador->direcao = 0;
jogador->movendo = 0;

    ALLEGRO_EVENT ev;
    ALLEGRO_KEYBOARD_STATE state;
    int rodando = 1;

    al_start_timer(timer);

    while (rodando) {
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            rodando = 0;

        if (ev.type == ALLEGRO_EVENT_TIMER) {

            jogador->frame += 0.15;
            jogador->movendo = 0;

            al_get_keyboard_state(&state);

            jogador->no_chao = esta_no_chao(mapa, jogador->mov.x, jogador->mov.y);

            float novo_x = jogador->mov.x;

            if (al_key_down(&state, ALLEGRO_KEY_D)) {
                novo_x += VELOCIDADE;
                jogador->direcao = 0;
                jogador->movendo = 1;
            }

            if (al_key_down(&state, ALLEGRO_KEY_A)) {
                novo_x -= VELOCIDADE;
                jogador->direcao = ALLEGRO_FLIP_HORIZONTAL;
                jogador->movendo = 1;
            }

            if (!colide_mapa(mapa, novo_x, jogador->mov.y)) {
                jogador->mov.x = novo_x;
            }

            if (al_key_down(&state, ALLEGRO_KEY_W) && jogador->no_chao) {
                jogador->mov.vel_y = FORCA_PULO;
            }

            jogador->mov.vel_y += GRAVIDADE;
            if (jogador->mov.vel_y > MAX_QUEDA) jogador->mov.vel_y = MAX_QUEDA;

            float novo_y = jogador->mov.y + jogador->mov.vel_y;

            if (!colide_mapa(mapa, jogador->mov.x, novo_y)) {
                jogador->mov.y = novo_y;
            } else {
                if (jogador->mov.vel_y > 0) {
                    while (!colide_mapa(mapa, jogador->mov.x, jogador->mov.y + 1)) jogador->mov.y++;
                } else if (jogador->mov.vel_y < 0) {
                    while (!colide_mapa(mapa, jogador->mov.x, jogador->mov.y - 1)) jogador->mov.y--;
                }
                jogador->mov.vel_y = 0;
            }

            jogador->no_chao = esta_no_chao(mapa, jogador->mov.x, jogador->mov.y);

            float draw_x = jogador->mov.x - HITBOX_OFFSET_X;
            float draw_y = jogador->mov.y - HITBOX_OFFSET_Y;

            al_clear_to_color(al_map_rgb(255,255,255));
            al_draw_bitmap(bg, 0, 0, 0);

            if (!jogador->no_chao) {
                if (jogador->frame >= 5) jogador->frame = 0;
                al_draw_scaled_bitmap(jump, 96*(int)jogador->frame, 0, 96, 84, draw_x, draw_y, DRAW_W, DRAW_H, jogador->direcao);
            } else if (jogador->movendo) {
                if (jogador->frame >= 8) jogador->frame = 0;
                al_draw_scaled_bitmap(run, 96*(int)jogador->frame, 0, 96, 84, draw_x, draw_y, DRAW_W, DRAW_H, jogador->direcao);
            } else {
                if (jogador->frame >= 7) jogador->frame = 0;
                al_draw_scaled_bitmap(idle, 96*(int)jogador->frame, 0, 96, 84, draw_x, draw_y, DRAW_W, DRAW_H, jogador->direcao);
            }

            al_flip_display();
        }
    }
    al_destroy_bitmap(bg);
    al_destroy_bitmap(mapa);
    al_destroy_bitmap(idle);
    al_destroy_bitmap(run);
    al_destroy_bitmap(jump);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);
    free(jogador);
    return 0;
}
