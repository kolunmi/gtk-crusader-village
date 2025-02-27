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
#include "gtk-crusader-village-map.h"

#define BASE_TILE_SIZE 64.0

struct _GtkCrusaderVillageMapEditor
{
  GtkWidget parent_instance;

  GtkSettings *gtk_settings;
  gboolean     dark_theme;

  GSettings *settings;
  gboolean   show_grid;
  gboolean   show_gradient;

  GtkCrusaderVillageMap      *map;
  GtkCrusaderVillageItemArea *item_area;
  int                         border_gap;

  double   zoom;
  gboolean queue_center;

  int    hover_x;
  int    hover_y;
  double hover_x_device;
  double hover_y_device;

  GtkScrollablePolicy hscroll_policy;
  GtkScrollablePolicy vscroll_policy;
  GtkAdjustment      *hadjustment;
  GtkAdjustment      *vadjustment;

  GtkGesture *drag_gesture;
  double      last_drag_x;
  double      last_drag_y;

  GtkGesture                   *draw_gesture;
  gboolean                      drawing;
  GtkGesture                   *cancel_gesture;
  gboolean                      draw_is_cancelled;
  GtkCrusaderVillageItemStroke *current_stroke;

  gboolean pending_scroll;
  double   pending_scroll_x;
  double   pending_scroll_y;
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

  PROP_MAP,
  PROP_ITEM_AREA,

  PROP_BORDER_GAP,

  PROP_HOVER_X,
  PROP_HOVER_Y,

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
grid_changed (GtkCrusaderVillageMap       *map,
              GParamSpec                  *pspec,
              GtkCrusaderVillageMapEditor *editor);

static void
update_scrollable (GtkCrusaderVillageMapEditor *self,
                   gboolean                     center);

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
    }
  g_clear_object (&self->settings);

  if (self->map != NULL)
    g_signal_handlers_disconnect_by_func (
        self->map, grid_changed, self);
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
    case PROP_MAP:
      g_value_set_object (value, self->map);
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
        }
      g_clear_object (&self->settings);

      self->settings = g_value_dup_object (value);
      if (self->settings != NULL)
        {
          g_signal_connect (self->settings, "changed::show-grid",
                            G_CALLBACK (show_grid_changed), self);
          g_signal_connect (self->settings, "changed::show-gradient",
                            G_CALLBACK (show_gradient_changed), self);

          self->show_grid     = g_settings_get_boolean (self->settings, "show-grid");
          self->show_gradient = g_settings_get_boolean (self->settings, "show-gradient");
        }
      else
        {
          self->show_grid     = TRUE;
          self->show_gradient = TRUE;
        }

      gtk_widget_queue_draw (GTK_WIDGET (self));
      break;

    case PROP_MAP:
      if (self->map != NULL)
        g_signal_handlers_disconnect_by_func (
            self->map, grid_changed, self);
      g_clear_object (&self->map);

      self->map = g_value_dup_object (value);
      if (self->map != NULL)
        g_signal_connect (self->map, "notify::grid",
                          G_CALLBACK (grid_changed), self);

      self->queue_center = TRUE;
      gtk_widget_queue_resize (GTK_WIDGET (self));
      break;

    case PROP_ITEM_AREA:
      g_clear_object (&self->item_area);
      self->item_area = g_value_dup_object (value);
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

  props[PROP_MAP] =
      g_param_spec_object (
          "map",
          "Map",
          "The map object this view is editing",
          GTK_CRUSADER_VILLAGE_TYPE_MAP,
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

  self->hover_x        = -1;
  self->hover_y        = -1;
  self->hover_x_device = -1.0;
  self->hover_y_device = -1.0;

  self->gtk_settings = gtk_settings_get_default ();
  g_signal_connect (self->gtk_settings, "notify::gtk-application-prefer-dark-theme",
                    G_CALLBACK (dark_theme_changed), self);

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

  GtkCrusaderVillageMapEditor   *editor          = GTK_CRUSADER_VILLAGE_MAP_EDITOR (widget);
  int                            widget_width    = 0;
  int                            widget_height   = 0;
  double                         tile_size       = 0.0;
  int                            map_tile_width  = 0;
  int                            map_tile_height = 0;
  const GtkCrusaderVillageItem **grid            = NULL;
  double                         map_width       = 0.0;
  double                         map_height      = 0.0;
  /* double                       canvas_width    = 0.0; */
  /* double                       canvas_height   = 0.0; */

  widget_width  = gtk_widget_get_width (widget);
  widget_height = gtk_widget_get_height (widget);

  gtk_snapshot_push_clip (
      snapshot,
      &GRAPHENE_RECT_INIT (
          0, 0, widget_width, widget_height));

  if (editor->hadjustment != NULL && editor->vadjustment != NULL)
    {
      double lower_x  = 0.0;
      double lower_y  = 0.0;
      double upper_x  = 0.0;
      double upper_y  = 0.0;
      double value_x  = 0.0;
      double value_y  = 0.0;
      double offset_x = 0.0;
      double offset_y = 0.0;

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

      offset_x = -value_x;
      offset_y = -value_y;

      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (offset_x, offset_y));
    }

  tile_size = BASE_TILE_SIZE * editor->zoom;

  g_object_get (
      editor->map,
      "width", &map_tile_width,
      "height", &map_tile_height,
      "grid", &grid,
      NULL);

  /* canvas_width  = (double) (map_tile_width + editor->border_gap * 2) * tile_size; */
  /* canvas_height = (double) (map_tile_height + editor->border_gap * 2) * tile_size; */
  map_width  = (double) map_tile_width * tile_size;
  map_height = (double) map_tile_height * tile_size;

  gtk_snapshot_translate (
      snapshot,
      &GRAPHENE_POINT_INIT (
          (double) editor->border_gap * BASE_TILE_SIZE * editor->zoom,
          (double) editor->border_gap * BASE_TILE_SIZE * editor->zoom));

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
          BORDER_WIDTH (2.0 * editor->zoom),
          BORDER_COLOR_LITERAL ({ 0.0, 0.0, 0.0, 1.0 }));

      gtk_snapshot_pop (snapshot);
    }

  gtk_snapshot_append_outset_shadow (
      snapshot,
      &GSK_ROUNDED_RECT_INIT (0, 0, map_width, map_height),
      &(GdkRGBA) { 0.0, 0.0, 0.0, 1.0 },
      0.0, 0.0, 20.0 * editor->zoom, 60.0 * editor->zoom);

  /* TODO: improve efficiency */
  for (int y = 0; y < map_tile_height; y++)
    {
      for (int x = 0; x < map_tile_width; x++)
        {
          const GtkCrusaderVillageItem *item = NULL;

          item = grid[y * map_tile_width + x];
          if (item != NULL)
            gtk_snapshot_append_color (
                snapshot,
                &(GdkRGBA) { 0.2, 0.37, 0.9, 0.3 },
                &GRAPHENE_RECT_INIT (
                    x * BASE_TILE_SIZE * editor->zoom,
                    y * BASE_TILE_SIZE * editor->zoom,
                    BASE_TILE_SIZE * editor->zoom,
                    BASE_TILE_SIZE * editor->zoom));
        }
    }

  if (editor->current_stroke != NULL)
    {
      g_autoptr (GArray) instances = NULL;

      g_object_get (
          editor->current_stroke,
          "instances", &instances,
          NULL);

      for (guint i = 0; i < instances->len; i++)
        {
          GtkCrusaderVillageItemStrokeInstance instance = { 0 };
          const GtkCrusaderVillageItem        *item     = NULL;

          instance = g_array_index (instances, GtkCrusaderVillageItemStrokeInstance, i);
          item     = grid[instance.y * map_tile_width + instance.x];

          gtk_snapshot_append_color (
              snapshot,
              item != NULL
                  ? &(GdkRGBA) { 0.75, 0.2, 0.2, 0.5 }
                  : &(GdkRGBA) { 0.2, 0.37, 0.9, 0.3 },
              &GRAPHENE_RECT_INIT (
                  instance.x * BASE_TILE_SIZE * editor->zoom,
                  instance.y * BASE_TILE_SIZE * editor->zoom,
                  BASE_TILE_SIZE * editor->zoom,
                  BASE_TILE_SIZE * editor->zoom));
        }
    }

  if (editor->hover_x >= 0 && editor->hover_y >= 0 &&
      editor->hover_x < map_tile_width && editor->hover_y < map_tile_height)
    {
      double top_left_x = 0.0;
      double top_left_y = 0.0;
      double center_x   = 0.0;
      double center_y   = 0.0;

      top_left_x = (double) editor->hover_x * BASE_TILE_SIZE * editor->zoom;
      top_left_y = (double) editor->hover_y * BASE_TILE_SIZE * editor->zoom;
      center_x   = top_left_x + (BASE_TILE_SIZE * editor->zoom) / 2.0;
      center_y   = top_left_y + (BASE_TILE_SIZE * editor->zoom) / 2.0;

      gtk_snapshot_append_radial_gradient (
          snapshot,
          &GRAPHENE_RECT_INIT (center_x - 100.0, center_y - 100.0, 200.0, 200.0),
          &GRAPHENE_POINT_INIT (center_x, center_y),
          100.0, 100.0, 0.0, 1.0,
          cursor_radial_gradient_color_stops, G_N_ELEMENTS (cursor_radial_gradient_color_stops));

      gtk_snapshot_append_border (
          snapshot,
          &GSK_ROUNDED_RECT_INIT (top_left_x, top_left_y, tile_size, tile_size),
          BORDER_WIDTH (8.0 * editor->zoom),
          BORDER_COLOR_LITERAL ({ 0.0, 0.0, 0.0, 1.0 }));
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

  editor->drawing = TRUE;
  draw_gesture_update (self, 0.0, 0.0, editor);
}

static void
draw_gesture_update (GtkGestureDrag              *gesture,
                     double                       offset_x,
                     double                       offset_y,
                     GtkCrusaderVillageMapEditor *editor)
{
  if (!editor->drawing)
    return;
  if (editor->hover_x < 0 || editor->hover_y < 0)
    return;

  g_assert (editor->current_stroke != NULL);
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
  g_autoptr (GListStore) strokes = NULL;

  if (!editor->drawing)
    return;

  g_assert (editor->current_stroke != NULL);

  g_object_get (
      editor->map,
      "strokes", &strokes,
      NULL);

  g_list_store_append (strokes, g_steal_pointer (&editor->current_stroke));
  editor->drawing = FALSE;
}

static void
cancel_gesture_pressed (GtkGestureClick             *self,
                        gint                         n_press,
                        double                       x,
                        double                       y,
                        GtkCrusaderVillageMapEditor *editor)
{
  if (!editor->drawing)
    return;

  g_clear_object (&editor->current_stroke);
  editor->drawing = FALSE;
  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static gboolean
scroll_event (GtkEventControllerScroll    *self,
              double                       dx,
              double                       dy,
              GtkCrusaderVillageMapEditor *editor)
{
  double delta_zoom = 0.0;
  double scroll_x   = 0.0;
  double scroll_y   = 0.0;

  // GdkModifierType mods = 0;
  // mods = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (self));

  delta_zoom = dy * -0.04 * editor->zoom;
  editor->zoom += delta_zoom;
  editor->zoom = CLAMP (editor->zoom, 0.15, 5.0);

  scroll_x = editor->hover_x_device * delta_zoom;
  scroll_y = editor->hover_y_device * delta_zoom;

  editor->pending_scroll   = TRUE;
  editor->pending_scroll_x = scroll_x;
  editor->pending_scroll_y = scroll_y;

  gtk_widget_queue_allocate (GTK_WIDGET (editor));

  return GDK_EVENT_STOP;
}

static void
motion_enter (GtkEventControllerMotion    *self,
              gdouble                      x,
              gdouble                      y,
              GtkCrusaderVillageMapEditor *editor)
{
  motion_event (self, x, y, editor);
}

static void
motion_event (GtkEventControllerMotion    *self,
              double                       x,
              double                       y,
              GtkCrusaderVillageMapEditor *editor)
{
  double   hscroll         = 0.0;
  double   vscroll         = 0.0;
  double   map_offset      = 0.0;
  double   abs_x           = 0.0;
  double   abs_y           = 0.0;
  int      new_hover_x     = 0;
  int      new_hover_y     = 0;
  int      map_tile_width  = 0;
  int      map_tile_height = 0;
  gboolean was_invalid     = FALSE;
  gboolean is_invalid      = FALSE;
  gboolean x_changed       = FALSE;
  gboolean y_changed       = FALSE;
  gboolean redraw          = FALSE;

  hscroll     = gtk_adjustment_get_value (editor->hadjustment);
  vscroll     = gtk_adjustment_get_value (editor->vadjustment);
  map_offset  = (double) editor->border_gap * BASE_TILE_SIZE * editor->zoom;
  abs_x       = x + hscroll - map_offset;
  abs_y       = y + vscroll - map_offset;
  new_hover_x = floor (abs_x / (BASE_TILE_SIZE * editor->zoom));
  new_hover_y = floor (abs_y / (BASE_TILE_SIZE * editor->zoom));

  g_object_get (
      editor->map,
      "width", &map_tile_width,
      "height", &map_tile_height,
      NULL);

  was_invalid = editor->hover_x < 0 || editor->hover_y < 0;
  is_invalid  = new_hover_x < 0 || new_hover_y < 0 ||
               new_hover_x >= map_tile_width || new_hover_y >= map_tile_height;

  if (is_invalid)
    new_hover_x = new_hover_y = -1;

  x_changed = new_hover_x != editor->hover_x;
  y_changed = new_hover_y != editor->hover_y;
  redraw    = (x_changed || y_changed) && (!was_invalid || !is_invalid);

  editor->hover_x        = new_hover_x;
  editor->hover_y        = new_hover_y;
  editor->hover_x_device = x;
  editor->hover_y_device = y;

  if (x_changed)
    g_object_notify_by_pspec (G_OBJECT (editor), props[PROP_HOVER_X]);
  if (y_changed)
    g_object_notify_by_pspec (G_OBJECT (editor), props[PROP_HOVER_Y]);
  if (redraw)
    gtk_widget_queue_draw (GTK_WIDGET (editor));
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

  editor->hover_x_device = -1.0;
  editor->hover_y_device = -1.0;

  if (redraw)
    gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
grid_changed (GtkCrusaderVillageMap       *map,
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
      int    widget_width    = 0;
      int    widget_height   = 0;
      int    map_tile_width  = 0;
      int    map_tile_height = 0;
      double width_lower     = 0.0;
      double height_lower    = 0.0;
      double width_upper     = 0.0;
      double height_upper    = 0.0;

      widget_width  = gtk_widget_get_width (GTK_WIDGET (self));
      widget_height = gtk_widget_get_height (GTK_WIDGET (self));

      g_object_get (
          self->map,
          "width", &map_tile_width,
          "height", &map_tile_height,
          NULL);

      width_upper  = (double) (map_tile_width + self->border_gap * 2) * BASE_TILE_SIZE * self->zoom;
      height_upper = (double) (map_tile_height + self->border_gap * 2) * BASE_TILE_SIZE * self->zoom;

      if (width_upper < (double) widget_width)
        {
          width_lower = width_upper = -((double) widget_width - width_upper) / 2.0;
          center                    = TRUE;
        }
      else
        width_upper -= (double) widget_width;

      if (height_upper < (double) widget_height)
        {
          height_lower = height_upper = -((double) widget_height - height_upper) / 2.0;
          center                      = TRUE;
        }
      else
        height_upper -= (double) widget_height;

      g_object_set (
          self->hadjustment,
          "lower", width_lower,
          "upper", width_upper,
          NULL);
      g_object_set (
          self->vadjustment,
          "lower", height_lower,
          "upper", height_upper,
          NULL);

      if (center || self->queue_center)
        {
          gtk_adjustment_set_value (
              self->hadjustment,
              (width_lower + width_upper) / 2.0);
          gtk_adjustment_set_value (
              self->vadjustment,
              (height_lower + height_upper) / 2.0);

          self->queue_center   = FALSE;
          self->pending_scroll = FALSE;
        }
      else if (self->pending_scroll)
        {
          gtk_adjustment_set_value (
              self->hadjustment,
              gtk_adjustment_get_value (self->hadjustment) +
                  self->pending_scroll_x);
          gtk_adjustment_set_value (
              self->vadjustment,
              gtk_adjustment_get_value (self->vadjustment) +
                  self->pending_scroll_y);

          self->pending_scroll = FALSE;
        }
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
