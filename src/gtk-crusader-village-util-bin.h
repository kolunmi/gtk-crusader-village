/* gtk-crusader-village-util-bin.h
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

#define GTK_CRUSADER_VILLAGE_TYPE_UTIL_BIN (gtk_crusader_village_util_bin_get_type ())

G_DECLARE_DERIVABLE_TYPE (GtkCrusaderVillageUtilBin, gtk_crusader_village_util_bin, GTK_CRUSADER_VILLAGE, UTIL_BIN, GtkWidget)

struct _GtkCrusaderVillageUtilBinClass
{
  GtkWidgetClass parent_class;
};

GtkWidget *
gtk_crusader_village_util_bin_new (void);

GtkWidget *
gtk_crusader_village_util_bin_get_child (GtkCrusaderVillageUtilBin *self);

void
gtk_crusader_village_util_bin_set_child (GtkCrusaderVillageUtilBin *self,
                                         GtkWidget                 *child);

G_END_DECLS
