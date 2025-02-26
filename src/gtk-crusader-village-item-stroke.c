/* gtk-crusader-village-item-stroke.c
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

struct _GtkCrusaderVillageItemStroke
{
  GObject parent_instance;

  GtkCrusaderVillageItem *item;
  GArray                 *instances;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageItemStroke, gtk_crusader_village_item_stroke, G_TYPE_OBJECT)

enum
{
  PROP_0,

  PROP_ITEM,
  PROP_INSTANCES,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
gtk_crusader_village_item_stroke_dispose (GObject *object)
{
  GtkCrusaderVillageItemStroke *self = GTK_CRUSADER_VILLAGE_ITEM_STROKE (object);

  g_clear_object (&self->item);
  g_clear_pointer (&self->instances, g_array_unref);

  G_OBJECT_CLASS (gtk_crusader_village_item_stroke_parent_class)->dispose (object);
}

static void
gtk_crusader_village_item_stroke_get_property (GObject    *object,
                                               guint       prop_id,
                                               GValue     *value,
                                               GParamSpec *pspec)
{
  GtkCrusaderVillageItemStroke *self = GTK_CRUSADER_VILLAGE_ITEM_STROKE (object);

  switch (prop_id)
    {
    case PROP_ITEM:
      g_value_set_object (value, self->item);
      break;
    case PROP_INSTANCES:
      g_value_set_boxed (value, self->instances);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_item_stroke_set_property (GObject      *object,
                                               guint         prop_id,
                                               const GValue *value,
                                               GParamSpec   *pspec)
{
  GtkCrusaderVillageItemStroke *self = GTK_CRUSADER_VILLAGE_ITEM_STROKE (object);

  switch (prop_id)
    {
    case PROP_ITEM:
      g_clear_object (&self->item);
      self->item = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_item_stroke_class_init (GtkCrusaderVillageItemStrokeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = gtk_crusader_village_item_stroke_dispose;
  object_class->get_property = gtk_crusader_village_item_stroke_get_property;
  object_class->set_property = gtk_crusader_village_item_stroke_set_property;

  props[PROP_ITEM] =
      g_param_spec_object (
          "item",
          "Item",
          "The item used for this stroke",
          GTK_CRUSADER_VILLAGE_TYPE_ITEM,
          G_PARAM_READWRITE);

  props[PROP_INSTANCES] =
      g_param_spec_boxed (
          "instances",
          "Instances",
          "An array of `GtkCrusaderVillageItemStrokeInstance`s "
          "representing the area of this stroke",
          G_TYPE_ARRAY,
          G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
gtk_crusader_village_item_stroke_init (GtkCrusaderVillageItemStroke *self)
{
  self->instances = g_array_new (FALSE, TRUE, sizeof (GtkCrusaderVillageItemStrokeInstance));
}

/* We trust that the caller won't pass an invalid instance */
gboolean
gtk_crusader_village_item_stroke_add_instance (GtkCrusaderVillageItemStroke        *self,
                                               GtkCrusaderVillageItemStrokeInstance instance)
{
  g_return_val_if_fail (GTK_CRUSADER_VILLAGE_IS_ITEM_STROKE (self), FALSE);

  for (guint i = 0; i < self->instances->len; i++)
    {
      GtkCrusaderVillageItemStrokeInstance check = { 0 };

      check = g_array_index (self->instances, GtkCrusaderVillageItemStrokeInstance, i);
      if (check.x == instance.x && check.y == instance.y)
        return FALSE;
    }

  g_array_append_val (self->instances, instance);
  return TRUE;
}
