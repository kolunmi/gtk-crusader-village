/* gtk-crusader-village-map-handle.c
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

#include <gtk/gtk.h>

#include "gtk-crusader-village-item-stroke.h"
#include "gtk-crusader-village-item.h"
#include "gtk-crusader-village-map-handle.h"
#include "gtk-crusader-village-map.h"

enum
{
  ACTION_REMOVED,
  ACTION_ADDED,
};

typedef struct
{
  int        type;
  guint      position;
  GPtrArray *pa;
} Action;

struct _GcvMapHandle
{
  GObject parent_instance;

  GcvMap     *map;
  GListStore *strokes;

  GListStore *mirror;
  GPtrArray  *memory;
  guint       n_undos;

  guint    cursor;
  guint    cursor_len;
  gboolean insert_mode;
  gboolean lock_hinted;

  GHashTable *cache;
  guint       last_append_position;
};

G_DEFINE_FINAL_TYPE (GcvMapHandle, gcv_map_handle, G_TYPE_OBJECT)

enum
{
  PROP_0,

  PROP_MAP,
  PROP_MODEL,
  PROP_CURSOR,
  PROP_CURSOR_LEN,
  PROP_INSERT_MODE,
  PROP_LOCK_HINTED,
  PROP_GRID,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
strokes_changed (GListModel   *self,
                 guint         position,
                 guint         removed,
                 guint         added,
                 GcvMapHandle *handle);

static void
dimensions_changed (GcvMap       *map,
                    GParamSpec   *pspec,
                    GcvMapHandle *handle);

static void
ensure_cache (GcvMapHandle *self);

static void
add_stroke_to_cache (GcvMapHandle  *self,
                     GcvItemStroke *stroke,
                     int            map_width,
                     int            map_height);

static inline Action *
new_action (int        type,
            guint      position,
            GPtrArray *pa);

static void
destroy_action (gpointer ptr);

static void
gcv_map_handle_dispose (GObject *object)
{
  GcvMapHandle *self = GCV_MAP_HANDLE (object);

  if (self->map != NULL)
    g_signal_handlers_disconnect_by_func (self->map, dimensions_changed, self);
  g_clear_object (&self->map);

  if (self->strokes != NULL)
    g_signal_handlers_disconnect_by_func (self->strokes, strokes_changed, self);
  g_clear_object (&self->strokes);

  g_clear_pointer (&self->memory, g_ptr_array_unref);
  g_clear_object (&self->mirror);
  g_clear_pointer (&self->cache, g_hash_table_unref);

  G_OBJECT_CLASS (gcv_map_handle_parent_class)->dispose (object);
}

static void
gcv_map_handle_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GcvMapHandle *self = GCV_MAP_HANDLE (object);

  switch (prop_id)
    {
    case PROP_MAP:
      g_value_set_object (value, self->map);
      break;
    case PROP_MODEL:
      g_value_set_object (value, self->strokes);
      break;
    case PROP_CURSOR:
      g_value_set_uint (value, self->cursor);
      break;
    case PROP_CURSOR_LEN:
      g_value_set_uint (value, self->cursor_len);
      break;
    case PROP_INSERT_MODE:
      g_value_set_boolean (value, self->insert_mode);
      break;
    case PROP_LOCK_HINTED:
      g_value_set_boolean (value, self->lock_hinted);
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
gcv_map_handle_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GcvMapHandle *self = GCV_MAP_HANDLE (object);

  switch (prop_id)
    {
    case PROP_MAP:
      {
        g_clear_object (&self->map);
        if (self->strokes != NULL)
          g_signal_handlers_disconnect_by_func (self->map, dimensions_changed, self);
        g_clear_object (&self->strokes);
        g_ptr_array_set_size (self->memory, 0);
        g_clear_pointer (&self->cache, g_hash_table_unref);

        self->map = g_value_dup_object (value);

        if (self->map != NULL)
          {
            guint strokes_len = 0;

            g_object_get (
                self->map,
                "strokes", &self->strokes,
                NULL);

            strokes_len = g_list_model_get_n_items (G_LIST_MODEL (self->strokes));
            if (strokes_len > 0)
              {
                g_autofree GcvItemStroke **read = NULL;

                read = g_malloc0_n (strokes_len, sizeof (*read));

                for (guint i = 0; i < strokes_len; i++)
                  read[i] = g_list_model_get_item (G_LIST_MODEL (self->strokes), i);
                g_list_store_splice (self->mirror, 0, 0, (gpointer *) read, strokes_len);
                for (guint i = 0; i < strokes_len; i++)
                  g_object_unref (read[i]);
              }

            self->cursor = g_list_model_get_n_items (G_LIST_MODEL (self->strokes));

            g_signal_connect (self->strokes, "items-changed",
                              G_CALLBACK (strokes_changed), self);
            g_signal_connect (self->map, "notify::width",
                              G_CALLBACK (dimensions_changed), self);
            g_signal_connect (self->map, "notify::height",
                              G_CALLBACK (dimensions_changed), self);
          }
        else
          self->cursor = 0;

        self->cursor_len = 1;

        g_object_notify_by_pspec (object, props[PROP_GRID]);
        g_object_notify_by_pspec (object, props[PROP_CURSOR]);
        g_object_notify_by_pspec (object, props[PROP_CURSOR_LEN]);
      }
      break;

    case PROP_CURSOR:
      {
        guint new_cursor = 0;

        new_cursor = MIN (g_value_get_uint (value),
                          g_list_model_get_n_items (G_LIST_MODEL (self->strokes)));

        if (new_cursor != self->cursor)
          {
            self->cursor = new_cursor;
            g_object_notify_by_pspec (object, props[PROP_CURSOR]);
          }
      }
      break;
    case PROP_CURSOR_LEN:
      {
        guint new_cursor_len = 0;

        new_cursor_len = g_value_get_uint (value);
        if (new_cursor_len != self->cursor_len)
          {
            self->cursor_len = new_cursor_len;
            g_object_notify_by_pspec (object, props[PROP_CURSOR_LEN]);
          }
      }
      break;
    case PROP_INSERT_MODE:
      self->insert_mode = g_value_get_boolean (value);
      break;
    case PROP_LOCK_HINTED:
      self->lock_hinted = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gcv_map_handle_class_init (GcvMapHandleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = gcv_map_handle_dispose;
  object_class->get_property = gcv_map_handle_get_property;
  object_class->set_property = gcv_map_handle_set_property;

  props[PROP_MAP] =
      g_param_spec_object (
          "map",
          "Map",
          "The map this object will manipulate",
          GCV_TYPE_MAP,
          G_PARAM_READWRITE);

  props[PROP_MODEL] =
      g_param_spec_object (
          "model",
          "Model",
          "The model of the map strokes, for convenience",
          G_TYPE_LIST_MODEL,
          G_PARAM_READABLE);

  props[PROP_CURSOR] =
      g_param_spec_uint (
          "cursor",
          "Cursor",
          "The splitting point",
          0, G_MAXUINT, 0,
          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_CURSOR_LEN] =
      g_param_spec_uint (
          "cursor-len",
          "Cursor Length",
          "The size of the splitting point",
          1, G_MAXUINT, 1,
          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_INSERT_MODE] =
      g_param_spec_boolean (
          "insert-mode",
          "Insert Mode",
          "Whether to disable the automatic discard of memory "
          "when the underlying map's strokes are appended to",
          TRUE,
          G_PARAM_READWRITE);

  props[PROP_LOCK_HINTED] =
      g_param_spec_boolean (
          "lock-hinted",
          "Lock Hinted",
          "Whether to hint to other reference holders that "
          "this handle should not be modified right now",
          FALSE,
          G_PARAM_READWRITE);

  props[PROP_GRID] =
      g_param_spec_boxed (
          "grid",
          "Grid",
          "A hash table mapping `guint` tile indices to `GcvItem`s "
          "which will be out-of-date after any other property is modified",
          G_TYPE_HASH_TABLE,
          G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
gcv_map_handle_init (GcvMapHandle *self)
{
  self->memory               = g_ptr_array_new_with_free_func (destroy_action);
  self->mirror               = g_list_store_new (GCV_TYPE_ITEM_STROKE);
  self->last_append_position = G_MAXUINT;
  self->insert_mode          = TRUE;
  self->cursor_len           = 1;
}

static void
strokes_changed (GListModel   *self,
                 guint         position,
                 guint         removed,
                 guint         added,
                 GcvMapHandle *handle)
{
  g_autofree GcvItemStroke **additions = NULL;
  guint                      n_items   = 0;

  g_ptr_array_set_size (handle->memory, handle->n_undos);

  if (removed > 0)
    {
      g_autofree GcvItemStroke **removals = NULL;

      removals = g_malloc0_n (removed, sizeof (*removals));
      for (guint i = 0; i < removed; i++)
        removals[i] = g_list_model_get_item (G_LIST_MODEL (handle->mirror), position + i);

      g_ptr_array_add (
          handle->memory,
          new_action (
              ACTION_REMOVED, position,
              g_ptr_array_new_take (
                  (gpointer *) g_steal_pointer (&removals),
                  removed, g_object_unref)));
    }

  if (added > 0)
    {
      additions = g_malloc0_n (added, sizeof (*additions));
      for (guint i = 0; i < added; i++)
        additions[i] = g_list_model_get_item (G_LIST_MODEL (handle->strokes), position + i);
    }

  g_list_store_splice (handle->mirror, position, removed,
                       (gpointer *) additions, added);

  if (added > 0)
    g_ptr_array_add (
        handle->memory,
        new_action (
            ACTION_ADDED, position,
            g_ptr_array_new_take (
                (gpointer *) g_steal_pointer (&additions),
                added, g_object_unref)));

  handle->n_undos++;

  n_items = g_list_model_get_n_items (G_LIST_MODEL (handle->strokes));
  if (removed > 0 || position < n_items - added)
    /* We need to completely regenerate cache */
    g_clear_pointer (&handle->cache, g_hash_table_unref);
  else
    /* It was just an append, no need to regenerate */
    handle->last_append_position = MIN (position, handle->last_append_position);

  g_object_notify_by_pspec (G_OBJECT (handle), props[PROP_GRID]);
}

static void
dimensions_changed (GcvMap       *map,
                    GParamSpec   *pspec,
                    GcvMapHandle *handle)
{
  g_clear_pointer (&handle->cache, g_hash_table_unref);
  g_object_notify_by_pspec (G_OBJECT (handle), props[PROP_GRID]);
}

void
gcv_map_handle_undo (GcvMapHandle *self)
{
  Action *action = NULL;

  g_return_if_fail (GCV_IS_MAP_HANDLE (self));
  g_return_if_fail (self->map != NULL);
  g_return_if_fail (self->n_undos > 0);

  action = g_ptr_array_index (self->memory, self->n_undos - 1);

  g_signal_handlers_block_by_func (self->strokes, strokes_changed, self);

  switch (action->type)
    {
    case ACTION_REMOVED:
      g_list_store_splice (self->strokes, action->position, 0,
                           action->pa->pdata, action->pa->len);
      g_list_store_splice (self->mirror, action->position, 0,
                           action->pa->pdata, action->pa->len);
      self->cursor_len = action->pa->len;
      break;
    case ACTION_ADDED:
      g_list_store_splice (self->strokes, action->position,
                           action->pa->len, NULL, 0);
      g_list_store_splice (self->mirror, action->position,
                           action->pa->len, NULL, 0);
      self->cursor_len = 1;
      break;
    default:
      g_assert_not_reached ();
    }

  g_signal_handlers_unblock_by_func (self->strokes, strokes_changed, self);

  self->n_undos--;
  self->cursor = action->position;
  g_clear_pointer (&self->cache, g_hash_table_unref);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_GRID]);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CURSOR]);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CURSOR_LEN]);
}

void
gcv_map_handle_redo (GcvMapHandle *self)
{
  Action *action = NULL;

  g_return_if_fail (GCV_IS_MAP_HANDLE (self));
  g_return_if_fail (self->map != NULL);
  g_return_if_fail (self->n_undos < self->memory->len);

  action = g_ptr_array_index (self->memory, self->n_undos);

  g_signal_handlers_block_by_func (self->strokes, strokes_changed, self);

  switch (action->type)
    {
    case ACTION_REMOVED:
      g_list_store_splice (self->strokes, action->position,
                           action->pa->len, NULL, 0);
      g_list_store_splice (self->mirror, action->position,
                           action->pa->len, NULL, 0);
      self->cursor_len = 1;
      break;
    case ACTION_ADDED:
      g_list_store_splice (self->strokes, action->position, 0,
                           action->pa->pdata, action->pa->len);
      g_list_store_splice (self->mirror, action->position, 0,
                           action->pa->pdata, action->pa->len);
      self->cursor_len = action->pa->len;
      break;
    default:
      g_assert_not_reached ();
    }

  g_signal_handlers_unblock_by_func (self->strokes, strokes_changed, self);

  self->n_undos++;
  self->cursor = action->position;
  g_clear_pointer (&self->cache, g_hash_table_unref);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_GRID]);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CURSOR]);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CURSOR_LEN]);
}

gboolean
gcv_map_handle_can_undo (GcvMapHandle *self)
{
  g_return_val_if_fail (GCV_IS_MAP_HANDLE (self), FALSE);

  return self->map != NULL && self->n_undos > 0;
}

gboolean
gcv_map_handle_can_redo (GcvMapHandle *self)
{
  g_return_val_if_fail (GCV_IS_MAP_HANDLE (self), FALSE);

  return self->map != NULL && self->n_undos < self->memory->len;
}

void
gcv_map_handle_clear_all (GcvMapHandle *self)
{
  g_return_if_fail (GCV_IS_MAP_HANDLE (self));
  g_return_if_fail (self->map != NULL);

  g_signal_handlers_block_by_func (self->strokes, strokes_changed, self);

  g_list_store_remove_all (self->strokes);
  g_list_store_remove_all (self->mirror);
  g_ptr_array_set_size (self->memory, 0);

  g_signal_handlers_unblock_by_func (self->strokes, strokes_changed, self);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_GRID]);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CURSOR]);
}

static void
ensure_cache (GcvMapHandle *self)
{
  guint start_stroke_idx = 0;
  guint total            = 0;
  int   map_width        = 0;
  int   map_height       = 0;

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

  total = g_list_model_get_n_items (G_LIST_MODEL (self->strokes));
  g_object_get (
      self->map,
      "width", &map_width,
      "height", &map_height,
      NULL);

  for (guint i = start_stroke_idx; i < total; i++)
    add_stroke_to_cache (
        self,
        g_list_model_get_item (G_LIST_MODEL (self->strokes), i),
        map_width, map_height);

  self->last_append_position = G_MAXUINT;
}

static void
add_stroke_to_cache (GcvMapHandle  *self,
                     GcvItemStroke *stroke,
                     int            map_width,
                     int            map_height)
{
  g_autoptr (GcvItem) item     = NULL;
  GcvItemKind item_kind        = GCV_ITEM_KIND_BUILDING;
  int         item_tile_width  = 0;
  int         item_tile_height = 0;
  g_autoptr (GArray) instances = NULL;

  g_assert (self->cache != NULL);

  g_object_get (
      stroke,
      "item", &item,
      NULL);

  g_object_get (
      item,
      "kind", &item_kind,
      NULL);

  /* TODO make a unit layer */
  if (item_kind == GCV_ITEM_KIND_UNIT)
    /* Ignore units for now */
    return;

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

  for (guint i = 0; i < instances->len; i++)
    {
      GcvItemStrokeInstance *instance = NULL;

      instance = &g_array_index (instances, GcvItemStrokeInstance, i);
      g_assert (instance->x >= 0 &&
                instance->y >= 0 &&
                instance->x + item_tile_width <= map_width &&
                instance->y + item_tile_height <= map_height);

      for (int y = 0; y < item_tile_height; y++)
        {
          for (int x = 0; x < item_tile_width; x++)
            {
              guint idx = 0;

              idx = (instance->y + y) * map_width + (instance->x + x);
              g_hash_table_replace (self->cache, GUINT_TO_POINTER (idx), g_object_ref (item));
            }
        }
    }
}

static inline Action *
new_action (int        type,
            guint      position,
            GPtrArray *pa)
{
  Action *action = NULL;

  action           = g_new0 (typeof (*action), 1);
  action->type     = type;
  action->position = position;
  action->pa       = pa;

  return action;
}

static void
destroy_action (gpointer ptr)
{
  Action *self = ptr;

  g_ptr_array_unref (self->pa);
  g_free (self);
}
