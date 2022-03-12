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

#include "gtk/gtkcssprovider.h"
#include "le_bon_mot-config.h"
#include "le_bon_mot-window.h"
#include "le_bon_mot-engine.h"

static void le_bon_mot_window_display_board ();
static void
le_bon_mot_window_on_key_released (
    GtkEventControllerKey *self,
    guint keyval,
    guint keycode,
    GdkModifierType state,
    gpointer user_data);

struct _LeBonMotWindow
{
  AdwApplicationWindow  parent_instance;

  /* Template widgets */
  AdwHeaderBar        *header_bar;
  GtkGrid             *game_grid;

  GtkCssProvider      *css_provider;
  LeBonMotEngine      *engine;
};

G_DEFINE_TYPE (LeBonMotWindow, le_bon_mot_window, ADW_TYPE_APPLICATION_WINDOW)

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
le_bon_mot_window_init (LeBonMotWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  // CSS style
  GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET (self));
  self->css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_resource(self->css_provider, "/ch/adorsaz/LeBonMot/le_bon_mot-window.css");  
  gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER (self->css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);


  // Engine
  self->engine = g_object_new(LE_BON_MOT_TYPE_ENGINE, NULL);
  le_bon_mot_window_display_board(self);

  // Event management
  GtkEventController *controller = gtk_event_controller_key_new();
  gtk_widget_add_controller(GTK_WIDGET(self), controller);
  gtk_event_controller_set_propagation_phase(controller, GTK_PHASE_CAPTURE);
  g_signal_connect(controller, "key-released", G_CALLBACK(le_bon_mot_window_on_key_released), NULL);

  gtk_widget_grab_focus(GTK_WIDGET (self));
}

typedef struct {
  GtkWidget *child;
  LeBonMotLetter *letter;
} SetCssUserData;

static gboolean le_bon_mot_window_set_css (gpointer user_data) {
  SetCssUserData *css_user_data = user_data;
  switch (css_user_data->letter->state) {
    case LE_BON_MOT_LETTER_NOT_PRESENT:
      gtk_widget_add_css_class(css_user_data->child, "not_present");
      break;
    case LE_BON_MOT_LETTER_WELL_PLACED:
      gtk_widget_add_css_class(css_user_data->child, "well_placed");
      break;
    case LE_BON_MOT_LETTER_PRESENT:
      gtk_widget_add_css_class(css_user_data->child, "present");
      break;
    default:
      break;
  }
  g_free(user_data);
  return G_SOURCE_REMOVE;
}

static void
le_bon_mot_window_display_board (LeBonMotWindow *self, guint delay_on_row)
{
  g_return_if_fail(LE_BON_MOT_IS_WINDOW(self));

  GPtrArray* board = le_bon_mot_engine_get_board_state(self->engine);

  for (guint rowIndex = 0; rowIndex < board->len; rowIndex +=1 ) {
    GPtrArray *row = g_ptr_array_index(board, rowIndex);
    for (guint columnIndex = 0; columnIndex < row->len; columnIndex += 1) {
      LeBonMotLetter *letter = g_ptr_array_index(row, columnIndex);

      GString *labelString = g_string_new("");
      g_string_append_c(labelString, letter->letter);

      GtkWidget* child = gtk_grid_get_child_at(self->game_grid, columnIndex, rowIndex);
      if (!child) {
        child = gtk_label_new(labelString->str);
        gtk_widget_add_css_class(child, "card");
        gtk_grid_attach(self->game_grid, child, columnIndex, rowIndex, 1, 1);
      } else {
        gtk_label_set_text(GTK_LABEL (child), labelString->str);
      }

      SetCssUserData *user_data = g_new(SetCssUserData, 1);
      user_data->child = child;
      user_data->letter = letter;

      if (delay_on_row >= 0 && rowIndex >= delay_on_row) {
        guint delay = 500 * columnIndex + 500 * row->len * (rowIndex - delay_on_row);
        g_timeout_add(delay, le_bon_mot_window_set_css, user_data);
      } else {
        le_bon_mot_window_set_css(user_data);
      }

    }
  }
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

  // Ignore all key sequence with Control key
  if (state & GDK_CONTROL_MASK)
  {
    return;
  }
  
  const char *keyname = gdk_keyval_name(keyval);
  guint delay_on_row = -1;
  // Window grabs focus on any recognized input to avoid propagate keyboard
  // event to other widgets (like the main menu button)
  if (g_regex_match_simple(
        "^[a-z](acute|diaeresis|grave)?$",
        keyname,
        G_REGEX_CASELESS,
        0)) {
    le_bon_mot_engine_add_letter(window->engine, keyname);
    le_bon_mot_window_display_board(window, delay_on_row);
    gtk_widget_grab_focus(widget);
  } else if (strcmp(keyname, "BackSpace") == 0) {
    le_bon_mot_engine_remove_letter(window->engine);
    le_bon_mot_window_display_board(window, delay_on_row);
    gtk_widget_grab_focus(widget);
  } else if (strcmp(keyname, "Return") == 0) {
    delay_on_row = le_bon_mot_engine_get_current_row(window->engine);
    le_bon_mot_engine_validate(window->engine);
    le_bon_mot_window_display_board(window, delay_on_row);
    gtk_widget_grab_focus(widget);
  }
}
