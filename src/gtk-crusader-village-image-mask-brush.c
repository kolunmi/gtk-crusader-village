/* gtk-crusader-village-image-mask-brush.c
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
#include "gtk-crusader-village-image-mask-brush.h"

/* clang-format off */
G_DEFINE_QUARK (gtk-crusader-village-image-mask-brush-error-quark, gcv_image_mask_brush_error);
/* clang-format on */

struct _GcvImageMaskBrush
{
  GObject parent_instance;

  char       *name;
  char       *file;
  guint8     *cache;
  int         width;
  int         height;
  GdkTexture *thumbnail;
};

static void
brushable_iface_init (GcvBrushableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (
    GcvImageMaskBrush,
    gcv_image_mask_brush,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GCV_TYPE_BRUSHABLE, brushable_iface_init))

static void
new_from_file_async_thread (GTask        *task,
                            gpointer      object,
                            gpointer      task_data,
                            GCancellable *cancellable);

static void
gcv_image_mask_brush_dispose (GObject *object)
{
  GcvImageMaskBrush *self = GCV_IMAGE_MASK_BRUSH (object);

  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->file, g_free);
  g_clear_pointer (&self->cache, g_free);
  g_clear_object (&self->thumbnail);

  G_OBJECT_CLASS (gcv_image_mask_brush_parent_class)->dispose (object);
}

static void
gcv_image_mask_brush_class_init (GcvImageMaskBrushClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gcv_image_mask_brush_dispose;
}

static void
gcv_image_mask_brush_init (GcvImageMaskBrush *self)
{
}

static char *
gcv_image_mask_brush_get_name (GcvBrushable *self)
{
  GcvImageMaskBrush *brush = GCV_IMAGE_MASK_BRUSH (self);

  g_return_val_if_fail (brush->name != NULL, NULL);

  return g_strdup (brush->name);
}

static char *
gcv_image_mask_brush_get_file (GcvBrushable *self)
{
  GcvImageMaskBrush *brush = GCV_IMAGE_MASK_BRUSH (self);

  return brush->file != NULL ? g_strdup (brush->file) : NULL;
}

static guint8 *
gcv_image_mask_brush_get_mask (GcvBrushable *self,
                               int          *width,
                               int          *height)
{
  GcvImageMaskBrush *brush = GCV_IMAGE_MASK_BRUSH (self);

  g_return_val_if_fail (brush->cache != NULL, NULL);

  *width  = brush->width;
  *height = brush->height;
  return g_memdup2 (brush->cache, brush->width * brush->height * sizeof (guint8));
}

static GtkAdjustment *
gcv_image_mask_brush_get_adjustment (GcvBrushable *self)
{
  /* We can't resize this brush (yet?) */
  return NULL;
}

static GdkTexture *
gcv_image_mask_brush_get_thumbnail (GcvBrushable *self)
{
  GcvImageMaskBrush *brush = GCV_IMAGE_MASK_BRUSH (self);

  g_return_val_if_fail (brush->thumbnail != NULL, NULL);

  return g_object_ref (brush->thumbnail);
}

static void
brushable_iface_init (GcvBrushableInterface *iface)
{
  iface->get_name       = gcv_image_mask_brush_get_name;
  iface->get_file       = gcv_image_mask_brush_get_file;
  iface->get_mask       = gcv_image_mask_brush_get_mask;
  iface->get_adjustment = gcv_image_mask_brush_get_adjustment;
  iface->get_thumbnail  = gcv_image_mask_brush_get_thumbnail;
}

void
gcv_image_mask_brush_new_from_file_async (GFile              *file,
                                          int                 io_priority,
                                          GCancellable       *cancellable,
                                          GAsyncReadyCallback callback,
                                          gpointer            user_data)
{
  g_autoptr (GTask) task = NULL;

  g_return_if_fail (G_IS_FILE (file));

  task = g_task_new (file, cancellable, callback, user_data);
  g_task_set_source_tag (task, gcv_image_mask_brush_new_from_file_async);
  g_task_set_priority (task, io_priority);
  g_task_set_check_cancellable (task, TRUE);
  g_task_run_in_thread (task, new_from_file_async_thread);
}

GcvImageMaskBrush *
gcv_image_mask_brush_new_from_file_finish (GAsyncResult *result,
                                           GError      **error)
{
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) ==
                            gcv_image_mask_brush_new_from_file_async,
                        NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

static void
new_from_file_async_thread (GTask        *task,
                            gpointer      object,
                            gpointer      task_data,
                            GCancellable *cancellable)
{
  GFile           *file               = object;
  g_autofree char *path               = NULL;
  g_autoptr (GError) local_error      = NULL;
  g_autoptr (GdkPixbuf) pixbuf        = NULL;
  int                width            = 0;
  int                height           = 0;
  gsize              stride           = 0;
  int                pixel_byte_width = 0;
  const guint8      *pixels           = NULL;
  g_autofree guint8 *data             = NULL;
  g_autoptr (GcvImageMaskBrush) brush = NULL;

  if (g_task_return_error_if_cancelled (task))
    return;

  path   = g_file_get_path (file);
  pixbuf = gdk_pixbuf_new_from_file (path, &local_error);

  if (pixbuf == NULL)
    goto err;

  width  = gdk_pixbuf_get_height (pixbuf);
  height = gdk_pixbuf_get_width (pixbuf);

  if (width > 100 || height > 100)
    {
      g_task_return_new_error (
          task,
          GCV_IMAGE_MASK_BRUSH_ERROR,
          GCV_IMAGE_MASK_BRUSH_ERROR_IMAGE_TOO_LARGE,
          "The mask image's pixel size must not exceed the size of the map "
          "in either dimension (100x100). Your image's size was %dx%d",
          width, height);
      return;
    }

  stride           = gdk_pixbuf_get_rowstride (pixbuf);
  pixel_byte_width = gdk_pixbuf_get_has_alpha (pixbuf) ? 4 : 3;
  pixels           = gdk_pixbuf_read_pixels (pixbuf);

  data = g_malloc0_n (width * height, sizeof (*data));
  for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
        {
          guint8 r = 0;
          guint8 g = 0;
          guint8 b = 0;

          r = pixels[y * stride + x * pixel_byte_width + 0];
          g = pixels[y * stride + x * pixel_byte_width + 1];
          b = pixels[y * stride + x * pixel_byte_width + 2];

          data[y * width + x] = (r > 0 || g > 0 || b > 0) ? 1 : 0;
        }
    }

  brush            = g_object_new (GCV_TYPE_IMAGE_MASK_BRUSH, NULL);
  brush->name      = g_file_get_basename (file);
  brush->file      = g_file_get_path (file);
  brush->cache     = g_steal_pointer (&data);
  brush->width     = width;
  brush->height    = height;
  brush->thumbnail = gdk_texture_new_for_pixbuf (pixbuf);

  g_task_return_pointer (task, g_steal_pointer (&brush), g_object_unref);
  return;

err:
  g_task_return_error (task, g_steal_pointer (&local_error));
}
