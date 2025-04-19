/* gtk-crusader-village-brush-area-item.c
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

#include "gtk-crusader-village-brush-area-item.h"
#include "gtk-crusader-village-brushable.h"

struct _GtkCrusaderVillageBrushAreaItem
{
  GtkCrusaderVillageUtilBin parent_instance;

  GtkCrusaderVillageBrushable *brush;

  /* Template widgets */
  GtkLabel   *name;
  GtkScale   *size;
  GtkPicture *thumbnail;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageBrushAreaItem, gtk_crusader_village_brush_area_item, GTK_CRUSADER_VILLAGE_TYPE_UTIL_BIN)

enum
{
  PROP_0,

  PROP_BRUSH,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
gtk_crusader_village_brush_area_item_dispose (GObject *object)
{
  GtkCrusaderVillageBrushAreaItem *self = GTK_CRUSADER_VILLAGE_BRUSH_AREA_ITEM (object);

  g_clear_object (&self->brush);

  G_OBJECT_CLASS (gtk_crusader_village_brush_area_item_parent_class)->dispose (object);
}

static void
gtk_crusader_village_brush_area_item_get_property (GObject    *object,
                                                   guint       prop_id,
                                                   GValue     *value,
                                                   GParamSpec *pspec)
{
  GtkCrusaderVillageBrushAreaItem *self = GTK_CRUSADER_VILLAGE_BRUSH_AREA_ITEM (object);

  switch (prop_id)
    {
    case PROP_BRUSH:
      g_value_set_object (value, self->brush);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_brush_area_item_set_property (GObject      *object,
                                                   guint         prop_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec)
{
  GtkCrusaderVillageBrushAreaItem *self = GTK_CRUSADER_VILLAGE_BRUSH_AREA_ITEM (object);

  switch (prop_id)
    {
    case PROP_BRUSH:
      {
        g_clear_object (&self->brush);
        self->brush = g_value_dup_object (value);

        if (self->brush != NULL)
          {
            g_autofree char *name                = NULL;
            g_autoptr (GtkAdjustment) adjustment = NULL;
            g_autoptr (GdkTexture) texture       = NULL;

            name       = gtk_crusader_village_brushable_get_name (self->brush);
            adjustment = gtk_crusader_village_brushable_get_adjustment (self->brush);
            texture    = gtk_crusader_village_brushable_get_thumbnail (self->brush);

            gtk_label_set_label (self->name, name);
            gtk_range_set_adjustment (GTK_RANGE (self->size), adjustment);
            gtk_widget_set_visible (GTK_WIDGET (self->size), adjustment != NULL);
            gtk_picture_set_paintable (self->thumbnail, GDK_PAINTABLE (texture));
          }
        else
          {
            gtk_label_set_label (self->name, NULL);
            gtk_range_set_adjustment (GTK_RANGE (self->size), NULL);
            gtk_widget_set_visible (GTK_WIDGET (self->size), FALSE);
            gtk_picture_set_paintable (self->thumbnail, NULL);
          }
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_brush_area_item_class_init (GtkCrusaderVillageBrushAreaItemClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gtk_crusader_village_brush_area_item_dispose;
  object_class->get_property = gtk_crusader_village_brush_area_item_get_property;
  object_class->set_property = gtk_crusader_village_brush_area_item_set_property;

  props[PROP_BRUSH] =
      g_param_spec_object (
          "brush",
          "Brush",
          "The brush this widget represents",
          GTK_CRUSADER_VILLAGE_TYPE_BRUSHABLE,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/GtkCrusaderVillage/gtk-crusader-village-brush-area-item.ui");
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageBrushAreaItem, name);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageBrushAreaItem, size);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageBrushAreaItem, thumbnail);
}

static void
gtk_crusader_village_brush_area_item_init (GtkCrusaderVillageBrushAreaItem *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
