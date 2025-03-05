/* gtk-crusader-village-window.c
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

#include "gtk-crusader-village-item-area.h"
#include "gtk-crusader-village-item-store.h"
#include "gtk-crusader-village-map-editor-overlay.h"
#include "gtk-crusader-village-map-editor-status.h"
#include "gtk-crusader-village-map-editor.h"
#include "gtk-crusader-village-map-handle.h"
#include "gtk-crusader-village-map.h"
#include "gtk-crusader-village-timeline-view.h"
#include "gtk-crusader-village-window.h"

struct _GtkCrusaderVillageWindow
{
  GtkApplicationWindow parent_instance;

  GSettings *settings;

  GtkCrusaderVillageItemStore *item_store;
  GtkCrusaderVillageMap       *map;
  GtkCrusaderVillageMapHandle *map_handle;

  /* Template widgets */
  GtkCrusaderVillageMapEditor       *map_editor;
  GtkCrusaderVillageMapEditorStatus *map_editor_status;
  GtkCrusaderVillageItemArea        *item_area;
  GtkCrusaderVillageTimelineView    *timeline_view;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageWindow, gtk_crusader_village_window, GTK_TYPE_APPLICATION_WINDOW)

enum
{
  PROP_0,

  PROP_SETTINGS,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
gtk_crusader_village_window_dispose (GObject *object)
{
  GtkCrusaderVillageWindow *self = GTK_CRUSADER_VILLAGE_WINDOW (object);

  g_clear_object (&self->settings);
  g_clear_object (&self->item_store);
  g_clear_object (&self->map);
  g_clear_object (&self->map_handle);

  G_OBJECT_CLASS (gtk_crusader_village_window_parent_class)->dispose (object);
}

static void
gtk_crusader_village_window_get_property (GObject    *object,
                                          guint       prop_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  GtkCrusaderVillageWindow *self = GTK_CRUSADER_VILLAGE_WINDOW (object);

  switch (prop_id)
    {
    case PROP_SETTINGS:
      g_value_set_object (value, self->settings);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_window_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  GtkCrusaderVillageWindow *self = GTK_CRUSADER_VILLAGE_WINDOW (object);

  switch (prop_id)
    {
    case PROP_SETTINGS:
      g_clear_object (&self->settings);
      self->settings = g_value_dup_object (value);
      g_object_set (
          self->map_editor,
          "settings", self->settings,
          NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_window_class_init (GtkCrusaderVillageWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gtk_crusader_village_window_dispose;
  object_class->get_property = gtk_crusader_village_window_get_property;
  object_class->set_property = gtk_crusader_village_window_set_property;

  props[PROP_SETTINGS] =
      g_param_spec_object (
          "settings",
          "Settings",
          "The settings object for this window",
          G_TYPE_SETTINGS,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  g_type_ensure (GTK_CRUSADER_VILLAGE_TYPE_MAP_EDITOR);
  g_type_ensure (GTK_CRUSADER_VILLAGE_TYPE_MAP_EDITOR_OVERLAY);
  g_type_ensure (GTK_CRUSADER_VILLAGE_TYPE_MAP_EDITOR_STATUS);
  g_type_ensure (GTK_CRUSADER_VILLAGE_TYPE_ITEM_AREA);
  g_type_ensure (GTK_CRUSADER_VILLAGE_TYPE_TIMELINE_VIEW);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/GtkCrusaderVillage/gtk-crusader-village-window.ui");
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageWindow, map_editor);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageWindow, map_editor_status);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageWindow, item_area);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageWindow, timeline_view);
}

static void
gtk_crusader_village_window_init (GtkCrusaderVillageWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#ifdef DEVELOPMENT_BUILD
  gtk_widget_add_css_class (GTK_WIDGET (self), "devel");
#endif

  self->item_store = g_object_new (GTK_CRUSADER_VILLAGE_TYPE_ITEM_STORE, NULL);
  gtk_crusader_village_item_store_read_resources (self->item_store);

  g_object_set (
      self->item_area,
      "item-store", self->item_store,
      NULL);

  self->map = g_object_new (
      GTK_CRUSADER_VILLAGE_TYPE_MAP,
      "name", "Untitled",
      "width", 32,
      "height", 32,
      NULL);
  self->map_handle = g_object_new (
      GTK_CRUSADER_VILLAGE_TYPE_MAP_HANDLE,
      "map", self->map,
      NULL);

  g_object_set (
      self->map_editor,
      "map", self->map,
      "item-area", self->item_area,
      NULL);

  g_object_set (
      self->timeline_view,
      "map-handle", self->map_handle,
      NULL);

  g_object_set (
      self->map_editor_status,
      "editor", self->map_editor,
      NULL);
}
