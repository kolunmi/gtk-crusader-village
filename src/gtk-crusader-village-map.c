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

#include <json-glib/json-glib.h>

#include "gtk-crusader-village-item-stroke.h"
#include "gtk-crusader-village-map.h"

/* clang-format off */
G_DEFINE_QUARK (gtk-crusader-village-map-error-quark, gtk_crusader_village_map_error);
/* clang-format on */

struct _GtkCrusaderVillageMap
{
  GObject parent_instance;

  char *name;
  int   width;
  int   height;

  GListStore *strokes;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageMap, gtk_crusader_village_map, G_TYPE_OBJECT)

enum
{
  PROP_0,

  PROP_NAME,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_STROKES,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

typedef struct
{
  GtkCrusaderVillageItemStore *store;
  char                        *python_exe;
} LoadData;

typedef struct
{
  GPtrArray *strokes;
  char      *python_exe;
} SaveData;

static void
destroy_load_data (gpointer data);

static void
destroy_save_data (gpointer data);

static void
new_from_aiv_file_async_thread (GTask        *task,
                                gpointer      object,
                                gpointer      task_data,
                                GCancellable *cancellable);

static void
save_to_aiv_file_async_thread (GTask        *task,
                               gpointer      object,
                               gpointer      task_data,
                               GCancellable *cancellable);

static void
gtk_crusader_village_map_dispose (GObject *object)
{
  GtkCrusaderVillageMap *self = GTK_CRUSADER_VILLAGE_MAP (object);

  g_clear_pointer (&self->name, g_free);
  g_clear_object (&self->strokes);

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
      break;
    case PROP_HEIGHT:
      self->height = g_value_get_int (value);
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
          16, 16384, 100,
          G_PARAM_READWRITE);

  props[PROP_HEIGHT] =
      g_param_spec_int (
          "height",
          "Height",
          "The height of the map",
          16, 16384, 100,
          G_PARAM_READWRITE);

  props[PROP_STROKES] =
      g_param_spec_object (
          "strokes",
          "Strokes",
          "A list store containing the ordered item strokes",
          G_TYPE_LIST_STORE,
          G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
gtk_crusader_village_map_init (GtkCrusaderVillageMap *self)
{
  self->width   = 100;
  self->height  = 100;
  self->strokes = g_list_store_new (GTK_CRUSADER_VILLAGE_TYPE_ITEM_STROKE);
}

void
gtk_crusader_village_map_new_from_aiv_file_async (GFile                       *file,
                                                  GtkCrusaderVillageItemStore *store,
                                                  const char                  *python_exe,
                                                  int                          io_priority,
                                                  GCancellable                *cancellable,
                                                  GAsyncReadyCallback          callback,
                                                  gpointer                     user_data)
{
  g_autoptr (GTask) task = NULL;
  LoadData *data         = NULL;

  g_return_if_fail (G_IS_FILE (file));
  g_return_if_fail (GTK_CRUSADER_VILLAGE_IS_ITEM_STORE (store));
  g_return_if_fail (python_exe != NULL);

  data             = g_new0 (typeof (*data), 1);
  data->store      = gtk_crusader_village_item_store_dup (store);
  data->python_exe = g_strdup (python_exe);

  task = g_task_new (file, cancellable, callback, user_data);
  g_task_set_source_tag (task, gtk_crusader_village_map_new_from_aiv_file_async);
  g_task_set_task_data (task, data, destroy_load_data);
  g_task_set_priority (task, io_priority);
  g_task_set_check_cancellable (task, TRUE);
  g_task_run_in_thread (task, new_from_aiv_file_async_thread);
}

GtkCrusaderVillageMap *
gtk_crusader_village_map_new_from_aiv_file_finish (GAsyncResult *result,
                                                   GError      **error)
{
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) ==
                            gtk_crusader_village_map_new_from_aiv_file_async,
                        NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

void
gtk_crusader_village_map_save_to_aiv_file_async (GtkCrusaderVillageMap *self,
                                                 GFile                 *file,
                                                 const char            *python_exe,
                                                 int                    io_priority,
                                                 GCancellable          *cancellable,
                                                 GAsyncReadyCallback    callback,
                                                 gpointer               user_data)
{
  g_autoptr (GTask) task = NULL;
  SaveData *data         = NULL;

  g_return_if_fail (GTK_CRUSADER_VILLAGE_IS_MAP (self));
  g_return_if_fail (G_IS_FILE (file));
  g_return_if_fail (python_exe != NULL);

  data             = g_new0 (typeof (*data), 1);
  data->strokes    = g_ptr_array_new_with_free_func (g_object_unref);
  data->python_exe = g_strdup (python_exe);

  g_ptr_array_set_size (data->strokes, g_list_model_get_n_items (G_LIST_MODEL (self->strokes)));
  for (guint i = 0; i < data->strokes->len; i++)
    g_ptr_array_index (data->strokes, i) = g_list_model_get_item (G_LIST_MODEL (self->strokes), i);

  task = g_task_new (file, cancellable, callback, user_data);
  g_task_set_source_tag (task, gtk_crusader_village_map_save_to_aiv_file_async);
  g_task_set_task_data (task, data, destroy_save_data);
  g_task_set_priority (task, io_priority);
  g_task_set_check_cancellable (task, TRUE);
  g_task_run_in_thread (task, save_to_aiv_file_async_thread);
}

gboolean
gtk_crusader_village_map_save_to_aiv_file_finish (GAsyncResult *result,
                                                  GError      **error)
{
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) ==
                            gtk_crusader_village_map_save_to_aiv_file_async,
                        FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
destroy_load_data (gpointer data)
{
  LoadData *self = data;

  g_clear_object (&self->store);
  g_clear_pointer (&self->python_exe, g_free);
  g_free (self);
}

static void
destroy_save_data (gpointer data)
{
  SaveData *self = data;

  g_clear_pointer (&self->strokes, g_ptr_array_unref);
  g_clear_pointer (&self->python_exe, g_free);
  g_free (self);
}

static void
new_from_aiv_file_async_thread (GTask        *task,
                                gpointer      object,
                                gpointer      task_data,
                                GCancellable *cancellable)
{
  GFile    *file                              = object;
  LoadData *data                              = task_data;
  g_autoptr (GtkCrusaderVillageMap) map       = NULL;
  g_autoptr (GError) local_error              = NULL;
  g_autoptr (GFile) tmp_file                  = NULL;
  g_autoptr (GFileIOStream) tmp_file_iostream = NULL;
  g_autofree char *aiv_file_path              = NULL;
  g_autofree char *tmp_file_path              = NULL;
  g_autoptr (GSubprocess) sourcehold          = NULL;
  g_autoptr (GFileInputStream) stream         = NULL;
  g_autoptr (JsonParser) parser               = NULL;
  gboolean  parse_result                      = FALSE;
  JsonNode *root                              = NULL;
  g_autoptr (GVariant) variant                = NULL;
  g_autoptr (GVariantIter) frames_iter        = NULL;
  gint64 pause_delay_amount                   = 0;

  if (g_task_return_error_if_cancelled (task))
    return;

  map       = g_object_new (GTK_CRUSADER_VILLAGE_TYPE_MAP, NULL);
  map->name = g_file_get_basename (file);

  tmp_file = g_file_new_tmp (NULL, &tmp_file_iostream, &local_error);
  if (tmp_file == NULL)
    goto err;
  if (!g_io_stream_close (G_IO_STREAM (tmp_file_iostream), cancellable, &local_error))
    goto err;

  aiv_file_path = g_file_get_path (file);
  tmp_file_path = g_file_get_path (tmp_file);

  sourcehold = g_subprocess_new (
      G_SUBPROCESS_FLAGS_NONE,
      &local_error,
      data->python_exe, "-m", "sourcehold", "convert", "aiv", "--input", aiv_file_path, "--output", tmp_file_path,
      NULL);
  if (sourcehold == NULL)
    goto err;
  if (!g_subprocess_wait (sourcehold, cancellable, &local_error))
    goto err;
  if (!g_subprocess_get_successful (sourcehold))
    goto err_sourcehold;

  stream = g_file_read (tmp_file, cancellable, &local_error);
  if (stream == NULL)
    goto err;

  parser       = json_parser_new_immutable ();
  parse_result = json_parser_load_from_stream (parser, G_INPUT_STREAM (stream), cancellable, &local_error);
  if (!parse_result)
    goto err;

  root    = json_parser_get_root (parser);
  variant = json_gvariant_deserialize (root, NULL, &local_error);
  if (variant == NULL)
    goto err;
  variant = g_variant_ref_sink (variant);

  if (!g_variant_lookup (variant, "pauseDelayAmount", "x", &pause_delay_amount))
    goto err_inval;
  if (!g_variant_lookup (variant, "frames", "av", &frames_iter))
    goto err_inval;

  for (;;)
    {
      g_autoptr (GVariant) frame                      = NULL;
      gint64 id                                       = 0;
      g_autoptr (GtkCrusaderVillageItem) item         = NULL;
      int item_tile_width                             = 0;
      int item_tile_height                            = 0;
      int item_tile_offset_x                          = 0;
      int item_tile_offset_y                          = 0;
      g_autoptr (GVariantIter) instances_iter         = NULL;
      g_autoptr (GtkCrusaderVillageItemStroke) stroke = NULL;

      if (!g_variant_iter_next (frames_iter, "v", &frame))
        break;
      if (!g_variant_is_of_type (frame, G_VARIANT_TYPE_DICTIONARY))
        goto err_inval;

      if (!g_variant_lookup (frame, "itemType", "x", &id))
        goto err_inval;
      item = gtk_crusader_village_item_store_query_id (data->store, id);
      if (item == NULL)
        continue;
      g_object_get (
          item,
          "tile-width", &item_tile_width,
          "tile-height", &item_tile_height,
          "tile-offset-x", &item_tile_offset_x,
          "tile-offset-y", &item_tile_offset_y,
          NULL);

      if (!g_variant_lookup (frame, "tilePositionOfsets", "av", &instances_iter))
        goto err_inval;

      stroke = g_object_new (
          GTK_CRUSADER_VILLAGE_TYPE_ITEM_STROKE,
          "item", item,
          NULL);
      for (;;)
        {
          g_autoptr (GVariant) tile_variant = NULL;
          gint64 tile                       = 0;

          if (!g_variant_iter_next (instances_iter, "v", &tile_variant))
            break;
          if (!g_variant_is_of_type (tile_variant, G_VARIANT_TYPE_INT64))
            goto err_inval;

          tile = g_variant_get_int64 (tile_variant);
          gtk_crusader_village_item_stroke_add_instance (
              stroke,
              (GtkCrusaderVillageItemStrokeInstance) {
                  tile % 100,
                  tile / 100,
              });
        }

      g_list_store_append (G_LIST_STORE (map->strokes), stroke);
    }

  g_task_return_pointer (task, g_steal_pointer (&map), g_object_unref);
  goto done;

err:
  g_task_return_error (task, g_steal_pointer (&local_error));
  goto done;

err_sourcehold:
  g_task_return_new_error_literal (
      task,
      GTK_CRUSADER_VILLAGE_MAP_ERROR,
      GTK_CRUSADER_VILLAGE_ERROR_SOURCEHOLD_FAILED,
      "The Sourcehold process terminated abnormally");
  goto done;

err_inval:
  g_task_return_new_error_literal (
      task,
      GTK_CRUSADER_VILLAGE_MAP_ERROR,
      GTK_CRUSADER_VILLAGE_ERROR_INVALID_JSON_STRUCTURE,
      "JSON structure is invalid");
  goto done;

done:
  if (tmp_file != NULL)
    g_file_delete (tmp_file, cancellable, NULL);
}

static void
save_to_aiv_file_async_thread (GTask        *task,
                               gpointer      object,
                               gpointer      task_data,
                               GCancellable *cancellable)
{
  GFile    *file                              = object;
  SaveData *data                              = task_data;
  g_autoptr (GError) local_error              = NULL;
  g_autoptr (GFile) tmp_file                  = NULL;
  g_autoptr (GFileIOStream) tmp_file_iostream = NULL;
  GOutputStream   *tmp_file_outstream         = NULL;
  g_autofree char *aiv_file_path              = NULL;
  g_autofree char *tmp_file_path              = NULL;
  g_autoptr (GSubprocess) sourcehold          = NULL;

  if (g_task_return_error_if_cancelled (task))
    return;

  tmp_file = g_file_new_tmp ("XXXXXX.json", &tmp_file_iostream, &local_error);
  if (tmp_file == NULL)
    goto err;
  tmp_file_outstream = g_io_stream_get_output_stream (G_IO_STREAM (tmp_file_iostream));

#define WRITE(...)                                                         \
  G_STMT_START                                                             \
  {                                                                        \
    gboolean result = g_output_stream_printf (                             \
        tmp_file_outstream, NULL, cancellable, &local_error, __VA_ARGS__); \
    if (!result)                                                           \
      goto err;                                                            \
  }                                                                        \
  G_STMT_END

  WRITE ("{\"frames\":[");
  if (data->strokes->len > 0)
    {
      for (guint i = 0; i < data->strokes->len; i++)
        {
          GtkCrusaderVillageItemStroke *stroke    = NULL;
          g_autoptr (GtkCrusaderVillageItem) item = NULL;
          g_autoptr (GArray) instances            = NULL;
          int id                                  = 0;

          stroke = g_ptr_array_index (data->strokes, i);
          g_object_get (
              stroke,
              "item", &item,
              "instances", &instances,
              NULL);
          g_object_get (
              item,
              "id", &id,
              NULL);

          WRITE ("{\"itemType\":%d,\"tilePositionOfsets\":[", id);
          if (instances->len > 0)
            {
              for (guint j = 0; j < instances->len; j++)
                {
                  GtkCrusaderVillageItemStrokeInstance *instance = NULL;

                  instance = &g_array_index (instances, GtkCrusaderVillageItemStrokeInstance, j);
                  WRITE (j < instances->len - 1 ? "%d," : "%d", 100 * instance->y + instance->x);
                }
            }
          WRITE (i < data->strokes->len - 1 ? "],\"shouldPause\":false}," : "],\"shouldPause\":false}");
        }
    }
  WRITE ("],\"miscItems\":[],\"pauseDelayAmount\": 100}");

  WRITE ("\n");
#undef WRITE

  if (!g_io_stream_close (G_IO_STREAM (tmp_file_iostream),
                          cancellable, &local_error))
    goto err;

  aiv_file_path = g_file_get_path (file);
  tmp_file_path = g_file_get_path (tmp_file);

  if (!g_str_has_suffix (aiv_file_path, ".aiv"))
    {
      char *tmp     = aiv_file_path;
      aiv_file_path = g_strdup_printf ("%s.aiv", aiv_file_path);
      g_free (tmp);
    }

  sourcehold = g_subprocess_new (
      G_SUBPROCESS_FLAGS_NONE,
      &local_error,
      data->python_exe, "-m", "sourcehold", "convert", "aiv", "--input", tmp_file_path, "--output", aiv_file_path,
      NULL);
  if (sourcehold == NULL)
    goto err;
  if (!g_subprocess_wait (sourcehold, cancellable, &local_error))
    goto err;
  if (!g_subprocess_get_successful (sourcehold))
    goto err_sourcehold;

  g_task_return_boolean (task, TRUE);
  goto done;

err:
  g_task_return_error (task, g_steal_pointer (&local_error));
  goto done;

err_sourcehold:
  g_task_return_new_error_literal (
      task,
      GTK_CRUSADER_VILLAGE_MAP_ERROR,
      GTK_CRUSADER_VILLAGE_ERROR_SOURCEHOLD_FAILED,
      "The Sourcehold process terminated abnormally");
  goto done;

done:
  if (tmp_file != NULL)
    g_file_delete (tmp_file, cancellable, NULL);
}
