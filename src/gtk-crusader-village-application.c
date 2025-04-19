/* gtk-crusader-village-application.c
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

/* The portal stuff in this file was taken from libadwaita */

#include "config.h"
#include <glib/gi18n.h>

#include "gtk-crusader-village-application.h"
#include "gtk-crusader-village-brushable.h"
#include "gtk-crusader-village-dialog-window.h"
#include "gtk-crusader-village-map.h"
#include "gtk-crusader-village-preferences-window.h"
#include "gtk-crusader-village-window.h"

#if defined(__APPLE__) || defined(G_OS_WIN32)
#undef USE_THEME_PORTAL
#else
#include <gio/gio.h>
#define USE_THEME_PORTAL
#define PORTAL_BUS_NAME           "org.freedesktop.portal.Desktop"
#define PORTAL_OBJECT_PATH        "/org/freedesktop/portal/desktop"
#define PORTAL_SETTINGS_INTERFACE "org.freedesktop.portal.Settings"
#define PORTAL_ERROR_NOT_FOUND    "org.freedesktop.portal.Error.NotFound"
static void
init_portal (GtkCrusaderVillageApplication *self);
#endif

struct _GtkCrusaderVillageApplication
{
  GtkApplication parent_instance;

  GSettings *settings;
  int        theme_setting;

  GtkCrusaderVillageItemStore *item_store;
  GListStore                  *brush_store;

  GtkCssProvider *custom_css;
  GtkCssProvider *shc_theme_light_css;
  GtkCssProvider *shc_theme_dark_css;

#ifdef USE_THEME_PORTAL
  GDBusProxy *settings_portal;
  gboolean    portal_wants_dark;
#endif
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageApplication, gtk_crusader_village_application, GTK_TYPE_APPLICATION)

enum
{
  PROP_0,

  PROP_SETTINGS,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
theme_changed (GSettings                     *self,
               char                          *key,
               GtkCrusaderVillageApplication *application);

static void
read_theme_from_settings (GtkCrusaderVillageApplication *self);

static void
apply_gtk_theme (GtkCrusaderVillageApplication *self);

static void
ensure_settings (GtkCrusaderVillageApplication *self);

static void
gtk_crusader_village_application_get_property (GObject    *object,
                                               guint       prop_id,
                                               GValue     *value,
                                               GParamSpec *pspec)
{
  GtkCrusaderVillageApplication *self = GTK_CRUSADER_VILLAGE_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_SETTINGS:
      ensure_settings (self);
      g_value_set_object (value, self->settings);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_application_set_property (GObject      *object,
                                               guint         prop_id,
                                               const GValue *value,
                                               GParamSpec   *pspec)
{
  GtkCrusaderVillageApplication *self = GTK_CRUSADER_VILLAGE_APPLICATION (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

GtkCrusaderVillageApplication *
gtk_crusader_village_application_new (const char       *application_id,
                                      GApplicationFlags flags)
{
  g_return_val_if_fail (application_id != NULL, NULL);

  return g_object_new (
      GTK_CRUSADER_VILLAGE_TYPE_APPLICATION,
      "application-id", application_id,
      "flags", flags,
      "resource-base-path", "/am/kolunmi/GtkCrusaderVillage",
      NULL);
}

static void
gtk_crusader_village_application_dispose (GObject *object)
{
  GtkCrusaderVillageApplication *self = GTK_CRUSADER_VILLAGE_APPLICATION (object);

#ifdef USE_THEME_PORTAL
  g_clear_object (&self->settings_portal);
#endif

  if (self->settings != NULL)
    g_signal_handlers_disconnect_by_func (
        self->settings, theme_changed, self);
  g_clear_object (&self->settings);

  g_clear_object (&self->item_store);
  g_clear_object (&self->brush_store);

  g_clear_object (&self->custom_css);
  g_clear_object (&self->shc_theme_light_css);
  g_clear_object (&self->shc_theme_dark_css);

  G_OBJECT_CLASS (gtk_crusader_village_application_parent_class)->dispose (object);
}

static void
gtk_crusader_village_application_activate (GApplication *app)
{
  GtkCrusaderVillageApplication *self   = GTK_CRUSADER_VILLAGE_APPLICATION (app);
  GtkWindow                     *window = NULL;

  self->custom_css = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (self->custom_css, "/am/kolunmi/GtkCrusaderVillage/gtk/styles.css");
  gtk_style_context_add_provider_for_display (
      gdk_display_get_default (),
      GTK_STYLE_PROVIDER (self->custom_css),
      GTK_STYLE_PROVIDER_PRIORITY_USER);

  self->shc_theme_light_css = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (self->shc_theme_light_css, "/am/kolunmi/GtkCrusaderVillage/gtk/shc-light.css");

  self->shc_theme_dark_css = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (self->shc_theme_dark_css, "/am/kolunmi/GtkCrusaderVillage/gtk/shc-dark.css");

#ifdef USE_THEME_PORTAL
  init_portal (self);
#endif
  ensure_settings (self);

  window = gtk_application_get_active_window (GTK_APPLICATION (app));

  if (window == NULL)
    window = g_object_new (
        GTK_CRUSADER_VILLAGE_TYPE_WINDOW,
        "application", app,
        "item-store", self->item_store,
        "brush-store", self->brush_store,
        "settings", self->settings,
        NULL);

  gtk_window_present (window);

  if (g_settings_get_boolean (self->settings, "show-startup-greeting"))
    g_action_group_activate_action (G_ACTION_GROUP (app), "greeting", NULL);
}

static void
gtk_crusader_village_application_class_init (GtkCrusaderVillageApplicationClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class    = G_APPLICATION_CLASS (klass);

  object_class->dispose      = gtk_crusader_village_application_dispose;
  object_class->get_property = gtk_crusader_village_application_get_property;
  object_class->set_property = gtk_crusader_village_application_set_property;

  app_class->activate = gtk_crusader_village_application_activate;

  props[PROP_SETTINGS] =
      g_param_spec_object (
          "settings",
          "Settings",
          "The settings object for this application",
          G_TYPE_SETTINGS,
          G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static char *
get_python_install (GtkCrusaderVillageApplication *self)
{
  g_autofree char *python_install = NULL;

  python_install = g_settings_get_string (self->settings, "sourcehold-python-installation");
  if (python_install[0] != '\0')
    return g_steal_pointer (&python_install);
  else
    return NULL;
}

#define INSTALLATION_WARNING_TEXT                                                              \
  "In order to perform import/export of AIV files, this application "                          \
  "needs a functional python installation with the \"sourcehold\" "                            \
  "module installed.\n\nTo enable AIV import/export, "                                         \
  "<a href=\"https://github.com/sourcehold/sourcehold-maps?tab=readme-ov-file#installation\">" \
  "follow Sourcehold's instructions</a> to install the module, then "                          \
  "open this application's preferences window and paste the ABSOLUTE "                         \
  "PATH of your python binary into the corresponding field. If you "                           \
  "don't have python installed yet, <a href=\"https://www.python.org/downloads/\">"            \
  "download it here</a> or install it via your package manager of choice."

static void
load_map_finish_cb (GObject      *source_object,
                    GAsyncResult *res,
                    gpointer      data)
{
  GtkCrusaderVillageApplication *self   = data;
  g_autoptr (GError) error              = NULL;
  g_autoptr (GtkCrusaderVillageMap) map = NULL;
  GtkWindow *window                     = NULL;

  map    = gtk_crusader_village_map_new_from_aiv_file_finish (res, &error);
  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  if (map != NULL)
    g_object_set (
        window,
        "map", map,
        NULL);
  else
    gtk_crusader_village_dialog (
        "An Error Occurred",
        "Could not parse file from disk.",
        error->message,
        window, NULL);

  g_object_set (
      window,
      "busy", FALSE,
      NULL);
}

static void
load_dialog_finish_cb (GObject      *source_object,
                       GAsyncResult *res,
                       gpointer      data)
{
  GtkCrusaderVillageApplication *self = data;
  g_autoptr (GError) local_error      = NULL;
  GtkWindow *window                   = NULL;
  g_autoptr (GFile) file              = NULL;

  window = gtk_application_get_active_window (GTK_APPLICATION (self));
  file   = gtk_file_dialog_open_finish (GTK_FILE_DIALOG (source_object), res, &local_error);

  if (file != NULL)
    {
      g_autofree char *python_exe = NULL;

      python_exe = get_python_install (self);

      if (python_exe != NULL)
        gtk_crusader_village_map_new_from_aiv_file_async (
            file, self->item_store, python_exe, G_PRIORITY_DEFAULT,
            NULL, load_map_finish_cb, self);
      else
        gtk_crusader_village_dialog (
            "Cannot Proceed",
            "Sourcehold Python Installation Not Configured",
            INSTALLATION_WARNING_TEXT,
            window, NULL);
    }
  else
    {
      gtk_crusader_village_dialog (
          "An Error Occurred",
          "Could not load file from disk.",
          local_error->message,
          window, NULL);
      g_object_set (
          window,
          "busy", FALSE,
          NULL);
    }
}

static void
gtk_crusader_village_application_load (GSimpleAction *action,
                                       GVariant      *parameter,
                                       gpointer       user_data)
{
  GtkCrusaderVillageApplication *self       = user_data;
  GtkWindow                     *window     = NULL;
  gboolean                       busy       = FALSE;
  g_autofree char               *python_exe = NULL;

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  g_object_get (
      window,
      "busy", &busy,
      NULL);
  if (busy)
    return;

  python_exe = get_python_install (self);
  if (python_exe != NULL)
    {
      g_autoptr (GtkFileDialog) file_dialog = NULL;
      g_autoptr (GtkFileFilter) filter      = NULL;

      file_dialog = gtk_file_dialog_new ();
      filter      = gtk_file_filter_new ();

      gtk_file_filter_add_pattern (filter, "*.aiv");
      gtk_file_dialog_set_default_filter (file_dialog, filter);

      gtk_file_dialog_open (file_dialog, window, NULL, load_dialog_finish_cb, self);

      g_object_set (
          window,
          "busy", TRUE,
          NULL);
    }
  else
    gtk_crusader_village_dialog (
        "Import Error",
        "Sourcehold Python Installation Not Configured",
        INSTALLATION_WARNING_TEXT,
        window, NULL);
}

static void
save_map_finish_cb (GObject      *source_object,
                    GAsyncResult *res,
                    gpointer      data)
{
  GtkCrusaderVillageApplication *self = data;
  g_autoptr (GError) error            = NULL;
  gboolean   result                   = FALSE;
  GtkWindow *window                   = NULL;

  result = gtk_crusader_village_map_save_to_aiv_file_finish (res, &error);
  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  if (!result)
    gtk_crusader_village_dialog (
        "An Error Occurred",
        "Could not save AIV to disk.",
        error->message,
        window, NULL);

  g_object_set (
      window,
      "busy", FALSE,
      NULL);
}

static void
save_dialog_finish_cb (GObject      *source_object,
                       GAsyncResult *res,
                       gpointer      data)
{
  GtkCrusaderVillageApplication *self = data;
  g_autoptr (GError) local_error      = NULL;
  GtkWindow *window                   = NULL;
  g_autoptr (GFile) file              = NULL;

  window = gtk_application_get_active_window (GTK_APPLICATION (self));
  file   = gtk_file_dialog_save_finish (GTK_FILE_DIALOG (source_object), res, &local_error);

  if (file != NULL)
    {
      g_autofree char *python_exe = NULL;

      python_exe = get_python_install (self);

      if (python_exe != NULL)
        {
          g_autoptr (GtkCrusaderVillageMap) map = NULL;

          g_object_get (
              window,
              "map", &map,
              NULL);

          gtk_crusader_village_map_save_to_aiv_file_async (
              map, file, python_exe, G_PRIORITY_DEFAULT,
              NULL, save_map_finish_cb, self);
        }
      else
        gtk_crusader_village_dialog (
            "Cannot Proceed",
            "Sourcehold Python Installation Not Configured",
            INSTALLATION_WARNING_TEXT,
            window, NULL);
    }
  else
    {
      gtk_crusader_village_dialog (
          "An Error Occurred",
          "Could not save file to disk.",
          local_error->message,
          window, NULL);
      g_object_set (
          window,
          "busy", FALSE,
          NULL);
    }
}

static void
gtk_crusader_village_application_export (GSimpleAction *action,
                                         GVariant      *parameter,
                                         gpointer       user_data)
{
  GtkCrusaderVillageApplication *self       = user_data;
  GtkWindow                     *window     = NULL;
  gboolean                       busy       = FALSE;
  g_autofree char               *python_exe = NULL;

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  g_object_get (
      window,
      "busy", &busy,
      NULL);
  if (busy)
    return;

  python_exe = get_python_install (self);
  if (python_exe != NULL)
    {
      g_autoptr (GtkFileDialog) file_dialog = NULL;
      g_autoptr (GtkFileFilter) filter      = NULL;

      file_dialog = gtk_file_dialog_new ();
      filter      = gtk_file_filter_new ();

      gtk_file_filter_add_pattern (filter, "*.aiv");
      gtk_file_dialog_set_default_filter (file_dialog, filter);

      gtk_file_dialog_save (file_dialog, window, NULL, save_dialog_finish_cb, self);

      g_object_set (
          window,
          "busy", TRUE,
          NULL);
    }
  else
    gtk_crusader_village_dialog (
        "Export Error",
        "Sourcehold Python Installation Not Configured",
        INSTALLATION_WARNING_TEXT,
        window, NULL);
}

static void
gtk_crusader_village_application_preferences (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data)
{
  GtkCrusaderVillageApplication *self   = user_data;
  GtkWindow                     *window = NULL;

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  gtk_crusader_village_preferences_window_spawn (self->settings, window);
}

static void
greeting_submission (GtkCrusaderVillageDialogWindow *dialog,
                     GParamSpec                     *pspec,
                     GtkCrusaderVillageApplication  *application)
{
  g_autoptr (GVariant) submission = NULL;
  gboolean show_startup_greeting  = FALSE;

  g_object_get (
      dialog,
      "final-submission", &submission,
      NULL);
  g_variant_get (submission, "(b)", &show_startup_greeting);

  g_settings_set_boolean (
      application->settings,
      "show-startup-greeting",
      show_startup_greeting);
}

static const char *sketches[] = {
  "armourer-sketch", "army-sketch", "baker-sketch", "bakery-sketch", "blacksmith-sketch",
  "brewer-sketch", "bsmith-sketch", "campfire-sketch", "cess-pit-sketch", "chicken-sketch",
  "child-sketch", "chopping-block-sketch", "church-sketch", "crossbowman-sketch", "dairy-sketch",
  "dancing-bear-sketch", "dog-cage-sketch", "drunk-sketch", "ducking-stool-sketch", "dungeon-sketch",
  "farmer-sketch", "fearfactor-sketch", "firewatch-sketch", "fletcher-building-sketch", "fletcher-sketch",
  "food-sketch", "fruit-sketch", "gallows-sketch", "gardens-sketch", "ghost-sketch",
  "gibbet-sketch", "heads-on-spikes-sketch", "healer-sketch", "healers-sketch", "hop-sketch",
  "house-sketch", "hunter-hut-sketch", "hunter-sketch", "innkeeper-sketch", "inn-sketch",
  "iron-miner-sketch", "iron-sketch", "jester-sketch", "keep-sketch", "killing-pits-sketch",
  "maypole-sketch", "mill-sketch", "mother-sketch", "oil-smelter-sketch", "ox-tether-sketch",
  "pitch-sketch", "pitchworker-sketch", "pole-sketch", "poleturner-sketch", "ponds-sketch",
  "popularity-sketch", "population-sketch", "priest-sketch", "quarry-sketch", "religion-sketch",
  "siege-engineer-guild-sketch", "siege-engineer-sketch", "stables-sketch", "stake-sketch", "statue-sketch",
  "stockpile-sketch", "stocks-sketch", "stonemason-sketch", "stone-quarry-sketch", "stretching-rack-sketch",
  "tanner-building-sketch", "tanner-sketch", "tower-sketch", "trader-sketch", "tunnelors-guild-sketch",
  "tunnelor-sketch", "waterpot-sketch", "weapons-sketch", "wedding-sketch", "well-sketch",
  "wheat-sketch", "woodcutter-hut-sketch", "woodcutter-sketch"
};

#define GREETING_TEXT                                                                               \
  "Greetings, Sire! The desert awaits you.\n"                                                       \
  "\n"                                                                                              \
  "❤️ <a href=\"https://github.com/sponsors/kolunmi\">Support my work!</a>\n"                        \
  "🖥️ <a href=\"https://github.com/kolunmi/gtk-crusader-village\">Contribute to development!</a>\n" \
  "🏰 <a href=\"https://fireflyworlds.com/\">Support Firefly Studios!</a>"

static void
do_greeting (GtkCrusaderVillageApplication *self)
{
  GtkWindow *window                      = NULL;
  g_autoptr (GVariant) dialog_structure  = NULL;
  GtkCrusaderVillageDialogWindow *dialog = NULL;

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  dialog_structure = g_variant_new_parsed (
      "{\"Show this dialog on startup\":<%b>}",
      g_settings_get_boolean (self->settings, "show-startup-greeting"));

  dialog = gtk_crusader_village_dialog (
      "Welcome",
      "Welcome",
      GREETING_TEXT,
      window, dialog_structure);
  g_object_set (
      dialog,
      "default-width", 650,
      NULL);

  g_signal_connect (dialog, "notify::final-submission", G_CALLBACK (greeting_submission), self);
  gtk_widget_add_css_class (GTK_WIDGET (dialog), "greeting-dialog");
  gtk_widget_add_css_class (GTK_WIDGET (dialog), sketches[g_random_int_range (0, G_N_ELEMENTS (sketches))]);
}

static void
installation_warning_submission (GtkCrusaderVillageDialogWindow *dialog,
                                 GParamSpec                     *pspec,
                                 GtkCrusaderVillageApplication  *application)
{
  do_greeting (application);
}

static void
gtk_crusader_village_application_greeting_action (GSimpleAction *action,
                                                  GVariant      *parameter,
                                                  gpointer       user_data)
{
  GtkCrusaderVillageApplication *self       = user_data;
  g_autofree char               *python_exe = NULL;

  python_exe = get_python_install (self);
  if (python_exe != NULL)
    do_greeting (self);
  else
    {
      GtkWindow                      *window = NULL;
      GtkCrusaderVillageDialogWindow *dialog = NULL;

      window = gtk_application_get_active_window (GTK_APPLICATION (self));

      dialog = gtk_crusader_village_dialog (
          "Warning",
          "Sourcehold Python Installation Not Configured",
          INSTALLATION_WARNING_TEXT,
          window, NULL);
      g_signal_connect (dialog, "notify::final-submission",
                        G_CALLBACK (installation_warning_submission), self);
    }
}

#define ABOUT_TEXT_FMT                                                             \
  "version: %s (GTK: %d.%d.%d)\n"                                                  \
  "\n"                                                                             \
  "Copyright 2025 Adam Masciola\n"                                                 \
  "\n"                                                                             \
  "This program is free software: you can redistribute it and/or modify "          \
  "it under the terms of the GNU General Public License as published by "          \
  "the Free Software Foundation, either version 3 of the License, or "             \
  "(at your option) any later version.\n"                                          \
  "\n"                                                                             \
  "This program is distributed in the hope that it will be useful, "               \
  "but WITHOUT ANY WARRANTY; without even the implied warranty of "                \
  "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "                 \
  "GNU General Public License for more details.\n"                                 \
  "\n"                                                                             \
  "You should have received a copy of the GNU General Public License "             \
  "along with this program.  If not, see "                                         \
  "<a href=\"https://www.gnu.org/licenses/\">https://www.gnu.org/licenses/</a>.\n" \
  "\n"                                                                             \
  "SPDX-License-Identifier: GPL-3.0-or-later"

static void
gtk_crusader_village_application_about_action (GSimpleAction *action,
                                               GVariant      *parameter,
                                               gpointer       user_data)
{
  GtkCrusaderVillageApplication *self    = user_data;
  GtkWindow                     *window  = NULL;
  g_autofree char               *message = NULL;

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  message = g_strdup_printf (
      ABOUT_TEXT_FMT,
      PACKAGE_VERSION
#ifdef DEVELOPMENT_BUILD
      " development"
#endif
      ,
      gtk_get_major_version (),
      gtk_get_minor_version (),
      gtk_get_micro_version ());

  gtk_crusader_village_dialog (
      "About",
      "GTK Crusader Village",
      message,
      window, NULL);
}

static void
gtk_crusader_village_application_quit_action (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data)
{
  GtkCrusaderVillageApplication *self = user_data;

  g_assert (GTK_CRUSADER_VILLAGE_IS_APPLICATION (self));

  g_application_quit (G_APPLICATION (self));
}

static const GActionEntry app_actions[] = {
  {        "quit",     gtk_crusader_village_application_quit_action },
  {       "about",    gtk_crusader_village_application_about_action },
  {    "greeting", gtk_crusader_village_application_greeting_action },
  { "preferences",     gtk_crusader_village_application_preferences },
  {      "export",          gtk_crusader_village_application_export },
  {        "load",            gtk_crusader_village_application_load },
};

static void
gtk_crusader_village_application_init (GtkCrusaderVillageApplication *self)
{
  self->theme_setting = GTK_CRUSADER_VILLAGE_THEME_OPTION_DEFAULT;

  self->item_store = g_object_new (GTK_CRUSADER_VILLAGE_TYPE_ITEM_STORE, NULL);
  gtk_crusader_village_item_store_read_resources (self->item_store);

  self->brush_store = g_list_store_new (GTK_CRUSADER_VILLAGE_TYPE_BRUSHABLE);

  g_action_map_add_action_entries (
      G_ACTION_MAP (self),
      app_actions,
      G_N_ELEMENTS (app_actions),
      self);

  gtk_application_set_accels_for_action (
      GTK_APPLICATION (self),
      "app.quit",
      (const char *[]) { "<primary>q", NULL });
  gtk_application_set_accels_for_action (
      GTK_APPLICATION (self),
      "app.load",
      (const char *[]) { "<primary>o", NULL });
  gtk_application_set_accels_for_action (
      GTK_APPLICATION (self),
      "app.export",
      (const char *[]) { "<primary>s", NULL });
  gtk_application_set_accels_for_action (
      GTK_APPLICATION (self),
      "app.about",
      (const char *[]) { "<primary>a", NULL });
  gtk_application_set_accels_for_action (
      GTK_APPLICATION (self),
      "app.greeting",
      (const char *[]) { "<primary>g", NULL });
  gtk_application_set_accels_for_action (
      GTK_APPLICATION (self),
      "app.preferences",
      (const char *[]) { "<primary>p", NULL });
}

static void
theme_changed (GSettings                     *self,
               char                          *key,
               GtkCrusaderVillageApplication *application)
{
  read_theme_from_settings (application);
}

static void
read_theme_from_settings (GtkCrusaderVillageApplication *self)
{
  g_autofree char *theme = NULL;

  theme               = g_settings_get_string (self->settings, "theme");
  self->theme_setting = gtk_crusader_village_theme_str_to_enum (theme);

  apply_gtk_theme (self);
}

static void
apply_gtk_theme (GtkCrusaderVillageApplication *self)
{
  gboolean    dark    = FALSE;
  GdkDisplay *display = NULL;

#ifdef USE_THEME_PORTAL
  dark = (self->theme_setting == GTK_CRUSADER_VILLAGE_THEME_OPTION_SHC_DARK ||
          self->theme_setting == GTK_CRUSADER_VILLAGE_THEME_OPTION_DARK) ||
         ((self->theme_setting == GTK_CRUSADER_VILLAGE_THEME_OPTION_SHC_DEFAULT ||
           self->theme_setting == GTK_CRUSADER_VILLAGE_THEME_OPTION_DEFAULT) &&
          self->portal_wants_dark);
#else
  dark = (self->theme_setting == GTK_CRUSADER_VILLAGE_THEME_OPTION_SHC_DARK ||
          self->theme_setting == GTK_CRUSADER_VILLAGE_THEME_OPTION_DARK);
#endif

  g_object_set (
      gtk_settings_get_default (),
      "gtk-application-prefer-dark-theme", dark,
      NULL);

  display = gdk_display_get_default ();
  gtk_style_context_remove_provider_for_display (display, GTK_STYLE_PROVIDER (self->custom_css));
  gtk_style_context_remove_provider_for_display (display, GTK_STYLE_PROVIDER (self->shc_theme_light_css));
  gtk_style_context_remove_provider_for_display (display, GTK_STYLE_PROVIDER (self->shc_theme_dark_css));

  if (self->theme_setting == GTK_CRUSADER_VILLAGE_THEME_OPTION_SHC_DEFAULT ||
      self->theme_setting == GTK_CRUSADER_VILLAGE_THEME_OPTION_SHC_LIGHT ||
      self->theme_setting == GTK_CRUSADER_VILLAGE_THEME_OPTION_SHC_DARK)
    {
      gtk_style_context_add_provider_for_display (
          display,
          GTK_STYLE_PROVIDER (
              dark
                  ? self->shc_theme_dark_css
                  : self->shc_theme_light_css),
          GTK_STYLE_PROVIDER_PRIORITY_USER);
    }
  gtk_style_context_add_provider_for_display (
      display,
      GTK_STYLE_PROVIDER (self->custom_css),
      GTK_STYLE_PROVIDER_PRIORITY_USER);
}

static void
ensure_settings (GtkCrusaderVillageApplication *self)
{
  const char *app_id = NULL;

  if (self->settings != NULL)
    return;

  app_id = g_application_get_application_id (G_APPLICATION (self));
  g_assert (app_id != NULL);

  self->settings = g_settings_new (app_id);

  g_signal_connect (self->settings, "changed::theme",
                    G_CALLBACK (theme_changed), self);
  read_theme_from_settings (self);
}

#ifdef USE_THEME_PORTAL
static gboolean
read_setting (GtkCrusaderVillageApplication *self,
              const char                    *schema,
              const char                    *name,
              const char                    *type,
              GVariant                     **out)
{
  GError       *error = NULL;
  GVariant     *ret;
  GVariant     *child, *child2;
  GVariantType *out_type;
  gboolean      result = FALSE;

  ret = g_dbus_proxy_call_sync (
      self->settings_portal,
      "Read",
      g_variant_new ("(ss)", schema, name),
      G_DBUS_CALL_FLAGS_NONE,
      G_MAXINT,
      NULL,
      &error);

  if (error != NULL)
    {
      if (error->domain == G_DBUS_ERROR &&
          error->code == G_DBUS_ERROR_SERVICE_UNKNOWN)
        {
          g_debug ("Portal not found: %s", error->message);
        }
      else if (error->domain == G_DBUS_ERROR &&
               error->code == G_DBUS_ERROR_UNKNOWN_METHOD)
        {
          g_debug ("Portal doesn't provide settings: %s", error->message);
        }
      else if (g_dbus_error_is_remote_error (error))
        {
          char *remote_error = g_dbus_error_get_remote_error (error);

          if (!g_strcmp0 (remote_error, PORTAL_ERROR_NOT_FOUND))
            {
              g_debug ("Setting %s.%s of type %s not found", schema, name, type);
            }
          g_free (remote_error);
        }
      else
        {
          g_critical ("Couldn't read the %s setting: %s", name, error->message);
        }

      g_clear_error (&error);

      return FALSE;
    }

  g_variant_get (ret, "(v)", &child);
  g_variant_get (child, "v", &child2);

  out_type = g_variant_type_new (type);
  if (g_variant_type_equal (g_variant_get_type (child2), out_type))
    {
      *out = child2;

      result = TRUE;
    }
  else
    {
      g_critical ("Invalid type for %s.%s: expected %s, got %s",
                  schema, name, type, g_variant_get_type_string (child2));

      g_variant_unref (child2);
    }

  g_variant_type_free (out_type);
  g_variant_unref (child);
  g_variant_unref (ret);
  g_clear_error (&error);

  return result;
}

static gboolean
is_dark (GVariant *variant)
{
  guint32 value = 0;

  value = g_variant_get_uint32 (variant);
  switch (value)
    {
    case 0:
    case 2:
      return FALSE;
    case 1:
      return TRUE;
    default:
      g_warning ("Invalid colorscheme from portal");
      return FALSE;
    }
}

static void
changed_cb (GDBusProxy                    *proxy,
            const char                    *sender_name,
            const char                    *signal_name,
            GVariant                      *parameters,
            GtkCrusaderVillageApplication *self)
{
  const char *namespace;
  const char *name;
  g_autoptr (GVariant) value = NULL;

  if (g_strcmp0 (signal_name, "SettingChanged") != 0)
    return;

  g_variant_get (parameters, "(&s&sv)", &namespace, &name, &value);

  if (g_strcmp0 (namespace, "org.freedesktop.appearance") == 0 &&
      g_strcmp0 (name, "color-scheme") == 0)
    {
      self->portal_wants_dark = is_dark (value);
      apply_gtk_theme (self);
    }
}

static void
init_portal (GtkCrusaderVillageApplication *self)
{
  g_autoptr (GError) error     = NULL;
  g_autoptr (GVariant) variant = NULL;
  gboolean was_read            = FALSE;

  self->settings_portal = g_dbus_proxy_new_for_bus_sync (
      G_BUS_TYPE_SESSION,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      PORTAL_BUS_NAME,
      PORTAL_OBJECT_PATH,
      PORTAL_SETTINGS_INTERFACE,
      NULL,
      &error);

  if (self->settings_portal == NULL)
    {
      g_debug ("Settings portal not found: %s", error->message);
      return;
    }

  was_read = read_setting (self, "org.freedesktop.appearance",
                           "color-scheme", "u", &variant);
  if (was_read)
    self->portal_wants_dark = is_dark (variant);
  else
    g_debug ("Could not read color scheme info from portal");

  g_signal_connect (self->settings_portal, "g-signal",
                    G_CALLBACK (changed_cb), self);
}
#endif
