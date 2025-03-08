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

  GtkCrusaderVillageItemArea *item_area;
  int                         border_gap;

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

  GtkGesture *drag_gesture;
  double      last_drag_x;
  double      last_drag_y;

  GtkGesture                   *draw_gesture;
  GtkGesture                   *cancel_gesture;
  gboolean                      draw_is_cancelled;
  GtkCrusaderVillageItemStroke *current_stroke;
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

  PROP_BORDER_GAP,

  PROP_HOVER_X,
  PROP_HOVER_Y,
  PROP_DRAWING,

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
adjustment_value_changed (GtkAdjustment               *adjustment,
                          GtkCrusaderVillageMapEditor *text_view);

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
cancel_gesture_pressed (GtkGestureClick             *self,
                        gint                         n_press,
                        double                       x,
                        double                       y,
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

  g_clear_object (&self->item_area);
  g_clear_object (&self->hadjustment);
  g_clear_object (&self->vadjustment);
  g_clear_object (&self->current_stroke);

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

          self->show_grid        = g_settings_get_boolean (self->settings, "show-grid");
          self->show_gradient    = g_settings_get_boolean (self->settings, "show-gradient");
          self->show_cursor_glow = g_settings_get_boolean (self->settings, "show-cursor-glow");
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
      gtk_widget_queue_resize (GTK_WIDGET (self));
      break;

    case PROP_ITEM_AREA:
      if (self->item_area != NULL)
        g_signal_handlers_disconnect_by_func (
            self->item_area, selected_item_changed, self);
      g_clear_object (&self->item_area);

      self->item_area = g_value_dup_object (value);
      if (self->item_area)
        g_signal_connect (self->item_area, "notify::selected-item",
                          G_CALLBACK (selected_item_changed), self);
      break;

    case PROP_BORDER_GAP:
      self->border_gap = g_value_get_int (value);
      gtk_widget_queue_allocate (GTK_WIDGET (self));
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

  g_object_class_install_properties (object_class, LAST_NATIVE_PROP, props);

  g_object_class_override_property (object_class, PROP_HADJUSTMENT, "hadjustment");
  g_object_class_override_property (object_class, PROP_VADJUSTMENT, "vadjustment");
  g_object_class_override_property (object_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (object_class, PROP_VSCROLL_POLICY, "vscroll-policy");

  widget_class->measure       = gtk_crusader_village_map_editor_measure;
  widget_class->size_allocate = gtk_crusader_village_map_editor_size_allocate;
  widget_class->snapshot      = gtk_crusader_village_map_editor_snapshot;
}

static void
gtk_crusader_village_map_editor_init (GtkCrusaderVillageMapEditor *self)
{
  GtkEventController *scroll_controller = NULL;
  GtkEventController *motion_controller = NULL;

  self->border_gap = 2;

  self->zoom        = 1.0;
  self->last_drag_x = -1.0;
  self->last_drag_y = -1.0;

  self->pointer_x = -1.0;
  self->pointer_y = -1.0;
  self->hover_x   = -1;
  self->hover_y   = -1;
  self->canvas_x  = -1.0;
  self->canvas_y  = -1.0;

  self->gtk_settings = gtk_settings_get_default ();
  g_signal_connect (self->gtk_settings, "notify::gtk-application-prefer-dark-theme",
                    G_CALLBACK (dark_theme_changed), self);
  g_object_get (
      self->gtk_settings,
      "gtk-application-prefer-dark-theme", &self->dark_theme,
      NULL);

  self->drag_gesture = gtk_gesture_drag_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->drag_gesture), 2);
  g_signal_connect (self->drag_gesture, "drag-update",
                    G_CALLBACK (drag_gesture_update),
                    GTK_WIDGET (self));
  g_signal_connect (self->drag_gesture, "drag-end",
                    G_CALLBACK (drag_gesture_end),
                    GTK_WIDGET (self));
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag_gesture));

  self->draw_gesture = gtk_gesture_drag_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->draw_gesture), 1);
  g_signal_connect (self->draw_gesture, "drag-begin",
                    G_CALLBACK (draw_gesture_begin),
                    GTK_WIDGET (self));
  g_signal_connect (self->draw_gesture, "drag-update",
                    G_CALLBACK (draw_gesture_update),
                    GTK_WIDGET (self));
  g_signal_connect (self->draw_gesture, "drag-end",
                    G_CALLBACK (draw_gesture_end),
                    GTK_WIDGET (self));
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->draw_gesture));

  self->cancel_gesture = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->cancel_gesture), 3);
  g_signal_connect (self->cancel_gesture, "pressed",
                    G_CALLBACK (cancel_gesture_pressed),
                    GTK_WIDGET (self));
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->cancel_gesture));

  scroll_controller = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  g_signal_connect (scroll_controller, "scroll", G_CALLBACK (scroll_event), self);
  gtk_widget_add_controller (GTK_WIDGET (self), scroll_controller);

  motion_controller = gtk_event_controller_motion_new ();
  g_signal_connect (motion_controller, "enter", G_CALLBACK (motion_enter), self);
  g_signal_connect (motion_controller, "motion", G_CALLBACK (motion_event), self);
  g_signal_connect (motion_controller, "leave", G_CALLBACK (motion_leave), self);
  gtk_widget_add_controller (GTK_WIDGET (self), motion_controller);
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
  double                       tile_size       = 0.0;
  int                          map_tile_width  = 0;
  int                          map_tile_height = 0;
  g_autoptr (GListStore) strokes               = NULL;
  double map_width                             = 0.0;
  double map_height                            = 0.0;
  guint  n_strokes                             = 0;

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
      viewport = GRAPHENE_RECT_INIT (value_x, value_y, (float) widget_width, (float) widget_height);
    }
  else
    viewport = GRAPHENE_RECT_INIT (0, 0, (float) widget_width, (float) widget_height);

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
  viewport.origin.x -= (double) editor->border_gap * tile_size;
  viewport.origin.y -= (double) editor->border_gap * tile_size;

  if (editor->show_gradient)
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

#define BORDER_WIDTH(w)           ((float[4]) { (w), (w), (w), (w) })
#define BORDER_COLOR_LITERAL(...) ((GdkRGBA[4]) { __VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__ })

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

  n_strokes = g_list_model_get_n_items (G_LIST_MODEL (strokes));
  for (guint i = 0; i < n_strokes; i++)
    {
      g_autoptr (GtkCrusaderVillageItemStroke) stroke = NULL;
      g_autoptr (GtkCrusaderVillageItem) item         = NULL;
      g_autoptr (GArray) instances                    = NULL;
      int   item_tile_width                           = 0;
      int   item_tile_height                          = 0;
      float r                                         = 0.0;
      float g                                         = 0.0;
      float b                                         = 0.0;

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
      gtk_crusader_village_item_get_name_color (item, &r, &g, &b);

      for (guint j = 0; j < instances->len; j++)
        {
          GtkCrusaderVillageItemStrokeInstance instance = { 0 };
          graphene_rect_t                      rect     = { 0 };

          instance = g_array_index (instances, GtkCrusaderVillageItemStrokeInstance, j);
          rect     = GRAPHENE_RECT_INIT (
              instance.x * tile_size,
              instance.y * tile_size,
              item_tile_width * tile_size,
              item_tile_height * tile_size);

          if (graphene_rect_intersection (&viewport, &rect, NULL))
            gtk_snapshot_append_color (
                snapshot,
                &(GdkRGBA) { r, g, b, 1.0 },
                &rect);
        }
    }

  if (editor->current_stroke != NULL || (editor->hover_x >= 0 && editor->hover_y >= 0))
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
              editor->current_stroke,
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
dark_theme_changed (GtkSettings                 *settings,
                    GParamSpec                  *pspec,
                    GtkCrusaderVillageMapEditor *editor)
{
  g_object_get (
      settings,
      "gtk-application-prefer-dark-theme", &editor->dark_theme,
      NULL);
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
adjustment_value_changed (GtkAdjustment               *adjustment,
                          GtkCrusaderVillageMapEditor *editor)
{
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
drag_gesture_update (GtkGestureDrag              *gesture,
                     double                       offset_x,
                     double                       offset_y,
                     GtkCrusaderVillageMapEditor *editor)
{
  if (editor->hadjustment == NULL || editor->vadjustment == NULL)
    return;

  if (editor->last_drag_x != 0.0 && editor->last_drag_y != 0.0)
    {
      gtk_adjustment_set_value (
          editor->hadjustment,
          gtk_adjustment_get_value (editor->hadjustment) -
              (offset_x - editor->last_drag_x));
      gtk_adjustment_set_value (
          editor->vadjustment,
          gtk_adjustment_get_value (editor->vadjustment) -
              (offset_y - editor->last_drag_y));
    }

  editor->last_drag_x = offset_x;
  editor->last_drag_y = offset_y;
}

static void
drag_gesture_end (GtkGestureDrag              *gesture,
                  double                       offset_x,
                  double                       offset_y,
                  GtkCrusaderVillageMapEditor *editor)
{
  editor->last_drag_x = 0.0;
  editor->last_drag_y = 0.0;

  if (editor->hadjustment == NULL || editor->vadjustment == NULL)
    return;
}

static void
draw_gesture_begin (GtkGestureDrag              *self,
                    double                       start_x,
                    double                       start_y,
                    GtkCrusaderVillageMapEditor *editor)
{
  g_autoptr (GtkCrusaderVillageItem) selected_item = NULL;

  if (gtk_gesture_is_active (editor->cancel_gesture))
    return;
  if (editor->item_area == NULL)
    return;

  g_object_get (
      editor->item_area,
      "selected-item", &selected_item,
      NULL);

  if (selected_item == NULL)
    return;

  g_clear_object (&editor->current_stroke);
  editor->current_stroke = g_object_new (
      GTK_CRUSADER_VILLAGE_TYPE_ITEM_STROKE,
      "item", selected_item,
      NULL);

  draw_gesture_update (self, 0.0, 0.0, editor);

  g_object_notify_by_pspec (G_OBJECT (editor), props[PROP_DRAWING]);
}

static void
draw_gesture_update (GtkGestureDrag              *gesture,
                     double                       offset_x,
                     double                       offset_y,
                     GtkCrusaderVillageMapEditor *editor)
{
  g_autoptr (GHashTable) grid             = NULL;
  int map_tile_width                      = 0;
  int map_tile_height                     = 0;
  g_autoptr (GtkCrusaderVillageItem) item = NULL;
  int item_tile_width                     = 0;
  int item_tile_height                    = 0;

  if (editor->current_stroke == NULL)
    return;
  if (editor->hover_x < 0 || editor->hover_y < 0)
    return;

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
      NULL);
  g_object_get (
      item,
      "tile-width", &item_tile_width,
      "tile-height", &item_tile_height,
      NULL);

  if (editor->hover_x + item_tile_width > map_tile_width ||
      editor->hover_y + item_tile_height > map_tile_height)
    return;

  for (int y = 0; y < item_tile_height; y++)
    {
      for (int x = 0; x < item_tile_width; x++)
        {
          guint tile_idx = 0;

          tile_idx = (editor->hover_y + y) * map_tile_width + (editor->hover_x + x);
          if (g_hash_table_contains (grid, GUINT_TO_POINTER (tile_idx)))
            /* can't place that there lord! */
            return;
        }
    }

  gtk_crusader_village_item_stroke_add_instance (
      editor->current_stroke,
      (GtkCrusaderVillageItemStrokeInstance) {
          .x = editor->hover_x,
          .y = editor->hover_y,
      });
}

static void
draw_gesture_end (GtkGestureDrag              *gesture,
                  double                       offset_x,
                  double                       offset_y,
                  GtkCrusaderVillageMapEditor *editor)
{
  g_autoptr (GArray) instances = NULL;

  if (editor->current_stroke == NULL)
    return;

  g_object_get (
      editor->current_stroke,
      "instances", &instances,
      NULL);

  if (instances->len > 0)
    {
      g_autoptr (GListStore) strokes = NULL;

      g_object_get (
          editor->map,
          "strokes", &strokes,
          NULL);
      g_list_store_append (strokes, g_steal_pointer (&editor->current_stroke));
    }
  else
    g_clear_object (&editor->current_stroke);

  g_object_notify_by_pspec (G_OBJECT (editor), props[PROP_DRAWING]);
}

static void
cancel_gesture_pressed (GtkGestureClick             *self,
                        gint                         n_press,
                        double                       x,
                        double                       y,
                        GtkCrusaderVillageMapEditor *editor)
{
  if (editor->current_stroke == NULL)
    return;

  g_clear_object (&editor->current_stroke);
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static gboolean
scroll_event (GtkEventControllerScroll    *self,
              double                       dx,
              double                       dy,
              GtkCrusaderVillageMapEditor *editor)
{
  editor->zoom += dy * -0.06 * editor->zoom;
  editor->zoom = CLAMP (editor->zoom, 0.5, 7.5);

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

  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
cursor_changed (GtkCrusaderVillageMapHandle *handle,
                GParamSpec                  *pspec,
                GtkCrusaderVillageMapEditor *editor)
{
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

  hscroll         = gtk_adjustment_get_value (self->hadjustment);
  vscroll         = gtk_adjustment_get_value (self->vadjustment);
  self->pointer_x = x;
  self->pointer_y = y;
  self->canvas_x  = x + hscroll;
  self->canvas_y  = y + vscroll;
  map_offset      = (double) self->border_gap * BASE_TILE_SIZE * self->zoom;
  new_hover_x     = floor ((self->canvas_x - map_offset) / (BASE_TILE_SIZE * self->zoom));
  new_hover_y     = floor ((self->canvas_y - map_offset) / (BASE_TILE_SIZE * self->zoom));

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
