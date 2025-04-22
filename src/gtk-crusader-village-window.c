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

#include "gtk-crusader-village-brush-area.h"
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

struct _GcvWindow
{
  GtkApplicationWindow parent_instance;

  GSettings *settings;

  GcvItemStore *item_store;
  GListStore   *brush_store;
  GcvMap       *map;
  GcvMapHandle *map_handle;

  /* Template widgets */
  GcvMapEditor       *map_editor;
  GcvMapEditorStatus *map_editor_status;
  GcvItemArea        *item_area;
  GcvTimelineView    *timeline_view;
  GcvBrushArea       *brush_area;
  GtkWidget          *busy;
};

G_DEFINE_FINAL_TYPE (GcvWindow, gcv_window, GTK_TYPE_APPLICATION_WINDOW)

enum
{
  PROP_0,

  PROP_ITEM_STORE,
  PROP_BRUSH_STORE,
  PROP_MAP,
  PROP_SETTINGS,
  PROP_BUSY,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
gcv_window_dispose (GObject *object)
{
  GcvWindow *self = GCV_WINDOW (object);

  g_clear_object (&self->settings);
  g_clear_object (&self->item_store);
  g_clear_object (&self->brush_store);
  g_clear_object (&self->map);
  g_clear_object (&self->map_handle);

  G_OBJECT_CLASS (gcv_window_parent_class)->dispose (object);
}

static void
gcv_window_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  GcvWindow *self = GCV_WINDOW (object);

  switch (prop_id)
    {
    case PROP_ITEM_STORE:
      g_value_set_object (value, self->item_store);
      break;
    case PROP_BRUSH_STORE:
      g_value_set_object (value, self->brush_store);
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
gcv_window_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GcvWindow *self = GCV_WINDOW (object);

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
    case PROP_BRUSH_STORE:
      g_clear_object (&self->brush_store);
      self->brush_store = g_value_dup_object (value);
      g_object_set (
          self->brush_area,
          "brush-store", self->brush_store,
          NULL);
      break;
    case PROP_MAP:
      g_clear_object (&self->map);
      g_clear_object (&self->map_handle);
      self->map        = g_value_dup_object (value);
      self->map_handle = g_object_new (
          GCV_TYPE_MAP_HANDLE,
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
          self->item_area,
          "settings", self->settings,
          NULL);
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
gcv_window_class_init (GcvWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gcv_window_dispose;
  object_class->get_property = gcv_window_get_property;
  object_class->set_property = gcv_window_set_property;

  props[PROP_ITEM_STORE] =
      g_param_spec_object (
          "item-store",
          "Item Store",
          "The item store object for this window",
          GCV_TYPE_ITEM_STORE,
          G_PARAM_READWRITE);

  props[PROP_BRUSH_STORE] =
      g_param_spec_object (
          "brush-store",
          "Brush Store",
          "The brush store object for this window",
          G_TYPE_LIST_STORE,
          G_PARAM_READWRITE);

  props[PROP_MAP] =
      g_param_spec_object (
          "map",
          "Map",
          "The main map this window is handling",
          GCV_TYPE_MAP,
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

  g_type_ensure (GCV_TYPE_MAP_EDITOR);
  g_type_ensure (GCV_TYPE_MAP_EDITOR_OVERLAY);
  g_type_ensure (GCV_TYPE_MAP_EDITOR_STATUS);
  g_type_ensure (GCV_TYPE_ITEM_AREA);
  g_type_ensure (GCV_TYPE_TIMELINE_VIEW);
  g_type_ensure (GCV_TYPE_BRUSH_AREA);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/Gcv/gtk-crusader-village-window.ui");
  gtk_widget_class_bind_template_child (widget_class, GcvWindow, map_editor);
  gtk_widget_class_bind_template_child (widget_class, GcvWindow, map_editor_status);
  gtk_widget_class_bind_template_child (widget_class, GcvWindow, item_area);
  gtk_widget_class_bind_template_child (widget_class, GcvWindow, timeline_view);
  gtk_widget_class_bind_template_child (widget_class, GcvWindow, brush_area);
  gtk_widget_class_bind_template_child (widget_class, GcvWindow, busy);
}

static void
gcv_window_init (GcvWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  gcv_register_themed_window (GTK_WINDOW (self), FALSE);

  self->map = g_object_new (
      GCV_TYPE_MAP,
      "name", "Untitled",
      NULL);
  self->map_handle = g_object_new (
      GCV_TYPE_MAP_HANDLE,
      "map", self->map,
      NULL);

  g_object_set (
      self->map_editor,
      "map-handle", self->map_handle,
      "item-area", self->item_area,
      "brush-area", self->brush_area,
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

void
gcv_window_add_subwindow_viewport (GcvWindow *self)
{
  GcvMapEditor       *editor          = NULL;
  GtkWidget          *scrolled_window = NULL;
  GtkWidget          *separator       = NULL;
  GcvMapEditorStatus *status          = NULL;
  GtkWidget          *box             = NULL;
  GtkWidget          *window          = NULL;

  editor          = g_object_new (GCV_TYPE_MAP_EDITOR, NULL);
  scrolled_window = gtk_scrolled_window_new ();
  separator       = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  status          = g_object_new (GCV_TYPE_MAP_EDITOR_STATUS, NULL);
  box             = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  window          = gtk_window_new ();

  gtk_widget_set_vexpand (GTK_WIDGET (editor), TRUE);
  g_object_bind_property (self->map_editor, "settings", editor, "settings", G_BINDING_SYNC_CREATE);
  g_object_bind_property (self->map_editor, "map-handle", editor, "map-handle", G_BINDING_SYNC_CREATE);
  g_object_bind_property (self->map_editor, "item-area", editor, "item-area", G_BINDING_SYNC_CREATE);
  g_object_bind_property (self->map_editor, "brush-area", editor, "brush-area", G_BINDING_SYNC_CREATE);
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled_window), GTK_WIDGET (editor));

  g_object_set (
      status,
      "editor", editor,
      NULL);

  gtk_box_append (GTK_BOX (box), scrolled_window);
  gtk_box_append (GTK_BOX (box), separator);
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (status));

  gtk_window_set_title (GTK_WINDOW (window), "GTK Crusader Village Viewport");
  gtk_window_set_default_size (GTK_WINDOW (window), 800, 800);
  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (self));
  gcv_register_themed_window (GTK_WINDOW (window), FALSE);
  gtk_window_set_child (GTK_WINDOW (window), GTK_WIDGET (box));

  gtk_window_present (GTK_WINDOW (window));
}
