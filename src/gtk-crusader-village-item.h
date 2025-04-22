/* gtk-crusader-village-item.h
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

#include <glib-object.h>

G_BEGIN_DECLS

#define GCV_TYPE_ITEM (gcv_item_get_type ())

G_DECLARE_FINAL_TYPE (GcvItem, gcv_item, GCV, ITEM, GObject)

typedef enum
{
  GCV_ITEM_KIND_BUILDING,
  GCV_ITEM_KIND_UNIT,
  GCV_ITEM_KIND_WALL,
  GCV_ITEM_KIND_MOAT,
} GcvItemKind;

GType gcv_item_kind_get_type (void);
#define GCV_TYPE_ITEM_KIND (gcv_item_kind_get_type ())

GcvItem *
gcv_item_new_for_resource (const char *resource_path,
                           GError    **error);

void
gcv_item_set_property_from_variant (GcvItem    *self,
                                    const char *property,
                                    GVariant   *variant);

const char *
gcv_item_get_name (GcvItem *self);

gpointer
gcv_item_get_tile_resource_hash (GcvItem *self);

G_END_DECLS
