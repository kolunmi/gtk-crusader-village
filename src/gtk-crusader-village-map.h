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

#define GTK_CRUSADER_VILLAGE_MAP_ERROR (gtk_crusader_village_map_error_quark ())
GQuark gtk_crusader_village_map_error_quark (void);

typedef enum
{
  GTK_CRUSADER_VILLAGE_MAP_ERROR_SOURCEHOLD_FAILED = 0,
  GTK_CRUSADER_VILLAGE_MAP_ERROR_INVALID_JSON_STRUCTURE,
} GtkCrusaderVillageMapError;

#define GTK_CRUSADER_VILLAGE_TYPE_MAP (gtk_crusader_village_map_get_type ())

G_DECLARE_FINAL_TYPE (GtkCrusaderVillageMap, gtk_crusader_village_map, GTK_CRUSADER_VILLAGE, MAP, GObject)

void
gtk_crusader_village_map_new_from_aiv_file_async (GFile                       *file,
                                                  GtkCrusaderVillageItemStore *store,
                                                  const char                  *python_exe,
                                                  int                          io_priority,
                                                  GCancellable                *cancellable,
                                                  GAsyncReadyCallback          callback,
                                                  gpointer                     user_data);

GtkCrusaderVillageMap *
gtk_crusader_village_map_new_from_aiv_file_finish (GAsyncResult *result,
                                                   GError      **error);

void
gtk_crusader_village_map_save_to_aiv_file_async (GtkCrusaderVillageMap *self,
                                                 GFile                 *file,
                                                 const char            *python_exe,
                                                 int                    io_priority,
                                                 GCancellable          *cancellable,
                                                 GAsyncReadyCallback    callback,
                                                 gpointer               user_data);

gboolean
gtk_crusader_village_map_save_to_aiv_file_finish (GAsyncResult *result,
                                                  GError      **error);

G_END_DECLS
