/* gtk-crusader-village-preferences-window.c
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

#include "gtk-crusader-village-preferences-window.h"

struct _GtkCrusaderVillagePreferencesWindow
{
  GtkApplicationWindow parent_instance;

  GSettings *settings;

  /* Template widgets */
  GtkSwitch *show_grid;
  GtkSwitch *show_gradient;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillagePreferencesWindow, gtk_crusader_village_preferences_window, GTK_TYPE_WINDOW)

enum
{
  PROP_0,

  PROP_SETTINGS,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
setting_changed (GSettings                           *self,
                 gchar                               *key,
                 GtkCrusaderVillagePreferencesWindow *window);

static void
ui_changed (GtkWidget                           *widget,
            GParamSpec                          *pspec,
            GtkCrusaderVillagePreferencesWindow *window);

static void
gtk_crusader_village_preferences_window_dispose (GObject *object)
{
  GtkCrusaderVillagePreferencesWindow *self = GTK_CRUSADER_VILLAGE_PREFERENCES_WINDOW (object);

  if (self->settings != NULL)
    g_signal_handlers_disconnect_by_func (
        self->settings, setting_changed, self);
  g_clear_object (&self->settings);

  G_OBJECT_CLASS (gtk_crusader_village_preferences_window_parent_class)->dispose (object);
}

static void
gtk_crusader_village_preferences_window_get_property (GObject    *object,
                                                      guint       prop_id,
                                                      GValue     *value,
                                                      GParamSpec *pspec)
{
  GtkCrusaderVillagePreferencesWindow *self = GTK_CRUSADER_VILLAGE_PREFERENCES_WINDOW (object);

  switch (prop_id)
    {
    case PROP_SETTINGS:
      g_value_set_object (value, self->settings);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_preferences_window_set_property (GObject      *object,
                                                      guint         prop_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec)
{
  GtkCrusaderVillagePreferencesWindow *self = GTK_CRUSADER_VILLAGE_PREFERENCES_WINDOW (object);

  switch (prop_id)
    {
    case PROP_SETTINGS:
      if (self->settings != NULL)
        g_signal_handlers_disconnect_by_func (
            self->settings, setting_changed, self);
      g_clear_object (&self->settings);

      self->settings = g_value_dup_object (value);
      if (self->settings != NULL)
        {
          gtk_switch_set_active (self->show_grid, g_settings_get_boolean (self->settings, "show-grid"));
          gtk_switch_set_active (self->show_gradient, g_settings_get_boolean (self->settings, "show-gradient"));
          g_signal_connect (self->settings, "changed",
                            G_CALLBACK (setting_changed), self);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_preferences_window_class_init (GtkCrusaderVillagePreferencesWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gtk_crusader_village_preferences_window_dispose;
  object_class->get_property = gtk_crusader_village_preferences_window_get_property;
  object_class->set_property = gtk_crusader_village_preferences_window_set_property;

  props[PROP_SETTINGS] =
      g_param_spec_object (
          "settings",
          "Settings",
          "The settings object for this window",
          G_TYPE_SETTINGS,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/GtkCrusaderVillage/gtk-crusader-village-preferences-window.ui");
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillagePreferencesWindow, show_grid);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillagePreferencesWindow, show_gradient);
}

static void
gtk_crusader_village_preferences_window_init (GtkCrusaderVillagePreferencesWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect (self->show_grid, "notify::active",
                    G_CALLBACK (ui_changed), self);
  g_signal_connect (self->show_gradient, "notify::active",
                    G_CALLBACK (ui_changed), self);
}

static void
setting_changed (GSettings                           *self,
                 gchar                               *key,
                 GtkCrusaderVillagePreferencesWindow *window)
{
  if (g_strcmp0 (key, "show-grid") == 0)
    gtk_switch_set_active (window->show_grid, g_settings_get_boolean (self, "show-grid"));
  else if (g_strcmp0 (key, "show-gradient") == 0)
    gtk_switch_set_active (window->show_gradient, g_settings_get_boolean (self, "show-gradient"));
}

static void
ui_changed (GtkWidget                           *widget,
            GParamSpec                          *pspec,
            GtkCrusaderVillagePreferencesWindow *window)
{
  if (window->settings == NULL)
    return;

  if (widget == (GtkWidget *) window->show_grid)
    g_settings_set_boolean (window->settings, "show-grid", gtk_switch_get_active (window->show_grid));
  else if (widget == (GtkWidget *) window->show_gradient)
    g_settings_set_boolean (window->settings, "show-gradient", gtk_switch_get_active (window->show_gradient));
}

void
gtk_crusader_village_preferences_window_spawn (GSettings *settings,
                                               GtkWindow *parent)
{
  GtkWindow *window = NULL;

  g_return_if_fail (G_IS_SETTINGS (settings));
  g_return_if_fail (GTK_IS_WINDOW (parent));

  window = g_object_new (
      GTK_CRUSADER_VILLAGE_TYPE_PREFERENCES_WINDOW,
      "settings", settings,
      "modal", TRUE,
      "destroy-with-parent", TRUE,
      "resizable", FALSE,
      NULL);

  gtk_window_set_transient_for (window, parent);
  gtk_window_present (window);
}
