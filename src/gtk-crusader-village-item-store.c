/* gtk-crusader-village-item-store.c
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

#include "gtk-crusader-village-item-store.h"

struct _GcvItemStore
{
  GObject parent_instance;

  GListStore *list_store;
  GHashTable *id_map;
};

static void list_model_iface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (
    GcvItemStore,
    gcv_item_store,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_model_iface_init))

static void
gcv_item_store_dispose (GObject *object)
{
  GcvItemStore *self = GCV_ITEM_STORE (object);

  g_clear_object (&self->list_store);
  g_clear_pointer (&self->id_map, g_hash_table_unref);

  G_OBJECT_CLASS (gcv_item_store_parent_class)->dispose (object);
}

static void
gcv_item_store_class_init (GcvItemStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gcv_item_store_dispose;
}

static void
gcv_item_store_init (GcvItemStore *self)
{
  self->list_store = g_list_store_new (GCV_TYPE_ITEM);
  self->id_map     = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static GType
list_model_get_item_type (GListModel *list)
{
  GcvItemStore *self = GCV_ITEM_STORE (list);

  return GCV_TYPE_ITEM;
}

static guint
list_model_get_n_items (GListModel *list)
{
  GcvItemStore *self = GCV_ITEM_STORE (list);

  return g_list_model_get_n_items (G_LIST_MODEL (self->list_store));
}

static gpointer
list_model_get_item (GListModel *list,
                     guint       position)
{
  GcvItemStore *self = GCV_ITEM_STORE (list);

  return g_list_model_get_item (G_LIST_MODEL (self->list_store), position);
}

static void
list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = list_model_get_item_type;
  iface->get_n_items   = list_model_get_n_items;
  iface->get_item      = list_model_get_item;
}

void
gcv_item_store_read_resources (GcvItemStore *self)
{
  g_auto (GStrv) children = NULL;

  g_return_if_fail (GCV_IS_ITEM_STORE (self));

#define ITEMS_PATH "/am/kolunmi/Gcv/items/"
  children = g_resources_enumerate_children (ITEMS_PATH, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  if (children == NULL)
    return;

  for (char **path = children; *path != NULL; path++)
    {
      g_autofree char *full_path     = NULL;
      g_autoptr (GError) local_error = NULL;
      g_autoptr (GcvItem) item       = NULL;
      GcvItemKind kind               = 0;
      int         id                 = 0;

      full_path = g_strdup_printf (ITEMS_PATH "%s", *path);

      item = gcv_item_new_for_resource (full_path, &local_error);
      if (item == NULL)
        {
          g_critical ("Could not load internal item spec at %s: %s", full_path, local_error->message);
          continue;
        }

      g_list_store_append (self->list_store, item);

      g_object_get (
          item,
          "kind", &kind,
          "id", &id,
          NULL);

      g_hash_table_replace (self->id_map, GINT_TO_POINTER (id), item);
    }
}

GcvItem *
gcv_item_store_query_id (GcvItemStore *self,
                         int           id)
{
  GcvItem *item = NULL;

  g_return_val_if_fail (GCV_IS_ITEM_STORE (self), NULL);
  g_return_val_if_fail (id > 0, NULL);

  item = g_hash_table_lookup (self->id_map, GINT_TO_POINTER (id));

  return item != NULL ? g_object_ref (item) : NULL;
}

GcvItemStore *
gcv_item_store_dup (GcvItemStore *self)
{
  GcvItemStore *dup     = NULL;
  guint         n_items = 0;

  g_return_val_if_fail (GCV_IS_ITEM_STORE (self), NULL);

  dup     = g_object_new (GCV_TYPE_ITEM_STORE, NULL);
  n_items = g_list_model_get_n_items (G_LIST_MODEL (self));

  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr (GcvItem) item = NULL;
      GcvItemKind kind         = 0;
      int         id           = 0;

      item = g_list_model_get_item (G_LIST_MODEL (self), i);
      g_list_store_append (dup->list_store, item);

      g_object_get (
          item,
          "kind", &kind,
          "id", &id,
          NULL);

      g_hash_table_replace (dup->id_map, GINT_TO_POINTER (id), item);
    }

  return dup;
}
