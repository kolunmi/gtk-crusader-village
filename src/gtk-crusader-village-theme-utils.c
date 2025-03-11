/* gtk-crusader-village-theme-utils.c
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

#include "gtk-crusader-village-theme-utils.h"

static void
gtk_decoration_layout_changed (GtkSettings *settings,
                               GParamSpec  *pspec,
                               GtkWindow   *window);

static void
apply_css (GtkSettings *settings,
           GtkWindow   *window);

void
gtk_crusader_village_register_themed_window (GtkWindow *window,
                                             gboolean   animated)
{
  GtkSettings *settings = NULL;

  g_return_if_fail (GTK_IS_WINDOW (window));

  settings = gtk_settings_get_default ();
  g_assert (settings != NULL);

  g_signal_connect_object (
      settings, "notify::gtk-decoration-layout",
      G_CALLBACK (gtk_decoration_layout_changed),
      window, G_CONNECT_DEFAULT);

  gtk_widget_add_css_class (GTK_WIDGET (window), "shc");
  if (animated)
    gtk_widget_add_css_class (GTK_WIDGET (window), "window-intro");

  apply_css (settings, window);
}

static void
gtk_decoration_layout_changed (GtkSettings *settings,
                               GParamSpec  *pspec,
                               GtkWindow   *window)
{
  apply_css (settings, window);
}

static void
apply_css (GtkSettings *settings,
           GtkWindow   *window)
{
  gboolean         deletable = FALSE;
  gboolean         modal     = FALSE;
  g_autofree char *layout    = NULL;
  char            *cursor    = NULL;
  int              n_buttons = 0;

  gtk_widget_remove_css_class (GTK_WIDGET (window), "btns-0");
  gtk_widget_remove_css_class (GTK_WIDGET (window), "btns-1");
  gtk_widget_remove_css_class (GTK_WIDGET (window), "btns-2");
  gtk_widget_remove_css_class (GTK_WIDGET (window), "btns-3");
  gtk_widget_remove_css_class (GTK_WIDGET (window), "btns-4");

  g_object_get (
      window,
      "deletable", &deletable,
      "modal", &modal,
      NULL);

  if (!deletable || modal)
    {
      gtk_widget_add_css_class (
          GTK_WIDGET (window),
          deletable ? "btns-1" : "btns-0");
      return;
    }

  g_object_get (
      settings,
      "gtk-decoration-layout", &layout,
      NULL);

  if (layout == NULL)
    {
      gtk_widget_add_css_class (GTK_WIDGET (window), "btns-0");
      return;
    }

  if ((cursor = strchr (layout, ':')) != NULL)
    cursor++;
  else
    cursor = layout;

  if (cursor != NULL)
    {
      n_buttons++;
      while ((cursor = strchr (cursor, ',')) != NULL)
        {
          n_buttons++;
          cursor++;
        }
    }

  if (GTK_IS_APPLICATION_WINDOW (window))
    /* assume there is a main menu */
    n_buttons++;

  switch (n_buttons)
    {
    case 0:
      gtk_widget_add_css_class (GTK_WIDGET (window), "btns-0");
      break;
    case 1:
      gtk_widget_add_css_class (GTK_WIDGET (window), "btns-1");
      break;
    case 2:
      gtk_widget_add_css_class (GTK_WIDGET (window), "btns-2");
      break;
    case 3:
      gtk_widget_add_css_class (GTK_WIDGET (window), "btns-3");
      break;
    case 4:
      gtk_widget_add_css_class (GTK_WIDGET (window), "btns-4");
      break;
    default:
      g_critical ("shc theme: unsupported decoration layout");
      break;
    }
}
