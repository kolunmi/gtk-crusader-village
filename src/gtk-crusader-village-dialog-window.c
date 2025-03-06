/* gtk-crusader-village-dialog-window.c
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

struct _GtkCrusaderVillageDialogWindow
{
  GtkWindow parent_instance;

  GPtrArray *results;
  GVariant  *final_submission;

  /* Template widgets */
  GtkLabel  *header;
  GtkLabel  *message;
  GtkBox    *option_box;
  GtkButton *ok_button;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageDialogWindow, gtk_crusader_village_dialog_window, GTK_TYPE_WINDOW)

enum
{
  PROP_0,

  PROP_FINAL_SUBMISSION,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static GVariant *static_open  = NULL;
static GVariant *static_close = NULL;

static void
ok_clicked (GtkButton                      *self,
            GtkCrusaderVillageDialogWindow *user_data);

static gboolean
register_dictionary_variant (GtkCrusaderVillageDialogWindow *self,
                             GVariant                       *dictionary,
                             GtkBox                         *container);

static void
boolean_changed (GtkCheckButton *button,
                 GParamSpec     *pspec,
                 GVariant      **write);

static void
string_changed (GtkEntry  *self,
                GVariant **write);

static void
string_opt_changed (GtkDropDown *dropdown,
                    GParamSpec  *pspec,
                    GVariant   **write);

static void
int32_spinner_changed (GtkSpinButton *button,
                       GParamSpec    *pspec,
                       GVariant     **write);

static void
int32_scale_changed (GtkScale  *self,
                     GVariant **write);

static void
maybe_free_variant_pp (gpointer pp);

static void
gtk_crusader_village_dialog_window_dispose (GObject *object)
{
  GtkCrusaderVillageDialogWindow *self = GTK_CRUSADER_VILLAGE_DIALOG_WINDOW (object);

  g_clear_pointer (&self->results, g_ptr_array_unref);
  g_clear_pointer (&self->final_submission, g_variant_unref);

  G_OBJECT_CLASS (gtk_crusader_village_dialog_window_parent_class)->dispose (object);
}

static void
gtk_crusader_village_dialog_window_get_property (GObject    *object,
                                                 guint       prop_id,
                                                 GValue     *value,
                                                 GParamSpec *pspec)
{
  GtkCrusaderVillageDialogWindow *self = GTK_CRUSADER_VILLAGE_DIALOG_WINDOW (object);

  switch (prop_id)
    {
    case PROP_FINAL_SUBMISSION:
      g_value_set_variant (value, self->final_submission);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_dialog_window_set_property (GObject      *object,
                                                 guint         prop_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec)
{
  GtkCrusaderVillageDialogWindow *self = GTK_CRUSADER_VILLAGE_DIALOG_WINDOW (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_dialog_window_class_init (GtkCrusaderVillageDialogWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gtk_crusader_village_dialog_window_dispose;
  object_class->get_property = gtk_crusader_village_dialog_window_get_property;
  object_class->set_property = gtk_crusader_village_dialog_window_set_property;

  props[PROP_FINAL_SUBMISSION] =
      g_param_spec_variant (
          "final-submission",
          "Final Submission",
          "A variant representing the ultimate choice(s) made by the user",
          G_VARIANT_TYPE_TUPLE,
          NULL,
          G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/GtkCrusaderVillage/gtk-crusader-village-dialog-window.ui");
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageDialogWindow, header);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageDialogWindow, message);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageDialogWindow, option_box);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageDialogWindow, ok_button);
}

static void
gtk_crusader_village_dialog_window_init (GtkCrusaderVillageDialogWindow *self)
{
  self->results = g_ptr_array_new_with_free_func (maybe_free_variant_pp);

  gtk_widget_init_template (GTK_WIDGET (self));
  gtk_widget_add_css_class (GTK_WIDGET (self), "shc");

  g_signal_connect (self->ok_button, "clicked", G_CALLBACK (ok_clicked), self);
}

static void
ok_clicked (GtkButton                      *self,
            GtkCrusaderVillageDialogWindow *dialog)
{
  g_autoptr (GVariantBuilder) builder = NULL;

  gtk_widget_set_sensitive (GTK_WIDGET (dialog), FALSE);

  builder = g_variant_builder_new (G_VARIANT_TYPE_TUPLE);
  for (guint i = 0; i < dialog->results->len; i++)
    {
      GVariant **variant_pp = g_ptr_array_index (dialog->results, i);

      if (variant_pp == &static_open)
        g_variant_builder_open (builder, G_VARIANT_TYPE_TUPLE);
      else if (variant_pp == &static_close)
        g_variant_builder_close (builder);
      else
        g_variant_builder_add_value (
            builder, g_variant_ref_sink (g_steal_pointer (variant_pp)));
    }

  g_clear_pointer (&dialog->final_submission, g_variant_unref);
  dialog->final_submission = g_variant_ref_sink (g_variant_builder_end (builder));

  g_object_notify_by_pspec (G_OBJECT (dialog), props[PROP_FINAL_SUBMISSION]);
  gtk_window_close (GTK_WINDOW (dialog));
}

/* These are inefficient, could just read at end,
 * but might want to hook into values later?
 */
static void
boolean_changed (GtkCheckButton *button,
                 GParamSpec     *pspec,
                 GVariant      **write)
{
  g_clear_pointer (write, g_variant_unref);
  *write = g_variant_new_boolean (gtk_check_button_get_active (button));
}

static void
string_changed (GtkEntry  *self,
                GVariant **write)
{
  g_clear_pointer (write, g_variant_unref);
  *write = g_variant_new_string (gtk_editable_get_text (GTK_EDITABLE (self)));
}

static void
string_opt_changed (GtkDropDown *dropdown,
                    GParamSpec  *pspec,
                    GVariant   **write)
{
  GtkStringObject *object = NULL;

  g_clear_pointer (write, g_variant_unref);
  object = gtk_drop_down_get_selected_item (dropdown);
  *write = g_variant_new_string (gtk_string_object_get_string (object));
}

static void
int32_spinner_changed (GtkSpinButton *button,
                       GParamSpec    *pspec,
                       GVariant     **write)
{
  g_clear_pointer (write, g_variant_unref);
  *write = g_variant_new_int32 (gtk_spin_button_get_value (button));
}

static void
int32_scale_changed (GtkScale  *self,
                     GVariant **write)
{
  g_clear_pointer (write, g_variant_unref);
  *write = g_variant_new_int32 (gtk_range_get_value (GTK_RANGE (self)));
}

GtkCrusaderVillageDialogWindow *
gtk_crusader_village_dialog (const char *title,
                             const char *header,
                             const char *message,
                             GtkWindow  *parent,
                             GVariant   *structure)
{
  g_autoptr (GtkCrusaderVillageDialogWindow) dialog = NULL;

  g_return_val_if_fail (header != NULL || message != NULL, NULL);
  g_return_val_if_fail (GTK_IS_WINDOW (parent), NULL);

  if (structure != NULL && !g_variant_is_of_type (structure, G_VARIANT_TYPE ("a{sv}")))
    {
      g_critical ("gtk_crusader_village_dialog(): invalid GVariant structure: expected toplevel dictionary");
      return NULL;
    }

  dialog = g_object_new (GTK_CRUSADER_VILLAGE_TYPE_DIALOG_WINDOW, NULL);

  if (structure != NULL)
    {
      gboolean result = FALSE;

      result = register_dictionary_variant (dialog, structure, dialog->option_box);
      if (!result)
        return NULL;
    }

  gtk_window_set_title (GTK_WINDOW (dialog), title != NULL ? title : "Dialog");
  gtk_label_set_label (dialog->header, header);
  gtk_label_set_markup (dialog->message, message);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
  gtk_window_present (GTK_WINDOW (dialog));

  return g_steal_pointer (&dialog);
}

static gboolean
register_dictionary_variant (GtkCrusaderVillageDialogWindow *self,
                             GVariant                       *dictionary,
                             GtkBox                         *container)
{
  GVariantIter iter = { 0 };

  g_variant_iter_init (&iter, dictionary);
  for (;;)
    {
      g_autofree char *key         = NULL;
      g_autoptr (GVariant) value   = NULL;
      g_autofree GVariant **write  = NULL;
      g_autoptr (GtkWidget) widget = NULL;

      if (!g_variant_iter_next (&iter, "{sv}", &key, &value))
        break;

      write = g_malloc0 (sizeof (*write));

      if (g_variant_is_of_type (value, G_VARIANT_TYPE ("a{sv}")))
        {
          g_ptr_array_add (self->results, static_open);
          register_dictionary_variant (self, value, container);
          g_ptr_array_add (self->results, static_close);
          continue;
        }
      else if (g_variant_is_of_type (value, G_VARIANT_TYPE_BOOLEAN))
        {
          gboolean initial = FALSE;

          initial = g_variant_get_boolean (value);
          *write  = g_variant_new_boolean (initial);
          widget  = gtk_check_button_new_with_label (key);

          gtk_check_button_set_active (GTK_CHECK_BUTTON (widget), initial);
          g_signal_connect (widget, "notify::active", G_CALLBACK (boolean_changed), write);
        }
      else if (g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        {
          const char *initial = NULL;
          GtkWidget  *entry   = NULL;

          initial = g_variant_get_string (value, NULL);
          *write  = g_variant_new_string (initial);
          widget  = gtk_frame_new (key);
          entry   = gtk_entry_new ();

          gtk_entry_set_placeholder_text (GTK_ENTRY (entry), initial);
          gtk_editable_set_text (GTK_EDITABLE (entry), initial);
          g_signal_connect (entry, "changed", G_CALLBACK (string_changed), write);
          gtk_frame_set_child (GTK_FRAME (widget), entry);
        }
      else if (g_variant_is_of_type (value, G_VARIANT_TYPE_STRING_ARRAY))
        {
          g_autoptr (GStrvBuilder) opts_builder = NULL;
          GVariantIter opt_iter                 = { 0 };
          g_auto (GStrv) opts                   = NULL;
          GtkWidget *dropdown                   = NULL;

          opts_builder = g_strv_builder_new ();
          g_variant_iter_init (&opt_iter, value);
          for (;;)
            {
              g_autofree char *opt = NULL;

              if (!g_variant_iter_next (&opt_iter, "s", &opt))
                break;
              g_strv_builder_take (opts_builder, g_steal_pointer (&opt));
            }
          opts = g_strv_builder_unref_to_strv (g_steal_pointer (&opts_builder));

          if (opts == NULL || *opts == NULL)
            {
              g_critical ("gtk_crusader_village_dialog(): invalid GVariant structure: "
                          "length of opts array must be greater than 0");
              return FALSE;
            }

          widget   = gtk_frame_new (key);
          dropdown = gtk_drop_down_new_from_strings ((const char *const *) opts);

          g_signal_connect (dropdown, "changed", G_CALLBACK (string_opt_changed), write);
          gtk_frame_set_child (GTK_FRAME (widget), dropdown);
        }
      else if (g_variant_is_of_type (value, G_VARIANT_TYPE ("(iiii)")))
        /* int32 spinner */
        {
          gint32     lower   = 0;
          gint32     upper   = 0;
          gint32     step    = 0;
          gint32     initial = 0;
          GtkWidget *spin    = NULL;

          g_variant_get (value, "(iiii)", &lower, &upper, &step, &initial);
          initial = CLAMP (initial, lower, upper);
          *write  = g_variant_new_int32 (initial);
          widget  = gtk_frame_new (key);
          spin    = gtk_spin_button_new_with_range (lower, upper, step);

          gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), initial);
          g_signal_connect (spin, "notify::value", G_CALLBACK (int32_spinner_changed), write);
          gtk_frame_set_child (GTK_FRAME (widget), spin);
        }
      else if (g_variant_is_of_type (value, G_VARIANT_TYPE ("(iiia(si)i)")))
        /* int32 slider with ticks */
        {
          gint32 lower                       = 0;
          gint32 upper                       = 0;
          gint32 step                        = 0;
          g_autoptr (GVariantIter) tick_iter = NULL;
          gint32     initial                 = 0;
          GtkWidget *scale                   = NULL;

          g_variant_get (value, "(iiia(si)i)", &lower, &upper, &step, &tick_iter, &initial);
          initial = CLAMP (initial, lower, upper);
          *write  = g_variant_new_int32 (initial);
          widget  = gtk_frame_new (key);
          scale   = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, lower, upper, step);

          gtk_range_set_value (GTK_RANGE (scale), initial);
          for (;;)
            {
              g_autofree char *markup = NULL;
              guint32          tick   = 0;

              if (!g_variant_iter_next (tick_iter, "(si)", &markup, &tick))
                break;
              gtk_scale_add_mark (GTK_SCALE (scale), tick, GTK_POS_BOTTOM, markup);
            }
          g_signal_connect (scale, "value-changed", G_CALLBACK (int32_scale_changed), write);
          gtk_frame_set_child (GTK_FRAME (widget), scale);
        }
      else
        {
          g_critical ("gtk_crusader_village_dialog(): invalid GVariant structure: invalid component type");
          return FALSE;
        }

      g_ptr_array_add (self->results, g_steal_pointer (&write));
      gtk_box_append (container, g_steal_pointer (&widget));
    }

  return TRUE;
}

static void
maybe_free_variant_pp (gpointer pp)
{
  GVariant **variant_pp = NULL;

  variant_pp = pp;
  if (variant_pp == &static_open || variant_pp == &static_close)
    return;

  g_clear_pointer (variant_pp, g_variant_unref);
  g_free (pp);
}
