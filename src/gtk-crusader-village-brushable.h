/* gtk-crusader-village-brushable.h
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

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_CRUSADER_VILLAGE_TYPE_BRUSHABLE (gtk_crusader_village_brushable_get_type ())

G_DECLARE_INTERFACE (GtkCrusaderVillageBrushable, gtk_crusader_village_brushable, GTK_CRUSADER_VILLAGE, BRUSHABLE, GObject)

struct _GtkCrusaderVillageBrushableInterface
{
  GTypeInterface parent_iface;

  char *(*get_name) (GtkCrusaderVillageBrushable *self);
  guint8 *(*get_mask) (GtkCrusaderVillageBrushable *self,
                       int                         *width,
                       int                         *height);
  GtkAdjustment *(*get_adjustment) (GtkCrusaderVillageBrushable *self);
  GdkTexture *(*get_thumbnail) (GtkCrusaderVillageBrushable *self);
};

char *
gtk_crusader_village_brushable_get_name (GtkCrusaderVillageBrushable *self);

guint8 *
gtk_crusader_village_brushable_get_mask (GtkCrusaderVillageBrushable *self,
                                         int                         *width,
                                         int                         *height);

GtkAdjustment *
gtk_crusader_village_brushable_get_adjustment (GtkCrusaderVillageBrushable *self);

GdkTexture *
gtk_crusader_village_brushable_get_thumbnail (GtkCrusaderVillageBrushable *self);

G_END_DECLS
