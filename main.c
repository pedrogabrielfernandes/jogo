#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <stdio.h>
#include <stdbool.h>

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

    float x = 60;
    float y = 253;

    float vel_y = 0;
    int no_chao = 0;
    int direcao = 0;
    int movendo = 0;
    float frame = 0;

    ALLEGRO_EVENT ev;
    ALLEGRO_KEYBOARD_STATE state;
    int rodando = 1;

    al_start_timer(timer);

    while (rodando) {
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            rodando = 0;

        if (ev.type == ALLEGRO_EVENT_TIMER) {

            frame += 0.15;
            movendo = 0;

            al_get_keyboard_state(&state);

            no_chao = esta_no_chao(mapa, x, y);

            float novo_x = x;

            if (al_key_down(&state, ALLEGRO_KEY_D)) {
                novo_x += VELOCIDADE;
                direcao = 0;
                movendo = 1;
            }

            if (al_key_down(&state, ALLEGRO_KEY_A)) {
                novo_x -= VELOCIDADE;
                direcao = ALLEGRO_FLIP_HORIZONTAL;
                movendo = 1;
            }

            if (!colide_mapa(mapa, novo_x, y)) {
                x = novo_x;
            }

            if (al_key_down(&state, ALLEGRO_KEY_W) && no_chao) {
                vel_y = FORCA_PULO;
            }

            vel_y += GRAVIDADE;
            if (vel_y > MAX_QUEDA) vel_y = MAX_QUEDA;

            float novo_y = y + vel_y;

            if (!colide_mapa(mapa, x, novo_y)) {
                y = novo_y;
            } else {
                if (vel_y > 0) {
                    while (!colide_mapa(mapa, x, y + 1)) y++;
                } else if (vel_y < 0) {
                    while (!colide_mapa(mapa, x, y - 1)) y--;
                }
                vel_y = 0;
            }

            no_chao = esta_no_chao(mapa, x, y);

            float draw_x = x - HITBOX_OFFSET_X;
            float draw_y = y - HITBOX_OFFSET_Y;

            al_clear_to_color(al_map_rgb(255,255,255));
            al_draw_bitmap(bg, 0, 0, 0);

            if (!no_chao) {
                if (frame >= 5) frame = 0;
                al_draw_scaled_bitmap(jump, 96*(int)frame, 0, 96, 84, draw_x, draw_y, DRAW_W, DRAW_H, direcao);
            } else if (movendo) {
                if (frame >= 8) frame = 0;
                al_draw_scaled_bitmap(run, 96*(int)frame, 0, 96, 84, draw_x, draw_y, DRAW_W, DRAW_H, direcao);
            } else {
                if (frame >= 7) frame = 0;
                al_draw_scaled_bitmap(idle, 96*(int)frame, 0, 96, 84, draw_x, draw_y, DRAW_W, DRAW_H, direcao);
            }

            al_flip_display();
        }
    }

    return 0;
}
