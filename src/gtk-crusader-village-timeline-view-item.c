/* gtk-crusader-village-timeline-view-item.c
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
#include "gtk-crusader-village-item.h"
#include "gtk-crusader-village-timeline-view-item.h"

struct _GtkCrusaderVillageTimelineViewItem
{
  GtkCrusaderVillageUtilBin parent_instance;

  GtkCrusaderVillageItemStroke *stroke;

  /* Template widgets */
  GtkLabel *left;
  GtkLabel *center;
  GtkLabel *right;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageTimelineViewItem, gtk_crusader_village_timeline_view_item, GTK_CRUSADER_VILLAGE_TYPE_UTIL_BIN)

enum
{
  PROP_0,

  PROP_STROKE,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
gtk_crusader_village_timeline_view_item_dispose (GObject *object)
{
  GtkCrusaderVillageTimelineViewItem *self = GTK_CRUSADER_VILLAGE_TIMELINE_VIEW_ITEM (object);

  g_clear_object (&self->stroke);

  G_OBJECT_CLASS (gtk_crusader_village_timeline_view_item_parent_class)->dispose (object);
}

static void
gtk_crusader_village_timeline_view_item_get_property (GObject    *object,
                                                      guint       prop_id,
                                                      GValue     *value,
                                                      GParamSpec *pspec)
{
  GtkCrusaderVillageTimelineViewItem *self = GTK_CRUSADER_VILLAGE_TIMELINE_VIEW_ITEM (object);

  switch (prop_id)
    {
    case PROP_STROKE:
      g_value_set_object (value, self->stroke);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_timeline_view_item_set_property (GObject      *object,
                                                      guint         prop_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec)
{
  GtkCrusaderVillageTimelineViewItem *self = GTK_CRUSADER_VILLAGE_TIMELINE_VIEW_ITEM (object);

  switch (prop_id)
    {
    case PROP_STROKE:
      {
        g_clear_object (&self->stroke);
        self->stroke = g_value_dup_object (value);

        if (self->stroke != NULL)
          {
            g_autoptr (GtkCrusaderVillageItem) item = NULL;
            g_autoptr (GArray) instances            = NULL;

            g_object_get (
                self->stroke,
                "item", &item,
                "instances", &instances,
                NULL);

            if (item != NULL)
              {
                g_autofree char *name = NULL;

                g_object_get (
                    item,
                    "name", &name,
                    NULL);

                gtk_label_set_label (self->left, name);
              }
            else
              gtk_label_set_label (self->left, "---");

            if (instances != NULL)
              {
                char buf[64] = { 0 };

                g_snprintf (buf, sizeof (buf), "%d", instances->len);
                gtk_label_set_label (self->right, buf);
              }
            else
              gtk_label_set_label (self->right, "---");
          }
        else
          {
            gtk_label_set_label (self->left, "---");
            gtk_label_set_label (self->right, "---");
          }
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_timeline_view_item_class_init (GtkCrusaderVillageTimelineViewItemClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gtk_crusader_village_timeline_view_item_dispose;
  object_class->get_property = gtk_crusader_village_timeline_view_item_get_property;
  object_class->set_property = gtk_crusader_village_timeline_view_item_set_property;

  props[PROP_STROKE] =
      g_param_spec_object (
          "stroke",
          "Stroke",
          "The stroke this widget represents",
          GTK_CRUSADER_VILLAGE_TYPE_ITEM_STROKE,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/GtkCrusaderVillage/gtk-crusader-village-timeline-view-item.ui");
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageTimelineViewItem, left);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageTimelineViewItem, center);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageTimelineViewItem, right);
}

static void
gtk_crusader_village_timeline_view_item_init (GtkCrusaderVillageTimelineViewItem *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
