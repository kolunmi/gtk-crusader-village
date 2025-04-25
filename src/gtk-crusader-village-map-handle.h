/* gtk-crusader-village-map-handle.h
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

#define GCV_TYPE_MAP_HANDLE (gcv_map_handle_get_type ())

G_DECLARE_FINAL_TYPE (GcvMapHandle, gcv_map_handle, GCV, MAP_HANDLE, GObject)

void
gcv_map_handle_undo (GcvMapHandle *self);

void
gcv_map_handle_redo (GcvMapHandle *self);

gboolean
gcv_map_handle_can_undo (GcvMapHandle *self);

gboolean
gcv_map_handle_can_redo (GcvMapHandle *self);

void
gcv_map_handle_clear_all (GcvMapHandle *self);

G_END_DECLS
