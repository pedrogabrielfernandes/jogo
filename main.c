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

int main() {
    //iniciando e verificando o programa, fontes e imagens
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

    //criando o evento de fila e fontes>
    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    ALLEGRO_FONT *fonte = al_load_ttf_font("assets/arial.ttf", 24, 0);
        if (!fonte) {
            printf("falha na inicialização da fonte arial");
            return 1;
        }

    //definindo o tamanho da janela e onde ela vai aparecer
    al_set_new_window_position(320, 180);
    ALLEGRO_DISPLAY *display = al_create_display(1920, 1080);

    //registrando novo evento
    al_register_event_source(queue, al_get_display_event_source(display));

    //definindo o background e verificando ele
    ALLEGRO_BITMAP *bg = al_load_bitmap("assets/background.png");
    if (!bg) {
        printf("falha na inicialização do background");
        return 1;
    }

    //um evento
    ALLEGRO_EVENT evento;

    //criando um inteiro para o laço de repetição
    int rodando = 1;

    //o laco de repeticao que vai fazer o jogo rodar ate fechar
    while (rodando){
            al_wait_for_event(queue, &evento);
            if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                rodando = 0;
            }
        al_clear_to_color(al_map_rgb(255, 255, 255));
        al_draw_bitmap(bg, 0, 0, 0);
        al_draw_text(fonte, al_map_rgb(0, 0, 0), 100, 100, 0, "Horario: 15:30");
        al_flip_display();
    }

    al_destroy_bitmap(bg);
    al_destroy_display(display);
    al_destroy_font(fonte);
    al_destroy_event_queue(queue);
    //destroi a janela
    return 0;

}


//(Linux) -> sudo "..." -S github-cli
//(Linux) -> gh auth login - faz o login
//(Linux) -> git clone https://github.com/pedrogabrielfernandes/jogo
//após cada alteração que fizerem dê ctrl+s atualizem o repositório do github
//exemplo: 
//git add .
//git commit -m "o que você alterou"
//git push
//logo após atualize seu repositorio e seu gitclone