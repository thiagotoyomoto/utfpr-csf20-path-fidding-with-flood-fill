#include "../include/Field.h"
#include "../include/Vector2Queue.h"
#include "../include/constants.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

/* Verifica se n está em [min, max]. */
bool in_range (int n, int min, int max);

/* Verifica se pos.x está em [0, w-1] e verifica se pos.y está em [0, h-1]. */
bool in_boundary (Vector2 pos, int w, int h);

/* Algoritmo que preenche o campo com os valores. */
void flood_fill (Field *, Positions *);

/* Algoritmo que determina o caminho entre o ponto inicial e final. */
void set_path (Field *, Positions *);

int main (int argc, char *argv[]) {
    Field *field = Field_create (40, 20);

    Positions *pos =
        Positions_create ((Vector2) { 0, 0 },
                          (Vector2) { field->width - 1, field->height - 1 },
                          (Vector2) { field->width / 2, field->height / 2 });

    // Flags
    bool run_loop     = true;
    bool was_modified = true;

    initscr ();

    // Inicialização das cores
    start_color ();

    init_pair (DEFAULT_COLOR, COLOR_BLACK, COLOR_BLACK);
    init_pair (START_COLOR, COLOR_GREEN, COLOR_BLACK);
    init_pair (TARGET_COLOR, COLOR_RED, COLOR_BLACK);
    init_pair (WALL_COLOR, COLOR_BLUE, COLOR_BLACK);
    init_pair (PATH_COLOR, COLOR_YELLOW, COLOR_BLACK);
    init_pair (SELECTED_DEFAULT_COLOR, COLOR_BLACK, COLOR_WHITE);
    init_pair (SELECTED_START_COLOR, COLOR_GREEN, COLOR_WHITE);
    init_pair (SELECTED_TARGET_COLOR, COLOR_RED, COLOR_WHITE);
    init_pair (SELECTED_WALL_COLOR, COLOR_BLUE, COLOR_WHITE);
    init_pair (SELECTED_PATH_COLOR, COLOR_YELLOW, COLOR_WHITE);

    curs_set (0);
    cbreak ();
    noecho ();
    keypad (stdscr, true);

    while (run_loop) {
        if (was_modified) {
            flood_fill (field, pos);
            set_path (field, pos);
            was_modified = false;
        }

        erase ();
        Field_draw (field, pos);

        // Imprime as posições ao lado do mapa ---------------------------------
        mvprintw (2, field->width + 4, "Posicoes:");
        mvprintw (3,
                  field->width + 4,
                  "  Inicial     = (%2d; %2d)\n",
                  pos->start.x,
                  pos->start.y);
        mvprintw (4,
                  field->width + 4,
                  "  Final       = (%2d; %2d)\n",
                  pos->target.x,
                  pos->target.y);
        mvprintw (5,
                  field->width + 4,
                  "  Selecionado = (%2d; %2d)\n",
                  pos->selected.x,
                  pos->selected.y);
        // -----------------------------------------------------------

        refresh ();

        int *selected_value = &field->data[pos->selected.y][pos->selected.x];
        bool is_start_pos   = Vector2_equals (pos->selected, pos->start);
        bool is_target_pos  = Vector2_equals (pos->selected, pos->target);
        bool is_wall        = *selected_value == WALL_VALUE;

        switch (getch ()) {
            case KEY_UP: // Selecionar a célula de cima
                if (pos->selected.y > 0)
                    pos->selected.y -= 1;
                break;
            case KEY_LEFT: // Selecionar a célula da esquerda
                if (pos->selected.x > 0)
                    pos->selected.x -= 1;
                break;
            case KEY_DOWN: // Selecionar a célula de baixo
                if (pos->selected.y < field->height - 1)
                    pos->selected.y += 1;
                break;
            case KEY_RIGHT: // Selecionar a célula da direita
                if (pos->selected.x < field->width - 1)
                    pos->selected.x += 1;
                break;
            case 'a': // Mudar a posição inicial
                if (!is_target_pos && !is_wall) {
                    pos->start   = pos->selected;
                    was_modified = true;
                }
                break;
            case 's': // Mudar a posição alvo
                if (!is_start_pos && !is_wall) {
                    pos->target  = pos->selected;
                    was_modified = true;
                }
                break;
            case 'd': { // Adicionar ou remover parede
                if (!is_start_pos && !is_target_pos) {
                    *selected_value = is_wall ? DEFAULT_VALUE : WALL_VALUE;
                    was_modified    = true;
                }
            } break;
            case 'q': // Sair
                run_loop = false;
                break;
        }
    }

    Field_destroy (field);
    Positions_destroy (pos);

    endwin ();

    return 0;
}

bool in_range (int n, int min, int max) {
    return n >= min && n <= max;
}

bool in_boundary (Vector2 pos, int w, int h) {
    return in_range (pos.x, 0, w - 1) && in_range (pos.y, 0, h - 1);
}

void flood_fill (Field *field, Positions *pos) {
    Vector2Queue *queue = Vector2Queue_create ();

    if (queue == NULL)
        return;

    int i, j, k = 0;

    // Resetando os valores
    for (i = 0; i < field->height; ++i)
        for (j = 0; j < field->width; ++j)
            if (field->data[i][j] != WALL_VALUE)
                field->data[i][j] = DEFAULT_VALUE;

    field->data[pos->start.y][pos->start.x] = 1;
    Vector2Queue_enqueue (queue, pos->start);

    while (!Vector2Queue_empty (queue)) {
        Vector2 curr_pos  = Vector2Queue_front (queue);
        int     new_value = field->data[curr_pos.y][curr_pos.x] + 1;

        for (i = 0; i < 4; ++i) {
            // posição do vizinho
            Vector2 n_pos = Vector2_add (curr_pos, offset[i]);

            // verifica a posição do vizinho
            if (in_boundary (n_pos, field->width, field->height)) {
                // valor do vizinho
                int *n_value = &field->data[n_pos.y][n_pos.x];

                // se não tiver valor, recebe um novo valor
                if (*n_value == 0) {
                    *n_value = new_value;
                    Vector2Queue_enqueue (queue, n_pos);
                }
            }
        }

        Vector2Queue_dequeue (queue);
    }

    Vector2Queue_destroy (queue);
}

void set_path (Field *field, Positions *pos) {
    Vector2 curr_pos = pos->target;

    Vector2Queue_clear (pos->path);

    bool has_next;
    do {
        has_next = false;
        int     i, num_neigh = 0;
        int     curr_value = field->data[curr_pos.y][curr_pos.x];
        Vector2 neigh_poss[4];
        for (i = 0; i < 4; ++i) {
            Vector2 n_pos = Vector2_add (curr_pos, offset[i]);
            if (in_boundary (n_pos, field->width, field->height)) {
                int n_value = field->data[n_pos.y][n_pos.x];
                if (n_value != WALL_VALUE && n_value < curr_value)
                    if (Vector2_equals (n_pos, pos->start))
                        break;
                neigh_poss[num_neigh++] = n_pos;
            }
        }

        if (num_neigh > 0) {
            // menor distância
            int min_dist = Vector2_sqr_distance (neigh_poss[0], pos->start);
            // índice do menor
            int min_i = 0;
            for (i = 1; i < num_neigh; ++i) {
                int other_dist =
                    Vector2_sqr_distance (neigh_poss[i], pos->start);
                if (other_dist < min_dist) {
                    min_dist = other_dist;
                    min_i    = i;
                }
            }
            Vector2Queue_enqueue (pos->path, neigh_poss[min_i]);
            curr_pos = neigh_poss[min_i];
            has_next = true;
        }
    } while (has_next);
}
