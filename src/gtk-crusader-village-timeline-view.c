/* gtk-crusader-village-timeline-view.c
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

#include "gtk-crusader-village-item-stroke.h"
#include "gtk-crusader-village-map-handle.h"
#include "gtk-crusader-village-timeline-view-item.h"
#include "gtk-crusader-village-timeline-view.h"

struct _GtkCrusaderVillageTimelineView
{
  GtkCrusaderVillageUtilBin parent_instance;

  GtkSingleSelection *selection;
  GListStore         *wrapper_store;

  GtkCrusaderVillageMapHandle *handle;
  GListModel                  *model;

  GBinding *insert_mode_binding;

  /* Template widgets */
  GtkLabel       *stats;
  GtkListView    *list_view;
  GtkCheckButton *insert_mode;
  GtkButton      *delete_stroke;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageTimelineView, gtk_crusader_village_timeline_view, GTK_CRUSADER_VILLAGE_TYPE_UTIL_BIN)

enum
{
  PROP_0,

  PROP_MAP_HANDLE,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
setup_listitem (GtkListItemFactory             *factory,
                GtkListItem                    *list_item,
                GtkCrusaderVillageTimelineView *self);

static void
bind_listitem (GtkListItemFactory             *factory,
               GtkListItem                    *list_item,
               GtkCrusaderVillageTimelineView *self);

static void
unbind_listitem (GtkListItemFactory             *factory,
                 GtkListItem                    *list_item,
                 GtkCrusaderVillageTimelineView *self);

static void
listitem_cursor_changed (GtkCrusaderVillageMapHandle *handle,
                         GParamSpec                  *pspec,
                         GtkListItem                 *list_item);

static void
listitem_mode_hint_changed (GtkCrusaderVillageMapHandle *handle,
                            GParamSpec                  *pspec,
                            GtkListItem                 *list_item);
static void
row_activated (GtkListView                    *self,
               guint                           position,
               GtkCrusaderVillageTimelineView *timeline_view);

static void
delete_stroke_clicked (GtkButton                      *self,
                       GtkCrusaderVillageTimelineView *timeline_view);

static void
cursor_changed (GtkCrusaderVillageMapHandle    *handle,
                GParamSpec                     *pspec,
                GtkCrusaderVillageTimelineView *timeline_view);

static void
lock_hint_changed (GtkCrusaderVillageMapHandle    *handle,
                   GParamSpec                     *pspec,
                   GtkCrusaderVillageTimelineView *timeline_view);

static void
update_selected (GtkCrusaderVillageTimelineView *self);

static void
update_ui (GtkCrusaderVillageTimelineView *self);

static void
gtk_crusader_village_timeline_view_dispose (GObject *object)
{
  GtkCrusaderVillageTimelineView *self = GTK_CRUSADER_VILLAGE_TIMELINE_VIEW (object);

  g_clear_object (&self->selection);
  g_clear_object (&self->wrapper_store);
  g_clear_object (&self->model);

  if (self->handle != NULL)
    {
      g_signal_handlers_disconnect_by_func (
          self->handle, cursor_changed, self);
      g_signal_handlers_disconnect_by_func (
          self->handle, lock_hint_changed, self);
      g_binding_unbind (self->insert_mode_binding);
    }
  g_clear_object (&self->handle);

  G_OBJECT_CLASS (gtk_crusader_village_timeline_view_parent_class)->dispose (object);
}

static void
gtk_crusader_village_timeline_view_get_property (GObject    *object,
                                                 guint       prop_id,
                                                 GValue     *value,
                                                 GParamSpec *pspec)
{
  GtkCrusaderVillageTimelineView *self = GTK_CRUSADER_VILLAGE_TIMELINE_VIEW (object);

  switch (prop_id)
    {
    case PROP_MAP_HANDLE:
      g_value_set_object (value, self->handle);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_timeline_view_set_property (GObject      *object,
                                                 guint         prop_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec)
{
  GtkCrusaderVillageTimelineView *self = GTK_CRUSADER_VILLAGE_TIMELINE_VIEW (object);

  switch (prop_id)
    {
    case PROP_MAP_HANDLE:
      {
        g_list_store_remove_all (self->wrapper_store);
        g_clear_object (&self->model);

        if (self->handle != NULL)
          {
            g_signal_handlers_disconnect_by_func (
                self->handle, cursor_changed, self);
            g_signal_handlers_disconnect_by_func (
                self->handle, lock_hint_changed, self);
            g_binding_unbind (self->insert_mode_binding);
          }
        g_clear_object (&self->handle);

        self->handle = g_value_dup_object (value);

        if (self->handle != NULL)
          {
            g_object_get (
                self->handle,
                "model", &self->model,
                NULL);

            g_list_store_append (self->wrapper_store, self->model);

            g_signal_connect (self->handle, "notify::cursor",
                              G_CALLBACK (cursor_changed), self);
            g_signal_connect (self->handle, "notify::lock-hinted",
                              G_CALLBACK (lock_hint_changed), self);
            self->insert_mode_binding = g_object_bind_property (
                self->handle, "insert-mode",
                self->insert_mode, "active",
                G_BINDING_BIDIRECTIONAL);
          }

        update_ui (self);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_timeline_view_class_init (GtkCrusaderVillageTimelineViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gtk_crusader_village_timeline_view_dispose;
  object_class->get_property = gtk_crusader_village_timeline_view_get_property;
  object_class->set_property = gtk_crusader_village_timeline_view_set_property;

  props[PROP_MAP_HANDLE] =
      g_param_spec_object (
          "map-handle",
          "Map Handle",
          "The map handle this widget will use",
          GTK_CRUSADER_VILLAGE_TYPE_MAP_HANDLE,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/GtkCrusaderVillage/gtk-crusader-village-timeline-view.ui");
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageTimelineView, stats);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageTimelineView, list_view);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageTimelineView, insert_mode);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageTimelineView, delete_stroke);
}

static void
gtk_crusader_village_timeline_view_init (GtkCrusaderVillageTimelineView *self)
{
  g_autoptr (GtkListItemFactory) factory      = NULL;
  g_autoptr (GListStore) left_model           = NULL;
  g_autoptr (GtkFlattenListModel) right_model = NULL;
  GListStore          *main_store             = NULL;
  GtkFlattenListModel *flatten_model          = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_listitem), self);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_listitem), self);
  g_signal_connect (factory, "unbind", G_CALLBACK (unbind_listitem), self);
  gtk_list_view_set_factory (self->list_view, factory);

  /* revisit this */
  left_model          = g_list_store_new (GTK_TYPE_STRING_OBJECT);
  self->wrapper_store = g_list_store_new (G_TYPE_LIST_MODEL);
  right_model         = gtk_flatten_list_model_new (g_object_ref (G_LIST_MODEL (self->wrapper_store)));
  main_store          = g_list_store_new (G_TYPE_LIST_MODEL);
  flatten_model       = gtk_flatten_list_model_new (G_LIST_MODEL (main_store));
  self->selection     = gtk_single_selection_new (G_LIST_MODEL (flatten_model));

  g_list_store_append (left_model, gtk_string_object_new ("Start!"));
  g_list_store_append (main_store, left_model);
  g_list_store_append (main_store, right_model);

  gtk_list_view_set_model (self->list_view, GTK_SELECTION_MODEL (self->selection));

  g_signal_connect (self->list_view, "activate",
                    G_CALLBACK (row_activated), self);
  g_signal_connect (self->delete_stroke, "clicked",
                    G_CALLBACK (delete_stroke_clicked), self);
}

static void
setup_listitem (GtkListItemFactory             *factory,
                GtkListItem                    *list_item,
                GtkCrusaderVillageTimelineView *self)
{
  GtkCrusaderVillageTimelineViewItem *area_item = NULL;

  area_item = g_object_new (GTK_CRUSADER_VILLAGE_TYPE_TIMELINE_VIEW_ITEM, NULL);
  gtk_list_item_set_child (list_item, GTK_WIDGET (area_item));
}

static void
bind_listitem (GtkListItemFactory             *factory,
               GtkListItem                    *list_item,
               GtkCrusaderVillageTimelineView *self)
{
  GObject *stroke = NULL;

  stroke = gtk_list_item_get_item (list_item);

  if (GTK_CRUSADER_VILLAGE_IS_ITEM_STROKE (stroke))
    {
      GtkWidget *view_item   = NULL;
      gboolean   insert_mode = FALSE;
      gboolean   lock_hinted = FALSE;

      view_item = gtk_list_item_get_child (list_item);

      g_object_get (
          self->handle,
          "insert-mode", &insert_mode,
          "lock-hinted", &lock_hinted,
          NULL);
      g_object_set (
          view_item,
          "stroke", stroke,
          "insert-mode", insert_mode,
          "drawing", lock_hinted,
          NULL);

      g_signal_connect (self->handle, "notify::cursor",
                        G_CALLBACK (listitem_cursor_changed), list_item);
      g_signal_connect (self->handle, "notify::insert-mode",
                        G_CALLBACK (listitem_mode_hint_changed), list_item);
      g_signal_connect (self->handle, "notify::lock-hinted",
                        G_CALLBACK (listitem_mode_hint_changed), list_item);
    }
}

static void
unbind_listitem (GtkListItemFactory             *factory,
                 GtkListItem                    *list_item,
                 GtkCrusaderVillageTimelineView *self)
{
  g_signal_handlers_disconnect_by_func (
      self->handle, listitem_cursor_changed, list_item);
  g_signal_handlers_disconnect_by_func (
      self->handle, listitem_mode_hint_changed, list_item);
}

static void
listitem_cursor_changed (GtkCrusaderVillageMapHandle *handle,
                         GParamSpec                  *pspec,
                         GtkListItem                 *list_item)
{
  guint      cursor    = 0;
  guint      position  = 0;
  gboolean   inactive  = FALSE;
  GtkWidget *view_item = NULL;

  g_object_get (
      handle,
      "cursor", &cursor,
      NULL);
  position = gtk_list_item_get_position (list_item);
  inactive = position > cursor;

  view_item = gtk_list_item_get_child (list_item);
  g_object_set (
      view_item,
      "inactive", inactive,
      NULL);
}

static void
listitem_mode_hint_changed (GtkCrusaderVillageMapHandle *handle,
                            GParamSpec                  *pspec,
                            GtkListItem                 *list_item)
{
  gboolean   insert_mode = FALSE;
  gboolean   lock_hinted = FALSE;
  GtkWidget *view_item   = NULL;

  g_object_get (
      handle,
      "insert-mode", &insert_mode,
      "lock-hinted", &lock_hinted,
      NULL);

  view_item = gtk_list_item_get_child (list_item);
  g_object_set (
      view_item,
      "insert-mode", insert_mode,
      "drawing", lock_hinted,
      NULL);
}

static void
row_activated (GtkListView                    *self,
               guint                           position,
               GtkCrusaderVillageTimelineView *timeline_view)
{
  g_object_set (
      timeline_view->handle,
      "cursor", position,
      NULL);
}

static void
delete_stroke_clicked (GtkButton                      *self,
                       GtkCrusaderVillageTimelineView *timeline_view)
{
  guint cursor = 0;

  if (timeline_view->handle == NULL)
    return;

  g_object_get (
      timeline_view->handle,
      "cursor", &cursor,
      NULL);
  if (cursor == 0)
    return;

  gtk_crusader_village_map_handle_delete_idx (
      timeline_view->handle, cursor - 1);
}

static void
cursor_changed (GtkCrusaderVillageMapHandle    *handle,
                GParamSpec                     *pspec,
                GtkCrusaderVillageTimelineView *timeline_view)
{
  update_selected (timeline_view);
  update_ui (timeline_view);
}

static void
lock_hint_changed (GtkCrusaderVillageMapHandle    *handle,
                   GParamSpec                     *pspec,
                   GtkCrusaderVillageTimelineView *timeline_view)
{
  gboolean lock_hinted = FALSE;

  g_object_get (
      handle,
      "lock-hinted", &lock_hinted,
      NULL);

  if (lock_hinted)
    update_selected (timeline_view);

  update_ui (timeline_view);
}

static void
update_selected (GtkCrusaderVillageTimelineView *self)
{
  guint cursor = 0;

  g_object_get (
      self->handle,
      "cursor", &cursor,
      NULL);

  gtk_list_view_scroll_to (self->list_view, cursor, GTK_LIST_SCROLL_FOCUS, NULL);
  gtk_single_selection_set_selected (self->selection, cursor);
}

static void
update_ui (GtkCrusaderVillageTimelineView *self)
{
  guint    selected    = 0;
  guint    n_strokes   = 0;
  char     buf[128]    = { 0 };
  gboolean lock_hinted = FALSE;

  if (self->handle == NULL)
    return;

  selected  = gtk_single_selection_get_selected (self->selection);
  n_strokes = g_list_model_get_n_items (G_LIST_MODEL (self->model));

  if (selected == n_strokes)
    {
      if (n_strokes == 1)
        g_snprintf (buf, sizeof (buf), "1 stroke");
      else
        g_snprintf (buf, sizeof (buf), "%d strokes", n_strokes);
    }
  else
    g_snprintf (buf, sizeof (buf), "cursor: %d", selected);

  gtk_label_set_label (self->stats, buf);

  g_object_get (
      self->handle,
      "lock-hinted", &lock_hinted,
      NULL);

  gtk_widget_set_sensitive (GTK_WIDGET (self->insert_mode), !lock_hinted);
  gtk_widget_set_sensitive (GTK_WIDGET (self->delete_stroke), !lock_hinted && selected > 0);
}
