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

#include "gtk-crusader-village-dialog-window.h"
#include "gtk-crusader-village-preferences-window.h"
#include "gtk-crusader-village-theme-utils.h"

#define KEY_PYTHON_INSTALL   "sourcehold-python-installation"
#define KEY_THEME            "theme"
#define KEY_SHOW_GRID        "show-grid"
#define KEY_BACKGROUND_IMAGE "background-image"
#define KEY_SHOW_GRADIENT    "show-gradient"
#define KEY_SHOW_CURSOR_GLOW "show-cursor-glow"

/* TODO: perhaps read from schema instead */
static const char *theme_choices[] = {
  [GTK_CRUSADER_VILLAGE_THEME_OPTION_SHC_DEFAULT] = "shc-default",
  [GTK_CRUSADER_VILLAGE_THEME_OPTION_SHC_LIGHT]   = "shc-light",
  [GTK_CRUSADER_VILLAGE_THEME_OPTION_SHC_DARK]    = "shc-dark",
  [GTK_CRUSADER_VILLAGE_THEME_OPTION_DEFAULT]     = "default",
  [GTK_CRUSADER_VILLAGE_THEME_OPTION_LIGHT]       = "light",
  [GTK_CRUSADER_VILLAGE_THEME_OPTION_DARK]        = "dark",
};

struct _GtkCrusaderVillagePreferencesWindow
{
  GtkWindow parent_instance;

  GSettings *settings;

  /* Template widgets */
  GtkEntry    *python_install;
  GtkDropDown *theme;
  GtkSwitch   *show_grid;
  GtkSwitch   *show_gradient;
  GtkSwitch   *show_cursor_glow;

  GtkLabel  *background_label;
  GtkButton *background_button;
  GtkButton *background_clear;
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
background_button_clicked (GtkButton                           *self,
                           GtkCrusaderVillagePreferencesWindow *window);

static void
background_clear_clicked (GtkButton                           *self,
                          GtkCrusaderVillagePreferencesWindow *window);

static void
image_dialog_finish_cb (GObject      *source_object,
                        GAsyncResult *res,
                        gpointer      data);

static void
read_theme (GtkCrusaderVillagePreferencesWindow *self,
            GSettings                           *settings,
            const char                          *key);

static void
write_theme (GtkCrusaderVillagePreferencesWindow *self,
             GSettings                           *settings,
             const char                          *key);

static void
read_python_install (GtkCrusaderVillagePreferencesWindow *self,
                     GSettings                           *settings,
                     const char                          *key);

static void
read_background_image (GtkCrusaderVillagePreferencesWindow *self,
                       GSettings                           *settings,
                       const char                          *key);

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
          g_signal_connect (self->settings, "changed",
                            G_CALLBACK (setting_changed), self);
          read_theme (self, self->settings, KEY_THEME);
          gtk_switch_set_active (self->show_grid, g_settings_get_boolean (self->settings, KEY_SHOW_GRID));
          gtk_switch_set_active (self->show_gradient, g_settings_get_boolean (self->settings, KEY_SHOW_GRADIENT));
          gtk_switch_set_active (self->show_cursor_glow, g_settings_get_boolean (self->settings, KEY_SHOW_CURSOR_GLOW));
          read_python_install (self, self->settings, KEY_PYTHON_INSTALL);
          read_background_image (self, self->settings, KEY_BACKGROUND_IMAGE);
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
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillagePreferencesWindow, theme);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillagePreferencesWindow, show_grid);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillagePreferencesWindow, show_gradient);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillagePreferencesWindow, show_cursor_glow);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillagePreferencesWindow, python_install);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillagePreferencesWindow, background_label);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillagePreferencesWindow, background_button);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillagePreferencesWindow, background_clear);
}

static void
gtk_crusader_village_preferences_window_init (GtkCrusaderVillagePreferencesWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  gtk_crusader_village_register_themed_window (GTK_WINDOW (self), TRUE);

  g_signal_connect (self->theme, "notify::selected-item",
                    G_CALLBACK (ui_changed), self);
  g_signal_connect (self->show_grid, "notify::active",
                    G_CALLBACK (ui_changed), self);
  g_signal_connect (self->show_gradient, "notify::active",
                    G_CALLBACK (ui_changed), self);
  g_signal_connect (self->show_cursor_glow, "notify::active",
                    G_CALLBACK (ui_changed), self);
  g_signal_connect (self->python_install, "notify::text",
                    G_CALLBACK (ui_changed), self);

  g_signal_connect (self->background_button, "clicked",
                    G_CALLBACK (background_button_clicked), self);
  g_signal_connect (self->background_clear, "clicked",
                    G_CALLBACK (background_clear_clicked), self);
}

static void
setting_changed (GSettings                           *self,
                 gchar                               *key,
                 GtkCrusaderVillagePreferencesWindow *window)
{
  if (g_strcmp0 (key, KEY_THEME) == 0)
    read_theme (window, self, key);
  else if (g_strcmp0 (key, KEY_SHOW_GRID) == 0)
    gtk_switch_set_active (window->show_grid, g_settings_get_boolean (self, KEY_SHOW_GRID));
  else if (g_strcmp0 (key, KEY_SHOW_GRADIENT) == 0)
    gtk_switch_set_active (window->show_gradient, g_settings_get_boolean (self, KEY_SHOW_GRADIENT));
  else if (g_strcmp0 (key, KEY_BACKGROUND_IMAGE) == 0)
    read_background_image (window, self, key);
  else if (g_strcmp0 (key, KEY_SHOW_CURSOR_GLOW) == 0)
    gtk_switch_set_active (window->show_cursor_glow, g_settings_get_boolean (self, KEY_SHOW_CURSOR_GLOW));
  else if (g_strcmp0 (key, KEY_PYTHON_INSTALL) == 0)
    read_python_install (window, self, key);
}

static void
ui_changed (GtkWidget                           *widget,
            GParamSpec                          *pspec,
            GtkCrusaderVillagePreferencesWindow *window)
{
  if (window->settings == NULL)
    return;

  g_signal_handlers_block_by_func (window->settings, setting_changed, window);

  if (widget == (GtkWidget *) window->theme)
    write_theme (window, window->settings, KEY_THEME);
  else if (widget == (GtkWidget *) window->show_grid)
    g_settings_set_boolean (window->settings, KEY_SHOW_GRID, gtk_switch_get_active (window->show_grid));
  else if (widget == (GtkWidget *) window->show_gradient)
    g_settings_set_boolean (window->settings, KEY_SHOW_GRADIENT, gtk_switch_get_active (window->show_gradient));
  else if (widget == (GtkWidget *) window->show_cursor_glow)
    g_settings_set_boolean (window->settings, KEY_SHOW_CURSOR_GLOW, gtk_switch_get_active (window->show_cursor_glow));
  else if (widget == (GtkWidget *) window->python_install)
    g_settings_set_string (window->settings, KEY_PYTHON_INSTALL, gtk_editable_get_text (GTK_EDITABLE (window->python_install)));

  g_signal_handlers_unblock_by_func (window->settings, setting_changed, window);
}

static void
background_button_clicked (GtkButton                           *self,
                           GtkCrusaderVillagePreferencesWindow *window)
{
  g_autoptr (GtkFileDialog) file_dialog = NULL;
  g_autoptr (GtkFileFilter) filter      = NULL;

  file_dialog = gtk_file_dialog_new ();
  filter      = gtk_file_filter_new ();

  gtk_file_filter_add_mime_type (filter, "image/*");
  gtk_file_dialog_set_default_filter (file_dialog, filter);

  gtk_file_dialog_open (file_dialog, GTK_WINDOW (window),
                        NULL, image_dialog_finish_cb, window);
}

static void
background_clear_clicked (GtkButton                           *self,
                          GtkCrusaderVillagePreferencesWindow *window)
{
  g_settings_set_string (window->settings, KEY_BACKGROUND_IMAGE, "");
}

static void
image_dialog_finish_cb (GObject      *source_object,
                        GAsyncResult *res,
                        gpointer      data)
{
  GtkCrusaderVillagePreferencesWindow *self = data;
  g_autoptr (GError) local_error            = NULL;
  g_autoptr (GFile) file                    = NULL;

  file = gtk_file_dialog_open_finish (GTK_FILE_DIALOG (source_object), res, &local_error);

  if (file != NULL)
    {
      g_autofree char *path = NULL;

      path = g_file_get_path (file);
      g_settings_set_string (self->settings, KEY_BACKGROUND_IMAGE, path);
    }
  else
    gtk_crusader_village_dialog (
        "An Error Occurred",
        "Could not retrieve image from disk.",
        local_error->message,
        GTK_WINDOW (self), NULL);
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
      NULL);

  gtk_window_set_transient_for (window, parent);
  gtk_window_present (window);
}

static void
read_theme (GtkCrusaderVillagePreferencesWindow *self,
            GSettings                           *settings,
            const char                          *key)
{
  g_autofree char *theme     = NULL;
  guint            theme_idx = GTK_CRUSADER_VILLAGE_THEME_OPTION_DEFAULT;

  theme     = g_settings_get_string (settings, key);
  theme_idx = gtk_crusader_village_theme_str_to_enum (theme);

  gtk_drop_down_set_selected (self->theme, theme_idx);
}

static void
write_theme (GtkCrusaderVillagePreferencesWindow *self,
             GSettings                           *settings,
             const char                          *key)
{
  guint idx = 0;

  idx = gtk_drop_down_get_selected (self->theme);
  g_assert (idx < G_N_ELEMENTS (theme_choices));

  g_settings_set_string (settings, key, theme_choices[idx]);
}

static void
read_python_install (GtkCrusaderVillagePreferencesWindow *self,
                     GSettings                           *settings,
                     const char                          *key)
{
  g_autofree char *path = NULL;

  path = g_settings_get_string (settings, key);
  gtk_editable_set_text (GTK_EDITABLE (self->python_install), path);
}

static void
read_background_image (GtkCrusaderVillagePreferencesWindow *self,
                       GSettings                           *settings,
                       const char                          *key)
{
  g_autofree char *path = NULL;

  path = g_settings_get_string (settings, key);
  gtk_label_set_label (self->background_label, path);
}

/* I don't like this whole setup */
int
gtk_crusader_village_theme_str_to_enum (const char *theme)
{
  for (int i = 0; i < G_N_ELEMENTS (theme_choices); i++)
    {
      if (g_strcmp0 (theme, theme_choices[i]) == 0)
        return i;
    }

  return GTK_CRUSADER_VILLAGE_THEME_OPTION_DEFAULT;
}
