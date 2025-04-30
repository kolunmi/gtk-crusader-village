/* gtk-crusader-village-map.h
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

#include "gtk-crusader-village-item-store.h"

G_BEGIN_DECLS

#define GCV_MAP_ERROR (gcv_map_error_quark ())
GQuark gcv_map_error_quark (void);

typedef enum
{
  GCV_MAP_ERROR_SOURCEHOLD_FAILED = 0,
  GCV_MAP_ERROR_INVALID_JSON_STRUCTURE,
} GcvMapError;

#define GCV_TYPE_MAP (gcv_map_get_type ())

G_DECLARE_FINAL_TYPE (GcvMap, gcv_map, GCV, MAP, GObject)

void
gcv_map_new_from_aiv_file_async (GFile              *file,
                                 GcvItemStore       *store,
                                 const char         *python_exe,
                                 const char         *module_dir,
                                 int                 io_priority,
                                 GCancellable       *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer            user_data);

GcvMap *
gcv_map_new_from_aiv_file_finish (GAsyncResult *result,
                                  GError      **error);

void
gcv_map_save_to_aiv_file_async (GcvMap             *self,
                                GFile              *file,
                                const char         *python_exe,
                                const char         *module_dir,
                                int                 io_priority,
                                GCancellable       *cancellable,
                                GAsyncReadyCallback callback,
                                gpointer            user_data);

gboolean
gcv_map_save_to_aiv_file_finish (GAsyncResult *result,
                                 GError      **error);

G_END_DECLS
