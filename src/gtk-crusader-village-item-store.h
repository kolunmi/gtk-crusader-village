/* gtk-crusader-village-item-store.h
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

#include "gtk-crusader-village-item.h"

G_BEGIN_DECLS

#define GTK_CRUSADER_VILLAGE_TYPE_ITEM_STORE (gtk_crusader_village_item_store_get_type ())

G_DECLARE_FINAL_TYPE (GtkCrusaderVillageItemStore, gtk_crusader_village_item_store, GTK_CRUSADER_VILLAGE, ITEM_STORE, GObject)

void
gtk_crusader_village_item_store_read_resources (GtkCrusaderVillageItemStore *self);

GtkCrusaderVillageItem *
gtk_crusader_village_item_store_query_id (GtkCrusaderVillageItemStore *self,
                                          int                          id);

GtkCrusaderVillageItemStore *
gtk_crusader_village_item_store_dup (GtkCrusaderVillageItemStore *self);

G_END_DECLS
