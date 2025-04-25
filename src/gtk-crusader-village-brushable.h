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

#define GCV_TYPE_BRUSHABLE (gcv_brushable_get_type ())

G_DECLARE_INTERFACE (GcvBrushable, gcv_brushable, GCV, BRUSHABLE, GObject)

struct _GcvBrushableInterface
{
  GTypeInterface parent_iface;

  char *(*get_name) (GcvBrushable *self);
  char *(*get_file) (GcvBrushable *self);
  guint8 *(*get_mask) (GcvBrushable *self,
                       int          *width,
                       int          *height);
  GtkAdjustment *(*get_adjustment) (GcvBrushable *self);
  GdkTexture *(*get_thumbnail) (GcvBrushable *self);
};

char *
gcv_brushable_get_name (GcvBrushable *self);

char *
gcv_brushable_get_file (GcvBrushable *self);

guint8 *
gcv_brushable_get_mask (GcvBrushable *self,
                        int          *width,
                        int          *height);

GtkAdjustment *
gcv_brushable_get_adjustment (GcvBrushable *self);

GdkTexture *
gcv_brushable_get_thumbnail (GcvBrushable *self);

G_END_DECLS
