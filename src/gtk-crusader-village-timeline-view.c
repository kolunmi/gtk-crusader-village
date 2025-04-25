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

struct _GcvTimelineView
{
  GcvUtilBin parent_instance;

  GtkMultiSelection *selection;
  GListStore        *wrapper_store;

  GcvMapHandle *handle;
  GListModel   *model;

  GBinding *insert_mode_binding;

  guint playback_handle;

  /* Template widgets */
  GtkLabel       *stats;
  GtkListView    *list_view;
  GtkCheckButton *insert_mode;
  GtkButton      *delete_stroke;
  GtkScale       *scale;
  GtkButton      *playback;
};

G_DEFINE_FINAL_TYPE (GcvTimelineView, gcv_timeline_view, GCV_TYPE_UTIL_BIN)

enum
{
  PROP_0,

  PROP_MAP_HANDLE,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
setup_listitem (GtkListItemFactory *factory,
                GtkListItem        *list_item,
                GcvTimelineView    *self);

static void
bind_listitem (GtkListItemFactory *factory,
               GtkListItem        *list_item,
               GcvTimelineView    *self);

static void
unbind_listitem (GtkListItemFactory *factory,
                 GtkListItem        *list_item,
                 GcvTimelineView    *self);

static void
listitem_cursor_changed (GcvMapHandle *handle,
                         GParamSpec   *pspec,
                         GtkListItem  *list_item);

static void
listitem_mode_hint_changed (GcvMapHandle *handle,
                            GParamSpec   *pspec,
                            GtkListItem  *list_item);

static void
model_changed (GListModel      *self,
               guint            position,
               guint            removed,
               guint            added,
               GcvTimelineView *timeline_view);

static void
selection_changed (GtkSelectionModel *self,
                   guint              position,
                   guint              n_items,
                   GcvTimelineView   *timeline_view);

static void
delete_stroke_clicked (GtkButton       *self,
                       GcvTimelineView *timeline_view);

static void
playback_clicked (GtkButton       *self,
                  GcvTimelineView *timeline_view);

static void
cursor_changed (GcvMapHandle    *handle,
                GParamSpec      *pspec,
                GcvTimelineView *timeline_view);

static void
lock_hint_changed (GcvMapHandle    *handle,
                   GParamSpec      *pspec,
                   GcvTimelineView *timeline_view);

static void
scale_change_value (GtkRange        *self,
                    GtkScrollType   *scroll,
                    gdouble          value,
                    GcvTimelineView *timeline_view);

static gboolean
playback_timeout (GcvTimelineView *timeline_view);

static void
update_ui (GcvTimelineView *self);

static void
gcv_timeline_view_dispose (GObject *object)
{
  GcvTimelineView *self = GCV_TIMELINE_VIEW (object);

  g_clear_object (&self->selection);
  g_clear_object (&self->wrapper_store);

  if (self->model != NULL)
    g_signal_handlers_disconnect_by_func (self->model, model_changed, self);
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

  g_clear_handle_id (&self->playback_handle, g_source_remove);

  G_OBJECT_CLASS (gcv_timeline_view_parent_class)->dispose (object);
}

static void
gcv_timeline_view_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GcvTimelineView *self = GCV_TIMELINE_VIEW (object);

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
gcv_timeline_view_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GcvTimelineView *self = GCV_TIMELINE_VIEW (object);

  switch (prop_id)
    {
    case PROP_MAP_HANDLE:
      {
        g_list_store_remove_all (self->wrapper_store);

        if (self->model != NULL)
          g_signal_handlers_disconnect_by_func (self->model, model_changed, self);
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

        g_clear_handle_id (&self->playback_handle, g_source_remove);

        self->handle = g_value_dup_object (value);

        if (self->handle != NULL)
          {
            g_object_get (
                self->handle,
                "model", &self->model,
                NULL);
            g_list_store_append (self->wrapper_store, self->model);
            g_signal_connect (self->model, "items-changed",
                              G_CALLBACK (model_changed), self);

            g_signal_connect (self->handle, "notify::cursor",
                              G_CALLBACK (cursor_changed), self);
            g_signal_connect (self->handle, "notify::cursor-len",
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
gcv_timeline_view_class_init (GcvTimelineViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gcv_timeline_view_dispose;
  object_class->get_property = gcv_timeline_view_get_property;
  object_class->set_property = gcv_timeline_view_set_property;

  props[PROP_MAP_HANDLE] =
      g_param_spec_object (
          "map-handle",
          "Map Handle",
          "The map handle this widget will use",
          GCV_TYPE_MAP_HANDLE,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/Gcv/gtk-crusader-village-timeline-view.ui");
  gtk_widget_class_bind_template_child (widget_class, GcvTimelineView, stats);
  gtk_widget_class_bind_template_child (widget_class, GcvTimelineView, list_view);
  gtk_widget_class_bind_template_child (widget_class, GcvTimelineView, insert_mode);
  gtk_widget_class_bind_template_child (widget_class, GcvTimelineView, delete_stroke);
  gtk_widget_class_bind_template_child (widget_class, GcvTimelineView, scale);
  gtk_widget_class_bind_template_child (widget_class, GcvTimelineView, playback);
}

static void
gcv_timeline_view_init (GcvTimelineView *self)
{
  g_autoptr (GtkListItemFactory) factory     = NULL;
  g_autoptr (GListStore) right_model         = NULL;
  g_autoptr (GtkFlattenListModel) left_model = NULL;
  GListStore          *main_store            = NULL;
  GtkFlattenListModel *flatten_model         = NULL;
  g_autoptr (GtkStringObject) dummy          = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_listitem), self);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_listitem), self);
  g_signal_connect (factory, "unbind", G_CALLBACK (unbind_listitem), self);
  gtk_list_view_set_factory (self->list_view, factory);

  /* revisit this */
  right_model         = g_list_store_new (GTK_TYPE_STRING_OBJECT);
  self->wrapper_store = g_list_store_new (G_TYPE_LIST_MODEL);
  left_model          = gtk_flatten_list_model_new (g_object_ref (G_LIST_MODEL (self->wrapper_store)));
  main_store          = g_list_store_new (G_TYPE_LIST_MODEL);
  flatten_model       = gtk_flatten_list_model_new (G_LIST_MODEL (main_store));
  self->selection     = gtk_multi_selection_new (G_LIST_MODEL (flatten_model));

  dummy = gtk_string_object_new ("");
  g_list_store_append (right_model, dummy);
  g_list_store_append (main_store, left_model);
  g_list_store_append (main_store, right_model);

  gtk_list_view_set_model (self->list_view, GTK_SELECTION_MODEL (self->selection));

  g_signal_connect (self->selection, "selection-changed",
                    G_CALLBACK (selection_changed), self);
  g_signal_connect (self->delete_stroke, "clicked",
                    G_CALLBACK (delete_stroke_clicked), self);
  g_signal_connect (self->playback, "clicked",
                    G_CALLBACK (playback_clicked), self);

  gtk_range_set_increments (GTK_RANGE (self->scale), 1, 5);
  g_signal_connect (self->scale, "change-value",
                    G_CALLBACK (scale_change_value), self);
}

static void
setup_listitem (GtkListItemFactory *factory,
                GtkListItem        *list_item,
                GcvTimelineView    *self)
{
  GcvTimelineViewItem *area_item = NULL;

  area_item = g_object_new (GCV_TYPE_TIMELINE_VIEW_ITEM, NULL);
  gtk_list_item_set_child (list_item, GTK_WIDGET (area_item));
}

static void
bind_listitem (GtkListItemFactory *factory,
               GtkListItem        *list_item,
               GcvTimelineView    *self)
{
  GObject *stroke = NULL;

  stroke = gtk_list_item_get_item (list_item);

  if (GCV_IS_ITEM_STROKE (stroke))
    {
      GtkWidget *view_item   = NULL;
      guint      cursor      = 0;
      guint      cursor_len  = 0;
      gboolean   insert_mode = FALSE;
      gboolean   lock_hinted = FALSE;
      guint      position    = 0;
      gboolean   inactive    = FALSE;
      gboolean   selected    = FALSE;

      view_item = gtk_list_item_get_child (list_item);

      g_object_get (
          self->handle,
          "cursor", &cursor,
          "cursor-len", &cursor_len,
          "insert-mode", &insert_mode,
          "lock-hinted", &lock_hinted,
          NULL);

      position = gtk_list_item_get_position (list_item);
      inactive = position >= cursor + cursor_len;
      selected = position >= cursor &&
                 (cursor_len == 0 || position < cursor + cursor_len);

      g_object_set (
          view_item,
          "stroke", stroke,
          "insert-mode", insert_mode,
          "drawing", lock_hinted,
          "selected", selected,
          "inactive", inactive,
          NULL);

      g_signal_connect (self->handle, "notify::cursor",
                        G_CALLBACK (listitem_cursor_changed), list_item);
      g_signal_connect (self->handle, "notify::cursor-len",
                        G_CALLBACK (listitem_cursor_changed), list_item);
      g_signal_connect (self->handle, "notify::insert-mode",
                        G_CALLBACK (listitem_mode_hint_changed), list_item);
      g_signal_connect (self->handle, "notify::lock-hinted",
                        G_CALLBACK (listitem_mode_hint_changed), list_item);
    }
}

static void
unbind_listitem (GtkListItemFactory *factory,
                 GtkListItem        *list_item,
                 GcvTimelineView    *self)
{
  GObject *stroke = NULL;

  stroke = gtk_list_item_get_item (list_item);

  if (GCV_IS_ITEM_STROKE (stroke))
    {
      GtkWidget *view_item = NULL;

      view_item = gtk_list_item_get_child (list_item);

      g_object_set (
          view_item,
          "stroke", NULL,
          "insert-mode", FALSE,
          "drawing", FALSE,
          "selected", FALSE,
          "inactive", FALSE,
          NULL);

      g_signal_handlers_disconnect_by_func (
          self->handle, listitem_cursor_changed, list_item);
      g_signal_handlers_disconnect_by_func (
          self->handle, listitem_mode_hint_changed, list_item);
    }
}

static void
listitem_cursor_changed (GcvMapHandle *handle,
                         GParamSpec   *pspec,
                         GtkListItem  *list_item)
{
  guint      cursor     = 0;
  guint      cursor_len = 0;
  guint      position   = 0;
  gboolean   inactive   = FALSE;
  gboolean   selected   = FALSE;
  GtkWidget *view_item  = NULL;

  g_object_get (
      handle,
      "cursor", &cursor,
      "cursor-len", &cursor_len,
      NULL);
  position = gtk_list_item_get_position (list_item);
  inactive = position >= cursor + cursor_len;
  selected = position >= cursor &&
             (cursor_len == 0 || position < cursor + cursor_len);

  view_item = gtk_list_item_get_child (list_item);
  g_object_set (
      view_item,
      "selected", selected,
      "inactive", inactive,
      NULL);
}

static void
listitem_mode_hint_changed (GcvMapHandle *handle,
                            GParamSpec   *pspec,
                            GtkListItem  *list_item)
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
model_changed (GListModel      *self,
               guint            position,
               guint            removed,
               guint            added,
               GcvTimelineView *timeline_view)
{
  update_ui (timeline_view);
}

static void
selection_changed (GtkSelectionModel *self,
                   guint              position,
                   guint              n_items,
                   GcvTimelineView   *timeline_view)
{
  g_autoptr (GtkBitset) selected = NULL;
  guint min                      = 0;
  guint max                      = 0;

  selected = gtk_selection_model_get_selection (
      GTK_SELECTION_MODEL (timeline_view->selection));

  min = gtk_bitset_get_minimum (selected);
  max = gtk_bitset_get_maximum (selected);

  g_object_set (
      timeline_view->handle,
      "cursor", min,
      "cursor-len", max - min + 1,
      NULL);

  update_ui (timeline_view);
}

static void
delete_stroke_clicked (GtkButton       *self,
                       GcvTimelineView *timeline_view)
{
  if (timeline_view->handle == NULL)
    return;

  gcv_map_handle_delete_at_cursor (timeline_view->handle);
}

static void
playback_clicked (GtkButton       *self,
                  GcvTimelineView *timeline_view)
{
  if (timeline_view->playback_handle > 0)
    g_clear_handle_id (&timeline_view->playback_handle, g_source_remove);
  else
    /* 20 fps */
    timeline_view->playback_handle =
        g_timeout_add ((1.0 / 20.0) * G_TIME_SPAN_MILLISECOND,
                       (GSourceFunc) playback_timeout, timeline_view);

  update_ui (timeline_view);
}

static void
cursor_changed (GcvMapHandle    *handle,
                GParamSpec      *pspec,
                GcvTimelineView *timeline_view)
{
  guint cursor     = 0;
  guint cursor_len = 0;

  g_object_get (
      timeline_view->handle,
      "cursor", &cursor,
      "cursor-len", &cursor_len,
      NULL);
  gtk_selection_model_select_range (
      GTK_SELECTION_MODEL (timeline_view->selection), cursor, MAX (1, cursor_len), TRUE);
  gtk_list_view_scroll_to (timeline_view->list_view, cursor, GTK_LIST_SCROLL_NONE, NULL);
}

static void
lock_hint_changed (GcvMapHandle    *handle,
                   GParamSpec      *pspec,
                   GcvTimelineView *timeline_view)
{
  g_clear_handle_id (&timeline_view->playback_handle, g_source_remove);
  update_ui (timeline_view);
}

static void
scale_change_value (GtkRange        *self,
                    GtkScrollType   *scroll,
                    gdouble          value,
                    GcvTimelineView *timeline_view)
{
  if (timeline_view->handle == NULL)
    return;

  g_object_set (
      timeline_view->handle,
      "cursor", (guint) MAX (0, round (value)),
      NULL);
}

static gboolean
playback_timeout (GcvTimelineView *timeline_view)
{
  guint cursor                       = 0;
  g_autoptr (GListModel) union_model = NULL;
  guint total_strokes                = 0;

  g_assert (timeline_view->handle != NULL);

  g_object_get (
      timeline_view->handle,
      "cursor", &cursor,
      "model", &union_model,
      NULL);
  total_strokes = g_list_model_get_n_items (union_model);

  g_object_set (
      timeline_view->handle,
      "cursor", (cursor + 1) % total_strokes,
      NULL);

  return G_SOURCE_CONTINUE;
}

static void
update_ui (GcvTimelineView *self)
{
  guint    n_strokes   = 0;
  guint    cursor      = 0;
  guint    cursor_len  = 0;
  char     buf[128]    = { 0 };
  gboolean lock_hinted = FALSE;

  if (self->handle == NULL)
    return;

  n_strokes = g_list_model_get_n_items (G_LIST_MODEL (self->model));
  g_object_get (
      self->handle,
      "lock-hinted", &lock_hinted,
      "cursor", &cursor,
      "cursor-len", &cursor_len,
      NULL);

  g_snprintf (buf, sizeof (buf), "cursor: %d-%d (total %d)", cursor, cursor_len, n_strokes);
  gtk_label_set_label (self->stats, buf);

  gtk_widget_set_sensitive (GTK_WIDGET (self->insert_mode), !lock_hinted && self->playback_handle == 0);
  gtk_widget_set_sensitive (GTK_WIDGET (self->delete_stroke), !lock_hinted && n_strokes > 0 && self->playback_handle == 0);
  gtk_widget_set_sensitive (GTK_WIDGET (self->playback), !lock_hinted && n_strokes > 1);
  gtk_widget_set_sensitive (GTK_WIDGET (self->scale), !lock_hinted && n_strokes > 0);

  gtk_button_set_label (
      self->playback,
      self->playback_handle > 0
          ? "Stop Playback"
          : "Begin Playback");

  g_signal_handlers_block_by_func (
      self->scale, scale_change_value, self);
  gtk_range_set_range (GTK_RANGE (self->scale), 0, n_strokes);
  gtk_range_set_value (GTK_RANGE (self->scale), cursor);
  g_signal_handlers_unblock_by_func (
      self->scale, scale_change_value, self);
}
