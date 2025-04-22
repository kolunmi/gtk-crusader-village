/* gtk-crusader-village-square-brush.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gtk-crusader-village-brushable.h"
#include "gtk-crusader-village-square-brush.h"

struct _GcvSquareBrush
{
  GObject parent_instance;

  GtkAdjustment *adjustment;
};

static void
brushable_iface_init (GcvBrushableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (
    GcvSquareBrush,
    gcv_square_brush,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GCV_TYPE_BRUSHABLE, brushable_iface_init))

static void
gcv_square_brush_dispose (GObject *object)
{
  GcvSquareBrush *self = GCV_SQUARE_BRUSH (object);

  g_clear_object (&self->adjustment);

  G_OBJECT_CLASS (gcv_square_brush_parent_class)->dispose (object);
}

static void
gcv_square_brush_class_init (GcvSquareBrushClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gcv_square_brush_dispose;
}

static void
gcv_square_brush_init (GcvSquareBrush *self)
{
  self->adjustment = gtk_adjustment_new (1.0, 1.0, 10.0, 1.0, 1.0, 0.0);
}

static char *
gcv_square_brush_get_name (GcvBrushable *self)
{
  return g_strdup ("Square Brush");
}

static char *
gcv_square_brush_get_file (GcvBrushable *self)
{
  return NULL;
}

static guint8 *
gcv_square_brush_get_mask (GcvBrushable *self,
                           int          *width,
                           int          *height)
{
  GcvSquareBrush *brush     = GCV_SQUARE_BRUSH (self);
  int             size      = 0;
  gsize           mask_size = 0;
  guint8         *mask      = NULL;

  size    = gtk_adjustment_get_value (brush->adjustment);
  *width  = size;
  *height = size;

  mask_size = size * size * sizeof (guint8);
  mask      = g_malloc (mask_size);
  memset (mask, 1, mask_size);
  return mask;
}

static GtkAdjustment *
gcv_square_brush_get_adjustment (GcvBrushable *self)
{
  GcvSquareBrush *brush = GCV_SQUARE_BRUSH (self);

  return g_object_ref (brush->adjustment);
}

static GdkTexture *
gcv_square_brush_get_thumbnail (GcvBrushable *self)
{
  return NULL;
}

static void
brushable_iface_init (GcvBrushableInterface *iface)
{
  iface->get_name       = gcv_square_brush_get_name;
  iface->get_file       = gcv_square_brush_get_file;
  iface->get_mask       = gcv_square_brush_get_mask;
  iface->get_adjustment = gcv_square_brush_get_adjustment;
  iface->get_thumbnail  = gcv_square_brush_get_thumbnail;
}
