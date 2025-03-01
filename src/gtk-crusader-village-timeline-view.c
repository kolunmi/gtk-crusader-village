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
#include "gtk-crusader-village-map.h"
#include "gtk-crusader-village-timeline-view-item.h"
#include "gtk-crusader-village-timeline-view.h"

struct _GtkCrusaderVillageTimelineView
{
  GtkCrusaderVillageUtilBin parent_instance;

  GtkCrusaderVillageMap *map;
  GListModel            *strokes;

  /* Template widgets */
  GtkLabel    *stats;
  GtkListView *list_view;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageTimelineView, gtk_crusader_village_timeline_view, GTK_CRUSADER_VILLAGE_TYPE_UTIL_BIN)

enum
{
  PROP_0,

  PROP_MAP,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
setup_listitem (GtkListItemFactory *factory,
                GtkListItem        *list_item,
                gpointer            user_data);

static void
bind_listitem (GtkListItemFactory *factory,
               GtkListItem        *list_item,
               gpointer            user_data);

static void
strokes_changed (GListModel                     *self,
                 guint                           position,
                 guint                           removed,
                 guint                           added,
                 GtkCrusaderVillageTimelineView *timeline_view);

static void
update_info (GtkCrusaderVillageTimelineView *self);

static void
gtk_crusader_village_timeline_view_dispose (GObject *object)
{
  GtkCrusaderVillageTimelineView *self = GTK_CRUSADER_VILLAGE_TIMELINE_VIEW (object);

  g_clear_object (&self->map);

  if (self->strokes != NULL)
    g_signal_handlers_disconnect_by_func (self->strokes, strokes_changed, self);
  g_clear_object (&self->strokes);

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
    case PROP_MAP:
      g_value_set_object (value, self->map);
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
    case PROP_MAP:
      {
        GtkSingleSelection *selection = NULL;

        g_clear_object (&self->map);

        if (self->strokes != NULL)
          g_signal_handlers_disconnect_by_func (self->strokes, strokes_changed, self);
        g_clear_object (&self->strokes);

        self->map = g_value_dup_object (value);

        selection = GTK_SINGLE_SELECTION (gtk_list_view_get_model (self->list_view));

        if (self->map != NULL)
          {
            g_object_get (
                self->map,
                "strokes", &self->strokes,
                NULL);
            g_assert (self->strokes != NULL);

            gtk_single_selection_set_model (selection, G_LIST_MODEL (self->strokes));

            g_signal_connect (self->strokes, "items-changed",
                              G_CALLBACK (strokes_changed), self);
          }
        else
          gtk_single_selection_set_model (selection, NULL);
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

  props[PROP_MAP] =
      g_param_spec_object (
          "map",
          "Map",
          "The map this widget will use",
          GTK_CRUSADER_VILLAGE_TYPE_MAP,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/GtkCrusaderVillage/gtk-crusader-village-timeline-view.ui");
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageTimelineView, stats);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageTimelineView, list_view);
}

static void
gtk_crusader_village_timeline_view_init (GtkCrusaderVillageTimelineView *self)
{
  GtkListItemFactory *factory         = NULL;
  GtkSelectionModel  *selection_model = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  factory         = gtk_signal_list_item_factory_new ();
  selection_model = GTK_SELECTION_MODEL (gtk_single_selection_new (NULL));

  g_signal_connect (factory, "setup", G_CALLBACK (setup_listitem), self);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_listitem), self);
  gtk_list_view_set_factory (self->list_view, factory);
  gtk_list_view_set_model (self->list_view, selection_model);
}

static void
setup_listitem (GtkListItemFactory *factory,
                GtkListItem        *list_item,
                gpointer            user_data)
{
  GtkCrusaderVillageTimelineViewItem *area_item = NULL;

  area_item = g_object_new (GTK_CRUSADER_VILLAGE_TYPE_TIMELINE_VIEW_ITEM, NULL);
  gtk_list_item_set_child (list_item, GTK_WIDGET (area_item));
}

static void
bind_listitem (GtkListItemFactory *factory,
               GtkListItem        *list_item,
               gpointer            user_data)
{
  GtkCrusaderVillageTimelineView     *self      = user_data;
  GtkCrusaderVillageItemStroke       *stroke    = NULL;
  GtkCrusaderVillageTimelineViewItem *view_item = NULL;

  stroke    = GTK_CRUSADER_VILLAGE_ITEM_STROKE (gtk_list_item_get_item (list_item));
  view_item = GTK_CRUSADER_VILLAGE_TIMELINE_VIEW_ITEM (gtk_list_item_get_child (list_item));

  g_object_set (
      view_item,
      "stroke", stroke,
      NULL);
}

static void
strokes_changed (GListModel                     *self,
                 guint                           position,
                 guint                           removed,
                 guint                           added,
                 GtkCrusaderVillageTimelineView *timeline_view)
{
  update_info (timeline_view);
}

static void
update_info (GtkCrusaderVillageTimelineView *self)
{
  guint n_strokes = 0;
  char  buf[128]  = { 0 };

  n_strokes = g_list_model_get_n_items (self->strokes);
  g_snprintf (buf, sizeof (buf), "%d stroke(s)", n_strokes);

  gtk_label_set_label (self->stats, buf);
}
