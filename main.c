#include <allegro5/allegro.h>
#include <allegro5/bitmap.h>
#include <allegro5/bitmap_draw.h>
#include <allegro5/bitmap_io.h>
#include <allegro5/color.h>
#include <allegro5/display.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/events.h>
#include <allegro5/system.h>
#include <allegro5/allegro_image.h>
#include <stdio.h>

#define altura 720
#define largura 1280

//spirte personagem: x = 96 y = 84

int main() {
    if (!al_init()){
        printf("falha na inicialização do jogo.");
        return 1;
    }
    if (!al_init_font_addon()){
        printf("falha na inicialização das fontes automaticas");
        return 1;
    }
    if (!al_init_ttf_addon()) {
        printf("falha na inicialização das fontes");
        return 1;
    }
    if (!al_init_image_addon()) {
        printf("falha no carramento da imagem");
        return 1;
    }
    if (!al_install_keyboard()){
        printf("falha na inicialização do teclado.");
    }

    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    ALLEGRO_FONT *fonte = al_load_ttf_font("assets/arial.ttf", 24, 0);
    if (!fonte) {
        printf("falha na inicialização da fonte arial");
        return 1;
    }

    al_set_new_window_position(320, 180);
    ALLEGRO_DISPLAY *display = al_create_display(largura, altura);

    al_register_event_source(queue, al_get_display_event_source(display));

    ALLEGRO_TIMER *timer = al_create_timer(1.0/30.0);
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_start_timer(timer);

    ALLEGRO_BITMAP *bg = al_load_bitmap("assets/background.png");
    if (!bg) {
        printf("falha na inicialização do background");
        return 1;
    }

    ALLEGRO_BITMAP *idle = al_load_bitmap("assets/sprites/IDLE.png");
    ALLEGRO_BITMAP *run = al_load_bitmap("assets/sprites/RUN.png");
    ALLEGRO_BITMAP *jump = al_load_bitmap("assets/sprites/JUMP.png");
    if (!idle || !run || !jump){
        printf("falha ao carregar os sprites");
        return 1;
    }

    ALLEGRO_EVENT evento;
    int rodando = 1;
    float frame = 0.f;
    int pos_x = 0;
    int pos_y = 560;
    int current_frame_y = 0;
    ALLEGRO_KEYBOARD_STATE state;

    al_set_window_title(display, "Expedition");

    while (rodando){
        al_wait_for_event(queue, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = 0;
        }

        if (evento.type == ALLEGRO_EVENT_TIMER){
            frame += 0.3f;

            al_clear_to_color(al_map_rgb(255, 255, 255));
            al_draw_bitmap(bg, 0, 0, 0);
            al_get_keyboard_state(&state);

            if (al_key_down(&state, ALLEGRO_KEY_D)){
                if (frame > 8) frame -= 8;
                pos_x += 5;
                al_draw_bitmap_region(run, 96 * (int)frame, current_frame_y, 96, 84, pos_x, pos_y, 0);
            }
            else if (al_key_down(&state, ALLEGRO_KEY_A)){
                if (frame > 8) frame -= 8;
                pos_x -= 5;
                al_draw_bitmap_region(run, 96 * (int)frame, current_frame_y, 96, 84, pos_x, pos_y, 0);
            }
            else if (al_key_down(&state, ALLEGRO_KEY_W)){
                if (frame > 5) frame -= 5;
                pos_y -= 40;
                al_draw_bitmap_region(jump, 96 * (int)frame, current_frame_y, 96, 84, pos_x, pos_y, 0);
                pos_y = 560;
            }
            else{
                if (frame > 7) frame -= 7;
                al_draw_bitmap_region(idle, 96 * (int)frame, current_frame_y, 96, 84, pos_x, pos_y, 0);
            }

            al_draw_text(fonte, al_map_rgb(0, 0, 0), 100, 100, 0, "");
            al_flip_display();
        }
    }

    al_destroy_bitmap(idle);
    al_destroy_bitmap(run);
    al_destroy_bitmap(jump);
    al_destroy_bitmap(bg);
    al_destroy_display(display);
    al_destroy_font(fonte);
    al_destroy_event_queue(queue);

    return 0;
}
 
//git add .
//git commit -m "mensagem do commit"
//git push

//se tem duvidas, leia as instuções na pasta do jogo.

//Novo código compilador universal -> gcc main.c -o jogo $(pkg-config --cflags --libs allegro-5 allegro_font-5 allegro_ttf-5 allegro_image-5) 

//TESTANDO AQ PCR (JOIA)