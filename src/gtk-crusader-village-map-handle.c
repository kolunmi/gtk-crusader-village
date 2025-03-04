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
#include "gtk-crusader-village-map-handle.h"
#include "gtk-crusader-village-map.h"

struct _GtkCrusaderVillageMapHandle
{
  GObject parent_instance;

  GtkCrusaderVillageMap *map;
  GListStore            *strokes;

  GListStore          *memory;
  GListStore          *backing_model;
  GtkFlattenListModel *flatten_model;

  guint    cursor;
  gboolean insert_mode;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageMapHandle, gtk_crusader_village_map_handle, G_TYPE_OBJECT)

enum
{
  PROP_0,

  PROP_MAP,
  PROP_MODEL,
  PROP_CURSOR,
  PROP_INSERT_MODE,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
strokes_changed (GListModel                  *self,
                 guint                        position,
                 guint                        removed,
                 guint                        added,
                 GtkCrusaderVillageMapHandle *handle);

static void
gtk_crusader_village_map_handle_dispose (GObject *object)
{
  GtkCrusaderVillageMapHandle *self = GTK_CRUSADER_VILLAGE_MAP_HANDLE (object);

  g_clear_object (&self->map);
  g_clear_object (&self->memory);
  g_clear_object (&self->backing_model);
  g_clear_object (&self->flatten_model);

  G_OBJECT_CLASS (gtk_crusader_village_map_handle_parent_class)->dispose (object);
}

static void
gtk_crusader_village_map_handle_get_property (GObject    *object,
                                              guint       prop_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  GtkCrusaderVillageMapHandle *self = GTK_CRUSADER_VILLAGE_MAP_HANDLE (object);

  switch (prop_id)
    {
    case PROP_MAP:
      g_value_set_object (value, self->map);
      break;
    case PROP_MODEL:
      g_value_set_object (value, self->flatten_model);
      break;
    case PROP_CURSOR:
      g_value_set_uint (value, self->cursor);
      break;
    case PROP_INSERT_MODE:
      g_value_set_boolean (value, self->insert_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_map_handle_set_property (GObject      *object,
                                              guint         prop_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  GtkCrusaderVillageMapHandle *self = GTK_CRUSADER_VILLAGE_MAP_HANDLE (object);

  switch (prop_id)
    {
    case PROP_MAP:
      {
        g_clear_object (&self->map);

        if (self->strokes != NULL)
          {
            gboolean result   = FALSE;
            guint    position = 0;

            g_signal_handlers_disconnect_by_func (self->strokes, strokes_changed, self);

            result = g_list_store_find (self->backing_model, self->strokes, &position);
            if (result)
              g_list_store_remove (self->backing_model, position);
          }
        g_clear_object (&self->strokes);

        self->map = g_value_dup_object (value);

        if (self->map != NULL)
          {
            g_object_get (
                self->map,
                "strokes", &self->strokes,
                NULL);

            self->cursor = g_list_model_get_n_items (G_LIST_MODEL (self->strokes));
            g_list_store_insert (self->backing_model, 0, g_object_ref (self->strokes));

            g_signal_connect (self->strokes, "items-changed",
                              G_CALLBACK (strokes_changed), self);
          }
        else
          self->cursor = 0;

        g_list_store_remove_all (self->memory);

        g_object_notify_by_pspec (object, props[PROP_CURSOR]);
      }
      break;

    case PROP_CURSOR:
      {
        guint old_cursor = 0;
        guint new_cursor = 0;

        old_cursor = self->cursor;
        new_cursor = g_value_get_uint (value);

        if (self->map != NULL)
          {
            guint n_strokes = 0;
            guint n_memory  = 0;

            n_strokes  = g_list_model_get_n_items (G_LIST_MODEL (self->strokes));
            n_memory   = g_list_model_get_n_items (G_LIST_MODEL (self->memory));
            new_cursor = MIN (new_cursor, n_strokes + n_memory);

            if (new_cursor != old_cursor)
              {
                self->cursor = new_cursor;

                g_signal_handlers_block_by_func (self->strokes, strokes_changed, self);
                g_object_freeze_notify (G_OBJECT (self->flatten_model));

                if (new_cursor < old_cursor)
                  {
                    guint                                     range_length = 0;
                    g_autofree GtkCrusaderVillageItemStroke **withdraw     = NULL;

                    range_length = old_cursor - new_cursor;

                    withdraw = g_malloc0_n (range_length, sizeof (*withdraw));
                    for (guint i = 0; i < range_length; i++)
                      withdraw[i] = g_object_ref (g_list_model_get_item (G_LIST_MODEL (self->strokes), new_cursor + i));

                    g_list_store_splice (self->strokes, new_cursor, range_length, NULL, 0);
                    g_list_store_splice (self->memory, 0, 0, (gpointer *) withdraw, range_length);

                    for (guint i = 0; i < range_length; i++)
                      g_object_unref (withdraw[i]);
                  }
                else if (new_cursor > old_cursor)
                  {
                    guint                                     range_length = 0;
                    g_autofree GtkCrusaderVillageItemStroke **restore      = NULL;

                    range_length = new_cursor - old_cursor;

                    restore = g_malloc0_n (range_length, sizeof (*restore));
                    for (guint i = 0; i < range_length; i++)
                      restore[i] = g_object_ref (g_list_model_get_item (G_LIST_MODEL (self->memory), i));

                    g_list_store_splice (self->memory, 0, range_length, NULL, 0);
                    g_list_store_splice (self->strokes, n_strokes, 0, (gpointer *) restore, range_length);

                    for (guint i = 0; i < range_length; i++)
                      g_object_unref (restore[i]);
                  }

                g_object_thaw_notify (G_OBJECT (self->flatten_model));
                g_signal_handlers_unblock_by_func (self->strokes, strokes_changed, self);
                g_object_notify_by_pspec (object, props[PROP_CURSOR]);
              }
          }
        else
          {
            self->cursor = 0;
            g_object_notify_by_pspec (object, props[PROP_CURSOR]);
          }
      }
      break;

    case PROP_INSERT_MODE:
      self->insert_mode = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_map_handle_class_init (GtkCrusaderVillageMapHandleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = gtk_crusader_village_map_handle_dispose;
  object_class->get_property = gtk_crusader_village_map_handle_get_property;
  object_class->set_property = gtk_crusader_village_map_handle_set_property;

  props[PROP_MAP] =
      g_param_spec_object (
          "map",
          "Map",
          "The map this object will manipulate",
          GTK_CRUSADER_VILLAGE_TYPE_MAP,
          G_PARAM_READWRITE);

  props[PROP_MODEL] =
      g_param_spec_object (
          "model",
          "Model",
          "The list model representing the map strokes "
          "and reserved strokes, concactenated",
          G_TYPE_LIST_MODEL,
          G_PARAM_READABLE);

  props[PROP_CURSOR] =
      g_param_spec_uint (
          "cursor",
          "Cursor",
          "The splitting point",
          0, G_MAXUINT, 0,
          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_INSERT_MODE] =
      g_param_spec_boolean (
          "insert-mode",
          "Insert Mode",
          "Whether to disable the automatic discard of memory "
          "when the underlying map's strokes are appended to",
          FALSE,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
gtk_crusader_village_map_handle_init (GtkCrusaderVillageMapHandle *self)
{
  self->memory        = g_list_store_new (GTK_CRUSADER_VILLAGE_TYPE_ITEM_STROKE);
  self->backing_model = g_list_store_new (G_TYPE_LIST_MODEL);
  self->flatten_model = gtk_flatten_list_model_new (G_LIST_MODEL (g_object_ref (self->backing_model)));

  g_list_store_append (self->backing_model, g_object_ref (self->memory));
}

static void
strokes_changed (GListModel                  *self,
                 guint                        position,
                 guint                        removed,
                 guint                        added,
                 GtkCrusaderVillageMapHandle *handle)
{
  guint n_strokes = 0;

  if (!handle->insert_mode && added > 0)
    /* Goodbye! */
    g_list_store_remove_all (handle->memory);

  n_strokes = g_list_model_get_n_items (G_LIST_MODEL (handle->strokes));
  if (handle->cursor != n_strokes)
    {
      handle->cursor = n_strokes;
      g_object_notify_by_pspec (G_OBJECT (handle), props[PROP_CURSOR]);
    }
}
