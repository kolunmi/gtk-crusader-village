/* gtk-crusader-village-preferences-window.h
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

#define GCV_TYPE_PREFERENCES_WINDOW (gcv_preferences_window_get_type ())

G_DECLARE_FINAL_TYPE (GcvPreferencesWindow, gcv_preferences_window, GCV, PREFERENCES_WINDOW, GtkWindow)

/* Keep these in sync with `gtk-crusader-village-preferences-window.ui` */
/* TODO: make this better */
#define GCV_THEME_OPTION_DEFAULT     0
#define GCV_THEME_OPTION_LIGHT       1
#define GCV_THEME_OPTION_DARK        2
// #define GCV_THEME_OPTION_SHC_DEFAULT 3
// #define GCV_THEME_OPTION_SHC_LIGHT   4
// #define GCV_THEME_OPTION_SHC_DARK    5

void
gcv_preferences_window_spawn (GSettings *settings,
                              GtkWindow *parent);

int
gcv_theme_str_to_enum (const char *theme);

G_END_DECLS
