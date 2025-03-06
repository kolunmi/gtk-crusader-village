/* gtk-crusader-village-dialog-window.h
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

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_CRUSADER_VILLAGE_TYPE_DIALOG_WINDOW (gtk_crusader_village_dialog_window_get_type ())

G_DECLARE_FINAL_TYPE (GtkCrusaderVillageDialogWindow, gtk_crusader_village_dialog_window, GTK_CRUSADER_VILLAGE, DIALOG_WINDOW, GtkWindow)

GtkCrusaderVillageDialogWindow *
gtk_crusader_village_dialog (const char *title,
                             const char *header,
                             const char *message,
                             GtkWindow  *parent,
                             GVariant   *structure);

G_END_DECLS
