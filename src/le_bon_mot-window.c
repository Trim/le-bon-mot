/* le_bon_mot-window.c
 *
 * Copyright 2022 Adrien Dorsaz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "le_bon_mot-config.h"
#include "le_bon_mot-window.h"
#include "le_bon_mot-engine.h"

static void le_bon_mot_window_display_board ();

struct _LeBonMotWindow
{
  GtkApplicationWindow  parent_instance;

  /* Template widgets */
  GtkHeaderBar        *header_bar;
  GtkGrid             *game_grid;

  LeBonMotEngine      *engine;
};

G_DEFINE_TYPE (LeBonMotWindow, le_bon_mot_window, GTK_TYPE_APPLICATION_WINDOW)

static void
le_bon_mot_window_dispose (GObject *gobject)
{
  LeBonMotWindow * self = LE_BON_MOT_WINDOW(gobject);
  g_clear_object(&self->engine);

  G_OBJECT_CLASS (le_bon_mot_window_parent_class)->dispose (gobject);
}

static void
le_bon_mot_window_class_init (LeBonMotWindowClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  g_object_class->dispose = le_bon_mot_window_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/ch/adorsaz/LeBonMot/le_bon_mot-window.ui");
  gtk_widget_class_bind_template_child (widget_class, LeBonMotWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, LeBonMotWindow, game_grid);
}

static void
le_bon_mot_window_on_key_released (
    GtkEventControllerKey *self,
    guint keyval,
    guint keycode,
    GdkModifierType state,
    gpointer user_data)
{
  GtkWidget* widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));
  g_return_if_fail(LE_BON_MOT_IS_WINDOW(widget));

  LeBonMotWindow* window = LE_BON_MOT_WINDOW(widget);

  printf("LeBonMotWindow: key released: val: %d, code: %d, name: %s\n", keyval, keycode, gdk_keyval_name(keyval));
  fflush(NULL);
  
  const char *keyname = gdk_keyval_name(keyval);
  if (g_regex_match_simple(
        "^[a-z](acute|diaeresis|grave)?$",
        keyname,
        G_REGEX_CASELESS,
        0)) {
    le_bon_mot_engine_add_letter(window->engine, keyname);
  } else if (strcmp(keyname, "BackSpace") == 0) {
    le_bon_mot_engine_remove_letter(window->engine);
  } else if (strcmp(keyname, "Return") == 0) {
    le_bon_mot_engine_validate(window->engine);
  }
  le_bon_mot_window_display_board(window);
}

static void
le_bon_mot_window_init (LeBonMotWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->engine = g_object_new(LE_BON_MOT_TYPE_ENGINE, NULL);
  le_bon_mot_window_display_board(self);

  GtkEventController *controller = gtk_event_controller_key_new();
  gtk_widget_add_controller(GTK_WIDGET(self), controller);
  gtk_event_controller_set_propagation_phase(controller, GTK_PHASE_CAPTURE);
  g_signal_connect(controller, "key-released", G_CALLBACK(le_bon_mot_window_on_key_released), NULL);
}

static void
le_bon_mot_window_display_board (LeBonMotWindow *self)
{
  g_return_if_fail(LE_BON_MOT_IS_WINDOW(self));

  GPtrArray* board = le_bon_mot_engine_get_board_state(self->engine);

  for (guint rowIndex = 0; rowIndex < board->len; rowIndex +=1 ) {
    GPtrArray *row = g_ptr_array_index(board, rowIndex);
    for (guint columnIndex = 0; columnIndex < row->len; columnIndex += 1) {
      LeBonMotLetter *letter = g_ptr_array_index(row, columnIndex);
      GString *label = g_string_new("");
      g_string_append_c(label, letter->letter);

      GtkWidget* child = gtk_grid_get_child_at(self->game_grid, columnIndex, rowIndex);
      if (child) {
        gtk_grid_remove(self->game_grid, child);
      }
      gtk_grid_attach(self->game_grid, gtk_label_new(label->str), columnIndex, rowIndex, 1, 1);
    }
  }
}
