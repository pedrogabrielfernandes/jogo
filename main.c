#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <stdio.h>
#include <stdbool.h>
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

#define ZUMBI_DRAW_W 180
#define ZUMBI_DRAW_H 180

#define ZUMBI_OFFSET_X 50
#define ZUMBI_OFFSET_Y 169

unsigned char colisao[ALTURA][3000];

typedef struct
{
    int ativa;
    char status[20];
} VidaStatus;

typedef struct
{
    float x;
    float y;
    float vel_y;
} Movimento;

typedef struct
{
    Movimento mov;

    float frame;
    float frame_ataque;

    int no_chao;
    int direcao;
    int movendo;

    // ATTACK
    int atacando;
    int tipo_ataque;

    double ultimo_dano;

} Jogador;

typedef struct
{
    float x;
    float y;
    float velocidade;
    int direcao;
    int vivo;
    float frame;
    int vida;
} Inimigo;

typedef struct
{
    Inimigo inimigos[50];
    int quantidade;
} SistemaInimigos;

typedef struct
{
    double inicio;
    double atual;
    double fim;
    int ativo;
    float ranking[10];
    int quantidade_scores;
} Temporizador;

typedef enum
{
    MENU_JOGAR = 0,
    MENU_SAIR = 1
} OpcaoMenu;

int **criar_matriz_decorativa(int linhas, int colunas)
{
    int **mat = (int **)malloc(linhas * sizeof(int *));
    for (int i = 0; i < linhas; i++)
    {
        mat[i] = (int *)malloc(colunas * sizeof(int));
        for (int j = 0; j < colunas; j++)
        {
            mat[i][j] = rand() % 2;
        }
    }
    return mat;
}

void desenhar_matriz_fundo(int **mat, int linhas, int colunas)
{
    for (int i = 0; i < linhas; i++)
    {
        for (int j = 0; j < colunas; j++)
        {
            if (mat[i][j] == 1)
            {
                al_draw_filled_rectangle(j * 100, i * 100, j * 100 + 3, i * 100 + 3, al_map_rgba(255, 255, 255, 40));
            }
        }
    }
}

void liberar_matriz(int **mat, int linhas)
{
    for (int i = 0; i < linhas; i++)
    {
        free(mat[i]);
    }
    free(mat);
}

OpcaoMenu executar_menu(ALLEGRO_EVENT_QUEUE *queue, ALLEGRO_TIMER *timer, ALLEGRO_BITMAP *bg_menu, ALLEGRO_FONT *fonte)
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
            const char *sair = (opcao == MENU_SAIR) ? "> SAIR <" : "SAIR";

            al_clear_to_color(al_map_rgb(0, 0, 0));

            al_draw_scaled_bitmap(bg_menu, 0, 0, al_get_bitmap_width(bg_menu), al_get_bitmap_height(bg_menu), 0, 0, LARGURA, ALTURA, 0);
            al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2.4, 990, ALLEGRO_ALIGN_CENTER, jogar);
            al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 1.6, 990, ALLEGRO_ALIGN_CENTER, sair);

            al_flip_display();
        }
    }
}

bool pixel_solido(ALLEGRO_BITMAP *mapa, int x, int y)
{
    if (x < 0 || y < 0 ||
        x >= 3000 || y >= ALTURA)
    {
        return true;
    }

    return colisao[y][x];
}

bool colide_mapa(ALLEGRO_BITMAP *mapa, float x, float y)
{
    int left = (int)x;
    int right = (int)x + HITBOX_W - 1;
    int top = (int)y;
    int bottom = (int)y + HITBOX_H - 1;

    // topo
    for (int px = left; px <= right; px += 4)
    {
        if (pixel_solido(mapa, px, top))
            return true;
    }

    // baixo
    for (int px = left; px <= right; px += 4)
    {
        if (pixel_solido(mapa, px, bottom))
            return true;
    }

    // esquerda
    for (int py = top; py <= bottom; py += 4)
    {
        if (pixel_solido(mapa, left, py))
            return true;
    }

    // direita
    for (int py = top; py <= bottom; py += 4)
    {
        if (pixel_solido(mapa, right, py))
            return true;
    }

    return false;
}

bool esta_no_chao(ALLEGRO_BITMAP *mapa, float x, float y)
{
    int left = (int)x + 4;
    int right = (int)x + HITBOX_W - 4;
    int foot = (int)y + HITBOX_H;
    return pixel_solido(mapa, left, foot) || pixel_solido(mapa, right, foot);
}

void desenhar_vidas(VidaStatus *vidas, ALLEGRO_BITMAP *coracao)
{
    for (int i = 0; i < MAX_VIDAS; i++)
    {
        float x = 20 + i * 60;
        float y = 80;

        if (vidas[i].ativa == 1)
            al_draw_bitmap(coracao, x, y, 0);
        else
            al_draw_tinted_bitmap(coracao, al_map_rgba(100, 100, 100, 120), x, y, 0);
    }
}

void perder_vida(VidaStatus *vidas)
{
    for (int i = MAX_VIDAS - 1; i >= 0; i--)
    {
        if (vidas[i].ativa == 1)
        {
            vidas[i].ativa = 0;
            strcpy(vidas[i].status, "Perdida");
            break;
        }
    }
}

void gerar_mapa_colisao(ALLEGRO_BITMAP *mapa)
{
    al_lock_bitmap(mapa, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);

    for (int y = 0; y < al_get_bitmap_height(mapa); y++)
    {
        for (int x = 0; x < al_get_bitmap_width(mapa); x++)
        {
            ALLEGRO_COLOR cor = al_get_pixel(mapa, x, y);

            unsigned char r, g, b;

            al_unmap_rgb(cor, &r, &g, &b);

            int brilho = (r + g + b) / 3;

            colisao[y][x] = (brilho < 120);
        }
    }

    al_unlock_bitmap(mapa);
}

void ordenar_ranking(float ranking[], int tamanho)
{
    for (int i = 0; i < tamanho - 1; i++)
    {
        for (int j = 0; j < tamanho - i - 1; j++)
        {
            if (ranking[j] > ranking[j + 1])
            {
                float temp = ranking[j];
                ranking[j] = ranking[j + 1];
                ranking[j + 1] = temp;
            }
        }
    }
}

void carregar_ranking(Temporizador *tempo)
{
    FILE *file = fopen("ranking.txt", "r");
    if (!file)
        return;

    while (fscanf(file, "%f", &tempo->ranking[tempo->quantidade_scores]) == 1)
    {
        tempo->quantidade_scores++;
        if (tempo->quantidade_scores >= 10)
            break;
    }
    fclose(file);
}

void salvar_ranking(Temporizador *tempo)
{
    FILE *file = fopen("ranking.txt", "w");
    if (!file)
        return;

    for (int i = 0; i < tempo->quantidade_scores; i++)
    {
        fprintf(file, "%.2f\n", tempo->ranking[i]);
    }
    fclose(file);
}

int main(void)
{
    srand(time(NULL));

    if (!al_init())
    {
        printf("Erro: al_init\n");
        return 1;
    }
    if (!al_init_primitives_addon())
    {
        printf("Erro: al_init_primitives_addon\n");
        return 1;
    }
    if (!al_install_keyboard())
    {
        printf("Erro: al_install_keyboard\n");
        return 1;
    }
    if (!al_init_image_addon())
    {
        printf("Erro: al_init_image_addon\n");
        return 1;
    }

    al_init_font_addon();
    al_init_ttf_addon();

    ALLEGRO_DISPLAY *display = al_create_display(LARGURA, ALTURA);
    if (!display)
    {
        printf("Erro: al_create_display\n");
        return 1;
    }

    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    if (!queue)
    {
        printf("Erro: al_create_event_queue\n");
        return 1;
    }

    ALLEGRO_TIMER *timer = al_create_timer(1.0 / FPS);
    if (!timer)
    {
        printf("Erro: al_create_timer\n");
        return 1;
    }

    ALLEGRO_FONT *fonte = al_load_ttf_font("assets/arial.ttf", 48, 0);
    if (!fonte)
    {
        printf("Erro ao carregar fonte: assets/arial.ttf\n");
        fonte = al_create_builtin_font();
        if (!fonte)
            return 1;
    }

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());

    ALLEGRO_BITMAP *bg_menu = al_load_bitmap("assets/cenarios/inicio.png");
    if (!bg_menu)
    {
        printf("Erro ao carregar background do menu\n");
        return 1;
    }

    al_start_timer(timer);

    OpcaoMenu escolha = executar_menu(queue, timer, bg_menu, fonte);

    if (escolha == MENU_SAIR)
    {
        return 0;
    }

    al_flush_event_queue(queue);

    ALLEGRO_BITMAP *bg = al_load_bitmap("assets/cenarios/background2.png");
    ALLEGRO_BITMAP *mapa = al_load_bitmap("assets/cenarios/colisao2.png");
    gerar_mapa_colisao(mapa);
    ALLEGRO_BITMAP *idle = al_load_bitmap("assets/sprites/Samurai/Idle.png");
    ALLEGRO_BITMAP *run = al_load_bitmap("assets/sprites/Samurai/Run.png");
    ALLEGRO_BITMAP *jump = al_load_bitmap("assets/sprites/Samurai/Jump.png");
    ALLEGRO_BITMAP *coracao = al_load_bitmap("assets/itens/vida.png");
    ALLEGRO_BITMAP *zumbi_sprite = al_load_bitmap("assets/sprites/Zombies/Walk.png");
    ALLEGRO_BITMAP *ataque1 = al_load_bitmap("assets/sprites/Samurai/Attack_1.png");
    ALLEGRO_BITMAP *ataque2 = al_load_bitmap("assets/sprites/Samurai/Attack_2.png");
    ALLEGRO_BITMAP *ataque3 = al_load_bitmap("assets/sprites/Samurai/Attack_3.png");

    if (!bg || !mapa || !idle || !run || !jump ||
        !coracao || !zumbi_sprite ||
        !ataque1 || !ataque2 || !ataque3)
    {
        printf("Erro ao carregar bitmaps do jogo\n");
        return 1;
    }

    Jogador jogador;
    jogador.mov.x = 60;
    jogador.mov.y = 253;
    jogador.mov.vel_y = 0;
    jogador.frame = 0;
    jogador.no_chao = 0;
    jogador.direcao = 0;
    jogador.movendo = 0;
    jogador.atacando = 0;
    jogador.frame_ataque = 0;
    jogador.tipo_ataque = 1;
    jogador.ultimo_dano = 0;

    Inimigo zumbi;
    zumbi.x = 1200;
    zumbi.y = 760;
    zumbi.velocidade = 1.35f;
    zumbi.direcao = 0;
    zumbi.vivo = 1;
    zumbi.frame = 0;
    zumbi.vida = 100;



    Temporizador tempo;
    tempo.inicio = al_get_time();
    tempo.atual = 0;
    tempo.fim = 0;
    tempo.ativo = 1;

    tempo.quantidade_scores = 0;
    carregar_ranking(&tempo);

    char mensagem_final[100] = "Fase Concluída!";

    VidaStatus *vetor_vidas = (VidaStatus *)malloc(MAX_VIDAS * sizeof(VidaStatus));
    for (int i = 0; i < MAX_VIDAS; i++)
    {
        vetor_vidas[i].ativa = 1;
        strcpy(vetor_vidas[i].status, "Inteira");
    }

    int linhas_matriz = ALTURA / 100 + 1;
    int colunas_matriz = LARGURA / 100 + 1;
    int **matriz_decorativa = criar_matriz_decorativa(linhas_matriz, colunas_matriz);

    int rodando = 1;
    ALLEGRO_EVENT ev;
    ALLEGRO_KEYBOARD_STATE state;

    while (rodando)
    {
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            rodando = 0;

        if (ev.type == ALLEGRO_EVENT_TIMER)
        {
            jogador.frame += 0.15f;
            jogador.movendo = 0;

            al_get_keyboard_state(&state);

            static int key_r_anterior = 0;

            int key_r_atual =
                al_key_down(&state, ALLEGRO_KEY_R);

            if (key_r_atual && !key_r_anterior)
            {
                jogador.mov.x = 2500;
            }
            key_r_anterior = key_r_atual;

            static int key_k_anterior = 0;

            int key_k_atual =
                al_key_down(&state, ALLEGRO_KEY_K);

            if (key_k_atual && !key_k_anterior)
            {
                jogador.tipo_ataque++;

                if (jogador.tipo_ataque > 3)
                    jogador.tipo_ataque = 1;
            }

            key_k_anterior = key_k_atual;

            static int key_j_anterior = 0;

            int key_j_atual =
                al_key_down(&state, ALLEGRO_KEY_J);

            if (key_j_atual &&
                !key_j_anterior &&
                !jogador.atacando)
            {
                jogador.atacando = 1;
                jogador.frame_ataque = 0;
            }

            key_j_anterior = key_j_atual;

            if (tempo.ativo)
                tempo.atual = al_get_time() - tempo.inicio;

            jogador.no_chao = esta_no_chao(mapa, jogador.mov.x, jogador.mov.y);

            // =========================
            // MOVIMENTAÇÃO HORIZONTAL
            // =========================

            float novo_x = jogador.mov.x;

            if (al_key_down(&state, ALLEGRO_KEY_D) && !jogador.atacando)
            {
                novo_x += VELOCIDADE;
                jogador.direcao = 0;
                jogador.movendo = 1;
            }

            if (al_key_down(&state, ALLEGRO_KEY_A) && !jogador.atacando)
            {
                novo_x -= VELOCIDADE;
                jogador.direcao = ALLEGRO_FLIP_HORIZONTAL;
                jogador.movendo = 1;
            }

            if (!colide_mapa(mapa, novo_x, jogador.mov.y))
                jogador.mov.x = novo_x;

            // =========================
            // PULO
            // =========================

            if (al_key_down(&state, ALLEGRO_KEY_W) &&
                jogador.no_chao &&
                !jogador.atacando)
            {
                jogador.mov.vel_y = FORCA_PULO;
            }

            jogador.mov.vel_y += GRAVIDADE;

            if (jogador.mov.vel_y > MAX_QUEDA)
                jogador.mov.vel_y = MAX_QUEDA;

            // =========================
            // MOVIMENTAÇÃO VERTICAL
            // =========================

            float novo_y = jogador.mov.y + jogador.mov.vel_y;

            if (!colide_mapa(mapa, jogador.mov.x, novo_y))
            {
                jogador.mov.y = novo_y;
            }
            else
            {
                jogador.mov.vel_y = 0;
            }

            jogador.no_chao = esta_no_chao(mapa, jogador.mov.x, jogador.mov.y);

            // =========================
            // QUEDA
            // =========================

            if (jogador.mov.y > ALTURA + 200)
            {
                perder_vida(vetor_vidas);

                jogador.mov.x = 60;
                jogador.mov.y = 253;
                jogador.mov.vel_y = 0;
            }

            // =========================
            // IA DO ZUMBI
            // =========================

            if (zumbi.vivo)
            {
                zumbi.frame += 0.08f;

                if (jogador.mov.x < zumbi.x)
                {
                    zumbi.x -= zumbi.velocidade;
                    zumbi.direcao = ALLEGRO_FLIP_HORIZONTAL;
                }
                else
                {
                    zumbi.x += zumbi.velocidade;
                    zumbi.direcao = 0;
                }
            }

            // =========================
            //      HITBOX DE ATAQUE
            // =========================

            if (jogador.atacando &&
                jogador.frame_ataque >= 2 &&
                jogador.frame_ataque <= 2.5 &&
                zumbi.vivo)
            {
                float atk_x;
                float atk_y = jogador.mov.y;

                if (jogador.direcao == 0)
                    atk_x = jogador.mov.x + 40;
                else
                    atk_x = jogador.mov.x - 60;

                if (
                    atk_x < zumbi.x + 50 &&
                    atk_x + 40 > zumbi.x &&
                    atk_y < zumbi.y + 70 &&
                    atk_y + 40 > zumbi.y)
                {
                    int dano = 0;

                    if (jogador.tipo_ataque == 1)
                        dano = 15;

                    else if (jogador.tipo_ataque == 2)
                        dano = 30;

                    else
                        dano = 50;

                    zumbi.vida -= dano;

                    if (zumbi.vida < 0)
                        zumbi.vida = 0;

                    if (zumbi.vida <= 0)
                    {
                        zumbi.vivo = 0;
                    }
                }
            }

            // =========================
            // COLISÃO COM ZUMBI
            // =========================

            if (zumbi.vivo)
            {
                if (
                    jogador.mov.x < zumbi.x + 60 &&
                    jogador.mov.x + HITBOX_W > zumbi.x &&
                    jogador.mov.y < zumbi.y + 80 &&
                    jogador.mov.y + HITBOX_H > zumbi.y)
                {
                    if (al_get_time() - jogador.ultimo_dano > 1.0)
                    {
                        perder_vida(vetor_vidas);

                        jogador.ultimo_dano = al_get_time();

                        if (jogador.direcao == 0)
                            jogador.mov.x -= 120;
                        else
                            jogador.mov.x += 120;

                        jogador.mov.vel_y = -8;
                    }
                }
            }

            // =========================
            // FINAL DO MAPA
            // =========================

            if (jogador.mov.x > 2400 && tempo.ativo)
            {
                tempo.fim = al_get_time();
                tempo.atual = tempo.fim - tempo.inicio;
                tempo.ativo = 0;

                if (tempo.quantidade_scores < 10)
                {
                    tempo.ranking[tempo.quantidade_scores] = tempo.atual;
                    tempo.quantidade_scores++;
                }
                else
                {
                    ordenar_ranking(tempo.ranking, 10);
                    if (tempo.atual < tempo.ranking[9])
                    {
                        tempo.ranking[9] = tempo.atual;
                    }
                }
                ordenar_ranking(tempo.ranking, tempo.quantidade_scores);
                salvar_ranking(&tempo);
            }

            float draw_x = jogador.mov.x - HITBOX_OFFSET_X;
            float draw_y = jogador.mov.y - HITBOX_OFFSET_Y;

            al_clear_to_color(al_map_rgb(255, 255, 255));

            al_draw_bitmap(bg, 0, 0, 0);

            desenhar_matriz_fundo(
                matriz_decorativa,
                linhas_matriz,
                colunas_matriz);

            // =========================
            // DESENHO DO ZUMBI
            // =========================

            if (zumbi.vivo)
            {
                int frame_zumbi = (int)zumbi.frame % FRAMES_ZUMBI;

                float zumbi_draw_x = zumbi.x - ZUMBI_OFFSET_X;
                float zumbi_draw_y = zumbi.y - ZUMBI_OFFSET_Y;

                al_draw_scaled_bitmap(
                    zumbi_sprite,

                    frame_zumbi * 96,
                    0,

                    96,
                    96,

                    zumbi_draw_x,
                    zumbi_draw_y,

                    ZUMBI_DRAW_W,
                    ZUMBI_DRAW_H,

                    zumbi.direcao);
            }
            al_draw_filled_rectangle(
                zumbi.x,
                zumbi.y - 20,

                zumbi.x + 100,
                zumbi.y - 10,

                al_map_rgb(80, 0, 0));

            al_draw_filled_rectangle(
                zumbi.x,
                zumbi.y - 20,

                zumbi.x + zumbi.vida,
                zumbi.y - 10,

                al_map_rgb(255, 0, 0));
            if (jogador.atacando)
            {
                int max_frames = 0;

                if (jogador.tipo_ataque == 1)
                    max_frames = 6;

                else if (jogador.tipo_ataque == 2)
                    max_frames = 4;

                else
                    max_frames = 3;

                int f = (int)jogador.frame_ataque;

                if (f >= max_frames)
                    f = max_frames - 1;

                ALLEGRO_BITMAP *atk = NULL;

                if (jogador.tipo_ataque == 1)
                    atk = ataque1;

                else if (jogador.tipo_ataque == 2)
                    atk = ataque2;

                else
                    atk = ataque3;


                float atk_draw_x = draw_x - 10;
                float atk_draw_y = draw_y - 40;

                al_draw_scaled_bitmap(
                    atk,

                    128 * f,
                    0,

                    128,
                    128,

                    atk_draw_x,
                    atk_draw_y,

                    ATTACK_DRAW_W,
                    ATTACK_DRAW_H,

                    jogador.direcao);

                jogador.frame_ataque += 0.30f;

                if (jogador.frame_ataque >= max_frames)
                {
                    jogador.atacando = 0;
                    jogador.frame_ataque = 0;
                }
            }
            else if (!jogador.no_chao)
            {
                int f = (int)jogador.frame % FRAMES_JUMP;

                al_draw_scaled_bitmap(
                    jump,
                    128 * f, 0,
                    128, 128,
                    draw_x,
                    draw_y,
                    DRAW_W,
                    DRAW_H,
                    jogador.direcao);
            }
            else if (jogador.movendo)
            {
                int f = (int)jogador.frame % FRAMES_RUN;

                al_draw_scaled_bitmap(
                    run,
                    128 * f, 0,
                    128, 128,
                    draw_x,
                    draw_y,
                    DRAW_W,
                    DRAW_H,
                    jogador.direcao);
            }
            else
            {
                int f = (int)jogador.frame % FRAMES_IDLE;

                al_draw_scaled_bitmap(
                    idle,
                    128 * f, 0,
                    128, 128,
                    draw_x,
                    draw_y,
                    DRAW_W,
                    DRAW_H,
                    jogador.direcao);
            }

            // =========================
            // HUD
            // =========================

            char texto[50];

            sprintf(texto, "Tempo: %.2f s", tempo.atual);

            al_draw_filled_rectangle(
                10, 10,
                340, 70,
                al_map_rgb(255, 255, 255));

            al_draw_text(
                fonte,
                al_map_rgb(255, 215, 0),
                1850,
                0,
                ALLEGRO_ALIGN_RIGHT,
                "RANKING"
            );

            for (int i = 0; i < tempo.quantidade_scores; i++)
            {
                char texto_rank[50];
                sprintf(texto_rank, "%d. %.2f s", i + 1, tempo.ranking[i]);
                al_draw_text(
                    fonte,
                    al_map_rgb(255, 255, 255),
                    1850,
                    40 + i * 45,
                    ALLEGRO_ALIGN_RIGHT,
                    texto_rank
                );
            }

            al_draw_text(
                fonte,
                al_map_rgb(0, 0, 0),
                20, 20,
                0,
                texto);

            desenhar_vidas(vetor_vidas, coracao);

            if (!tempo.ativo)
            {
                al_draw_text(
                    fonte,
                    al_map_rgb(255, 215, 0),
                    LARGURA / 2.0,
                    ALTURA / 3.0,
                    ALLEGRO_ALIGN_CENTER,
                    mensagem_final);
            }

            al_flip_display();
        }
    }

    free(vetor_vidas);
    liberar_matriz(matriz_decorativa, linhas_matriz);

    al_destroy_bitmap(bg_menu);
    al_destroy_bitmap(bg);
    al_destroy_bitmap(mapa);
    al_destroy_bitmap(idle);
    al_destroy_bitmap(run);
    al_destroy_bitmap(jump);
    al_destroy_bitmap(coracao);

    al_destroy_font(fonte);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);
    al_destroy_bitmap(zumbi_sprite);

    al_destroy_bitmap(ataque1);
    al_destroy_bitmap(ataque2);
    al_destroy_bitmap(ataque3);

    return 0;
}
