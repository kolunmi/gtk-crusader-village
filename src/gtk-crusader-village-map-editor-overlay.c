/* gtk-crusader-village-map-editor-overlay.c
 *
 * Copyright 2025 Adam Masciola
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include "gtk-crusader-village-map-editor-overlay.h"
#include "gtk-crusader-village-map-editor.h"
#include "gtk-crusader-village-map.h"

struct _GtkCrusaderVillageMapEditorOverlay
{
  GtkCrusaderVillageUtilBin parent_instance;

  GtkCrusaderVillageMapEditor *editor;
  GtkCrusaderVillageMap       *map;
  GListStore                  *strokes;

  /* Template widgets */
  GtkOverlay        *overlay;
  GtkScrolledWindow *scrolled_window;
  GtkButton         *select;
  GtkButton         *select_all;
  GtkButton         *copy;
  GtkButton         *cut;
  GtkButton         *paste;
  GtkButton         *clear;
  GtkButton         *undo;
  GtkButton         *redo;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageMapEditorOverlay, gtk_crusader_village_map_editor_overlay, GTK_CRUSADER_VILLAGE_TYPE_UTIL_BIN)

enum
{
  PROP_0,

  PROP_EDITOR,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
map_changed (GtkCrusaderVillageMapEditor        *editor,
             GParamSpec                         *pspec,
             GtkCrusaderVillageMapEditorOverlay *overlay);

static void
strokes_changed (GListModel                         *self,
                 guint                               position,
                 guint                               removed,
                 guint                               added,
                 GtkCrusaderVillageMapEditorOverlay *timeline_view);

static void
read_map (GtkCrusaderVillageMapEditorOverlay *self);

static void
update_strokes_ui (GtkCrusaderVillageMapEditorOverlay *self);

static void
gtk_crusader_village_map_editor_overlay_dispose (GObject *object)
{
  GtkCrusaderVillageMapEditorOverlay *self = GTK_CRUSADER_VILLAGE_MAP_EDITOR_OVERLAY (object);

  if (self->editor != NULL)
    g_signal_handlers_disconnect_by_func (self->editor, map_changed, self);
  g_clear_object (&self->editor);

  g_clear_object (&self->map);
  if (self->strokes != NULL)
    g_signal_handlers_disconnect_by_func (self->strokes, strokes_changed, self);
  g_clear_object (&self->strokes);

  G_OBJECT_CLASS (gtk_crusader_village_map_editor_overlay_parent_class)->dispose (object);
}

static void
gtk_crusader_village_map_editor_overlay_get_property (GObject    *object,
                                                      guint       prop_id,
                                                      GValue     *value,
                                                      GParamSpec *pspec)
{
  GtkCrusaderVillageMapEditorOverlay *self = GTK_CRUSADER_VILLAGE_MAP_EDITOR_OVERLAY (object);

  switch (prop_id)
    {
    case PROP_EDITOR:
      g_value_set_object (value, self->editor);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_map_editor_overlay_set_property (GObject      *object,
                                                      guint         prop_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec)
{
  GtkCrusaderVillageMapEditorOverlay *self = GTK_CRUSADER_VILLAGE_MAP_EDITOR_OVERLAY (object);

  switch (prop_id)
    {
    case PROP_EDITOR:
      {
        if (self->editor != NULL)
          g_signal_handlers_disconnect_by_func (self->editor, map_changed, self);
        g_clear_object (&self->editor);

        g_clear_object (&self->map);
        if (self->strokes != NULL)
          g_signal_handlers_disconnect_by_func (self->strokes, strokes_changed, self);
        g_clear_object (&self->strokes);

        self->editor = g_value_dup_object (value);

        if (self->editor != NULL)
          {
            read_map (self);
            g_signal_connect (self->editor, "notify::map",
                              G_CALLBACK (map_changed), self);
          }
        else
          update_strokes_ui (self);

        gtk_scrolled_window_set_child (self->scrolled_window, GTK_WIDGET (self->editor));
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_map_editor_overlay_class_init (GtkCrusaderVillageMapEditorOverlayClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gtk_crusader_village_map_editor_overlay_dispose;
  object_class->get_property = gtk_crusader_village_map_editor_overlay_get_property;
  object_class->set_property = gtk_crusader_village_map_editor_overlay_set_property;

  props[PROP_EDITOR] =
      g_param_spec_object (
          "editor",
          "Editor",
          "The map editor this widget will use",
          GTK_CRUSADER_VILLAGE_TYPE_MAP_EDITOR,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/GtkCrusaderVillage/gtk-crusader-village-map-editor-overlay.ui");
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageMapEditorOverlay, overlay);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageMapEditorOverlay, scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageMapEditorOverlay, select);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageMapEditorOverlay, select_all);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageMapEditorOverlay, copy);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageMapEditorOverlay, cut);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageMapEditorOverlay, paste);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageMapEditorOverlay, clear);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageMapEditorOverlay, undo);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageMapEditorOverlay, redo);
}

static void
gtk_crusader_village_map_editor_overlay_init (GtkCrusaderVillageMapEditorOverlay *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
map_changed (GtkCrusaderVillageMapEditor        *editor,
             GParamSpec                         *pspec,
             GtkCrusaderVillageMapEditorOverlay *overlay)
{
  read_map (overlay);
}

static void
strokes_changed (GListModel                         *self,
                 guint                               position,
                 guint                               removed,
                 guint                               added,
                 GtkCrusaderVillageMapEditorOverlay *overlay)
{
  update_strokes_ui (overlay);
}

static void
read_map (GtkCrusaderVillageMapEditorOverlay *self)
{
  g_clear_object (&self->map);
  if (self->strokes != NULL)
    g_signal_handlers_disconnect_by_func (self->strokes, strokes_changed, self);
  g_clear_object (&self->strokes);

  if (self->editor == NULL)
    return;

  g_object_get (
      self->editor,
      "map", &self->map,
      NULL);

  if (self->map != NULL)
    {
      g_object_get (
          self->map,
          "strokes", &self->strokes,
          NULL);
      g_assert (self->strokes != NULL);

      g_signal_connect (self->strokes, "items-changed",
                        G_CALLBACK (strokes_changed), self);
    }

  update_strokes_ui (self);
}

static void
update_strokes_ui (GtkCrusaderVillageMapEditorOverlay *self)
{
  if (self->strokes != NULL)
    {
      /* TEMP */
      guint n_strokes = 0;

      n_strokes = g_list_model_get_n_items (G_LIST_MODEL (self->strokes));

      gtk_widget_set_sensitive (GTK_WIDGET (self->undo), n_strokes > 0);
      gtk_widget_set_sensitive (GTK_WIDGET (self->redo), FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (self->undo), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (self->redo), FALSE);
    }
}
