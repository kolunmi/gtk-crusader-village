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

struct _GcvTimelineViewItem
{
  GcvUtilBin parent_instance;

  GcvItemStroke *stroke;
  gboolean       selected;
  gboolean       inactive;
  gboolean       insert_mode;
  gboolean       drawing;

  /* Template widgets */
  GtkImage *invisible_indicator;
  GtkImage *overwrite_indicator;
  GtkImage *overwriting_indicator;
  GtkImage *insert_indicator;
  GtkImage *inserting_indicator;
  GtkLabel *left;
  GtkLabel *center;
  GtkLabel *right;
};

G_DEFINE_FINAL_TYPE (GcvTimelineViewItem, gcv_timeline_view_item, GCV_TYPE_UTIL_BIN)

enum
{
  PROP_0,

  PROP_STROKE,

  PROP_SELECTED,
  PROP_INACTIVE,
  PROP_INSERT_MODE,
  PROP_DRAWING,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
update_indicators (GcvTimelineViewItem *self);

static void
gcv_timeline_view_item_dispose (GObject *object)
{
  GcvTimelineViewItem *self = GCV_TIMELINE_VIEW_ITEM (object);

  g_clear_object (&self->stroke);

  G_OBJECT_CLASS (gcv_timeline_view_item_parent_class)->dispose (object);
}

static void
gcv_timeline_view_item_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GcvTimelineViewItem *self = GCV_TIMELINE_VIEW_ITEM (object);

  switch (prop_id)
    {
    case PROP_STROKE:
      g_value_set_object (value, self->stroke);
      break;
    case PROP_SELECTED:
      g_value_set_boolean (value, self->selected);
      break;
    case PROP_INACTIVE:
      g_value_set_boolean (value, self->inactive);
      break;
    case PROP_INSERT_MODE:
      g_value_set_boolean (value, self->insert_mode);
      break;
    case PROP_DRAWING:
      g_value_set_boolean (value, self->drawing);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gcv_timeline_view_item_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GcvTimelineViewItem *self = GCV_TIMELINE_VIEW_ITEM (object);

  switch (prop_id)
    {
    case PROP_STROKE:
      {
        g_clear_object (&self->stroke);
        self->stroke = g_value_dup_object (value);

        if (self->stroke != NULL)
          {
            g_autoptr (GcvItem) item     = NULL;
            g_autoptr (GArray) instances = NULL;

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

            gtk_label_set_label (self->center, NULL);
          }
        else
          {
            gtk_label_set_label (self->left, NULL);
            gtk_label_set_label (self->center, "---");
            gtk_label_set_label (self->right, NULL);
          }
        update_indicators (self);
      }
      break;
    case PROP_SELECTED:
      self->selected = g_value_get_boolean (value);
      update_indicators (self);
      break;
    case PROP_INACTIVE:
      self->inactive = g_value_get_boolean (value);
      update_indicators (self);
      break;
    case PROP_INSERT_MODE:
      self->insert_mode = g_value_get_boolean (value);
      update_indicators (self);
      break;
    case PROP_DRAWING:
      self->drawing = g_value_get_boolean (value);
      update_indicators (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gcv_timeline_view_item_class_init (GcvTimelineViewItemClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gcv_timeline_view_item_dispose;
  object_class->get_property = gcv_timeline_view_item_get_property;
  object_class->set_property = gcv_timeline_view_item_set_property;

  props[PROP_STROKE] =
      g_param_spec_object (
          "stroke",
          "Stroke",
          "The stroke this widget represents",
          GCV_TYPE_ITEM_STROKE,
          G_PARAM_READWRITE);

  props[PROP_SELECTED] =
      g_param_spec_boolean (
          "selected",
          "Selected",
          "Whether to indicate that this widget is selected",
          FALSE,
          G_PARAM_READWRITE);

  props[PROP_INACTIVE] =
      g_param_spec_boolean (
          "inactive",
          "Inactive",
          "Whether to indicate that this widget is inactive",
          FALSE,
          G_PARAM_READWRITE);

  props[PROP_INSERT_MODE] =
      g_param_spec_boolean (
          "insert-mode",
          "Insert Mode",
          "Whether to indicate that this widget will not be overwritten when it is inactive",
          FALSE,
          G_PARAM_READWRITE);

  props[PROP_DRAWING] =
      g_param_spec_boolean (
          "drawing",
          "Drawing",
          "Whether to hint that the stroke's relation to its list is about to change",
          FALSE,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/Gcv/gtk-crusader-village-timeline-view-item.ui");
  gtk_widget_class_bind_template_child (widget_class, GcvTimelineViewItem, invisible_indicator);
  gtk_widget_class_bind_template_child (widget_class, GcvTimelineViewItem, overwrite_indicator);
  gtk_widget_class_bind_template_child (widget_class, GcvTimelineViewItem, overwriting_indicator);
  gtk_widget_class_bind_template_child (widget_class, GcvTimelineViewItem, insert_indicator);
  gtk_widget_class_bind_template_child (widget_class, GcvTimelineViewItem, inserting_indicator);
  gtk_widget_class_bind_template_child (widget_class, GcvTimelineViewItem, left);
  gtk_widget_class_bind_template_child (widget_class, GcvTimelineViewItem, center);
  gtk_widget_class_bind_template_child (widget_class, GcvTimelineViewItem, right);
}

static void
gcv_timeline_view_item_init (GcvTimelineViewItem *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_label_set_label (self->left, NULL);
  gtk_label_set_label (self->center, "---");
  gtk_label_set_label (self->right, NULL);
}

static void
update_indicators (GcvTimelineViewItem *self)
{
  gtk_widget_set_visible (GTK_WIDGET (self->invisible_indicator), self->inactive);
  gtk_widget_set_visible (GTK_WIDGET (self->overwrite_indicator), self->selected && !self->insert_mode);
  gtk_widget_set_visible (GTK_WIDGET (self->overwriting_indicator), self->selected && self->drawing && !self->insert_mode);
  gtk_widget_set_visible (GTK_WIDGET (self->insert_indicator), self->selected && self->insert_mode);
  gtk_widget_set_visible (GTK_WIDGET (self->inserting_indicator), self->selected && self->drawing && self->insert_mode);
}
