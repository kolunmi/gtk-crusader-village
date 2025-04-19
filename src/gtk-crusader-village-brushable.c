/* gtk-crusader-village-brushable.c
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

#include "gtk-crusader-village-brushable.h"

G_DEFINE_INTERFACE (GtkCrusaderVillageBrushable, gtk_crusader_village_brushable, G_TYPE_OBJECT);

static char *
gtk_crusader_village_brushable_real_get_name (GtkCrusaderVillageBrushable *self)
{
  return NULL;
}

static guint8 *
gtk_crusader_village_brushable_real_get_mask (GtkCrusaderVillageBrushable *self,
                                              int                         *width,
                                              int                         *height)
{
  *width  = 0;
  *height = 0;
  return NULL;
}

static GtkAdjustment *
gtk_crusader_village_brushable_real_get_adjustment (GtkCrusaderVillageBrushable *self)
{
  return NULL;
}

static GdkTexture *
gtk_crusader_village_brushable_real_get_thumbnail (GtkCrusaderVillageBrushable *self)
{
  return NULL;
}

static void
gtk_crusader_village_brushable_default_init (GtkCrusaderVillageBrushableInterface *iface)
{
  iface->get_name       = gtk_crusader_village_brushable_real_get_name;
  iface->get_mask       = gtk_crusader_village_brushable_real_get_mask;
  iface->get_adjustment = gtk_crusader_village_brushable_real_get_adjustment;
  iface->get_thumbnail  = gtk_crusader_village_brushable_real_get_thumbnail;
}

char *
gtk_crusader_village_brushable_get_name (GtkCrusaderVillageBrushable *self)
{
  g_return_val_if_fail (GTK_CRUSADER_VILLAGE_IS_BRUSHABLE (self), NULL);

  return GTK_CRUSADER_VILLAGE_BRUSHABLE_GET_IFACE (self)->get_name (self);
}

guint8 *
gtk_crusader_village_brushable_get_mask (GtkCrusaderVillageBrushable *self,
                                         int                         *width,
                                         int                         *height)
{
  g_return_val_if_fail (GTK_CRUSADER_VILLAGE_IS_BRUSHABLE (self), NULL);
  g_return_val_if_fail (width != NULL, NULL);
  g_return_val_if_fail (height != NULL, NULL);

  return GTK_CRUSADER_VILLAGE_BRUSHABLE_GET_IFACE (self)->get_mask (self, width, height);
}

GtkAdjustment *
gtk_crusader_village_brushable_get_adjustment (GtkCrusaderVillageBrushable *self)
{
  g_return_val_if_fail (GTK_CRUSADER_VILLAGE_IS_BRUSHABLE (self), NULL);

  return GTK_CRUSADER_VILLAGE_BRUSHABLE_GET_IFACE (self)->get_adjustment (self);
}

GdkTexture *
gtk_crusader_village_brushable_get_thumbnail (GtkCrusaderVillageBrushable *self)
{
  g_return_val_if_fail (GTK_CRUSADER_VILLAGE_IS_BRUSHABLE (self), NULL);

  return GTK_CRUSADER_VILLAGE_BRUSHABLE_GET_IFACE (self)->get_thumbnail (self);
}
