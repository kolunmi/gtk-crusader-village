/* gtk-crusader-village-image-mask-brush.h
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

#include <gio/gio.h>

G_BEGIN_DECLS

#define GTK_CRUSADER_VILLAGE_IMAGE_MASK_BRUSH_ERROR (gtk_crusader_village_image_mask_brush_error_quark ())
GQuark gtk_crusader_village_image_mask_brush_error_quark (void);

typedef enum
{
  GTK_CRUSADER_VILLAGE_IMAGE_MASK_BRUSH_ERROR_IMAGE_TOO_LARGE = 0,
} GtkCrusaderVillageImageMaskBrushError;

#define GTK_CRUSADER_VILLAGE_TYPE_IMAGE_MASK_BRUSH (gtk_crusader_village_image_mask_brush_get_type ())

G_DECLARE_FINAL_TYPE (GtkCrusaderVillageImageMaskBrush, gtk_crusader_village_image_mask_brush, GTK_CRUSADER_VILLAGE, IMAGE_MASK_BRUSH, GObject)

void
gtk_crusader_village_image_mask_brush_new_from_file_async (GFile              *file,
                                                           int                 io_priority,
                                                           GCancellable       *cancellable,
                                                           GAsyncReadyCallback callback,
                                                           gpointer            user_data);

GtkCrusaderVillageImageMaskBrush *
gtk_crusader_village_image_mask_brush_new_from_file_finish (GAsyncResult *result,
                                                            GError      **error);

G_END_DECLS
