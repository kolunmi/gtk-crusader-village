/* gtk-crusader-village-map-editor-overlay.c
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
#include "gtk-crusader-village-map-editor-overlay.h"
#include "gtk-crusader-village-map-editor.h"
#include "gtk-crusader-village-map-handle.h"

#define TOOLBAR_HIDDEN_MAX_POINTER_DISTANCE 150.0
#define TOOLBAR_HIDDEN_MIN_OPACITY          0.3

struct _GcvMapEditorOverlay
{
  GcvUtilBin parent_instance;

  GcvMapEditor *editor;
  GcvMapHandle *handle;
  GListStore   *model;

  double   x;
  double   y;
  gboolean drawing;

  /* Template widgets */
  GtkOverlay        *overlay;
  GtkScrolledWindow *scrolled_window;
  GtkFrame          *frame;
  GtkToggleButton   *pencil;
  GtkToggleButton   *draw_line;
  GtkButton         *clear;
  GtkButton         *undo;
  GtkButton         *redo;
};

G_DEFINE_FINAL_TYPE (GcvMapEditorOverlay, gcv_map_editor_overlay, GCV_TYPE_UTIL_BIN)

enum
{
  PROP_0,

  PROP_EDITOR,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
map_handle_changed (GcvMapEditor        *editor,
                    GParamSpec          *pspec,
                    GcvMapEditorOverlay *overlay);

static void
model_items_changed (GListModel          *self,
                     guint                position,
                     guint                removed,
                     guint                added,
                     GcvMapEditorOverlay *timeline_view);

static void
cursor_changed (GcvMapHandle        *editor,
                GParamSpec          *pspec,
                GcvMapEditorOverlay *overlay);

static void
drawing_changed (GcvMapEditor        *editor,
                 GParamSpec          *pspec,
                 GcvMapEditorOverlay *overlay);

static void
line_mode_changed (GcvMapEditor        *editor,
                   GParamSpec          *pspec,
                   GcvMapEditorOverlay *overlay);

static void
clear_final_submission (GcvDialogWindow     *dialog,
                        GParamSpec          *pspec,
                        GcvMapEditorOverlay *overlay);

static void
motion_enter (GtkEventControllerMotion *self,
              gdouble                   x,
              gdouble                   y,
              GcvMapEditorOverlay      *overlay);

static void
motion_event (GtkEventControllerMotion *self,
              gdouble                   x,
              gdouble                   y,
              GcvMapEditorOverlay      *overlay);

static void
motion_leave (GtkEventControllerMotion *self,
              GcvMapEditorOverlay      *overlay);

static void
clear_clicked (GtkButton           *self,
               GcvMapEditorOverlay *overlay);

static void
undo_clicked (GtkButton           *self,
              GcvMapEditorOverlay *overlay);

static void
redo_clicked (GtkButton           *self,
              GcvMapEditorOverlay *overlay);

static void
draw_line_active_changed (GtkToggleButton     *button,
                          GParamSpec          *pspec,
                          GcvMapEditorOverlay *overlay);

static void
update_ui_opacity (GcvMapEditorOverlay *self);

static void
read_map_handle (GcvMapEditorOverlay *self);

static void
update_ui_for_model (GcvMapEditorOverlay *self);

static void
gcv_map_editor_overlay_dispose (GObject *object)
{
  GcvMapEditorOverlay *self = GCV_MAP_EDITOR_OVERLAY (object);

  if (self->editor != NULL)
    {
      g_signal_handlers_disconnect_by_func (self->editor, map_handle_changed, self);
      g_signal_handlers_disconnect_by_func (self->editor, drawing_changed, self);
      g_signal_handlers_disconnect_by_func (self->editor, line_mode_changed, self);
    }
  g_clear_object (&self->editor);

  if (self->handle != NULL)
    g_signal_handlers_disconnect_by_func (self->handle, cursor_changed, self);
  g_clear_object (&self->handle);

  if (self->model != NULL)
    g_signal_handlers_disconnect_by_func (self->model, model_items_changed, self);
  g_clear_object (&self->model);

  G_OBJECT_CLASS (gcv_map_editor_overlay_parent_class)->dispose (object);
}

static void
gcv_map_editor_overlay_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GcvMapEditorOverlay *self = GCV_MAP_EDITOR_OVERLAY (object);

  switch (prop_id)
    {
    case PROP_EDITOR:
      g_value_set_object (value, self->editor);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gcv_map_editor_overlay_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GcvMapEditorOverlay *self = GCV_MAP_EDITOR_OVERLAY (object);

  switch (prop_id)
    {
    case PROP_EDITOR:
      {
        if (self->editor != NULL)
          {
            g_signal_handlers_disconnect_by_func (self->editor, map_handle_changed, self);
            g_signal_handlers_disconnect_by_func (self->editor, drawing_changed, self);
            g_signal_handlers_disconnect_by_func (self->editor, line_mode_changed, self);
          }
        g_clear_object (&self->editor);

        g_clear_object (&self->handle);
        if (self->model != NULL)
          g_signal_handlers_disconnect_by_func (self->model, model_items_changed, self);
        g_clear_object (&self->model);

        self->editor = g_value_dup_object (value);

        if (self->editor != NULL)
          {
            read_map_handle (self);
            g_signal_connect (self->editor, "notify::map-handle",
                              G_CALLBACK (map_handle_changed), self);
            g_signal_connect (self->editor, "notify::drawing",
                              G_CALLBACK (drawing_changed), self);
            g_signal_connect (self->editor, "notify::line-mode",
                              G_CALLBACK (line_mode_changed), self);
          }
        else
          update_ui_for_model (self);

        gtk_scrolled_window_set_child (self->scrolled_window, GTK_WIDGET (self->editor));
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gcv_map_editor_overlay_class_init (GcvMapEditorOverlayClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gcv_map_editor_overlay_dispose;
  object_class->get_property = gcv_map_editor_overlay_get_property;
  object_class->set_property = gcv_map_editor_overlay_set_property;

  props[PROP_EDITOR] =
      g_param_spec_object (
          "editor",
          "Editor",
          "The map editor this widget will use",
          GCV_TYPE_MAP_EDITOR,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/Gcv/gtk-crusader-village-map-editor-overlay.ui");
  gtk_widget_class_bind_template_child (widget_class, GcvMapEditorOverlay, overlay);
  gtk_widget_class_bind_template_child (widget_class, GcvMapEditorOverlay, scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, GcvMapEditorOverlay, frame);
  gtk_widget_class_bind_template_child (widget_class, GcvMapEditorOverlay, pencil);
  gtk_widget_class_bind_template_child (widget_class, GcvMapEditorOverlay, draw_line);
  gtk_widget_class_bind_template_child (widget_class, GcvMapEditorOverlay, clear);
  gtk_widget_class_bind_template_child (widget_class, GcvMapEditorOverlay, undo);
  gtk_widget_class_bind_template_child (widget_class, GcvMapEditorOverlay, redo);
}

static void
gcv_map_editor_overlay_init (GcvMapEditorOverlay *self)
{
  GtkEventController *motion_controller = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_toggle_button_set_group (self->draw_line, self->pencil);
  gtk_toggle_button_set_active (self->pencil, TRUE);

  g_signal_connect (self->clear, "clicked", G_CALLBACK (clear_clicked), self);
  g_signal_connect (self->undo, "clicked", G_CALLBACK (undo_clicked), self);
  g_signal_connect (self->redo, "clicked", G_CALLBACK (redo_clicked), self);

  g_signal_connect (self->draw_line, "notify::active", G_CALLBACK (draw_line_active_changed), self);

  motion_controller = gtk_event_controller_motion_new ();
  g_signal_connect (motion_controller, "enter", G_CALLBACK (motion_enter), self);
  g_signal_connect (motion_controller, "motion", G_CALLBACK (motion_event), self);
  g_signal_connect (motion_controller, "leave", G_CALLBACK (motion_leave), self);
  gtk_widget_add_controller (GTK_WIDGET (self), motion_controller);
}

static void
map_handle_changed (GcvMapEditor        *editor,
                    GParamSpec          *pspec,
                    GcvMapEditorOverlay *overlay)
{
  read_map_handle (overlay);
}

static void
model_items_changed (GListModel          *self,
                     guint                position,
                     guint                removed,
                     guint                added,
                     GcvMapEditorOverlay *overlay)
{
  update_ui_for_model (overlay);
}

static void
cursor_changed (GcvMapHandle        *editor,
                GParamSpec          *pspec,
                GcvMapEditorOverlay *overlay)
{
  update_ui_for_model (overlay);
}

static void
drawing_changed (GcvMapEditor        *editor,
                 GParamSpec          *pspec,
                 GcvMapEditorOverlay *overlay)
{
  g_object_get (
      overlay->editor,
      "drawing", &overlay->drawing,
      NULL);

  update_ui_opacity (overlay);
}

static void
line_mode_changed (GcvMapEditor        *editor,
                   GParamSpec          *pspec,
                   GcvMapEditorOverlay *overlay)
{
  gboolean line_mode = FALSE;

  g_object_get (
      editor,
      "line-mode", &line_mode,
      NULL);

  if (line_mode)
    gtk_toggle_button_set_active (overlay->draw_line, TRUE);
  else
    gtk_toggle_button_set_active (overlay->pencil, TRUE);
}

static void
clear_final_submission (GcvDialogWindow     *dialog,
                        GParamSpec          *pspec,
                        GcvMapEditorOverlay *overlay)
{
  g_autoptr (GVariant) submission = NULL;
  gboolean clear                  = FALSE;

  g_object_get (
      dialog,
      "final-submission", &submission,
      NULL);
  g_variant_get (submission, "(b)", &clear);

  if (clear)
    gcv_map_handle_clear_all (overlay->handle);
}

static void
motion_enter (GtkEventControllerMotion *self,
              gdouble                   x,
              gdouble                   y,
              GcvMapEditorOverlay      *overlay)
{
  overlay->x = x;
  overlay->y = y;
  update_ui_opacity (overlay);
}

static void
motion_event (GtkEventControllerMotion *self,
              gdouble                   x,
              gdouble                   y,
              GcvMapEditorOverlay      *overlay)
{
  overlay->x = x;
  overlay->y = y;
  update_ui_opacity (overlay);
}

static void
motion_leave (GtkEventControllerMotion *self,
              GcvMapEditorOverlay      *overlay)
{
  overlay->x = -1.0;
  overlay->y = -1.0;
  update_ui_opacity (overlay);
}

static void
clear_clicked (GtkButton           *self,
               GcvMapEditorOverlay *overlay)
{
  g_autoptr (GListModel) model = NULL;
  guint n_strokes              = 0;

  if (overlay->handle == NULL)
    return;

  g_object_get (
      overlay->handle,
      "model", &model,
      NULL);
  n_strokes = g_list_model_get_n_items (model);

  if (n_strokes <= 5)
    gcv_map_handle_clear_all (overlay->handle);
  else
    {
      GtkWidget       *window               = NULL;
      g_autofree char *body                 = NULL;
      g_autoptr (GVariant) dialog_structure = NULL;
      GcvDialogWindow *dialog               = NULL;

      window = gtk_widget_get_ancestor (GTK_WIDGET (self), GTK_TYPE_WINDOW);
      g_assert (window != NULL);

      body             = g_strdup_printf ("You will clear all %d strokes. This action is irreversible.", n_strokes);
      dialog_structure = g_variant_new_parsed ("{\"I want to clear the map\":<%b>}", TRUE);

      dialog = gcv_dialog (
          "Confirmation",
          "Clear the Map?",
          body,
          GTK_WINDOW (window),
          dialog_structure);

      g_signal_connect (dialog, "notify::final-submission", G_CALLBACK (clear_final_submission), overlay);
    }
}

static void
undo_clicked (GtkButton           *self,
              GcvMapEditorOverlay *overlay)
{
  guint cursor = 0;

  if (overlay->handle == NULL)
    return;

  g_object_get (
      overlay->handle,
      "cursor", &cursor,
      NULL);
  if (cursor > 0)
    g_object_set (
        overlay->handle,
        "cursor", cursor - 1,
        NULL);
}

static void
redo_clicked (GtkButton           *self,
              GcvMapEditorOverlay *overlay)
{
  guint cursor = 0;

  if (overlay->handle == NULL)
    return;

  g_object_get (
      overlay->handle,
      "cursor", &cursor,
      NULL);
  g_object_set (
      overlay->handle,
      "cursor", cursor + 1,
      NULL);
}

static void
draw_line_active_changed (GtkToggleButton     *button,
                          GParamSpec          *pspec,
                          GcvMapEditorOverlay *overlay)
{
  gboolean line_mode = FALSE;

  if (overlay->editor == NULL)
    return;

  line_mode = gtk_toggle_button_get_active (button);
  g_object_set (
      overlay->editor,
      "line-mode", line_mode,
      NULL);
}

static double
distance_to_rect (double rx,
                  double ry,
                  double rw,
                  double rh,
                  double x,
                  double y)
{
  if (x >= rx && x < rx + rw)
    return y < ry ? ry - y : y - (ry + rh);
  else if (y >= ry && y < ry + rh)
    return x < rx ? rx - x : x - (rx + rw);
  else
    return graphene_point_distance (
        &GRAPHENE_POINT_INIT (x, y),
        &GRAPHENE_POINT_INIT (
            x < rx ? rx : rx + rw,
            y < ry ? ry : ry + rh),
        NULL, NULL);
}

static void
update_ui_opacity (GcvMapEditorOverlay *self)
{
  if (!self->drawing || self->x < 0.0 || self->y < 0.0)
    gtk_widget_set_opacity (GTK_WIDGET (self->frame), 1.0);
  else
    {
      graphene_rect_t bounds   = { 0 };
      gboolean        computed = FALSE;

      computed = gtk_widget_compute_bounds (GTK_WIDGET (self->frame), GTK_WIDGET (self), &bounds);

      if (computed &&
          (self->x < bounds.origin.x ||
           self->y < bounds.origin.y ||
           self->x >= bounds.origin.x + bounds.size.width ||
           self->y >= bounds.origin.y + bounds.size.height))
        {
          double distance = 0.0;
          double opacity  = 0.0;

          distance = distance_to_rect (
              bounds.origin.x, bounds.origin.y,
              bounds.size.width, bounds.size.height,
              self->x, self->y);

          opacity = distance >= TOOLBAR_HIDDEN_MAX_POINTER_DISTANCE
                        ? 1.0
                        : TOOLBAR_HIDDEN_MIN_OPACITY +
                              distance /
                                  (TOOLBAR_HIDDEN_MAX_POINTER_DISTANCE *
                                   (1.0 + TOOLBAR_HIDDEN_MIN_OPACITY));

          gtk_widget_set_opacity (GTK_WIDGET (self->frame), opacity);
        }
      else
        gtk_widget_set_opacity (GTK_WIDGET (self->frame), TOOLBAR_HIDDEN_MIN_OPACITY);
    }
}

static void
read_map_handle (GcvMapEditorOverlay *self)
{
  if (self->model != NULL)
    g_signal_handlers_disconnect_by_func (self->model, model_items_changed, self);
  g_clear_object (&self->model);
  if (self->handle != NULL)
    g_signal_handlers_disconnect_by_func (self->handle, cursor_changed, self);
  g_clear_object (&self->handle);

  if (self->editor == NULL)
    return;

  g_object_get (
      self->editor,
      "map-handle", &self->handle,
      NULL);

  if (self->handle != NULL)
    {
      g_signal_connect (self->handle, "notify::cursor",
                        G_CALLBACK (cursor_changed), self);

      g_object_get (
          self->handle,
          "model", &self->model,
          NULL);
      g_assert (self->model != NULL);

      g_signal_connect (self->model, "items-changed",
                        G_CALLBACK (model_items_changed), self);
    }

  update_ui_for_model (self);
}

static void
update_ui_for_model (GcvMapEditorOverlay *self)
{
  if (self->model != NULL)
    {
      guint n_strokes = 0;
      guint cursor    = 0;

      n_strokes = g_list_model_get_n_items (G_LIST_MODEL (self->model));
      g_object_get (
          self->handle,
          "cursor", &cursor,
          NULL);

      gtk_widget_set_sensitive (GTK_WIDGET (self->undo), cursor > 0);
      gtk_widget_set_sensitive (GTK_WIDGET (self->redo), cursor < n_strokes);
      gtk_widget_set_sensitive (GTK_WIDGET (self->clear), n_strokes > 0);
    }
  else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (self->undo), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (self->redo), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (self->clear), FALSE);
    }
}
