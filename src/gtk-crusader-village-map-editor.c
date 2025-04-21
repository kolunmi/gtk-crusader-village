/* gtk-crusader-village-map-editor.c
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

#include "gtk-crusader-village-brush-area.h"
#include "gtk-crusader-village-brushable.h"
#include "gtk-crusader-village-dialog-window.h"
#include "gtk-crusader-village-item-area.h"
#include "gtk-crusader-village-item-stroke.h"
#include "gtk-crusader-village-item.h"
#include "gtk-crusader-village-map-editor.h"
#include "gtk-crusader-village-map-handle.h"
#include "gtk-crusader-village-map.h"

#define BASE_TILE_SIZE 16.0

struct _GtkCrusaderVillageMapEditor
{
  GtkWidget parent_instance;

  GtkSettings *gtk_settings;
  gboolean     dark_theme;

  GSettings *settings;
  gboolean   show_grid;
  gboolean   show_gradient;
  gboolean   show_cursor_glow;

  GtkCrusaderVillageMap       *map;
  GtkCrusaderVillageMapHandle *handle;

  GtkCrusaderVillageItemArea  *item_area;
  GtkCrusaderVillageBrushArea *brush_area;

  int      border_gap;
  gboolean line_mode;

  GHashTable     *tile_textures;
  GskRenderNode  *render_cache;
  graphene_rect_t viewport;
  GdkTexture     *bg_image_tex;

  double   zoom;
  gboolean queue_center;

  double pointer_x;
  double pointer_y;
  int    hover_x;
  int    hover_y;
  double canvas_x;
  double canvas_y;

  GtkScrollablePolicy hscroll_policy;
  GtkScrollablePolicy vscroll_policy;
  GtkAdjustment      *hadjustment;
  GtkAdjustment      *vadjustment;

  double drag_start_hadjustment_val;
  double drag_start_vadjustment_val;
  double zoom_gesture_start_val;

  GtkGesture *drag_gesture;
  GtkGesture *draw_gesture;
  GtkGesture *cancel_gesture;
  GtkGesture *zoom_gesture;

  GtkCrusaderVillageItemStroke *current_stroke;
  GtkCrusaderVillageItemStroke *brush_stroke;

  guint8        *brush_mask;
  int            brush_width;
  int            brush_height;
  GskRenderNode *brush_node;
  GtkAdjustment *brush_adjustment;
};

static void scrollable_iface_init (GtkScrollableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (
    GtkCrusaderVillageMapEditor,
    gtk_crusader_village_map_editor,
    GTK_TYPE_WIDGET,
    G_IMPLEMENT_INTERFACE (GTK_TYPE_SCROLLABLE, scrollable_iface_init))

enum
{
  PROP_0,

  PROP_SETTINGS,
  PROP_MAP_HANDLE,
  PROP_ITEM_AREA,
  PROP_BRUSH_AREA,
  PROP_BORDER_GAP,
  PROP_HOVER_X,
  PROP_HOVER_Y,
  PROP_DRAWING,
  PROP_LINE_MODE,

  LAST_NATIVE_PROP,

  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_HSCROLL_POLICY,
  PROP_VSCROLL_POLICY,

  LAST_PROP,
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
gtk_crusader_village_map_editor_measure (GtkWidget     *widget,
                                         GtkOrientation orientation,
                                         int            for_size,
                                         int           *minimum,
                                         int           *natural,
                                         int           *minimum_baseline,
                                         int           *natural_baseline);

static void
gtk_crusader_village_map_editor_size_allocate (GtkWidget *widget,
                                               int        widget_width,
                                               int        widget_height,
                                               int        baseline);

static void
gtk_crusader_village_map_editor_snapshot (GtkWidget   *widget,
                                          GtkSnapshot *snapshot);

static void
back_page_cb (GtkWidget  *widget,
              const char *action_name,
              GVariant   *parameter);

static void
back_one_cb (GtkWidget  *widget,
             const char *action_name,
             GVariant   *parameter);

static void
forward_one_cb (GtkWidget  *widget,
                const char *action_name,
                GVariant   *parameter);

static void
forward_page_cb (GtkWidget  *widget,
                 const char *action_name,
                 GVariant   *parameter);

static void
beginning_cb (GtkWidget  *widget,
              const char *action_name,
              GVariant   *parameter);

static void
end_cb (GtkWidget  *widget,
        const char *action_name,
        GVariant   *parameter);

static void
jump_to_search_cb (GtkWidget  *widget,
                   const char *action_name,
                   GVariant   *parameter);

static void
dark_theme_changed (GtkSettings                 *settings,
                    GParamSpec                  *pspec,
                    GtkCrusaderVillageMapEditor *editor);

static void
show_grid_changed (GSettings                   *self,
                   gchar                       *key,
                   GtkCrusaderVillageMapEditor *editor);

static void
show_gradient_changed (GSettings                   *self,
                       gchar                       *key,
                       GtkCrusaderVillageMapEditor *editor);

static void
show_cursor_glow_changed (GSettings                   *self,
                          gchar                       *key,
                          GtkCrusaderVillageMapEditor *editor);

static void
background_image_changed (GSettings                   *self,
                          gchar                       *key,
                          GtkCrusaderVillageMapEditor *editor);

static void
adjustment_value_changed (GtkAdjustment               *adjustment,
                          GtkCrusaderVillageMapEditor *text_view);

static void
drag_gesture_begin_gesture (GtkGesture                  *self,
                            GdkEventSequence            *sequence,
                            GtkCrusaderVillageMapEditor *editor);

static void
drag_gesture_begin (GtkGestureDrag              *self,
                    double                       start_x,
                    double                       start_y,
                    GtkCrusaderVillageMapEditor *editor);

static void
drag_gesture_update (GtkGestureDrag              *gesture,
                     double                       offset_x,
                     double                       offset_y,
                     GtkCrusaderVillageMapEditor *editor);

static void
drag_gesture_end (GtkGestureDrag              *gesture,
                  double                       offset_x,
                  double                       offset_y,
                  GtkCrusaderVillageMapEditor *editor);

static void
drag_gesture_end_gesture (GtkGesture                  *self,
                          GdkEventSequence            *sequence,
                          GtkCrusaderVillageMapEditor *editor);

static void
draw_gesture_begin_gesture (GtkGesture                  *self,
                            GdkEventSequence            *sequence,
                            GtkCrusaderVillageMapEditor *editor);

static void
draw_gesture_begin (GtkGestureDrag              *self,
                    double                       start_x,
                    double                       start_y,
                    GtkCrusaderVillageMapEditor *editor);

static void
draw_gesture_update (GtkGestureDrag              *gesture,
                     double                       offset_x,
                     double                       offset_y,
                     GtkCrusaderVillageMapEditor *editor);

static void
draw_gesture_end (GtkGestureDrag              *gesture,
                  double                       offset_x,
                  double                       offset_y,
                  GtkCrusaderVillageMapEditor *editor);

static void
draw_gesture_end_gesture (GtkGesture                  *self,
                          GdkEventSequence            *sequence,
                          GtkCrusaderVillageMapEditor *editor);

static void
cancel_gesture_begin_gesture (GtkGesture                  *self,
                              GdkEventSequence            *sequence,
                              GtkCrusaderVillageMapEditor *editor);

static void
cancel_gesture_end_gesture (GtkGesture                  *self,
                            GdkEventSequence            *sequence,
                            GtkCrusaderVillageMapEditor *editor);

static void
zoom_gesture_begin (GtkGesture                  *self,
                    GdkEventSequence            *sequence,
                    GtkCrusaderVillageMapEditor *editor);

static void
zoom_gesture_scale_changed (GtkGestureZoom              *self,
                            double                       scale,
                            GtkCrusaderVillageMapEditor *editor);

static void
zoom_gesture_end (GtkGesture                  *self,
                  GdkEventSequence            *sequence,
                  GtkCrusaderVillageMapEditor *editor);

static gboolean
scroll_event (GtkEventControllerScroll    *self,
              double                       dx,
              double                       dy,
              GtkCrusaderVillageMapEditor *editor);

static void
motion_enter (GtkEventControllerMotion    *self,
              gdouble                      x,
              gdouble                      y,
              GtkCrusaderVillageMapEditor *editor);

static void
motion_event (GtkEventControllerMotion    *self,
              gdouble                      x,
              gdouble                      y,
              GtkCrusaderVillageMapEditor *editor);

static void
motion_leave (GtkEventControllerMotion    *self,
              GtkCrusaderVillageMapEditor *editor);

static void
selected_item_changed (GtkCrusaderVillageItemArea  *item_area,
                       GParamSpec                  *pspec,
                       GtkCrusaderVillageMapEditor *editor);

static void
selected_brush_changed (GtkCrusaderVillageBrushArea *brush_area,
                        GParamSpec                  *pspec,
                        GtkCrusaderVillageMapEditor *editor);

static void
selected_brush_adjustment_value_changed (GtkAdjustment               *adjustment,
                                         GtkCrusaderVillageMapEditor *editor);

static void
grid_changed (GtkCrusaderVillageMapHandle *handle,
              GParamSpec                  *pspec,
              GtkCrusaderVillageMapEditor *editor);

static void
cursor_changed (GtkCrusaderVillageMapHandle *handle,
                GParamSpec                  *pspec,
                GtkCrusaderVillageMapEditor *editor);

static void
update_scrollable (GtkCrusaderVillageMapEditor *self,
                   gboolean                     center);

static void
update_motion (GtkCrusaderVillageMapEditor *self,
               double                       x,
               double                       y);

static void
read_brush (GtkCrusaderVillageMapEditor *self,
            gboolean                     different);

static void
read_background_image (GtkCrusaderVillageMapEditor *self);

static void
background_texture_load_async_thread (GTask        *task,
                                      gpointer      object,
                                      gpointer      task_data,
                                      GCancellable *cancellable);

static void
background_texture_load_finish_cb (GObject      *source_object,
                                   GAsyncResult *res,
                                   gpointer      data);

static void
gtk_crusader_village_map_editor_dispose (GObject *object)
{
  GtkCrusaderVillageMapEditor *self = GTK_CRUSADER_VILLAGE_MAP_EDITOR (object);

  g_signal_handlers_disconnect_by_func (
      self->gtk_settings, dark_theme_changed, self);

  if (self->settings != NULL)
    {
      g_signal_handlers_disconnect_by_func (
          self->settings, show_grid_changed, self);
      g_signal_handlers_disconnect_by_func (
          self->settings, show_gradient_changed, self);
      g_signal_handlers_disconnect_by_func (
          self->settings, show_cursor_glow_changed, self);
      g_signal_handlers_disconnect_by_func (
          self->settings, background_image_changed, self);
    }
  g_clear_object (&self->settings);

  if (self->handle != NULL)
    {
      g_signal_handlers_disconnect_by_func (
          self->handle, grid_changed, self);
      g_signal_handlers_disconnect_by_func (
          self->handle, cursor_changed, self);
    }
  g_clear_object (&self->handle);
  g_clear_object (&self->map);

  if (self->item_area != NULL)
    g_signal_handlers_disconnect_by_func (
        self->item_area, selected_item_changed, self);
  g_clear_object (&self->item_area);
  if (self->item_area != NULL)
    g_signal_handlers_disconnect_by_func (
        self->item_area, selected_brush_changed, self);
  g_clear_object (&self->brush_area);

  g_clear_object (&self->hadjustment);
  g_clear_object (&self->vadjustment);
  g_clear_object (&self->current_stroke);
  g_clear_object (&self->brush_stroke);
  g_clear_pointer (&self->tile_textures, g_hash_table_unref);
  g_clear_object (&self->bg_image_tex);
  g_clear_pointer (&self->render_cache, gsk_render_node_unref);
  g_clear_pointer (&self->brush_node, gsk_render_node_unref);

  if (self->brush_adjustment != NULL)
    g_signal_handlers_disconnect_by_func (
        self->brush_adjustment, selected_brush_adjustment_value_changed, self);
  g_clear_object (&self->brush_adjustment);

  G_OBJECT_CLASS (gtk_crusader_village_map_editor_parent_class)->dispose (object);
}

static void
gtk_crusader_village_map_editor_get_property (GObject    *object,
                                              guint       prop_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  GtkCrusaderVillageMapEditor *self = GTK_CRUSADER_VILLAGE_MAP_EDITOR (object);

  switch (prop_id)
    {
    case PROP_SETTINGS:
      g_value_set_object (value, self->settings);
      break;
    case PROP_MAP_HANDLE:
      g_value_set_object (value, self->handle);
      break;
    case PROP_ITEM_AREA:
      g_value_set_object (value, self->item_area);
      break;
    case PROP_BRUSH_AREA:
      g_value_set_object (value, self->brush_area);
      break;
    case PROP_BORDER_GAP:
      g_value_set_int (value, self->border_gap);
      break;
    case PROP_HOVER_X:
      g_value_set_int (value, self->hover_x);
      break;
    case PROP_HOVER_Y:
      g_value_set_int (value, self->hover_y);
      break;
    case PROP_DRAWING:
      g_value_set_boolean (value, self->current_stroke != NULL);
      break;
    case PROP_LINE_MODE:
      g_value_set_boolean (value, self->line_mode);
      break;
    case PROP_HADJUSTMENT:
      g_value_set_object (value, self->hadjustment);
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value, self->vadjustment);
      break;
    case PROP_HSCROLL_POLICY:
      g_value_set_enum (value, self->hscroll_policy);
      break;
    case PROP_VSCROLL_POLICY:
      g_value_set_enum (value, self->vscroll_policy);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_map_editor_set_property (GObject      *object,
                                              guint         prop_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  GtkCrusaderVillageMapEditor *self = GTK_CRUSADER_VILLAGE_MAP_EDITOR (object);

  switch (prop_id)
    {
    case PROP_SETTINGS:
      if (self->settings != NULL)
        {
          g_signal_handlers_disconnect_by_func (
              self->settings, show_grid_changed, self);
          g_signal_handlers_disconnect_by_func (
              self->settings, show_gradient_changed, self);
          g_signal_handlers_disconnect_by_func (
              self->settings, show_cursor_glow_changed, self);
        }
      g_clear_object (&self->settings);

      self->settings = g_value_dup_object (value);
      if (self->settings != NULL)
        {
          g_signal_connect (self->settings, "changed::show-grid",
                            G_CALLBACK (show_grid_changed), self);
          g_signal_connect (self->settings, "changed::show-gradient",
                            G_CALLBACK (show_gradient_changed), self);
          g_signal_connect (self->settings, "changed::show-cursor-glow",
                            G_CALLBACK (show_cursor_glow_changed), self);
          g_signal_connect (self->settings, "changed::background-image",
                            G_CALLBACK (background_image_changed), self);

          self->show_grid        = g_settings_get_boolean (self->settings, "show-grid");
          self->show_gradient    = g_settings_get_boolean (self->settings, "show-gradient");
          self->show_cursor_glow = g_settings_get_boolean (self->settings, "show-cursor-glow");
          read_background_image (self);
        }
      else
        {
          self->show_grid     = TRUE;
          self->show_gradient = TRUE;
        }

      gtk_widget_queue_draw (GTK_WIDGET (self));
      break;

    case PROP_MAP_HANDLE:
      if (self->handle != NULL)
        {
          g_signal_handlers_disconnect_by_func (
              self->handle, grid_changed, self);
          g_signal_handlers_disconnect_by_func (
              self->handle, cursor_changed, self);
        }
      g_clear_object (&self->handle);
      g_clear_object (&self->map);

      self->handle = g_value_dup_object (value);

      if (self->handle != NULL)
        {
          g_object_get (
              self->handle,
              "map", &self->map,
              NULL);
          g_signal_connect (self->handle, "notify::grid",
                            G_CALLBACK (grid_changed), self);
          g_signal_connect (self->handle, "notify::cursor",
                            G_CALLBACK (cursor_changed), self);
        }

      self->queue_center = TRUE;
      g_clear_pointer (&self->render_cache, gsk_render_node_unref);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      break;

    case PROP_ITEM_AREA:
      if (self->item_area != NULL)
        g_signal_handlers_disconnect_by_func (
            self->item_area, selected_item_changed, self);
      g_clear_object (&self->item_area);

      self->item_area = g_value_dup_object (value);
      if (self->item_area != NULL)
        g_signal_connect (self->item_area, "notify::selected-item",
                          G_CALLBACK (selected_item_changed), self);
      break;

    case PROP_BRUSH_AREA:
      if (self->brush_area != NULL)
        g_signal_handlers_disconnect_by_func (
            self->brush_area, selected_brush_changed, self);
      g_clear_object (&self->brush_area);

      self->brush_area = g_value_dup_object (value);
      if (self->brush_area != NULL)
        g_signal_connect (self->brush_area, "notify::selected-brush",
                          G_CALLBACK (selected_brush_changed), self);
      read_brush (self, TRUE);
      break;

    case PROP_BORDER_GAP:
      self->border_gap = g_value_get_int (value);
      gtk_widget_queue_allocate (GTK_WIDGET (self));
      break;

    case PROP_LINE_MODE:
      self->line_mode = g_value_get_boolean (value);
      break;

    case PROP_HADJUSTMENT:
      {
        GtkAdjustment *adjustment = NULL;

        if (self->hadjustment != NULL)
          g_signal_handlers_disconnect_by_func (
              self->hadjustment,
              adjustment_value_changed,
              self);
        g_clear_object (&self->hadjustment);

        adjustment = g_value_get_object (value);
        if (adjustment != NULL)
          {
            g_signal_connect (adjustment, "value-changed",
                              G_CALLBACK (adjustment_value_changed), self);
            self->hadjustment = g_object_ref (adjustment);
            update_scrollable (self, TRUE);
          }
      }
      break;

    case PROP_VADJUSTMENT:
      {
        GtkAdjustment *adjustment = NULL;

        if (self->vadjustment != NULL)
          g_signal_handlers_disconnect_by_func (
              self->vadjustment,
              adjustment_value_changed,
              self);
        g_clear_object (&self->vadjustment);

        adjustment = g_value_get_object (value);
        if (adjustment != NULL)
          {
            g_signal_connect (adjustment, "value-changed",
                              G_CALLBACK (adjustment_value_changed), self);
            self->vadjustment = g_object_ref (adjustment);
            update_scrollable (self, TRUE);
          }
      }
      break;

    case PROP_HSCROLL_POLICY:
      self->hscroll_policy = g_value_get_enum (value);
      gtk_widget_queue_resize (GTK_WIDGET (self));
      break;

    case PROP_VSCROLL_POLICY:
      self->vscroll_policy = g_value_get_enum (value);
      gtk_widget_queue_resize (GTK_WIDGET (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_map_editor_class_init (GtkCrusaderVillageMapEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gtk_crusader_village_map_editor_dispose;
  object_class->get_property = gtk_crusader_village_map_editor_get_property;
  object_class->set_property = gtk_crusader_village_map_editor_set_property;

  props[PROP_SETTINGS] =
      g_param_spec_object (
          "settings",
          "Settings",
          "The settings object for this editor",
          G_TYPE_SETTINGS,
          G_PARAM_READWRITE);

  props[PROP_MAP_HANDLE] =
      g_param_spec_object (
          "map-handle",
          "Map Handle",
          "The map handle this view is editing through",
          GTK_CRUSADER_VILLAGE_TYPE_MAP_HANDLE,
          G_PARAM_READWRITE);

  props[PROP_ITEM_AREA] =
      g_param_spec_object (
          "item-area",
          "Item Area",
          "The item area to use for tracking the current brush",
          GTK_CRUSADER_VILLAGE_TYPE_ITEM_AREA,
          G_PARAM_READWRITE);

  props[PROP_BRUSH_AREA] =
      g_param_spec_object (
          "brush-area",
          "Brush Area",
          "The brush area to use for tracking the current brush",
          GTK_CRUSADER_VILLAGE_TYPE_BRUSH_AREA,
          G_PARAM_READWRITE);

  props[PROP_BORDER_GAP] =
      g_param_spec_int (
          "border-gap",
          "Border Gap",
          "The size of the empty space around the visual map",
          0, 32, 2,
          G_PARAM_READWRITE);

  props[PROP_HOVER_X] =
      g_param_spec_int (
          "hover-x",
          "Hover X",
          "The x coordinate of the tile over which the editor is hovered, or -1 if no tile is currently hovered",
          -1, G_MAXINT, -1,
          G_PARAM_READABLE);

  props[PROP_HOVER_Y] =
      g_param_spec_int (
          "hover-y",
          "Hover Y",
          "The y coordinate of the tile over which the editor is hovered, or -1 if no tile is currently hovered",
          -1, G_MAXINT, -1,
          G_PARAM_READABLE);

  props[PROP_DRAWING] =
      g_param_spec_boolean (
          "drawing",
          "Drawing",
          "Whether a drawing operation is currently taking place",
          FALSE,
          G_PARAM_READABLE);

  props[PROP_LINE_MODE] =
      g_param_spec_boolean (
          "line-mode",
          "Line Mode",
          "Whether this widget draws lines instead of freehand",
          FALSE,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_NATIVE_PROP, props);

  g_object_class_override_property (object_class, PROP_HADJUSTMENT, "hadjustment");
  g_object_class_override_property (object_class, PROP_VADJUSTMENT, "vadjustment");
  g_object_class_override_property (object_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (object_class, PROP_VSCROLL_POLICY, "vscroll-policy");

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/GtkCrusaderVillage/gtk-crusader-village-map-editor.ui");

  widget_class->measure       = gtk_crusader_village_map_editor_measure;
  widget_class->size_allocate = gtk_crusader_village_map_editor_size_allocate;
  widget_class->snapshot      = gtk_crusader_village_map_editor_snapshot;

  gtk_widget_class_install_action (widget_class, "back-page", NULL, back_page_cb);
  gtk_widget_class_install_action (widget_class, "back-one", NULL, back_one_cb);
  gtk_widget_class_install_action (widget_class, "forward-one", NULL, forward_one_cb);
  gtk_widget_class_install_action (widget_class, "forward-page", NULL, forward_page_cb);
  gtk_widget_class_install_action (widget_class, "beginning", NULL, beginning_cb);
  gtk_widget_class_install_action (widget_class, "end", NULL, end_cb);
  gtk_widget_class_install_action (widget_class, "jump-to-search", NULL, jump_to_search_cb);
}

static void
gtk_crusader_village_map_editor_init (GtkCrusaderVillageMapEditor *self)
{
  GtkEventController *scroll_controller = NULL;
  GtkEventController *motion_controller = NULL;

  self->border_gap = 2;
  self->zoom       = 1.0;

  self->pointer_x = -1.0;
  self->pointer_y = -1.0;
  self->hover_x   = -1;
  self->hover_y   = -1;
  self->canvas_x  = -1.0;
  self->canvas_y  = -1.0;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->gtk_settings = gtk_settings_get_default ();
  g_signal_connect (self->gtk_settings, "notify::gtk-application-prefer-dark-theme",
                    G_CALLBACK (dark_theme_changed), self);
  g_object_get (
      self->gtk_settings,
      "gtk-application-prefer-dark-theme", &self->dark_theme,
      NULL);

  self->tile_textures = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_object_unref);

  self->drag_gesture = gtk_gesture_drag_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->drag_gesture), 2);
  g_signal_connect (self->drag_gesture, "begin", G_CALLBACK (drag_gesture_begin_gesture), self);
  g_signal_connect (self->drag_gesture, "drag-begin", G_CALLBACK (drag_gesture_begin), self);
  g_signal_connect (self->drag_gesture, "drag-update", G_CALLBACK (drag_gesture_update), self);
  g_signal_connect (self->drag_gesture, "drag-end", G_CALLBACK (drag_gesture_end), self);
  g_signal_connect (self->drag_gesture, "end", G_CALLBACK (drag_gesture_end_gesture), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag_gesture));

  self->draw_gesture = gtk_gesture_drag_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->draw_gesture), 1);
  g_signal_connect (self->draw_gesture, "begin", G_CALLBACK (draw_gesture_begin_gesture), self);
  g_signal_connect (self->draw_gesture, "drag-begin", G_CALLBACK (draw_gesture_begin), self);
  g_signal_connect (self->draw_gesture, "drag-update", G_CALLBACK (draw_gesture_update), self);
  g_signal_connect (self->draw_gesture, "drag-end", G_CALLBACK (draw_gesture_end), self);
  g_signal_connect (self->draw_gesture, "end", G_CALLBACK (draw_gesture_end_gesture), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->draw_gesture));

  self->cancel_gesture = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->cancel_gesture), 3);
  g_signal_connect (self->cancel_gesture, "begin", G_CALLBACK (cancel_gesture_begin_gesture), self);
  g_signal_connect (self->cancel_gesture, "end", G_CALLBACK (cancel_gesture_end_gesture), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->cancel_gesture));

  self->zoom_gesture = gtk_gesture_zoom_new ();
  g_signal_connect (self->zoom_gesture, "begin", G_CALLBACK (zoom_gesture_begin), self);
  g_signal_connect (self->zoom_gesture, "scale-changed", G_CALLBACK (zoom_gesture_scale_changed), self);
  g_signal_connect (self->zoom_gesture, "end", G_CALLBACK (zoom_gesture_end), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->zoom_gesture));

  gtk_gesture_group (self->draw_gesture, self->drag_gesture);

  scroll_controller = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  g_signal_connect (scroll_controller, "scroll", G_CALLBACK (scroll_event), self);
  gtk_widget_add_controller (GTK_WIDGET (self), scroll_controller);

  motion_controller = gtk_event_controller_motion_new ();
  g_signal_connect (motion_controller, "enter", G_CALLBACK (motion_enter), self);
  g_signal_connect (motion_controller, "motion", G_CALLBACK (motion_event), self);
  g_signal_connect (motion_controller, "leave", G_CALLBACK (motion_leave), self);
  gtk_widget_add_controller (GTK_WIDGET (self), motion_controller);

  gtk_widget_set_cursor_from_name (GTK_WIDGET (self), "crosshair");
}

static void
gtk_crusader_village_map_editor_measure (GtkWidget     *widget,
                                         GtkOrientation orientation,
                                         int            for_size,
                                         int           *minimum,
                                         int           *natural,
                                         int           *minimum_baseline,
                                         int           *natural_baseline)
{
  GtkCrusaderVillageMapEditor *editor         = GTK_CRUSADER_VILLAGE_MAP_EDITOR (widget);
  int                          map_width      = 0;
  int                          map_height     = 0;
  int                          dimension      = 0;
  double                       minimum_double = 0.0;

  if (editor->map == NULL)
    {
      *minimum = 0;
      *natural = 0;
      return;
    }

  g_object_get (
      editor->map,
      "width", &map_width,
      "height", &map_height,
      NULL);

  dimension = orientation == GTK_ORIENTATION_VERTICAL ? map_height : map_width;

  minimum_double = (double) dimension * BASE_TILE_SIZE * editor->zoom;
  *minimum       = minimum_double;
  *natural       = minimum_double + BASE_TILE_SIZE * (double) editor->border_gap;
}

static void
gtk_crusader_village_map_editor_size_allocate (GtkWidget *widget,
                                               int        widget_width,
                                               int        widget_height,
                                               int        baseline)
{
  GtkCrusaderVillageMapEditor *editor = GTK_CRUSADER_VILLAGE_MAP_EDITOR (widget);

  update_scrollable (editor, FALSE);
}

static void
gtk_crusader_village_map_editor_snapshot (GtkWidget   *widget,
                                          GtkSnapshot *snapshot)
{
#define BORDER_WIDTH(w)           ((float[4]) { (w), (w), (w), (w) })
#define BORDER_COLOR_LITERAL(...) ((GdkRGBA[4]) { __VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__ })

  const GskColorStop bg_radial_gradient_color_stops[2] = {
    { 0.25, { 0.75, 0.55, 0.80, 1.0 } },
    {  1.0, { 0.90, 0.75, 0.80, 1.0 } },
  };
  const GskColorStop bg_radial_gradient_color_stops_dark[2] = {
    { 0.25, { 0.55, 0.20, 0.40, 1.0 } },
    {  1.0, { 0.90, 0.40, 0.30, 1.0 } },
  };
  const GskColorStop cursor_radial_gradient_color_stops[2] = {
    { 0.0, { 0.3, 0.2, 0.4, 0.5 } },
    { 1.0, { 0.1, 0.1, 0.1, 0.0 } },
  };

  GtkCrusaderVillageMapEditor *editor          = GTK_CRUSADER_VILLAGE_MAP_EDITOR (widget);
  int                          widget_width    = 0;
  int                          widget_height   = 0;
  graphene_rect_t              viewport        = { 0 };
  graphene_rect_t              extents         = { 0 };
  double                       tile_size       = 0.0;
  int                          map_tile_width  = 0;
  int                          map_tile_height = 0;
  g_autoptr (GListStore) strokes               = NULL;
  double  map_width                            = 0.0;
  double  map_height                           = 0.0;
  GdkRGBA widget_rgba                          = { 0 };

  widget_width  = gtk_widget_get_width (widget);
  widget_height = gtk_widget_get_height (widget);

  gtk_snapshot_push_clip (
      snapshot,
      &GRAPHENE_RECT_INIT (
          0, 0, widget_width, widget_height));

  if (editor->hadjustment != NULL && editor->vadjustment != NULL)
    {
      double lower_x = 0.0;
      double lower_y = 0.0;
      double upper_x = 0.0;
      double upper_y = 0.0;
      double value_x = 0.0;
      double value_y = 0.0;

      g_object_get (
          editor->hadjustment,
          "lower", &lower_x,
          "upper", &upper_x,
          "value", &value_x,
          NULL);
      g_object_get (
          editor->vadjustment,
          "lower", &lower_y,
          "upper", &upper_y,
          "value", &value_y,
          NULL);

      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (-value_x, -value_y));
      viewport = extents = GRAPHENE_RECT_INIT (value_x, value_y, (float) widget_width, (float) widget_height);
    }
  else
    viewport = extents = GRAPHENE_RECT_INIT (0, 0, (float) widget_width, (float) widget_height);

  viewport.origin.x /= editor->zoom;
  viewport.origin.y /= editor->zoom;
  viewport.size.width /= editor->zoom;
  viewport.size.height /= editor->zoom;

  tile_size = BASE_TILE_SIZE * editor->zoom;

  g_object_get (
      editor->map,
      "width", &map_tile_width,
      "height", &map_tile_height,
      "strokes", &strokes,
      NULL);
  map_width  = (double) map_tile_width * tile_size;
  map_height = (double) map_tile_height * tile_size;

  gtk_snapshot_translate (
      snapshot,
      &GRAPHENE_POINT_INIT (
          (double) editor->border_gap * tile_size,
          (double) editor->border_gap * tile_size));
  viewport.origin.x -= (double) editor->border_gap * BASE_TILE_SIZE;
  viewport.origin.y -= (double) editor->border_gap * BASE_TILE_SIZE;
  extents.origin.x -= (double) editor->border_gap * tile_size;
  extents.origin.y -= (double) editor->border_gap * tile_size;

  if (editor->bg_image_tex != NULL)
    gtk_snapshot_append_scaled_texture (
        snapshot,
        editor->bg_image_tex,
        GSK_SCALING_FILTER_NEAREST,
        &GRAPHENE_RECT_INIT (0, 0, map_width, map_height));
  else if (editor->show_gradient)
    gtk_snapshot_append_radial_gradient (
        snapshot,
        &GRAPHENE_RECT_INIT (0, 0, map_width, map_height),
        &GRAPHENE_POINT_INIT (map_width / 2.0, map_height / 2.0),
        map_width / 2.0, map_height / 2.0, 0.0, 1.0,
        editor->dark_theme ? bg_radial_gradient_color_stops_dark : bg_radial_gradient_color_stops,
        editor->dark_theme ? G_N_ELEMENTS (bg_radial_gradient_color_stops_dark) : G_N_ELEMENTS (bg_radial_gradient_color_stops));

  if (editor->show_grid)
    {
      gtk_snapshot_push_repeat (
          snapshot,
          &GRAPHENE_RECT_INIT (0, 0, map_width, map_height),
          &GRAPHENE_RECT_INIT (0, 0, tile_size, tile_size));
      gtk_snapshot_append_border (
          snapshot,
          &GSK_ROUNDED_RECT_INIT (0, 0, tile_size, tile_size),
          BORDER_WIDTH (BASE_TILE_SIZE / 32.0 * editor->zoom),
          BORDER_COLOR_LITERAL ({ 0.0, 0.0, 0.0, 1.0 }));
      gtk_snapshot_pop (snapshot);
    }

  gtk_snapshot_append_outset_shadow (
      snapshot,
      &GSK_ROUNDED_RECT_INIT (0, 0, map_width, map_height),
      &(GdkRGBA) { 0.0, 0.0, 0.0, 1.0 },
      0.0, 0.0,
      BASE_TILE_SIZE / 2.0 * editor->zoom,
      BASE_TILE_SIZE * 2.0 * editor->zoom);

  if (editor->render_cache == NULL ||
      editor->viewport.origin.x > viewport.origin.x ||
      editor->viewport.origin.y > viewport.origin.y ||
      editor->viewport.origin.x + editor->viewport.size.width < viewport.origin.x + viewport.size.width ||
      editor->viewport.origin.y + editor->viewport.size.height < viewport.origin.y + viewport.size.height)
    {
      g_clear_pointer (&editor->render_cache, gsk_render_node_unref);

      editor->viewport = GRAPHENE_RECT_INIT (
          viewport.origin.x - viewport.size.width / 2.0,
          viewport.origin.y - viewport.size.height / 2.0,
          viewport.size.width * 2.0,
          viewport.size.height * 2.0);
      extents = GRAPHENE_RECT_INIT (
          extents.origin.x - extents.size.width / 2.0,
          extents.origin.y - extents.size.height / 2.0,
          extents.size.width * 2.0,
          extents.size.height * 2.0);
    }

  if (editor->render_cache == NULL)
    {
      g_autoptr (GtkSnapshot) regen                = NULL;
      g_autoptr (GtkSnapshot) layouts              = NULL;
      g_autoptr (GHashTable) texture_to_mask       = NULL;
      guint          n_strokes                     = 0;
      GdkRGBA        bg_rgba                       = { 0 };
      GHashTableIter iter                          = { 0 };
      g_autoptr (GskRenderNode) layouts_node       = NULL;
      g_autoptr (GskPathBuilder) keep_path_builder = NULL;
      g_autoptr (GskPath) keep_path                = NULL;
      g_autoptr (GskStroke) keep_stroke            = NULL;

      regen           = gtk_snapshot_new ();
      layouts         = gtk_snapshot_new ();
      texture_to_mask = g_hash_table_new (g_direct_hash, g_direct_equal);
      n_strokes       = g_list_model_get_n_items (G_LIST_MODEL (strokes));

      gtk_widget_get_color (GTK_WIDGET (editor), &widget_rgba);
      bg_rgba = (GdkRGBA) {
        .red   = 1.0 - widget_rgba.red,
        .green = 1.0 - widget_rgba.green,
        .blue  = 1.0 - widget_rgba.blue,
        .alpha = 0.75,
      };

      for (guint i = 0; i < n_strokes; i++)
        {
          g_autoptr (GtkCrusaderVillageItemStroke) stroke = NULL;
          g_autoptr (GtkCrusaderVillageItem) item         = NULL;
          g_autoptr (GArray) instances                    = NULL;
          int         item_tile_width                     = 0;
          int         item_tile_height                    = 0;
          gpointer    tile_hash                           = NULL;
          GdkTexture *tile_texture                        = NULL;
          gboolean    wants_layout                        = FALSE;
          g_autoptr (PangoLayout) tile_layout             = NULL;
          PangoRectangle tile_layout_rect                 = { 0 };

          stroke = g_list_model_get_item (G_LIST_MODEL (strokes), i);
          g_object_get (
              stroke,
              "item", &item,
              "instances", &instances,
              NULL);
          g_object_get (
              item,
              "tile-width", &item_tile_width,
              "tile-height", &item_tile_height,
              NULL);

          wants_layout = editor->zoom >= 3.5 ||
                         item_tile_width > 1 || item_tile_height > 1;

          tile_hash = gtk_crusader_village_item_get_tile_resource_hash (item);
          if (tile_hash != NULL)
            {
              tile_texture = g_hash_table_lookup (editor->tile_textures, tile_hash);
              if (tile_texture == NULL)
                {
                  g_autofree char *tile_resource = NULL;

                  g_object_get (
                      item,
                      "tile-resource", &tile_resource,
                      NULL);

                  tile_texture = gdk_texture_new_from_resource (tile_resource);
                  g_hash_table_replace (editor->tile_textures, tile_hash, tile_texture);
                }
            }

          for (guint j = 0; j < instances->len; j++)
            {
              GtkCrusaderVillageItemStrokeInstance instance = { 0 };
              graphene_rect_t                      rect     = { 0 };

              instance = g_array_index (instances, GtkCrusaderVillageItemStrokeInstance, j);

              rect = GRAPHENE_RECT_INIT (
                  instance.x * tile_size - 1.0,
                  instance.y * tile_size - 1.0,
                  item_tile_width * tile_size + 2.0,
                  item_tile_height * tile_size + 2.0);

              if (graphene_rect_intersection (&extents, &rect, NULL))
                {
                  if (tile_texture != NULL)
                    {
                      GtkSnapshot *mask = NULL;

                      mask = g_hash_table_lookup (texture_to_mask, tile_texture);
                      if (mask == NULL)
                        {
                          mask = gtk_snapshot_new ();
                          g_hash_table_replace (texture_to_mask, tile_texture, mask);

                          gtk_snapshot_push_mask (mask, GSK_MASK_MODE_ALPHA);
                        }

                      gtk_snapshot_append_color (
                          mask,
                          &(GdkRGBA) { 1.0, 1.0, 1.0, 1.0 },
                          &rect);
                    }
                  else
                    gtk_snapshot_append_color (
                        regen,
                        &(GdkRGBA) { 0.5, 0.6, 0.75, 1.0 },
                        &rect);

                  if (wants_layout && tile_layout == NULL)
                    {
                      const char *item_name = NULL;
                      char        buf[256]  = { 0 };
                      const char *ptr       = NULL;

                      item_name = gtk_crusader_village_item_get_name (item);

                      if (editor->zoom >= 4.5)
                        {
                          g_snprintf (buf, sizeof (buf), "%s (stroke %d)", item_name, i);
                          ptr = buf;
                        }
                      else if (item_name != NULL &&
                               editor->zoom <= 0.5 &&
                               (item_tile_width <= 5 || item_tile_height <= 5))
                        {
                          g_snprintf (buf, sizeof (buf), "%c", item_name[0]);
                          ptr = buf;
                        }
                      else
                        ptr = item_name;

                      tile_layout = gtk_widget_create_pango_layout (GTK_WIDGET (editor), ptr);
                      pango_layout_set_single_paragraph_mode (tile_layout, TRUE);
                      pango_layout_set_alignment (tile_layout, PANGO_ALIGN_CENTER);
                      pango_layout_set_width (tile_layout, pango_units_from_double ((double) item_tile_width * tile_size));

                      pango_layout_get_extents (tile_layout, NULL, &tile_layout_rect);
                    }

                  if (tile_layout != NULL)
                    {
                      gtk_snapshot_save (layouts);
                      gtk_snapshot_translate (
                          layouts,
                          &GRAPHENE_POINT_INIT (
                              rect.origin.x,
                              rect.origin.y + rect.size.height / 2.0 -
                                  (float) PANGO_PIXELS ((float) tile_layout_rect.height / 2.0)));

                      gtk_snapshot_append_color (
                          layouts, &bg_rgba,
                          &GRAPHENE_RECT_INIT (
                              (rect.size.width - (float) PANGO_PIXELS (tile_layout_rect.width)) / 2.0,
                              0.0,
                              (float) PANGO_PIXELS (tile_layout_rect.width),
                              (float) PANGO_PIXELS (tile_layout_rect.height)));
                      gtk_snapshot_append_layout (layouts, tile_layout, &widget_rgba);

                      gtk_snapshot_restore (layouts);
                    }
                }
            }
        }

      g_hash_table_iter_init (&iter, texture_to_mask);
      for (;;)
        {
          GdkTexture  *texture                = NULL;
          GtkSnapshot *mask                   = NULL;
          g_autoptr (GskRenderNode) mask_node = NULL;

          if (!g_hash_table_iter_next (
                  &iter, (gpointer *) &texture, (gpointer *) &mask))
            break;

          gtk_snapshot_pop (mask);

          gtk_snapshot_push_repeat (
              mask, &extents,
              &GRAPHENE_RECT_INIT (0, 0, tile_size, tile_size / 2));
          gtk_snapshot_append_texture (
              mask, texture,
              &GRAPHENE_RECT_INIT (0, 0, tile_size, tile_size / 2));
          gtk_snapshot_pop (mask);

          gtk_snapshot_pop (mask);

          mask_node = gtk_snapshot_free_to_node (mask);
          if (mask_node != NULL)
            gtk_snapshot_append_node (regen, mask_node);
        }

      keep_path_builder = gsk_path_builder_new ();
      /* Stone Keep outline */
      gsk_path_builder_move_to (keep_path_builder, map_width / 2.0 - 7.0 * tile_size, map_height / 2.0 - 7.0 * tile_size);
      gsk_path_builder_rel_line_to (keep_path_builder, 7.0 * tile_size, 0.0);
      gsk_path_builder_rel_line_to (keep_path_builder, 0.0, 2.0 * tile_size);
      gsk_path_builder_rel_line_to (keep_path_builder, 5.0 * tile_size, 0.0);
      gsk_path_builder_rel_line_to (keep_path_builder, 0.0, 5.0 * tile_size);
      gsk_path_builder_rel_line_to (keep_path_builder, -7.0 * tile_size, 0.0);
      gsk_path_builder_rel_line_to (keep_path_builder, 0.0, 1.0 * tile_size);
      gsk_path_builder_rel_line_to (keep_path_builder, 2.0 * tile_size, 0.0);
      gsk_path_builder_rel_line_to (keep_path_builder, 0.0, 7.0 * tile_size);
      gsk_path_builder_rel_line_to (keep_path_builder, -7.0 * tile_size, 0.0);
      gsk_path_builder_rel_line_to (keep_path_builder, 0.0, -7.0 * tile_size);
      gsk_path_builder_rel_line_to (keep_path_builder, 2.0 * tile_size, 0.0);
      gsk_path_builder_rel_line_to (keep_path_builder, 0.0, -1.0 * tile_size);
      gsk_path_builder_rel_line_to (keep_path_builder, -2.0 * tile_size, 0.0);
      gsk_path_builder_close (keep_path_builder);
      gsk_path_builder_add_circle (
          keep_path_builder,
          &GRAPHENE_POINT_INIT (map_width / 2.0 - 3.5 * tile_size, map_height / 2.0 + 4.5 * tile_size),
          1.5 * tile_size);
      keep_path = gsk_path_builder_free_to_path (g_steal_pointer (&keep_path_builder));

      keep_stroke = gsk_stroke_new (tile_size * 0.25);
      gsk_stroke_set_dash (keep_stroke, (const float[]) { tile_size * 0.25, tile_size * 0.5 }, 2);
      gsk_stroke_set_line_cap (keep_stroke, GSK_LINE_CAP_SQUARE);

      gtk_snapshot_append_stroke (
          regen, keep_path, keep_stroke,
          editor->dark_theme
              ? &(const GdkRGBA) { 1.0, 1.0, 1.0, 1.0 }
              : &(const GdkRGBA) { 0.0, 0.0, 0.0, 1.0 });

      layouts_node = gtk_snapshot_to_node (layouts);
      if (layouts_node != NULL)
        gtk_snapshot_append_node (regen, layouts_node);

      editor->render_cache = gtk_snapshot_to_node (regen);
    }

  if (editor->render_cache != NULL)
    gtk_snapshot_append_node (snapshot, editor->render_cache);

  if (!gtk_gesture_is_recognized (editor->drag_gesture) &&
      !gtk_gesture_is_recognized (editor->zoom_gesture) &&
      !gtk_gesture_is_active (editor->cancel_gesture) &&
      (editor->current_stroke != NULL ||
       (editor->hover_x >= 0 && editor->hover_y >= 0)))
    {
      g_autoptr (GtkCrusaderVillageItem) current_item = NULL;
      int item_tile_width                             = 0;
      int item_tile_height                            = 0;

      if (editor->current_stroke != NULL)
        g_object_get (
            editor->current_stroke,
            "item", &current_item,
            NULL);
      else
        g_object_get (
            editor->item_area,
            "selected-item", &current_item,
            NULL);

      if (current_item != NULL)
        g_object_get (
            current_item,
            "tile-width", &item_tile_width,
            "tile-height", &item_tile_height,
            NULL);

      if (editor->current_stroke != NULL)
        {
          g_autoptr (GArray) instances = NULL;

          g_assert (current_item != NULL);

          g_object_get (
              editor->brush_stroke != NULL
                  ? editor->brush_stroke
                  : editor->current_stroke,
              "instances", &instances,
              NULL);

          for (guint i = 0; i < instances->len; i++)
            {
              GtkCrusaderVillageItemStrokeInstance instance = { 0 };

              instance = g_array_index (instances, GtkCrusaderVillageItemStrokeInstance, i);
              gtk_snapshot_append_color (
                  snapshot,
                  &(GdkRGBA) { 0.2, 0.37, 0.9, 0.5 },
                  &GRAPHENE_RECT_INIT (
                      instance.x * tile_size,
                      instance.y * tile_size,
                      item_tile_width * tile_size,
                      item_tile_height * tile_size));
            }
        }

      if (current_item != NULL &&
          editor->hover_x >= 0 && editor->hover_y >= 0)
        {
          if (item_tile_width == 1 &&
              item_tile_height == 1 &&
              editor->brush_node != NULL)
            {
              double top_left_x = 0.0;
              double top_left_y = 0.0;

              top_left_x = (editor->hover_x - (double) (int) (editor->brush_width / 2)) * tile_size;
              top_left_y = (editor->hover_y - (double) (int) (editor->brush_height / 2)) * tile_size;

              gtk_snapshot_save (snapshot);
              gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (top_left_x, top_left_y));
              gtk_snapshot_scale (snapshot, tile_size, tile_size);
              gtk_snapshot_append_node (snapshot, editor->brush_node);
              gtk_snapshot_restore (snapshot);
            }
          else
            {
              double top_left_x  = 0.0;
              double top_left_y  = 0.0;
              double rect_width  = 0;
              double rect_height = 0;

              top_left_x  = (double) editor->hover_x * tile_size;
              top_left_y  = (double) editor->hover_y * tile_size;
              rect_width  = (double) MIN (item_tile_width, map_tile_width - editor->hover_x) * tile_size;
              rect_height = (double) MIN (item_tile_height, map_tile_height - editor->hover_y) * tile_size;

              if (editor->show_cursor_glow)
                {
                  double center_x        = 0.0;
                  double center_y        = 0.0;
                  double glow_top_left_x = 0.0;
                  double glow_top_left_y = 0.0;
                  double glow_width      = 0.0;
                  double glow_height     = 0.0;

                  center_x        = top_left_x + rect_width / 2.0;
                  center_y        = top_left_y + rect_height / 2.0;
                  glow_top_left_x = MIN (top_left_x, center_x - BASE_TILE_SIZE * 4.0);
                  glow_top_left_y = MIN (top_left_y, center_y - BASE_TILE_SIZE * 4.0);
                  glow_width      = MAX (rect_width, (center_x + BASE_TILE_SIZE * 4.0) - glow_top_left_x);
                  glow_height     = MAX (rect_height, (center_y + BASE_TILE_SIZE * 4.0) - glow_top_left_y);

                  gtk_snapshot_append_radial_gradient (
                      snapshot,
                      &GRAPHENE_RECT_INIT (glow_top_left_x, glow_top_left_y, glow_width, glow_height),
                      &GRAPHENE_POINT_INIT (center_x, center_y),
                      glow_width / 2.0, glow_height / 2.0, 0.0, 1.0,
                      cursor_radial_gradient_color_stops,
                      G_N_ELEMENTS (cursor_radial_gradient_color_stops));
                }

              gtk_snapshot_append_border (
                  snapshot,
                  &GSK_ROUNDED_RECT_INIT (top_left_x, top_left_y, rect_width, rect_height),
                  BORDER_WIDTH (BASE_TILE_SIZE / 8.0 * editor->zoom),
                  editor->dark_theme
                      ? BORDER_COLOR_LITERAL ({ 1.0, 1.0, 1.0, 1.0 })
                      : BORDER_COLOR_LITERAL ({ 0.0, 0.0, 0.0, 1.0 }));
            }
        }
    }

  gtk_snapshot_pop (snapshot);
}

static gboolean
scrollable_get_border (GtkScrollable *scrollable,
                       GtkBorder     *border)
{
  return FALSE;
}

static void
scrollable_iface_init (GtkScrollableInterface *iface)
{
  iface->get_border = scrollable_get_border;
}

static void
back_page_cb (GtkWidget  *widget,
              const char *action_name,
              GVariant   *parameter)
{
  GtkCrusaderVillageMapEditor *self   = GTK_CRUSADER_VILLAGE_MAP_EDITOR (widget);
  guint                        cursor = 0;

  if (self->handle == NULL)
    return;

  g_object_get (
      self->handle,
      "cursor", &cursor,
      NULL);
  if (cursor == 0)
    return;

  g_object_set (
      self->handle,
      "cursor", cursor - MIN (cursor, 5),
      NULL);
}

static void
back_one_cb (GtkWidget  *widget,
             const char *action_name,
             GVariant   *parameter)
{
  GtkCrusaderVillageMapEditor *self   = GTK_CRUSADER_VILLAGE_MAP_EDITOR (widget);
  guint                        cursor = 0;

  if (self->handle == NULL)
    return;

  g_object_get (
      self->handle,
      "cursor", &cursor,
      NULL);
  if (cursor == 0)
    return;

  g_object_set (
      self->handle,
      "cursor", cursor - MIN (cursor, 1),
      NULL);
}

static void
forward_one_cb (GtkWidget  *widget,
                const char *action_name,
                GVariant   *parameter)
{
  GtkCrusaderVillageMapEditor *self   = GTK_CRUSADER_VILLAGE_MAP_EDITOR (widget);
  guint                        cursor = 0;

  if (self->handle == NULL)
    return;

  g_object_get (
      self->handle,
      "cursor", &cursor,
      NULL);
  g_object_set (
      self->handle,
      "cursor", cursor + 1,
      NULL);
}

static void
forward_page_cb (GtkWidget  *widget,
                 const char *action_name,
                 GVariant   *parameter)
{
  GtkCrusaderVillageMapEditor *self   = GTK_CRUSADER_VILLAGE_MAP_EDITOR (widget);
  guint                        cursor = 0;

  if (self->handle == NULL)
    return;

  g_object_get (
      self->handle,
      "cursor", &cursor,
      NULL);
  g_object_set (
      self->handle,
      "cursor", cursor + 5,
      NULL);
}

static void
beginning_cb (GtkWidget  *widget,
              const char *action_name,
              GVariant   *parameter)
{
  GtkCrusaderVillageMapEditor *self = GTK_CRUSADER_VILLAGE_MAP_EDITOR (widget);

  if (self->handle == NULL)
    return;

  g_object_set (
      self->handle,
      "cursor", 0,
      NULL);
}

static void
end_cb (GtkWidget  *widget,
        const char *action_name,
        GVariant   *parameter)
{
  GtkCrusaderVillageMapEditor *self = GTK_CRUSADER_VILLAGE_MAP_EDITOR (widget);

  if (self->handle == NULL)
    return;

  g_object_set (
      self->handle,
      "cursor", G_MAXUINT,
      NULL);
}

static void
jump_to_search_cb (GtkWidget  *widget,
                   const char *action_name,
                   GVariant   *parameter)
{
  GtkCrusaderVillageMapEditor *self = GTK_CRUSADER_VILLAGE_MAP_EDITOR (widget);

  if (self->item_area == NULL)
    return;

  gtk_crusader_village_item_area_grab_search_focus (self->item_area);
}

static void
dark_theme_changed (GtkSettings                 *settings,
                    GParamSpec                  *pspec,
                    GtkCrusaderVillageMapEditor *editor)
{
  g_object_get (
      settings,
      "gtk-application-prefer-dark-theme", &editor->dark_theme,
      NULL);

  g_clear_pointer (&editor->render_cache, gsk_render_node_unref);
  read_brush (editor, FALSE);

  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
show_grid_changed (GSettings                   *self,
                   gchar                       *key,
                   GtkCrusaderVillageMapEditor *editor)
{
  editor->show_grid = g_settings_get_boolean (self, key);
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
show_gradient_changed (GSettings                   *self,
                       gchar                       *key,
                       GtkCrusaderVillageMapEditor *editor)
{
  editor->show_gradient = g_settings_get_boolean (self, key);
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
show_cursor_glow_changed (GSettings                   *self,
                          gchar                       *key,
                          GtkCrusaderVillageMapEditor *editor)
{
  editor->show_cursor_glow = g_settings_get_boolean (self, key);
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
background_image_changed (GSettings                   *self,
                          gchar                       *key,
                          GtkCrusaderVillageMapEditor *editor)
{
  read_background_image (editor);
}

static void
adjustment_value_changed (GtkAdjustment               *adjustment,
                          GtkCrusaderVillageMapEditor *editor)
{
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
drag_gesture_begin_gesture (GtkGesture                  *self,
                            GdkEventSequence            *sequence,
                            GtkCrusaderVillageMapEditor *editor)
{
  gtk_widget_grab_focus (GTK_WIDGET (editor));

  if (editor->hadjustment == NULL || editor->vadjustment == NULL)
    gtk_gesture_set_state (GTK_GESTURE (self), GTK_EVENT_SEQUENCE_DENIED);
}

static void
drag_gesture_begin (GtkGestureDrag              *self,
                    double                       start_x,
                    double                       start_y,
                    GtkCrusaderVillageMapEditor *editor)
{
  editor->drag_start_hadjustment_val = gtk_adjustment_get_value (editor->hadjustment);
  editor->drag_start_vadjustment_val = gtk_adjustment_get_value (editor->vadjustment);

  gtk_widget_set_cursor_from_name (GTK_WIDGET (editor), "move");
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
drag_gesture_update (GtkGestureDrag              *gesture,
                     double                       offset_x,
                     double                       offset_y,
                     GtkCrusaderVillageMapEditor *editor)
{
  double start_x = 0.0;
  double start_y = 0.0;
  double dx      = 0.0;
  double dy      = 0.0;

  gtk_gesture_drag_get_start_point (gesture, &start_x, &start_y);
  gtk_gesture_drag_get_offset (gesture, &dx, &dy);

  gtk_adjustment_set_value (editor->hadjustment, editor->drag_start_hadjustment_val - dx);
  gtk_adjustment_set_value (editor->vadjustment, editor->drag_start_vadjustment_val - dy);
}

static void
drag_gesture_end (GtkGestureDrag              *gesture,
                  double                       offset_x,
                  double                       offset_y,
                  GtkCrusaderVillageMapEditor *editor)
{
  update_motion (editor, editor->pointer_x, editor->pointer_y);
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
drag_gesture_end_gesture (GtkGesture                  *self,
                          GdkEventSequence            *sequence,
                          GtkCrusaderVillageMapEditor *editor)
{
  gtk_widget_set_cursor_from_name (GTK_WIDGET (editor), "crosshair");
}

static void
draw_gesture_begin_gesture (GtkGesture                  *self,
                            GdkEventSequence            *sequence,
                            GtkCrusaderVillageMapEditor *editor)
{
  gtk_widget_grab_focus (GTK_WIDGET (editor));

  if (editor->item_area == NULL)
    gtk_gesture_set_state (GTK_GESTURE (self), GTK_EVENT_SEQUENCE_DENIED);
}

static void
draw_gesture_begin (GtkGestureDrag              *self,
                    double                       start_x,
                    double                       start_y,
                    GtkCrusaderVillageMapEditor *editor)
{
  g_autoptr (GtkCrusaderVillageItem) selected_item = NULL;

  g_object_get (
      editor->item_area,
      "selected-item", &selected_item,
      NULL);
  if (selected_item == NULL)
    {
      gtk_gesture_set_state (GTK_GESTURE (self), GTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  gtk_widget_set_cursor_from_name (GTK_WIDGET (editor), "crosshair");

  g_clear_object (&editor->current_stroke);
  editor->current_stroke = g_object_new (
      GTK_CRUSADER_VILLAGE_TYPE_ITEM_STROKE,
      "item", selected_item,
      NULL);
  g_clear_object (&editor->brush_stroke);

  draw_gesture_update (self, 0.0, 0.0, editor);

  g_object_set (
      editor->handle,
      "lock-hinted", TRUE,
      NULL);

  g_object_notify_by_pspec (G_OBJECT (editor), props[PROP_DRAWING]);
}

static void
draw_gesture_update (GtkGestureDrag              *gesture,
                     double                       offset_x,
                     double                       offset_y,
                     GtkCrusaderVillageMapEditor *editor)
{
  g_autoptr (GHashTable) grid                           = NULL;
  int map_tile_width                                    = 0;
  int map_tile_height                                   = 0;
  g_autoptr (GtkCrusaderVillageItem) item               = NULL;
  g_autoptr (GArray) instances                          = NULL;
  g_autoptr (GArray) brush_instances                    = NULL;
  int                                  item_tile_width  = 0;
  int                                  item_tile_height = 0;
  GtkCrusaderVillageItemStrokeInstance last_instance    = { 0 };
  int                                  dx               = 0;
  int                                  dy               = 0;
  int                                  divisor          = 0;

  if (editor->hover_x < 0 || editor->hover_y < 0)
    return;
  g_assert (editor->current_stroke != NULL);

  g_object_get (
      editor->handle,
      "grid", &grid,
      NULL);
  g_object_get (
      editor->map,
      "width", &map_tile_width,
      "height", &map_tile_height,
      NULL);
  g_object_get (
      editor->current_stroke,
      "item", &item,
      "instances", &instances,
      NULL);
  g_object_get (
      item,
      "tile-width", &item_tile_width,
      "tile-height", &item_tile_height,
      NULL);

  if (instances->len == 0)
    last_instance = (GtkCrusaderVillageItemStrokeInstance) {
      .x = editor->hover_x,
      .y = editor->hover_y
    };
  else if (editor->line_mode)
    last_instance = g_array_index (
        instances, GtkCrusaderVillageItemStrokeInstance, 0);
  else
    last_instance = g_array_index (
        instances, GtkCrusaderVillageItemStrokeInstance, instances->len - 1);

  if (editor->brush_stroke == NULL &&
      item_tile_height == 1 &&
      item_tile_width == 1 &&
      editor->brush_mask != NULL)
    editor->brush_stroke = g_object_new (
        GTK_CRUSADER_VILLAGE_TYPE_ITEM_STROKE,
        "item", item,
        NULL);

  if (editor->brush_stroke != NULL)
    g_object_get (
        editor->brush_stroke,
        "instances", &brush_instances,
        NULL);

  if (editor->line_mode)
    {
      g_array_set_size (instances, 0);
      if (brush_instances != NULL)
        g_array_set_size (brush_instances, 0);
    }

  dx = editor->hover_x - last_instance.x;
  dy = editor->hover_y - last_instance.y;
  dx += CLAMP (dx, -1, 1);
  dy += CLAMP (dy, -1, 1);
  divisor = MAX (MAX (ABS (dx), ABS (dy)), 1);

  for (int i = 0; i < divisor; i++)
    {
      int      cx  = 0;
      int      cy  = 0;
      gboolean add = TRUE;

      cx = last_instance.x + i * dx / divisor;
      cy = last_instance.y + i * dy / divisor;

      if (cx + item_tile_width > map_tile_width ||
          cy + item_tile_height > map_tile_height)
        continue;

      for (int y = 0; y < item_tile_height; y++)
        {
          for (int x = 0; x < item_tile_width; x++)
            {
              guint tile_idx = 0;

              tile_idx = (cy + y) * map_tile_width + (cx + x);
              if (g_hash_table_contains (grid, GUINT_TO_POINTER (tile_idx)))
                {
                  /* can't place that there lord! */
                  add = FALSE;
                  break;
                }
            }

          if (!add)
            break;
        }

      if (add)
        {
          gtk_crusader_village_item_stroke_add_instance (
              editor->current_stroke,
              (GtkCrusaderVillageItemStrokeInstance) {
                  .x = cx,
                  .y = cy,
              });

          if (editor->brush_stroke != NULL)
            {
              int bx = 0;
              int by = 0;

              bx = cx - editor->brush_width / 2;
              by = cy - editor->brush_height / 2;

              for (int y = 0; y < editor->brush_height; y++)
                {
                  for (int x = 0; x < editor->brush_width; x++)
                    {
                      if (bx + x < 0 ||
                          by + y < 0 ||
                          bx + x >= map_tile_width ||
                          by + y >= map_tile_height)
                        continue;

                      if (editor->brush_mask[y * editor->brush_width + x] != 0)
                        {
                          guint tile_idx = 0;

                          tile_idx = (by + y) * map_tile_width + (bx + x);
                          if (!g_hash_table_contains (grid, GUINT_TO_POINTER (tile_idx)))
                            gtk_crusader_village_item_stroke_add_instance (
                                editor->brush_stroke,
                                (GtkCrusaderVillageItemStrokeInstance) {
                                    .x = bx + x,
                                    .y = by + y,
                                });
                        }
                    }
                }
            }
        }
    }
}

static void
draw_gesture_end (GtkGestureDrag              *gesture,
                  double                       offset_x,
                  double                       offset_y,
                  GtkCrusaderVillageMapEditor *editor)
{
  g_autoptr (GtkCrusaderVillageItem) item = NULL;
  g_autoptr (GArray) instances            = NULL;

  if (editor->current_stroke == NULL)
    return;

  g_object_get (
      editor->current_stroke,
      "item", &item,
      "instances", &instances,
      NULL);

  if (instances->len > 0)
    {
      g_autoptr (GListStore) strokes = NULL;

      g_object_get (
          editor->map,
          "strokes", &strokes,
          NULL);
      g_list_store_append (
          strokes,
          editor->brush_stroke != NULL
              ? editor->brush_stroke
              : editor->current_stroke);

      if (editor->settings != NULL)
        {
          g_autofree char *name                 = NULL;
          g_autoptr (GVariant) frequencies      = NULL;
          g_autoptr (GVariantDict) variant_dict = NULL;
          guint count                           = 0;
          g_autoptr (GVariant) reinitialized    = NULL;

          g_object_get (
              item,
              "name", &name,
              NULL);

          /* Update the recency list */
          frequencies  = g_settings_get_value (editor->settings, "item-frequencies");
          variant_dict = g_variant_dict_new (frequencies);

          if (g_variant_dict_contains (variant_dict, name))
            g_variant_dict_lookup (variant_dict, name, "u", &count);
          g_variant_dict_insert (variant_dict, name, "u", count + 1);

          reinitialized = g_variant_ref_sink (g_variant_dict_end (variant_dict));
          g_settings_set_value (editor->settings, "item-frequencies", reinitialized);
        }
    }

  g_clear_object (&editor->current_stroke);
  g_clear_object (&editor->brush_stroke);

  g_object_notify_by_pspec (G_OBJECT (editor), props[PROP_DRAWING]);
}

static void
draw_gesture_end_gesture (GtkGesture                  *self,
                          GdkEventSequence            *sequence,
                          GtkCrusaderVillageMapEditor *editor)
{
  g_object_set (
      editor->handle,
      "lock-hinted", FALSE,
      NULL);

  if (editor->current_stroke == NULL)
    g_object_notify_by_pspec (G_OBJECT (editor), props[PROP_DRAWING]);

  gtk_widget_set_cursor_from_name (GTK_WIDGET (editor), "crosshair");
}

static void
cancel_gesture_begin_gesture (GtkGesture                  *self,
                              GdkEventSequence            *sequence,
                              GtkCrusaderVillageMapEditor *editor)
{
  g_clear_object (&editor->current_stroke);

  gtk_gesture_set_state (GTK_GESTURE (self), GTK_EVENT_SEQUENCE_CLAIMED);
  gtk_gesture_set_state (editor->draw_gesture, GTK_EVENT_SEQUENCE_DENIED);

  gtk_widget_set_cursor_from_name (GTK_WIDGET (editor), "not-allowed");
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
cancel_gesture_end_gesture (GtkGesture                  *self,
                            GdkEventSequence            *sequence,
                            GtkCrusaderVillageMapEditor *editor)
{
  gtk_widget_set_cursor_from_name (GTK_WIDGET (editor), "crosshair");
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
zoom_gesture_begin (GtkGesture                  *self,
                    GdkEventSequence            *sequence,
                    GtkCrusaderVillageMapEditor *editor)
{
  gtk_gesture_set_state (editor->draw_gesture, GTK_EVENT_SEQUENCE_DENIED);

  editor->zoom_gesture_start_val = editor->zoom;

  gtk_widget_set_cursor_from_name (GTK_WIDGET (editor), NULL);
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
zoom_gesture_scale_changed (GtkGestureZoom              *self,
                            double                       scale,
                            GtkCrusaderVillageMapEditor *editor)
{
  double scale_delta = 0.0;

  scale_delta  = gtk_gesture_zoom_get_scale_delta (self);
  editor->zoom = CLAMP (editor->zoom_gesture_start_val + scale_delta * editor->zoom, 0.5, 7.5);

  g_clear_pointer (&editor->render_cache, gsk_render_node_unref);
  update_motion (editor, editor->pointer_x, editor->pointer_y);
}

static void
zoom_gesture_end (GtkGesture                  *self,
                  GdkEventSequence            *sequence,
                  GtkCrusaderVillageMapEditor *editor)
{
  gtk_widget_set_cursor_from_name (GTK_WIDGET (editor), "crosshair");
}

static gboolean
scroll_event (GtkEventControllerScroll    *self,
              double                       dx,
              double                       dy,
              GtkCrusaderVillageMapEditor *editor)
{
  if (gtk_gesture_is_recognized (editor->drag_gesture) ||
      gtk_gesture_is_recognized (editor->draw_gesture) ||
      gtk_gesture_is_recognized (editor->zoom_gesture))
    return GDK_EVENT_STOP;

  gtk_widget_grab_focus (GTK_WIDGET (editor));

  editor->zoom += dy * -0.06 * editor->zoom;
  editor->zoom = CLAMP (editor->zoom, 0.5, 7.5);

  g_clear_pointer (&editor->render_cache, gsk_render_node_unref);

  update_scrollable (editor, FALSE);
  /* Sometimes two scroll events happend before motion
   * is invoked for some reason, update now to be safe.
   */
  update_motion (editor, editor->pointer_x, editor->pointer_y);

  return GDK_EVENT_STOP;
}

static void
motion_enter (GtkEventControllerMotion    *self,
              gdouble                      x,
              gdouble                      y,
              GtkCrusaderVillageMapEditor *editor)
{
  gtk_widget_grab_focus (GTK_WIDGET (editor));
  update_motion (editor, x, y);
}

static void
motion_event (GtkEventControllerMotion    *self,
              double                       x,
              double                       y,
              GtkCrusaderVillageMapEditor *editor)
{
  update_motion (editor, x, y);
}

static void
motion_leave (GtkEventControllerMotion    *self,
              GtkCrusaderVillageMapEditor *editor)
{
  gboolean redraw = FALSE;

  redraw = editor->hover_x >= 0 || editor->hover_y >= 0;

  if (editor->hover_x >= 0)
    {
      editor->hover_x = -1;
      g_object_notify_by_pspec (G_OBJECT (editor), props[PROP_HOVER_X]);
    }
  if (editor->hover_y >= 0)
    {
      editor->hover_y = -1;
      g_object_notify_by_pspec (G_OBJECT (editor), props[PROP_HOVER_Y]);
    }

  editor->pointer_x = (double) gtk_widget_get_width (GTK_WIDGET (editor)) / 2.0;
  editor->pointer_y = (double) gtk_widget_get_height (GTK_WIDGET (editor)) / 2.0;

  if (editor->hadjustment != NULL && editor->vadjustment != NULL)
    {
      editor->canvas_x = gtk_adjustment_get_value (editor->hadjustment) + editor->pointer_x;
      editor->canvas_y = gtk_adjustment_get_value (editor->vadjustment) + editor->pointer_y;
    }
  else
    {
      editor->canvas_x = editor->pointer_x;
      editor->canvas_y = editor->pointer_y;
    }

  if (redraw)
    gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
selected_item_changed (GtkCrusaderVillageItemArea  *item_area,
                       GParamSpec                  *pspec,
                       GtkCrusaderVillageMapEditor *editor)
{
  if (editor->hover_x >= 0 && editor->hover_y >= 0)
    gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
selected_brush_changed (GtkCrusaderVillageBrushArea *brush_area,
                        GParamSpec                  *pspec,
                        GtkCrusaderVillageMapEditor *editor)
{
  read_brush (editor, TRUE);
}

static void
selected_brush_adjustment_value_changed (GtkAdjustment               *adjustment,
                                         GtkCrusaderVillageMapEditor *editor)
{
  read_brush (editor, FALSE);
}

static void
grid_changed (GtkCrusaderVillageMapHandle *handle,
              GParamSpec                  *pspec,
              GtkCrusaderVillageMapEditor *editor)
{
  g_autoptr (GtkCrusaderVillageMap) map = NULL;

  g_object_get (
      handle,
      "map", &map,
      NULL);
  if (map != editor->map)
    {
      g_clear_object (&editor->map);
      editor->map = g_steal_pointer (&map);
    }

  g_clear_pointer (&editor->render_cache, gsk_render_node_unref);
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
cursor_changed (GtkCrusaderVillageMapHandle *handle,
                GParamSpec                  *pspec,
                GtkCrusaderVillageMapEditor *editor)
{
  g_clear_pointer (&editor->render_cache, gsk_render_node_unref);
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
update_scrollable (GtkCrusaderVillageMapEditor *self,
                   gboolean                     center)
{
  if (self->hadjustment == NULL || self->vadjustment == NULL)
    return;

  if (self->map != NULL)
    {
      double   old_h_lower     = 0.0;
      double   old_v_lower     = 0.0;
      double   old_h_upper     = 0.0;
      double   old_v_upper     = 0.0;
      int      widget_width    = 0;
      int      widget_height   = 0;
      int      map_tile_width  = 0;
      int      map_tile_height = 0;
      double   h_lower         = 0.0;
      double   v_lower         = 0.0;
      double   h_upper         = 0.0;
      double   v_upper         = 0.0;
      gboolean force_center_h  = FALSE;
      gboolean force_center_v  = FALSE;

      g_object_get (
          self->hadjustment,
          "lower", &old_h_lower,
          "upper", &old_h_upper,
          NULL);
      g_object_get (
          self->vadjustment,
          "lower", &old_v_lower,
          "upper", &old_v_upper,
          NULL);

      widget_width  = gtk_widget_get_width (GTK_WIDGET (self));
      widget_height = gtk_widget_get_height (GTK_WIDGET (self));

      g_object_get (
          self->map,
          "width", &map_tile_width,
          "height", &map_tile_height,
          NULL);

      h_upper = (double) (map_tile_width + self->border_gap * 2) * BASE_TILE_SIZE * self->zoom;
      v_upper = (double) (map_tile_height + self->border_gap * 2) * BASE_TILE_SIZE * self->zoom;

      if (h_upper < (double) widget_width)
        {
          h_lower        = -((double) widget_width - h_upper) / 2.0;
          h_upper        = h_lower + (double) widget_width;
          force_center_h = TRUE;
        }

      if (v_upper < (double) widget_height)
        {
          v_lower        = -((double) widget_height - v_upper) / 2.0;
          v_upper        = v_lower + (double) widget_height;
          force_center_v = TRUE;
        }

      g_object_set (
          self->hadjustment,
          "page-size", (double) widget_width,
          "lower", h_lower,
          "upper", h_upper,
          NULL);
      g_object_set (
          self->vadjustment,
          "page-size", (double) widget_height,
          "lower", v_lower,
          "upper", v_upper,
          NULL);

      if (center || self->queue_center || force_center_h)
        gtk_adjustment_set_value (
            self->hadjustment,
            force_center_h
                ? h_lower
                : (h_lower + h_upper - (double) widget_width) / 2.0);
      else
        {
          double ratio_x    = 0.0;
          double adjusted_x = 0.0;

          ratio_x    = (h_upper - h_lower) / (old_h_upper - old_h_lower);
          adjusted_x = self->canvas_x * ratio_x - self->pointer_x;
          gtk_adjustment_set_value (self->hadjustment, adjusted_x);
        }

      if (center || self->queue_center || force_center_v)
        gtk_adjustment_set_value (
            self->vadjustment,
            force_center_v
                ? v_lower
                : (v_lower + v_upper - (double) widget_height) / 2.0);
      else
        {
          double ratio_y    = 0.0;
          double adjusted_y = 0.0;

          ratio_y    = (v_upper - v_lower) / (old_v_upper - old_v_lower);
          adjusted_y = self->canvas_y * ratio_y - self->pointer_y;
          gtk_adjustment_set_value (self->vadjustment, adjusted_y);
        }

      self->queue_center = FALSE;
    }
  else
    {
      g_object_set (
          self->hadjustment,
          "lower", 0.0,
          "upper", 0.0,
          NULL);
      g_object_set (
          self->vadjustment,
          "lower", 0.0,
          "upper", 0.0,
          NULL);
    }
}

static void
update_motion (GtkCrusaderVillageMapEditor *self,
               double                       x,
               double                       y)
{
  double   hscroll         = 0.0;
  double   vscroll         = 0.0;
  double   map_offset      = 0.0;
  int      new_hover_x     = 0;
  int      new_hover_y     = 0;
  int      map_tile_width  = 0;
  int      map_tile_height = 0;
  gboolean was_invalid     = FALSE;
  gboolean is_invalid      = FALSE;
  gboolean x_changed       = FALSE;
  gboolean y_changed       = FALSE;
  gboolean redraw          = FALSE;

  self->pointer_x = x;
  self->pointer_y = y;

  if (gtk_gesture_is_recognized (self->drag_gesture))
    return;

  hscroll        = gtk_adjustment_get_value (self->hadjustment);
  vscroll        = gtk_adjustment_get_value (self->vadjustment);
  self->canvas_x = x + hscroll;
  self->canvas_y = y + vscroll;
  map_offset     = (double) self->border_gap * BASE_TILE_SIZE * self->zoom;
  new_hover_x    = floor ((self->canvas_x - map_offset) / (BASE_TILE_SIZE * self->zoom));
  new_hover_y    = floor ((self->canvas_y - map_offset) / (BASE_TILE_SIZE * self->zoom));

  g_object_get (
      self->map,
      "width", &map_tile_width,
      "height", &map_tile_height,
      NULL);

  was_invalid = self->hover_x < 0 || self->hover_y < 0;
  is_invalid  = new_hover_x < 0 || new_hover_y < 0 ||
               new_hover_x >= map_tile_width || new_hover_y >= map_tile_height;

  if (is_invalid)
    new_hover_x = new_hover_y = -1;

  x_changed = new_hover_x != self->hover_x;
  y_changed = new_hover_y != self->hover_y;
  redraw    = (x_changed || y_changed) && (!was_invalid || !is_invalid);

  self->hover_x = new_hover_x;
  self->hover_y = new_hover_y;

  if (x_changed)
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HOVER_X]);
  if (y_changed)
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HOVER_Y]);
  if (redraw)
    gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
read_brush (GtkCrusaderVillageMapEditor *self,
            gboolean                     different)
{
  g_autoptr (GtkCrusaderVillageBrushable) brush = NULL;
  int                mask_width                 = 0;
  int                mask_height                = 0;
  g_autofree guint8 *mask                       = NULL;
  g_autoptr (GtkSnapshot) snapshot              = NULL;
  g_autoptr (GskPathBuilder) builder            = NULL;
  g_autoptr (GskPath) path                      = NULL;
  g_autoptr (GskStroke) stroke                  = NULL;

  g_clear_pointer (&self->brush_mask, g_free);
  g_clear_pointer (&self->brush_node, gsk_render_node_unref);

  if (different)
    {
      if (self->brush_adjustment != NULL)
        g_signal_handlers_disconnect_by_func (
            self->brush_adjustment, selected_brush_adjustment_value_changed, self);
      g_clear_object (&self->brush_adjustment);
    }

  if (self->hover_x >= 0 && self->hover_y >= 0)
    gtk_widget_queue_draw (GTK_WIDGET (self));

  if (self->brush_area == NULL)
    return;

  g_object_get (
      self->brush_area,
      "selected-brush", &brush,
      NULL);
  if (brush == NULL)
    return;

  mask = gtk_crusader_village_brushable_get_mask (brush, &mask_width, &mask_height);
  g_assert (mask != NULL);

  snapshot = gtk_snapshot_new ();
  builder  = gsk_path_builder_new ();
  for (int y = 0; y < mask_height; y++)
    {
      for (int x = 0; x < mask_width; x++)
        {
          if (mask[y * mask_width + x] != 0)
            {
              if (x == 0 || mask[y * mask_width + (x - 1)] == 0)
                {
                  gsk_path_builder_move_to (builder, (float) x, (float) y);
                  gsk_path_builder_rel_line_to (builder, 0.0, 1.0);
                }
              if (y == 0 || mask[(y - 1) * mask_width + x] == 0)
                {
                  gsk_path_builder_move_to (builder, (float) x, (float) y);
                  gsk_path_builder_rel_line_to (builder, 1.0, 0.0);
                }
              if (x == mask_width - 1)
                {
                  gsk_path_builder_move_to (builder, (float) x + 1.0, (float) y + 1.0);
                  gsk_path_builder_rel_line_to (builder, 0.0, -1.0);
                }
              if (y == mask_height - 1)
                {
                  gsk_path_builder_move_to (builder, (float) x + 1.0, (float) y + 1.0);
                  gsk_path_builder_rel_line_to (builder, -1.0, 0.0);
                }
            }
          else
            {
              if (x > 0 && mask[y * mask_width + (x - 1)] != 0)
                {
                  gsk_path_builder_move_to (builder, (float) x, (float) y);
                  gsk_path_builder_rel_line_to (builder, 0.0, 1.0);
                }
              if (y > 0 && mask[(y - 1) * mask_width + x] != 0)
                {
                  gsk_path_builder_move_to (builder, (float) x, (float) y);
                  gsk_path_builder_rel_line_to (builder, 1.0, 0.0);
                }
            }
        }
    }
  path = gsk_path_builder_free_to_path (g_steal_pointer (&builder));

  stroke = gsk_stroke_new (0.15);
  gsk_stroke_set_line_cap (stroke, GSK_LINE_CAP_SQUARE);

  gtk_snapshot_append_stroke (
      snapshot, path, stroke,
      self->dark_theme
          ? &(const GdkRGBA) { 1.0, 1.0, 1.0, 1.0 }
          : &(const GdkRGBA) { 0.0, 0.0, 0.0, 1.0 });

  self->brush_mask   = g_steal_pointer (&mask);
  self->brush_width  = mask_width;
  self->brush_height = mask_height;
  self->brush_node   = gtk_snapshot_free_to_node (g_steal_pointer (&snapshot));

  if (different)
    {
      self->brush_adjustment = gtk_crusader_village_brushable_get_adjustment (brush);
      if (self->brush_adjustment != NULL)
        g_signal_connect (self->brush_adjustment, "value-changed",
                          G_CALLBACK (selected_brush_adjustment_value_changed), self);
    }
}

static void
read_background_image (GtkCrusaderVillageMapEditor *self)
{
  g_autofree char *path = NULL;

  g_clear_object (&self->bg_image_tex);
  gtk_widget_queue_draw (GTK_WIDGET (self));

  path = g_settings_get_string (self->settings, "background-image");

  if (path[0] != '\0')
    {
      g_autoptr (GTask) task = NULL;

      task = g_task_new (self, NULL, background_texture_load_finish_cb, NULL);
      g_task_set_task_data (task, g_steal_pointer (&path), g_free);
      g_task_set_priority (task, G_PRIORITY_HIGH);
      g_task_set_check_cancellable (task, TRUE);
      g_task_run_in_thread (task, background_texture_load_async_thread);
    }
}

static void
background_texture_load_async_thread (GTask        *task,
                                      gpointer      object,
                                      gpointer      task_data,
                                      GCancellable *cancellable)
{
  const char *path               = task_data;
  g_autoptr (GError) local_error = NULL;
  g_autoptr (GdkTexture) texture = NULL;

  if (g_task_return_error_if_cancelled (task))
    return;

  texture = gdk_texture_new_from_filename (path, &local_error);
  if (texture == NULL)
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      return;
    }

  g_task_return_pointer (task, g_steal_pointer (&texture), g_object_unref);
}

static void
background_texture_load_finish_cb (GObject      *source_object,
                                   GAsyncResult *res,
                                   gpointer      data)
{
  GtkCrusaderVillageMapEditor *editor = GTK_CRUSADER_VILLAGE_MAP_EDITOR (source_object);
  g_autoptr (GError) local_error      = NULL;
  GdkTexture *texture                 = NULL;

  texture = g_task_propagate_pointer (G_TASK (res), &local_error);

  if (texture == NULL)
    {
      GtkWidget      *app_window  = NULL;
      GtkApplication *application = NULL;
      GtkWindow      *window      = NULL;

      app_window = gtk_widget_get_ancestor (GTK_WIDGET (editor), GTK_TYPE_WINDOW);
      g_assert (app_window != NULL);

      application = gtk_window_get_application (GTK_WINDOW (app_window));
      window      = gtk_application_get_active_window (application);

      gtk_crusader_village_dialog (
          "Error",
          "Could not load background image ",
          local_error->message,
          window, NULL);

      if (editor->settings != NULL)
        g_settings_set_string (editor->settings, "background-image", "");
    }

  g_clear_object (&editor->bg_image_tex);
  editor->bg_image_tex = texture;
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}
