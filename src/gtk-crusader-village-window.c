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
#include "gtk-crusader-village-theme-utils.h"
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
  GtkWidget                         *busy;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageWindow, gtk_crusader_village_window, GTK_TYPE_APPLICATION_WINDOW)

enum
{
  PROP_0,

  PROP_ITEM_STORE,
  PROP_MAP,
  PROP_SETTINGS,
  PROP_BUSY,

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
    case PROP_ITEM_STORE:
      g_value_set_object (value, self->item_store);
      break;
    case PROP_MAP:
      g_value_set_object (value, self->map);
      break;
    case PROP_SETTINGS:
      g_value_set_object (value, self->settings);
      break;
    case PROP_BUSY:
      g_value_set_boolean (value, gtk_widget_get_visible (self->busy));
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
    case PROP_ITEM_STORE:
      g_clear_object (&self->item_store);
      self->item_store = g_value_dup_object (value);
      g_object_set (
          self->item_area,
          "item-store", self->item_store,
          NULL);
      break;
    case PROP_MAP:
      g_clear_object (&self->map);
      g_clear_object (&self->map_handle);
      self->map        = g_value_dup_object (value);
      self->map_handle = g_object_new (
          GTK_CRUSADER_VILLAGE_TYPE_MAP_HANDLE,
          "map", self->map,
          NULL);
      g_object_set (
          self->map_editor,
          "map-handle", self->map_handle,
          NULL);
      g_object_set (
          self->timeline_view,
          "map-handle", self->map_handle,
          NULL);
      break;
    case PROP_SETTINGS:
      g_clear_object (&self->settings);
      self->settings = g_value_dup_object (value);
      g_object_set (
          self->map_editor,
          "settings", self->settings,
          NULL);
      break;
    case PROP_BUSY:
      gtk_widget_set_visible (self->busy, g_value_get_boolean (value));
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

  props[PROP_ITEM_STORE] =
      g_param_spec_object (
          "item-store",
          "Item Store",
          "The item store object for this window",
          GTK_CRUSADER_VILLAGE_TYPE_ITEM_STORE,
          G_PARAM_READWRITE);

  props[PROP_MAP] =
      g_param_spec_object (
          "map",
          "Map",
          "The main map this window is handling",
          GTK_CRUSADER_VILLAGE_TYPE_MAP,
          G_PARAM_READWRITE);

  props[PROP_SETTINGS] =
      g_param_spec_object (
          "settings",
          "Settings",
          "The settings object for this window",
          G_TYPE_SETTINGS,
          G_PARAM_READWRITE);

  props[PROP_BUSY] =
      g_param_spec_boolean (
          "busy",
          "Busy",
          "Whether to indicate that the application is busy",
          FALSE,
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
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageWindow, busy);
}

static void
gtk_crusader_village_window_init (GtkCrusaderVillageWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  gtk_crusader_village_register_themed_window (GTK_WINDOW (self), FALSE);

  self->map = g_object_new (
      GTK_CRUSADER_VILLAGE_TYPE_MAP,
      "name", "Untitled",
      NULL);
  self->map_handle = g_object_new (
      GTK_CRUSADER_VILLAGE_TYPE_MAP_HANDLE,
      "map", self->map,
      NULL);

  g_object_set (
      self->map_editor,
      "map-handle", self->map_handle,
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
