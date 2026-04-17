#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define LARGURA 1920
#define ALTURA  1080
#define FPS     60

#define SPRITE_SRC_W 96
#define SPRITE_SRC_H 84

#define SPRITE_SCALE 1.95f
#define DRAW_W (SPRITE_SRC_W * SPRITE_SCALE)
#define DRAW_H (SPRITE_SRC_H * SPRITE_SCALE)

#define HITBOX_W        35
#define HITBOX_H        80
#define HITBOX_OFFSET_X 66
#define HITBOX_OFFSET_Y 42

#define VELOCIDADE  5.0f
#define GRAVIDADE   1.0f
#define FORCA_PULO -15.05f
#define MAX_QUEDA   18.0f

#define FRAMES_IDLE 7
#define FRAMES_RUN  8
#define FRAMES_JUMP 5

typedef struct {
    float x;
    float y;
    float vel_y;
} Movimento;

typedef struct {
    Movimento mov;
    float frame;
    int   no_chao;
    int   direcao;
    int   movendo;
} Jogador;

typedef struct {
    double inicio;
    double atual;
    double fim;
    int    ativo;
} Temporizador;

typedef enum {
    MENU_JOGAR = 0,
    MENU_SAIR = 1
} OpcaoMenu;

OpcaoMenu executar_menu(ALLEGRO_EVENT_QUEUE *queue,
                  ALLEGRO_BITMAP *bg_menu,
                  ALLEGRO_FONT *fonte)
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

        if (ev.type == ALLEGRO_EVENT_TIMER)
        {
            char jogar[50];
            char sair[50];

            if (opcao == MENU_JOGAR)
            {
                sprintf(jogar, "> JOGAR <");
                sprintf(sair, "SAIR");
            }
            else
            {
                sprintf(jogar, "JOGAR");
                sprintf(sair, "> SAIR <");
            }

            al_clear_to_color(al_map_rgb(0,0,0));

            al_draw_scaled_bitmap(
                bg_menu,
                0,0,
                al_get_bitmap_width(bg_menu),
                al_get_bitmap_height(bg_menu),
                0,0,
                LARGURA,ALTURA,
                0
            );

            al_draw_text(
                fonte,
                al_map_rgb(255,255,255),
                LARGURA/2.4,
                990,
                ALLEGRO_ALIGN_CENTER,
                jogar
            );

            al_draw_text(
                fonte,
                al_map_rgb(255,255,255),
                LARGURA/1.6,
                990,
                ALLEGRO_ALIGN_CENTER,
                sair
            );

            al_flip_display();
        }
    }
}

bool pixel_solido(ALLEGRO_BITMAP *mapa, int x, int y) {
    if (x < 0 || y < 0 ||
        x >= al_get_bitmap_width(mapa) ||
        y >= al_get_bitmap_height(mapa))
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

    return pixel_solido(mapa, left,  top)    ||
           pixel_solido(mapa, right, top)    ||
           pixel_solido(mapa, left,  bottom) ||
           pixel_solido(mapa, right, bottom);
}

bool esta_no_chao(ALLEGRO_BITMAP *mapa, float x, float y) {
    int left  = (int)x + 4;
    int right = (int)x + HITBOX_W - 4;
    int foot  = (int)y + HITBOX_H;

    return pixel_solido(mapa, left,  foot) ||
           pixel_solido(mapa, right, foot);
}

int main(void) {

    if (!al_init())             { printf("Erro: al_init\n");             return 1; }
    if (!al_install_keyboard()) { printf("Erro: al_install_keyboard\n"); return 1; }
    if (!al_init_image_addon()) { printf("Erro: al_init_image_addon\n"); return 1; }

    al_init_font_addon();
    al_init_ttf_addon();

    ALLEGRO_DISPLAY *display = al_create_display(LARGURA, ALTURA);
    if (!display) { printf("Erro: al_create_display\n"); return 1; }

    ALLEGRO_FONT *fonte = al_load_ttf_font("assets/arial.ttf", 48, 0);
    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    ALLEGRO_TIMER       *timer = al_create_timer(1.0 / FPS);

    if (!queue || !timer) { printf("Erro: queue ou timer\n"); return 1; }

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());
    
    ALLEGRO_BITMAP *bg_menu =
        al_load_bitmap("assets/cenarios/background1.png");

    al_start_timer(timer);

    OpcaoMenu escolha = executar_menu(queue, bg_menu, fonte);

    if (escolha == MENU_SAIR)
        return 0;

    ALLEGRO_BITMAP *bg   = al_load_bitmap("assets/cenarios/background2.png");
    ALLEGRO_BITMAP *mapa = al_load_bitmap("assets/cenarios/colisao2.png");
    ALLEGRO_BITMAP *idle = al_load_bitmap("assets/sprites/IDLE.png");
    ALLEGRO_BITMAP *run  = al_load_bitmap("assets/sprites/RUN.png");
    ALLEGRO_BITMAP *jump = al_load_bitmap("assets/sprites/JUMP.png");


    if (!bg || !mapa || !idle || !run || !jump) {
        printf("Erro ao carregar bitmaps\n");
        return 1;
    }

    if (!fonte) {
        printf("Erro ao carregar fonte\n");
        return 1;
    }

    Jogador *jogador = malloc(sizeof(Jogador));
    if (!jogador) { printf("Erro ao alocar memoria\n"); return 1; }

    jogador->mov.x     = 60;
    jogador->mov.y     = 253;
    jogador->mov.vel_y = 0;
    jogador->frame     = 0;
    jogador->no_chao   = 0;
    jogador->direcao   = 0;
    jogador->movendo   = 0;

    Temporizador tempo;
    tempo.inicio = al_get_time();
    tempo.atual  = 0;
    tempo.fim    = 0;
    tempo.ativo  = 1;

    int rodando = 1;

    char mensagem_final[100] = "";

    ALLEGRO_EVENT ev;
    ALLEGRO_KEYBOARD_STATE state;

    al_start_timer(timer);

    while (rodando) {

        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            rodando = 0;

        if (ev.type == ALLEGRO_EVENT_TIMER) {

            jogador->frame  += 0.15f;
            jogador->movendo = 0;

            al_get_keyboard_state(&state);

            if (tempo.ativo)
                tempo.atual = al_get_time() - tempo.inicio;

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

            if (!colide_mapa(mapa, novo_x, jogador->mov.y))
                jogador->mov.x = novo_x;

            if (al_key_down(&state, ALLEGRO_KEY_W) && jogador->no_chao)
                jogador->mov.vel_y = FORCA_PULO;

            jogador->mov.vel_y += GRAVIDADE;
            if (jogador->mov.vel_y > MAX_QUEDA)
                jogador->mov.vel_y = MAX_QUEDA;

            float novo_y = jogador->mov.y + jogador->mov.vel_y;

            if (!colide_mapa(mapa, jogador->mov.x, novo_y)) {
                jogador->mov.y = novo_y;
            } else {
                if (jogador->mov.vel_y > 0) {
                    int limite = 0;
                    while (!colide_mapa(mapa, jogador->mov.x, jogador->mov.y + 1) && limite < 10) {
                        jogador->mov.y++;
                        limite++;
                    }
                } else if (jogador->mov.vel_y < 0) {
                    int limite = 0;
                    while (!colide_mapa(mapa, jogador->mov.x, jogador->mov.y - 1) && limite < 10) {
                        jogador->mov.y--;
                        limite++;
                    }
                }
                jogador->mov.vel_y = 0;
            }

            jogador->no_chao = esta_no_chao(mapa, jogador->mov.x, jogador->mov.y);

            if (jogador->mov.x > 1800 && tempo.ativo) {
                tempo.fim   = al_get_time();
                tempo.atual = tempo.fim - tempo.inicio;
                tempo.ativo = 0;

                char temp_str[50];
                
                strcpy(temp_str, "Fim de Jogo!");
                
                if (strcmp(temp_str, "Fim de Jogo!") == 0) {
                    // Concatenação (strcat)
                    strcat(temp_str, " Parabens!");
                }
                
                strcpy(mensagem_final, temp_str);
                
                int tamanho_msg = strlen(mensagem_final);
                printf("Tamanho da string gerada: %d caracteres.\n", tamanho_msg);
            }

            float draw_x = jogador->mov.x - HITBOX_OFFSET_X;
            float draw_y = jogador->mov.y - HITBOX_OFFSET_Y;

            al_clear_to_color(al_map_rgb(255, 255, 255));
            al_draw_bitmap(bg, 0, 0, 0);

            if (!jogador->no_chao) {
                if (jogador->frame >= FRAMES_JUMP) jogador->frame = 0;
                al_draw_scaled_bitmap(jump, 96 * (int)jogador->frame, 0, 96, 84, draw_x, draw_y, DRAW_W, DRAW_H, jogador->direcao);
            } else if (jogador->movendo) {
                if (jogador->frame >= FRAMES_RUN) jogador->frame = 0;
                al_draw_scaled_bitmap(run, 96 * (int)jogador->frame, 0, 96, 84, draw_x, draw_y, DRAW_W, DRAW_H, jogador->direcao);
            } else {
                if (jogador->frame >= FRAMES_IDLE) jogador->frame = 0;
                al_draw_scaled_bitmap(idle, 96 * (int)jogador->frame, 0, 96, 84, draw_x, draw_y, DRAW_W, DRAW_H, jogador->direcao);
            }
            char texto[50];
            sprintf(texto, "Tempo: %.2f s", tempo.atual);
            al_draw_text(fonte, al_map_rgb(0,0,0), 20, 20, 0, texto);

            if (!tempo.ativo) {
                al_draw_text(fonte, al_map_rgb(255,215,0), LARGURA / 2.0, ALTURA / 3.0, ALLEGRO_ALIGN_CENTER, mensagem_final);
            }

            al_flip_display();
        }
    }

    free(jogador);

    al_destroy_bitmap(bg);
    al_destroy_bitmap(mapa);
    al_destroy_bitmap(idle);
    al_destroy_bitmap(run);
    al_destroy_bitmap(jump);
    al_destroy_font(fonte);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);

    return 0;
}