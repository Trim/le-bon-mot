#include "le_bon_mot.h"

const guint LE_BON_MOT_NUMBER_OF_ROWS = 6;

typedef enum {
  UNKOWN,
  NOT_PRESENT,
  PRESENT,
  WELL_PLACED,
} LetterState;

typedef struct {
  gchar letter;
  LetterState state;
} LeBonMotLetter;

static GArray *le_bon_mot_new_board(GString *word);

LeBonMot *le_bon_mot_init() {
  LeBonMot *le_bon_mot = g_new(LeBonMot, 1);

  le_bon_mot->word = g_string_new("animal");
  le_bon_mot->board = le_bon_mot_new_board(le_bon_mot->word);

  return le_bon_mot;
}

static GArray *le_bon_mot_new_board(GString *word) {
  GArray *board = g_array_sized_new(FALSE, FALSE, sizeof(GArray *), LE_BON_MOT_NUMBER_OF_ROWS);
  for (guint rowIndex = 0; rowIndex < LE_BON_MOT_NUMBER_OF_ROWS; rowIndex += 1) {
    GArray *row = g_array_sized_new(FALSE, FALSE, sizeof(LeBonMotLetter *), word->len);
    for (guint columnIndex = 0; columnIndex < word->len; columnIndex += 1) {
      LeBonMotLetter *letter = g_new(LeBonMotLetter, 1);
      if (rowIndex == 0 && columnIndex == 0) {
        letter->letter = word->str[0];
        letter->state = WELL_PLACED;
      } else {
        letter->letter = '_';
        letter->state = UNKOWN;
      }
      g_array_append_val(row, letter);
    }
    g_array_append_val(board, row);
  }
  return board;
}

void le_bon_mot_show_board(LeBonMot* le_bon_mot, GtkGrid *grid) {
  for (guint rowIndex = 0; rowIndex < le_bon_mot->board->len; rowIndex +=1 ) {
    GArray *row = g_array_index(le_bon_mot->board, GArray*, rowIndex);
    for (guint columnIndex = 0; columnIndex < row->len; columnIndex += 1) {
      LeBonMotLetter *letter = g_array_index(row, LeBonMotLetter*, columnIndex);
      GString *label = g_string_new("");
      g_string_append_c(label, letter->letter);
      gtk_grid_attach(grid, gtk_label_new(label->str), columnIndex, rowIndex, 1, 1);
    }
  }
}
