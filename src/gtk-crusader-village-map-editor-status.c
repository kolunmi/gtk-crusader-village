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
#include "gtk-crusader-village-map.h"

struct _GtkCrusaderVillageMapEditorStatus
{
  GtkCrusaderVillageUtilBin parent_instance;

  GtkCrusaderVillageMapEditor *editor;

  /* Template widgets */
  GtkLabel *name_label;
  GtkLabel *hover_label;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageMapEditorStatus, gtk_crusader_village_map_editor_status, GTK_CRUSADER_VILLAGE_TYPE_UTIL_BIN)

enum
{
  PROP_0,

  PROP_EDITOR,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
hover_changed (GtkCrusaderVillageMapEditor       *editor,
               GParamSpec                        *pspec,
               GtkCrusaderVillageMapEditorStatus *status);

static void
update_hover (GtkCrusaderVillageMapEditorStatus *self,
              int                                hover_x,
              int                                hover_y);

static void
gtk_crusader_village_map_editor_status_dispose (GObject *object)
{
  GtkCrusaderVillageMapEditorStatus *self = GTK_CRUSADER_VILLAGE_MAP_EDITOR_STATUS (object);

  if (self->editor != NULL)
    g_signal_handlers_disconnect_by_func (self->editor, hover_changed, self);
  g_clear_object (&self->editor);

  G_OBJECT_CLASS (gtk_crusader_village_map_editor_status_parent_class)->dispose (object);
}

static void
gtk_crusader_village_map_editor_status_get_property (GObject    *object,
                                                     guint       prop_id,
                                                     GValue     *value,
                                                     GParamSpec *pspec)
{
  GtkCrusaderVillageMapEditorStatus *self = GTK_CRUSADER_VILLAGE_MAP_EDITOR_STATUS (object);

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
gtk_crusader_village_map_editor_status_set_property (GObject      *object,
                                                     guint         prop_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec)
{
  GtkCrusaderVillageMapEditorStatus *self = GTK_CRUSADER_VILLAGE_MAP_EDITOR_STATUS (object);

  switch (prop_id)
    {
    case PROP_EDITOR:
      {
        if (self->editor != NULL)
          g_signal_handlers_disconnect_by_func (self->editor, hover_changed, self);
        g_clear_object (&self->editor);

        self->editor = g_value_dup_object (value);

        if (self->editor != NULL)
          {
            int hover_x                           = 0;
            int hover_y                           = 0;
            g_autoptr (GtkCrusaderVillageMap) map = NULL;
            g_autofree char *name                 = NULL;

            g_object_get (
                self->editor,
                "hover-x", &hover_x,
                "hover-y", &hover_y,
                "map", &map,
                NULL);

            g_object_get (
                map,
                "name", &name,
                NULL);

            gtk_label_set_label (self->name_label, name);
            update_hover (self, hover_x, hover_y);

            g_signal_connect (self->editor, "notify::hover-x",
                              G_CALLBACK (hover_changed), self);
            g_signal_connect (self->editor, "notify::hover-y",
                              G_CALLBACK (hover_changed), self);
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
gtk_crusader_village_map_editor_status_class_init (GtkCrusaderVillageMapEditorStatusClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gtk_crusader_village_map_editor_status_dispose;
  object_class->get_property = gtk_crusader_village_map_editor_status_get_property;
  object_class->set_property = gtk_crusader_village_map_editor_status_set_property;

  props[PROP_EDITOR] =
      g_param_spec_object (
          "editor",
          "Editor",
          "The map editor this widget will use",
          GTK_CRUSADER_VILLAGE_TYPE_MAP_EDITOR,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/GtkCrusaderVillage/gtk-crusader-village-map-editor-status.ui");
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageMapEditorStatus, name_label);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageMapEditorStatus, hover_label);
}

static void
gtk_crusader_village_map_editor_status_init (GtkCrusaderVillageMapEditorStatus *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
hover_changed (GtkCrusaderVillageMapEditor       *editor,
               GParamSpec                        *pspec,
               GtkCrusaderVillageMapEditorStatus *status)
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
update_hover (GtkCrusaderVillageMapEditorStatus *self,
              int                                hover_x,
              int                                hover_y)
{
  char buf[64] = { 0 };

  if (hover_x < 0 || hover_y < 0)
    g_snprintf (buf, sizeof (buf), "---");
  else
    g_snprintf (buf, sizeof (buf), "X=%d Y=%d", hover_x, hover_y);

  gtk_label_set_label (self->hover_label, buf);
}
