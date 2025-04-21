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

struct _GtkCrusaderVillageSquareBrush
{
  GObject parent_instance;

  GtkAdjustment *adjustment;
};

static void
brushable_iface_init (GtkCrusaderVillageBrushableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (
    GtkCrusaderVillageSquareBrush,
    gtk_crusader_village_square_brush,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GTK_CRUSADER_VILLAGE_TYPE_BRUSHABLE, brushable_iface_init))

static void
gtk_crusader_village_square_brush_dispose (GObject *object)
{
  GtkCrusaderVillageSquareBrush *self = GTK_CRUSADER_VILLAGE_SQUARE_BRUSH (object);

  g_clear_object (&self->adjustment);

  G_OBJECT_CLASS (gtk_crusader_village_square_brush_parent_class)->dispose (object);
}

static void
gtk_crusader_village_square_brush_class_init (GtkCrusaderVillageSquareBrushClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gtk_crusader_village_square_brush_dispose;
}

static void
gtk_crusader_village_square_brush_init (GtkCrusaderVillageSquareBrush *self)
{
  self->adjustment = gtk_adjustment_new (1.0, 1.0, 10.0, 1.0, 1.0, 0.0);
}

static char *
gtk_crusader_village_square_brush_get_name (GtkCrusaderVillageBrushable *self)
{
  return g_strdup ("Square Brush");
}

static char *
gtk_crusader_village_square_brush_get_file (GtkCrusaderVillageBrushable *self)
{
  return NULL;
}

static guint8 *
gtk_crusader_village_square_brush_get_mask (GtkCrusaderVillageBrushable *self,
                                            int                         *width,
                                            int                         *height)
{
  GtkCrusaderVillageSquareBrush *brush     = GTK_CRUSADER_VILLAGE_SQUARE_BRUSH (self);
  int                            size      = 0;
  gsize                          mask_size = 0;
  guint8                        *mask      = NULL;

  size    = gtk_adjustment_get_value (brush->adjustment);
  *width  = size;
  *height = size;

  mask_size = size * size * sizeof (guint8);
  mask      = g_malloc (mask_size);
  memset (mask, 1, mask_size);
  return mask;
}

static GtkAdjustment *
gtk_crusader_village_square_brush_get_adjustment (GtkCrusaderVillageBrushable *self)
{
  GtkCrusaderVillageSquareBrush *brush = GTK_CRUSADER_VILLAGE_SQUARE_BRUSH (self);

  return g_object_ref (brush->adjustment);
}

static GdkTexture *
gtk_crusader_village_square_brush_get_thumbnail (GtkCrusaderVillageBrushable *self)
{
  return NULL;
}

static void
brushable_iface_init (GtkCrusaderVillageBrushableInterface *iface)
{
  iface->get_name       = gtk_crusader_village_square_brush_get_name;
  iface->get_file       = gtk_crusader_village_square_brush_get_file;
  iface->get_mask       = gtk_crusader_village_square_brush_get_mask;
  iface->get_adjustment = gtk_crusader_village_square_brush_get_adjustment;
  iface->get_thumbnail  = gtk_crusader_village_square_brush_get_thumbnail;
}
