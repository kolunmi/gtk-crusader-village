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

  G_OBJECT_CLASS (gtk_crusader_village_application_parent_class)->dispose (object);
}

static void
gtk_crusader_village_application_activate (GApplication *app)
{
  GtkCrusaderVillageApplication *self   = GTK_CRUSADER_VILLAGE_APPLICATION (app);
  GtkWindow                     *window = NULL;

  ensure_settings (self);

  window = gtk_application_get_active_window (GTK_APPLICATION (app));

  if (window == NULL)
    window = g_object_new (
        GTK_CRUSADER_VILLAGE_TYPE_WINDOW,
        "application", app,
        "settings", self->settings,
        NULL);

  gtk_window_present (window);

#ifdef USE_THEME_PORTAL
  init_portal (self);
#endif
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
gtk_crusader_village_application_about_action (GSimpleAction *action,
                                               GVariant      *parameter,
                                               gpointer       user_data)
{
  static const char             *authors[] = { "Adam Masciola", NULL };
  GtkCrusaderVillageApplication *self      = user_data;
  GtkWindow                     *window    = NULL;

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  gtk_show_about_dialog (
      window,
      "program-name", "gtk-crusader-village",
      "logo-icon-name", "am.kolunmi.GtkCrusaderVillage",
      "authors", authors,
      "translator-credits", _ ("translator-credits"),
      "version", "0.1.0",
      "copyright", "© 2025 Adam Masciola",
      NULL);
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
  {        "quit",  gtk_crusader_village_application_quit_action },
  {       "about", gtk_crusader_village_application_about_action },
  { "preferences",  gtk_crusader_village_application_preferences },
};

static void
gtk_crusader_village_application_init (GtkCrusaderVillageApplication *self)
{
  self->theme_setting = GTK_CRUSADER_VILLAGE_THEME_OPTION_DEFAULT;

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   app_actions,
                                   G_N_ELEMENTS (app_actions),
                                   self);
  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                         "app.quit",
                                         (const char *[]) { "<primary>q", NULL });
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
  gboolean dark = FALSE;

#ifdef USE_THEME_PORTAL
  dark = self->theme_setting == GTK_CRUSADER_VILLAGE_THEME_OPTION_DARK ||
         (self->theme_setting == GTK_CRUSADER_VILLAGE_THEME_OPTION_DEFAULT && self->portal_wants_dark);
#else
  dark = self->theme_setting == GTK_CRUSADER_VILLAGE_THEME_OPTION_DARK;
#endif

  g_object_set (
      gtk_settings_get_default (),
      "gtk-application-prefer-dark-theme", dark,
      NULL);
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
  if (!was_read)
    {
      g_debug ("Could not read color scheme info from portal");
      return;
    }

  self->portal_wants_dark = is_dark (variant);
  g_signal_connect (self->settings_portal, "g-signal",
                    G_CALLBACK (changed_cb), self);
}
#endif
