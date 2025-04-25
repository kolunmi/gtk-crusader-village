/* gtk-crusader-village-brush-area.c
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

#include "gtk-crusader-village-brush-area-item.h"
#include "gtk-crusader-village-brush-area.h"
#include "gtk-crusader-village-brushable.h"
#include "gtk-crusader-village-dialog-window.h"
#include "gtk-crusader-village-image-mask-brush.h"

struct _GcvBrushArea
{
  GcvUtilBin parent_instance;

  GListStore *brush_store;

  /* Template widgets */
  GtkButton   *new_mask_brush;
  GtkSpinner  *new_mask_brush_spinner;
  GtkListView *list_view;
  GtkButton   *delete_brush;
};

G_DEFINE_FINAL_TYPE (GcvBrushArea, gcv_brush_area, GCV_TYPE_UTIL_BIN)

enum
{
  PROP_0,

  PROP_BRUSH_STORE,
  PROP_SELECTED_BRUSH,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
setup_listitem (GtkListItemFactory *factory,
                GtkListItem        *list_item,
                GcvBrushArea       *brush_area);

static void
bind_listitem (GtkListItemFactory *factory,
               GtkListItem        *list_item,
               GcvBrushArea       *brush_area);

static void
selected_item_changed (GtkSingleSelection *selection,
                       GParamSpec         *pspec,
                       GcvBrushArea       *brush_area);

static void
new_mask_brush_clicked (GtkButton    *self,
                        GcvBrushArea *brush_area);

static void
delete_brush_clicked (GtkButton    *self,
                      GcvBrushArea *brush_area);

static void
image_dialog_finish_cb (GObject      *source_object,
                        GAsyncResult *res,
                        gpointer      data);

static void
load_brush_image_file_finish_cb (GObject      *source_object,
                                 GAsyncResult *res,
                                 gpointer      data);

static void
gcv_brush_area_dispose (GObject *object)
{
  GcvBrushArea *self = GCV_BRUSH_AREA (object);

  g_clear_object (&self->brush_store);

  G_OBJECT_CLASS (gcv_brush_area_parent_class)->dispose (object);
}

static void
gcv_brush_area_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GcvBrushArea *self = GCV_BRUSH_AREA (object);

  switch (prop_id)
    {
    case PROP_BRUSH_STORE:
      g_value_set_object (value, self->brush_store);
      break;
    case PROP_SELECTED_BRUSH:
      {
        GtkSingleSelection *single_selection_model = NULL;
        GcvBrushable       *brush                  = NULL;

        single_selection_model = GTK_SINGLE_SELECTION (gtk_list_view_get_model (self->list_view));
        brush                  = GCV_BRUSHABLE (gtk_single_selection_get_selected_item (single_selection_model));

        g_value_set_object (value, brush);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gcv_brush_area_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GcvBrushArea *self = GCV_BRUSH_AREA (object);

  switch (prop_id)
    {
    case PROP_BRUSH_STORE:
      {
        GtkSingleSelection *single_selection_model = NULL;

        g_clear_object (&self->brush_store);
        self->brush_store = g_value_dup_object (value);

        single_selection_model = GTK_SINGLE_SELECTION (gtk_list_view_get_model (self->list_view));
        gtk_single_selection_set_model (single_selection_model, G_LIST_MODEL (self->brush_store));
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gcv_brush_area_class_init (GcvBrushAreaClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gcv_brush_area_dispose;
  object_class->get_property = gcv_brush_area_get_property;
  object_class->set_property = gcv_brush_area_set_property;

  props[PROP_BRUSH_STORE] =
      g_param_spec_object (
          "brush-store",
          "Brush Store",
          "The brush store this widget should reflect",
          G_TYPE_LIST_STORE,
          G_PARAM_READWRITE);

  props[PROP_SELECTED_BRUSH] =
      g_param_spec_object (
          "selected-brush",
          "Selected Brush",
          "The currently selected brush in the list",
          GCV_TYPE_BRUSHABLE,
          G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/Gcv/gtk-crusader-village-brush-area.ui");
  gtk_widget_class_bind_template_child (widget_class, GcvBrushArea, new_mask_brush);
  gtk_widget_class_bind_template_child (widget_class, GcvBrushArea, new_mask_brush_spinner);
  gtk_widget_class_bind_template_child (widget_class, GcvBrushArea, list_view);
  gtk_widget_class_bind_template_child (widget_class, GcvBrushArea, delete_brush);
}

static void
gcv_brush_area_init (GcvBrushArea *self)
{
  GtkListItemFactory *factory         = NULL;
  GtkSelectionModel  *selection_model = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  factory         = gtk_signal_list_item_factory_new ();
  selection_model = GTK_SELECTION_MODEL (gtk_single_selection_new (NULL));

  g_signal_connect (factory, "setup", G_CALLBACK (setup_listitem), self);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_listitem), self);
  gtk_list_view_set_factory (self->list_view, factory);
  gtk_list_view_set_model (self->list_view, selection_model);

  g_signal_connect (selection_model, "notify::selected-item",
                    G_CALLBACK (selected_item_changed), self);

  g_signal_connect (self->new_mask_brush, "clicked",
                    G_CALLBACK (new_mask_brush_clicked), self);
  g_signal_connect (self->delete_brush, "clicked",
                    G_CALLBACK (delete_brush_clicked), self);
}

static void
setup_listitem (GtkListItemFactory *factory,
                GtkListItem        *list_item,
                GcvBrushArea       *brush_area)
{
  GcvBrushAreaItem *area_item = NULL;

  area_item = g_object_new (GCV_TYPE_BRUSH_AREA_ITEM, NULL);
  gtk_list_item_set_child (list_item, GTK_WIDGET (area_item));
}

static void
bind_listitem (GtkListItemFactory *factory,
               GtkListItem        *list_item,
               GcvBrushArea       *brush_area)
{
  GcvBrushable     *brush     = NULL;
  GcvBrushAreaItem *area_item = NULL;

  brush     = GCV_BRUSHABLE (gtk_list_item_get_item (list_item));
  area_item = GCV_BRUSH_AREA_ITEM (gtk_list_item_get_child (list_item));

  g_object_set (
      area_item,
      "brush", brush,
      NULL);
}

static void
selected_item_changed (GtkSingleSelection *selection,
                       GParamSpec         *pspec,
                       GcvBrushArea       *brush_area)
{
  g_object_notify_by_pspec (G_OBJECT (brush_area), props[PROP_SELECTED_BRUSH]);
}

static void
new_mask_brush_clicked (GtkButton    *self,
                        GcvBrushArea *brush_area)
{
  GtkWidget      *app_window            = NULL;
  GtkApplication *application           = NULL;
  GtkWindow      *window                = NULL;
  g_autoptr (GtkFileDialog) file_dialog = NULL;
  g_autoptr (GtkFileFilter) filter      = NULL;

  app_window = gtk_widget_get_ancestor (GTK_WIDGET (brush_area), GTK_TYPE_WINDOW);
  g_assert (app_window != NULL);
  application = gtk_window_get_application (GTK_WINDOW (app_window));
  window      = gtk_application_get_active_window (application);

  file_dialog = gtk_file_dialog_new ();
  filter      = gtk_file_filter_new ();

  gtk_file_filter_add_mime_type (filter, "image/*");
  gtk_file_dialog_set_default_filter (file_dialog, filter);

  gtk_file_dialog_open (file_dialog, GTK_WINDOW (window),
                        NULL, image_dialog_finish_cb, brush_area);

  gtk_widget_set_sensitive (GTK_WIDGET (brush_area->new_mask_brush), FALSE);
  gtk_widget_set_visible (GTK_WIDGET (brush_area->new_mask_brush_spinner), TRUE);
}

static void
delete_brush_clicked (GtkButton    *self,
                      GcvBrushArea *brush_area)
{
  GtkSingleSelection *single_selection_model = NULL;
  guint               selected               = 0;

  single_selection_model = GTK_SINGLE_SELECTION (gtk_list_view_get_model (brush_area->list_view));
  selected               = gtk_single_selection_get_selected (single_selection_model);

  if (selected != GTK_INVALID_LIST_POSITION)
    g_list_store_remove (brush_area->brush_store, selected);
}

static void
image_dialog_finish_cb (GObject      *source_object,
                        GAsyncResult *res,
                        gpointer      data)
{
  GcvBrushArea *self             = data;
  g_autoptr (GError) local_error = NULL;
  g_autoptr (GFile) file         = NULL;

  file = gtk_file_dialog_open_finish (GTK_FILE_DIALOG (source_object), res, &local_error);

  if (file != NULL)
    gcv_image_mask_brush_new_from_file_async (
        file, G_PRIORITY_DEFAULT, NULL, load_brush_image_file_finish_cb, self);
  else
    {
      GtkWidget      *app_window  = NULL;
      GtkApplication *application = NULL;
      GtkWindow      *window      = NULL;

      /* TODO create function for this */
      app_window = gtk_widget_get_ancestor (GTK_WIDGET (self), GTK_TYPE_WINDOW);
      g_assert (app_window != NULL);
      application = gtk_window_get_application (GTK_WINDOW (app_window));
      window      = gtk_application_get_active_window (application);

      gcv_dialog (
          "An Error Occurred",
          "Could not retrieve image mask from disk.",
          local_error->message,
          window, NULL);

      gtk_widget_set_sensitive (GTK_WIDGET (self->new_mask_brush), TRUE);
      gtk_widget_set_visible (GTK_WIDGET (self->new_mask_brush_spinner), FALSE);
    }
}

static void
load_brush_image_file_finish_cb (GObject      *source_object,
                                 GAsyncResult *res,
                                 gpointer      data)
{
  GcvBrushArea *self                  = data;
  g_autoptr (GError) local_error      = NULL;
  g_autoptr (GcvImageMaskBrush) brush = NULL;

  brush = gcv_image_mask_brush_new_from_file_finish (res, &local_error);

  if (brush != NULL)
    g_list_store_append (self->brush_store, brush);
  else
    {
      GtkWidget      *app_window  = NULL;
      GtkApplication *application = NULL;
      GtkWindow      *window      = NULL;

      /* TODO create function for this */
      app_window = gtk_widget_get_ancestor (GTK_WIDGET (self), GTK_TYPE_WINDOW);
      g_assert (app_window != NULL);
      application = gtk_window_get_application (GTK_WINDOW (app_window));
      window      = gtk_application_get_active_window (application);

      gcv_dialog (
          "An Error Occurred",
          "Could not retrieve image mask from disk.",
          local_error->message,
          window, NULL);
    }

  gtk_widget_set_sensitive (GTK_WIDGET (self->new_mask_brush), TRUE);
  gtk_widget_set_visible (GTK_WIDGET (self->new_mask_brush_spinner), FALSE);
}
