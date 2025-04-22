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

G_DEFINE_INTERFACE (GcvBrushable, gcv_brushable, G_TYPE_OBJECT);

static char *
gcv_brushable_real_get_name (GcvBrushable *self)
{
  return NULL;
}

static char *
gcv_brushable_real_get_file (GcvBrushable *self)
{
  return NULL;
}

static guint8 *
gcv_brushable_real_get_mask (GcvBrushable *self,
                             int          *width,
                             int          *height)
{
  *width  = 0;
  *height = 0;
  return NULL;
}

static GtkAdjustment *
gcv_brushable_real_get_adjustment (GcvBrushable *self)
{
  return NULL;
}

static GdkTexture *
gcv_brushable_real_get_thumbnail (GcvBrushable *self)
{
  return NULL;
}

static void
gcv_brushable_default_init (GcvBrushableInterface *iface)
{
  iface->get_name       = gcv_brushable_real_get_name;
  iface->get_file       = gcv_brushable_real_get_file;
  iface->get_mask       = gcv_brushable_real_get_mask;
  iface->get_adjustment = gcv_brushable_real_get_adjustment;
  iface->get_thumbnail  = gcv_brushable_real_get_thumbnail;
}

char *
gcv_brushable_get_name (GcvBrushable *self)
{
  g_return_val_if_fail (GCV_IS_BRUSHABLE (self), NULL);

  return GCV_BRUSHABLE_GET_IFACE (self)->get_name (self);
}

char *
gcv_brushable_get_file (GcvBrushable *self)
{
  g_return_val_if_fail (GCV_IS_BRUSHABLE (self), NULL);

  return GCV_BRUSHABLE_GET_IFACE (self)->get_file (self);
}

guint8 *
gcv_brushable_get_mask (GcvBrushable *self,
                        int          *width,
                        int          *height)
{
  g_return_val_if_fail (GCV_IS_BRUSHABLE (self), NULL);
  g_return_val_if_fail (width != NULL, NULL);
  g_return_val_if_fail (height != NULL, NULL);

  return GCV_BRUSHABLE_GET_IFACE (self)->get_mask (self, width, height);
}

GtkAdjustment *
gcv_brushable_get_adjustment (GcvBrushable *self)
{
  g_return_val_if_fail (GCV_IS_BRUSHABLE (self), NULL);

  return GCV_BRUSHABLE_GET_IFACE (self)->get_adjustment (self);
}

GdkTexture *
gcv_brushable_get_thumbnail (GcvBrushable *self)
{
  g_return_val_if_fail (GCV_IS_BRUSHABLE (self), NULL);

  return GCV_BRUSHABLE_GET_IFACE (self)->get_thumbnail (self);
}
