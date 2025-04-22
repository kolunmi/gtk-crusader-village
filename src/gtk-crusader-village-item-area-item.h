/* gtk-crusader-village-item-area-item.h
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

#include "gtk-crusader-village-util-bin.h"

G_BEGIN_DECLS

#define GCV_TYPE_ITEM_AREA_ITEM (gcv_item_area_item_get_type ())

G_DECLARE_FINAL_TYPE (GcvItemAreaItem, gcv_item_area_item, GCV, ITEM_AREA_ITEM, GcvUtilBin)

G_END_DECLS
