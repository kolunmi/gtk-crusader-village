/* gtk-crusader-village-map.c
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

#include <gio/gio.h>

#include "gtk-crusader-village-item-stroke.h"
#include "gtk-crusader-village-item.h"
#include "gtk-crusader-village-map.h"

struct _GtkCrusaderVillageMap
{
  GObject parent_instance;

  char *name;
  int   width;
  int   height;

  GListStore *strokes;

  GPtrArray *cache_grid;
  gboolean   cache_refresh;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageMap, gtk_crusader_village_map, G_TYPE_OBJECT)

enum
{
  PROP_0,

  PROP_NAME,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_STROKES,
  PROP_GRID,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
strokes_changed (GListModel            *self,
                 guint                  position,
                 guint                  removed,
                 guint                  added,
                 GtkCrusaderVillageMap *map);

static void
ensure_cache (GtkCrusaderVillageMap *self);

static void
maybe_unref_object (gpointer object);

static void
gtk_crusader_village_map_dispose (GObject *object)
{
  GtkCrusaderVillageMap *self = GTK_CRUSADER_VILLAGE_MAP (object);

  g_clear_pointer (&self->name, g_free);
  g_clear_object (&self->strokes);
  g_clear_pointer (&self->cache_grid, g_ptr_array_unref);

  G_OBJECT_CLASS (gtk_crusader_village_map_parent_class)->dispose (object);
}

static void
gtk_crusader_village_map_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GtkCrusaderVillageMap *self = GTK_CRUSADER_VILLAGE_MAP (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, self->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, self->height);
      break;
    case PROP_STROKES:
      g_value_set_object (value, self->strokes);
      break;
    case PROP_GRID:
      ensure_cache (self);
      g_value_set_pointer (value, self->cache_grid->pdata);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_map_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GtkCrusaderVillageMap *self = GTK_CRUSADER_VILLAGE_MAP (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_clear_pointer (&self->name, g_free);
      self->name = g_value_dup_string (value);
      break;
    case PROP_WIDTH:
      self->width         = g_value_get_int (value);
      self->cache_refresh = TRUE;
      g_object_notify_by_pspec (object, props[PROP_GRID]);
      break;
    case PROP_HEIGHT:
      self->height        = g_value_get_int (value);
      self->cache_refresh = TRUE;
      g_object_notify_by_pspec (object, props[PROP_GRID]);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_map_class_init (GtkCrusaderVillageMapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = gtk_crusader_village_map_dispose;
  object_class->get_property = gtk_crusader_village_map_get_property;
  object_class->set_property = gtk_crusader_village_map_set_property;

  props[PROP_NAME] =
      g_param_spec_string (
          "name",
          "Name",
          "The displayable map name",
          NULL,
          G_PARAM_READWRITE);

  props[PROP_WIDTH] =
      g_param_spec_int (
          "width",
          "Width",
          "The width of the map",
          16, 512, 98,
          G_PARAM_READWRITE);

  props[PROP_HEIGHT] =
      g_param_spec_int (
          "height",
          "Height",
          "The height of the map",
          16, 512, 98,
          G_PARAM_READWRITE);

  props[PROP_STROKES] =
      g_param_spec_object (
          "strokes",
          "Strokes",
          "A list store containing the ordered item strokes",
          G_TYPE_LIST_STORE,
          G_PARAM_READABLE);

  props[PROP_GRID] =
      g_param_spec_pointer (
          "grid",
          "Grid",
          "A const pointer to an array of `GtkCrusaderVillageItem *` "
          "the length of which is the map width times the map height",
          G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
gtk_crusader_village_map_init (GtkCrusaderVillageMap *self)
{
  self->width  = 98;
  self->height = 98;

  self->strokes = g_list_store_new (GTK_CRUSADER_VILLAGE_TYPE_ITEM_STROKE);
  g_signal_connect (self->strokes, "items-changed",
                    G_CALLBACK (strokes_changed), self);

  self->cache_refresh = TRUE;
}

static void
strokes_changed (GListModel            *self,
                 guint                  position,
                 guint                  removed,
                 guint                  added,
                 GtkCrusaderVillageMap *map)
{
  map->cache_refresh = TRUE;
  g_object_notify_by_pspec (G_OBJECT (map), props[PROP_GRID]);
}

static void
ensure_cache (GtkCrusaderVillageMap *self)
{
  if (self->cache_grid == NULL)
    {
      self->cache_grid    = g_ptr_array_new_with_free_func (maybe_unref_object);
      self->cache_refresh = TRUE;
    }

  if (self->cache_refresh)
    {
      guint n_strokes = 0;

      /* Clear all existing items */
      g_ptr_array_set_size (self->cache_grid, 0);
      g_ptr_array_set_size (self->cache_grid, self->width * self->height);

      n_strokes = g_list_model_get_n_items (G_LIST_MODEL (self->strokes));

      for (guint i = 0; i < n_strokes; i++)
        {
          g_autoptr (GtkCrusaderVillageItemStroke) stroke = NULL;
          g_autoptr (GtkCrusaderVillageItem) item         = NULL;
          GtkCrusaderVillageItemKind item_kind            = GTK_CRUSADER_VILLAGE_ITEM_KIND_BUILDING;

          stroke = g_list_model_get_item (G_LIST_MODEL (self->strokes), i);
          g_object_get (
              stroke,
              "item", &item,
              NULL);

          g_object_get (
              item,
              "kind", &item_kind,
              NULL);

          if (item_kind != GTK_CRUSADER_VILLAGE_ITEM_KIND_UNIT)
            {
              int tile_width               = 0;
              int tile_height              = 0;
              g_autoptr (GArray) instances = NULL;

              g_object_get (
                  item,
                  "tile-width", &tile_width,
                  "tile-height", &tile_height,
                  NULL);
              g_assert (tile_width > 0 && tile_height > 0);

              g_object_get (
                  stroke,
                  "instances", &instances,
                  NULL);

              for (guint j = 0; j < instances->len; j++)
                {
                  GtkCrusaderVillageItemStrokeInstance *instance = NULL;

                  instance = &g_array_index (instances, GtkCrusaderVillageItemStrokeInstance, j);
                  g_assert (instance->x >= 0 && instance->y >= 0);

                  if (instance->x < self->width || instance->y < self->height)
                    {
                      guint idx = 0;

                      idx = instance->y * self->width + instance->x;
                      g_clear_object (&g_ptr_array_index (self->cache_grid, idx));
                      g_ptr_array_index (self->cache_grid, idx) = g_object_ref (item);
                    }
                }
            }
        }

      self->cache_refresh = FALSE;
    }
}

static void
maybe_unref_object (gpointer object)
{
  if (object != NULL)
    g_object_unref (object);
}
