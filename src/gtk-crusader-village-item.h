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

#define GTK_CRUSADER_VILLAGE_TYPE_ITEM (gtk_crusader_village_item_get_type ())

G_DECLARE_FINAL_TYPE (GtkCrusaderVillageItem, gtk_crusader_village_item, GTK_CRUSADER_VILLAGE, ITEM, GObject)

typedef enum
{
  GTK_CRUSADER_VILLAGE_ITEM_KIND_BUILDING,
  GTK_CRUSADER_VILLAGE_ITEM_KIND_UNIT,
  GTK_CRUSADER_VILLAGE_ITEM_KIND_WALL,
} GtkCrusaderVillageItemKind;

GType gtk_crusader_village_item_kind_get_type (void);
#define GTK_CRUSADER_VILLAGE_TYPE_ITEM_KIND (gtk_crusader_village_item_kind_get_type ())

GtkCrusaderVillageItem *
gtk_crusader_village_item_new_for_resource (const char *resource_path,
                                            GError    **error);

void
gtk_crusader_village_item_set_property_from_variant (GtkCrusaderVillageItem *self,
                                                     const char             *property,
                                                     GVariant               *variant);

G_END_DECLS
