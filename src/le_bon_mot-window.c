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

#include <gtk/gtkcssprovider.h>
#include <glib/gi18n.h>
#include "le_bon_mot-config.h"
#include "le_bon_mot-window.h"
#include "le_bon_mot-engine.h"

typedef struct {
  GtkLabel *label;
  LeBonMotLetter *letter;
} labelData;

static void le_bon_mot_window_display_board (
    LeBonMotWindow* self,
    gboolean apply_delay,
    guint delay_on_row
    );
static void
le_bon_mot_window_on_key_released (
    GtkEventControllerKey *self,
    guint keyval,
    guint keycode,
    GdkModifierType state,
    gpointer user_data);
static void
le_bon_mot_window_action_new_game (
    GtkWidget *sender,
    G_GNUC_UNUSED const char *action,
    G_GNUC_UNUSED GVariant      *parameter);
static void
le_bon_mot_window_display_alphabet (
    LeBonMotWindow *self);

struct _LeBonMotWindow
{
  AdwApplicationWindow  parent_instance;

  /* Template widgets */
  AdwHeaderBar        *header_bar;
  AdwToastOverlay     *toast_overlay;
  GtkGrid             *game_grid;
  GtkGrid             *alphabet_grid;

  GtkCssProvider      *css_provider;
  LeBonMotEngine      *engine;
  gboolean            is_validating;
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
  gtk_widget_class_bind_template_child (widget_class, LeBonMotWindow, alphabet_grid);
  gtk_widget_class_bind_template_child (widget_class, LeBonMotWindow, toast_overlay);

  gtk_widget_class_install_action(widget_class, "game.new", NULL, le_bon_mot_window_action_new_game);
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
  self->is_validating = FALSE;
  le_bon_mot_window_display_board(self, FALSE, 0);

  // Event management
  GtkEventController *controller = gtk_event_controller_key_new();
  gtk_widget_add_controller(GTK_WIDGET(self), controller);
  gtk_event_controller_set_propagation_phase(controller, GTK_PHASE_CAPTURE);
  g_signal_connect(controller, "key-released", G_CALLBACK(le_bon_mot_window_on_key_released), NULL);

  gtk_widget_grab_focus(GTK_WIDGET (self));
}

static void
le_bon_mot_window_action_new_game (
    GtkWidget *sender,
    G_GNUC_UNUSED const char *action,
    G_GNUC_UNUSED GVariant      *parameter)
{
  g_return_if_fail(LE_BON_MOT_IS_WINDOW(sender));
  LeBonMotWindow *self = LE_BON_MOT_WINDOW (sender);

  // Reset Game Grid
  GPtrArray* board = le_bon_mot_engine_get_board_state(self->engine);
  for (guint i = 0; i < board->len; i += 1) {
    gtk_grid_remove_row(self->game_grid, 0);
  }

  // Reset Alphabet Grid
  for (guint i = 0; i < 2; i += 1) {
    gtk_grid_remove_row(self->alphabet_grid, 0);
  }

  // Reset Engine
  g_clear_object(&self->engine);
  self->engine = g_object_new(LE_BON_MOT_TYPE_ENGINE, NULL);
  self->is_validating = FALSE;

  // Display board and alphabet
  le_bon_mot_window_display_board(self, FALSE, 0);
  le_bon_mot_window_display_alphabet(self);
}

static gboolean le_bon_mot_window_set_label_data (gpointer user_data) {
  labelData *label_data = user_data;

  GString *labelString = g_string_new(NULL);
  g_string_append_c(labelString, label_data->letter->letter);
  gtk_label_set_text(GTK_LABEL (label_data->label), labelString->str);
  g_string_free(labelString, TRUE);

  switch (label_data->letter->state) {
    case LE_BON_MOT_LETTER_NOT_PRESENT:
      gtk_widget_add_css_class(GTK_WIDGET(label_data->label), "not_present");
      break;
    case LE_BON_MOT_LETTER_WELL_PLACED:
      gtk_widget_add_css_class(GTK_WIDGET(label_data->label), "well_placed");
      break;
    case LE_BON_MOT_LETTER_PRESENT:
      gtk_widget_add_css_class(GTK_WIDGET(label_data->label), "present");
      break;
    default:
      break;
  }
  g_free(user_data);
  return G_SOURCE_REMOVE;
}

static gboolean le_bon_mot_window_terminate_validation (gpointer user_data) {
  g_return_val_if_fail(LE_BON_MOT_IS_WINDOW(user_data), G_SOURCE_REMOVE);
  LeBonMotWindow *self = user_data;
  le_bon_mot_window_display_alphabet(self);
  LeBonMotEngineState state = le_bon_mot_engine_get_game_state(self->engine);
  if (state != LE_BON_MOT_ENGINE_STATE_CONTINUE) {
      AdwToast *toast = adw_toast_new(_("Congratulation you won!"));
      adw_toast_set_button_label(toast, _("New game"));
      adw_toast_set_action_name(toast, "game.new");
      if (state == LE_BON_MOT_ENGINE_STATE_LOST) {
        GString *word = le_bon_mot_engine_get_word(self->engine);
        GString *title = g_string_new(NULL);
        if (!title) {
          g_error("Unable to retrieve word from engine.");
        }
        g_string_printf(title, _("Game Over! The word was: %s."), word->str);
        adw_toast_set_title(toast, title->str);
        g_string_free(title, TRUE);
      }
      adw_toast_set_timeout(toast, 0);
      adw_toast_set_priority(toast, ADW_TOAST_PRIORITY_NORMAL);
      adw_toast_overlay_add_toast(self->toast_overlay, toast);
  }
  self->is_validating = FALSE;
  return G_SOURCE_REMOVE;
}

static void
le_bon_mot_window_display_board (
    LeBonMotWindow *self,
    gboolean apply_delay,
    guint delay_on_row)
{
  g_return_if_fail(LE_BON_MOT_IS_WINDOW(self));

  GPtrArray* board = le_bon_mot_engine_get_board_state(self->engine);

  guint longest_delay = 0;
  for (guint rowIndex = 0; rowIndex < board->len; rowIndex +=1 ) {
    GPtrArray *row = g_ptr_array_index(board, rowIndex);
    for (guint columnIndex = 0; columnIndex < row->len; columnIndex += 1) {
      LeBonMotLetter *letter = g_ptr_array_index(row, columnIndex);

      GtkWidget* child = gtk_grid_get_child_at(self->game_grid, columnIndex, rowIndex);
      if (!child) {
        child = gtk_label_new(NULL);
        gtk_widget_add_css_class(child, "card");
        gtk_grid_attach(self->game_grid, child, columnIndex, rowIndex, 1, 1);
      }

      labelData *label_data = g_new(labelData, 1);
      label_data->label = GTK_LABEL(child);
      label_data->letter = letter;

      if (self->is_validating && apply_delay
          && (rowIndex == delay_on_row || rowIndex == delay_on_row + 1)) {
        guint delay = 200 * columnIndex + 200 * row->len * (rowIndex - delay_on_row);
        // Delay display of all letters on delay row
        if (rowIndex == delay_on_row) {
          g_timeout_add(delay, le_bon_mot_window_set_label_data, label_data);
        } else {
          // On row just after, only the first letter need to be delayed
          if (columnIndex == 0) {
            g_timeout_add(delay, le_bon_mot_window_set_label_data, label_data);
            longest_delay = delay + 200;
          } else {
            le_bon_mot_window_set_label_data(label_data);
          }
        }
      } else {
        le_bon_mot_window_set_label_data(label_data);
      }

    }
  }

  // Terminate validation with the current longest delay
  g_timeout_add(longest_delay, le_bon_mot_window_terminate_validation, self);
}

static void
le_bon_mot_window_display_alphabet (
    LeBonMotWindow *self)
{
  g_return_if_fail(LE_BON_MOT_IS_WINDOW(self));

  GPtrArray* alphabet = le_bon_mot_engine_get_alphabet_state(self->engine);

  for (guint alphaIndex = 0; alphaIndex < alphabet->len; alphaIndex +=1 ) {
    LeBonMotLetter *letter = g_ptr_array_index(alphabet, alphaIndex);
    guint rowIndex = alphaIndex / 13;
    guint columnIndex = alphaIndex % 13;
    GtkWidget* child = gtk_grid_get_child_at(self->alphabet_grid, columnIndex, rowIndex);
    if (!child) {
      child = gtk_label_new(NULL);
      gtk_widget_add_css_class(child, "card");
      gtk_grid_attach(self->alphabet_grid, child, columnIndex, rowIndex, 1, 1);
    }

    labelData *label_data = g_new(labelData, 1);
    label_data->label = GTK_LABEL(child);
    label_data->letter = letter;

    le_bon_mot_window_set_label_data(label_data);
  }
}

static void
le_bon_mot_window_on_key_released (
    GtkEventControllerKey *self,
    guint keyval,
    guint keycode,
    GdkModifierType state,
    G_GNUC_UNUSED gpointer user_data)
{
  GtkWidget* widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));
  g_return_if_fail(LE_BON_MOT_IS_WINDOW(widget));

  LeBonMotWindow* window = LE_BON_MOT_WINDOW(widget);

  g_debug("LeBonMotWindow: key released: val: %d, code: %d, name: %s\n", keyval, keycode, gdk_keyval_name(keyval));
  fflush(NULL);

  // Ignore all key sequence with Control key
  if (state & GDK_CONTROL_MASK)
  {
    return;
  }

  // Ignore all keys while validating
  if (window->is_validating)
  {
    return;
  }
  
  const char *keyname = gdk_keyval_name(keyval);
  // Window grabs focus on any recognized input to avoid propagate keyboard
  // event to other widgets (like the main menu button)
  if (g_regex_match_simple(
        "^[a-z](acute|diaeresis|grave)?$",
        keyname,
        G_REGEX_CASELESS,
        0)) {
    le_bon_mot_engine_add_letter(window->engine, keyname);
    le_bon_mot_window_display_board(window, FALSE, 0);
    gtk_widget_grab_focus(widget);
  } else if (strcmp(keyname, "BackSpace") == 0) {
    le_bon_mot_engine_remove_letter(window->engine);
    le_bon_mot_window_display_board(window, FALSE, 0);
    gtk_widget_grab_focus(widget);
  } else if (strcmp(keyname, "Return") == 0) {
    guint delay_on_row = le_bon_mot_engine_get_current_row(window->engine);
    window->is_validating = TRUE;
    GError *error = NULL;
    le_bon_mot_engine_validate(window->engine, &error);
    if (error) {
      AdwToast *toast = adw_toast_new(error->message);
      adw_toast_set_timeout(toast, 2);
      adw_toast_set_priority(toast, ADW_TOAST_PRIORITY_NORMAL);
      adw_toast_overlay_add_toast(window->toast_overlay, toast);
      window->is_validating = FALSE;
    } else {
      le_bon_mot_window_display_board(window, TRUE, delay_on_row);
    }
    gtk_widget_grab_focus(widget);
  }
}
