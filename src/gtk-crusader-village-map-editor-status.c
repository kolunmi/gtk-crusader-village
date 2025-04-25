/* gtk-crusader-village-map-editor-status.c
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

#include "gtk-crusader-village-map-editor-status.h"
#include "gtk-crusader-village-map-editor.h"
#include "gtk-crusader-village-map-handle.h"
#include "gtk-crusader-village-map.h"

struct _GcvMapEditorStatus
{
  GcvUtilBin parent_instance;

  GcvMap       *map;
  GcvMapEditor *editor;

  /* Template widgets */
  GtkLabel *name_label;
  GtkLabel *hover_label;
};

G_DEFINE_FINAL_TYPE (GcvMapEditorStatus, gcv_map_editor_status, GCV_TYPE_UTIL_BIN)

enum
{
  PROP_0,

  PROP_EDITOR,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
hover_changed (GcvMapEditor       *editor,
               GParamSpec         *pspec,
               GcvMapEditorStatus *status);

static void
map_changed (GcvMapEditor       *editor,
             GParamSpec         *pspec,
             GcvMapEditorStatus *status);

static void
name_changed (GcvMap             *map,
              GParamSpec         *pspec,
              GcvMapEditorStatus *status);

static void
update_hover (GcvMapEditorStatus *self,
              int                 hover_x,
              int                 hover_y);

static void
read_map (GcvMapEditorStatus *self);

static void
update_name (GcvMapEditorStatus *self);

static void
gcv_map_editor_status_dispose (GObject *object)
{
  GcvMapEditorStatus *self = GCV_MAP_EDITOR_STATUS (object);

  if (self->editor != NULL)
    {
      g_signal_handlers_disconnect_by_func (self->editor, hover_changed, self);
      g_signal_handlers_disconnect_by_func (self->editor, map_changed, self);
    }
  g_clear_object (&self->editor);

  if (self->map != NULL)
    g_signal_handlers_disconnect_by_func (self->map, name_changed, self);
  g_clear_object (&self->map);

  G_OBJECT_CLASS (gcv_map_editor_status_parent_class)->dispose (object);
}

static void
gcv_map_editor_status_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GcvMapEditorStatus *self = GCV_MAP_EDITOR_STATUS (object);

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
gcv_map_editor_status_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GcvMapEditorStatus *self = GCV_MAP_EDITOR_STATUS (object);

  switch (prop_id)
    {
    case PROP_EDITOR:
      {
        if (self->editor != NULL)
          {
            g_signal_handlers_disconnect_by_func (self->editor, hover_changed, self);
            g_signal_handlers_disconnect_by_func (self->editor, map_changed, self);
          }
        g_clear_object (&self->editor);

        if (self->map != NULL)
          g_signal_handlers_disconnect_by_func (self->map, name_changed, self);
        g_clear_object (&self->map);

        self->editor = g_value_dup_object (value);

        if (self->editor != NULL)
          {
            int hover_x = 0;
            int hover_y = 0;

            g_object_get (
                self->editor,
                "hover-x", &hover_x,
                "hover-y", &hover_y,
                NULL);

            update_hover (self, hover_x, hover_y);
            g_signal_connect (self->editor, "notify::hover-x",
                              G_CALLBACK (hover_changed), self);
            g_signal_connect (self->editor, "notify::hover-y",
                              G_CALLBACK (hover_changed), self);

            read_map (self);
            g_signal_connect (self->editor, "notify::map-handle",
                              G_CALLBACK (map_changed), self);
          }
        else
          {
            gtk_label_set_label (self->name_label, "---");
            gtk_label_set_label (self->hover_label, "---");
          }
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gcv_map_editor_status_class_init (GcvMapEditorStatusClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gcv_map_editor_status_dispose;
  object_class->get_property = gcv_map_editor_status_get_property;
  object_class->set_property = gcv_map_editor_status_set_property;

  props[PROP_EDITOR] =
      g_param_spec_object (
          "editor",
          "Editor",
          "The map editor this widget will use",
          GCV_TYPE_MAP_EDITOR,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/Gcv/gtk-crusader-village-map-editor-status.ui");
  gtk_widget_class_bind_template_child (widget_class, GcvMapEditorStatus, name_label);
  gtk_widget_class_bind_template_child (widget_class, GcvMapEditorStatus, hover_label);
}

static void
gcv_map_editor_status_init (GcvMapEditorStatus *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
hover_changed (GcvMapEditor       *editor,
               GParamSpec         *pspec,
               GcvMapEditorStatus *status)
{
  int hover_x = 0;
  int hover_y = 0;

  g_object_get (
      editor,
      "hover-x", &hover_x,
      "hover-y", &hover_y,
      NULL);

  update_hover (status, hover_x, hover_y);
}

static void
map_changed (GcvMapEditor       *editor,
             GParamSpec         *pspec,
             GcvMapEditorStatus *status)
{
  read_map (status);
}

static void
name_changed (GcvMap             *map,
              GParamSpec         *pspec,
              GcvMapEditorStatus *status)
{
  update_name (status);
}

static void
update_hover (GcvMapEditorStatus *self,
              int                 hover_x,
              int                 hover_y)
{
  char buf[64] = { 0 };

  if (hover_x < 0 || hover_y < 0)
    g_snprintf (buf, sizeof (buf), "---");
  else
    g_snprintf (buf, sizeof (buf), "X=%d Y=%d", hover_x, hover_y);

  gtk_label_set_label (self->hover_label, buf);
}

static void
read_map (GcvMapEditorStatus *self)
{
  g_autoptr (GcvMapHandle) handle = NULL;

  if (self->map != NULL)
    g_signal_handlers_disconnect_by_func (self->map, name_changed, self);
  g_clear_object (&self->map);

  if (self->editor == NULL)
    return;

  g_object_get (
      self->editor,
      "map-handle", &handle,
      NULL);

  if (handle != NULL)
    g_object_get (
        handle,
        "map", &self->map,
        NULL);

  update_name (self);

  if (self->map != NULL)
    g_signal_connect (self->map, "notify::name",
                      G_CALLBACK (name_changed), self);
}

static void
update_name (GcvMapEditorStatus *self)
{
  if (self->map != NULL)
    {
      g_autofree char *name = NULL;

      g_object_get (
          self->map,
          "name", &name,
          NULL);
      gtk_label_set_label (self->name_label, name);
    }
  else
    {
      gtk_label_set_label (self->name_label, "---");
    }
}
