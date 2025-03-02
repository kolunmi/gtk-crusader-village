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

  GHashTable *cache;
  guint       last_append_position;
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
gtk_crusader_village_map_dispose (GObject *object)
{
  GtkCrusaderVillageMap *self = GTK_CRUSADER_VILLAGE_MAP (object);

  g_clear_pointer (&self->name, g_free);
  g_clear_object (&self->strokes);
  g_clear_pointer (&self->cache, g_hash_table_unref);

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
      g_value_set_boxed (value, self->cache);
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
      self->width = g_value_get_int (value);
      g_clear_pointer (&self->cache, g_hash_table_unref);
      g_object_notify_by_pspec (object, props[PROP_GRID]);
      break;
    case PROP_HEIGHT:
      self->height = g_value_get_int (value);
      g_clear_pointer (&self->cache, g_hash_table_unref);
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
          16, 16384, 98,
          G_PARAM_READWRITE);

  props[PROP_HEIGHT] =
      g_param_spec_int (
          "height",
          "Height",
          "The height of the map",
          16, 16384, 98,
          G_PARAM_READWRITE);

  props[PROP_STROKES] =
      g_param_spec_object (
          "strokes",
          "Strokes",
          "A list store containing the ordered item strokes",
          G_TYPE_LIST_STORE,
          G_PARAM_READABLE);

  props[PROP_GRID] =
      g_param_spec_boxed (
          "grid",
          "Grid",
          "A hash table mapping `guint` tile indices to `GtkCrusaderVillageItem`s "
          "which will be out-of-date after any other property is modified",
          G_TYPE_HASH_TABLE,
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

  self->last_append_position = G_MAXUINT;
}

static void
strokes_changed (GListModel            *self,
                 guint                  position,
                 guint                  removed,
                 guint                  added,
                 GtkCrusaderVillageMap *map)
{
  if (removed > 0 || position < g_list_model_get_n_items (self) - added)
    /* We need to completely regenerate cache */
    g_clear_pointer (&map->cache, g_hash_table_unref);
  else
    /* It was just an append, no need to regenerate */
    map->last_append_position = MIN (position, map->last_append_position);

  g_object_notify_by_pspec (G_OBJECT (map), props[PROP_GRID]);
}

static void
ensure_cache (GtkCrusaderVillageMap *self)
{
  guint start_stroke_idx = 0;
  guint n_strokes        = 0;

  if (self->cache != NULL && self->last_append_position == G_MAXUINT)
    return;

  if (self->cache == NULL)
    {
      self->cache = g_hash_table_new_full (
          /* |------ key: tile index -------|  | val:  Item | */
          g_direct_hash, g_direct_equal, NULL, g_object_unref);
      start_stroke_idx = 0;
    }
  else
    start_stroke_idx = self->last_append_position;

  n_strokes = g_list_model_get_n_items (G_LIST_MODEL (self->strokes));

  for (guint i = start_stroke_idx; i < n_strokes; i++)
    {
      g_autoptr (GtkCrusaderVillageItemStroke) stroke = NULL;
      g_autoptr (GtkCrusaderVillageItem) item         = NULL;
      GtkCrusaderVillageItemKind item_kind            = GTK_CRUSADER_VILLAGE_ITEM_KIND_BUILDING;
      int                        item_tile_width      = 0;
      int                        item_tile_height     = 0;
      g_autoptr (GArray) instances                    = NULL;

      stroke = g_list_model_get_item (G_LIST_MODEL (self->strokes), i);
      g_object_get (
          stroke,
          "item", &item,
          NULL);

      g_object_get (
          item,
          "kind", &item_kind,
          NULL);

      if (item_kind == GTK_CRUSADER_VILLAGE_ITEM_KIND_UNIT)
        /* Ignore units for now */
        continue;

      g_object_get (
          item,
          "tile-width", &item_tile_width,
          "tile-height", &item_tile_height,
          NULL);
      g_assert (item_tile_width > 0 && item_tile_height > 0);

      g_object_get (
          stroke,
          "instances", &instances,
          NULL);

      for (guint j = 0; j < instances->len; j++)
        {
          GtkCrusaderVillageItemStrokeInstance *instance = NULL;

          instance = &g_array_index (instances, GtkCrusaderVillageItemStrokeInstance, j);
          g_assert (instance->x >= 0 && instance->y >= 0);

          if (instance->x + item_tile_width > self->width ||
              instance->y + item_tile_height > self->height)
            continue;

          for (int y = 0; y < item_tile_height; y++)
            {
              for (int x = 0; x < item_tile_width; x++)
                {
                  guint idx = 0;

                  idx = (instance->y + y) * self->width + (instance->x + x);
                  g_hash_table_replace (self->cache, GUINT_TO_POINTER (idx), g_object_ref (item));
                }
            }
        }
    }

  self->last_append_position = G_MAXUINT;
}
