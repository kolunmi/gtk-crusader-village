/* gtk-crusader-village-timeline-view.c
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

#include "config.h"

#include "gtk-crusader-village-timeline-view.h"

struct _GtkCrusaderVillageTimelineView
{
  GtkCrusaderVillageUtilBin parent_instance;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageTimelineView, gtk_crusader_village_timeline_view, GTK_CRUSADER_VILLAGE_TYPE_UTIL_BIN)

static void
gtk_crusader_village_timeline_view_class_init (GtkCrusaderVillageTimelineViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
}

static void
gtk_crusader_village_timeline_view_init (GtkCrusaderVillageTimelineView *self)
{
}
