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
le_bon_mot_window_init (LeBonMotWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->engine = g_object_new(LE_BON_MOT_TYPE_ENGINE, NULL);
  le_bon_mot_engine_show_board (self->engine, self->game_grid);
}

