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
#include "gtk-crusader-village-item.h"

struct _GtkCrusaderVillageItemStore
{
  GObject parent_instance;

  GListStore *list_store;
};

static void list_model_iface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (
    GtkCrusaderVillageItemStore,
    gtk_crusader_village_item_store,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_model_iface_init))

static void
gtk_crusader_village_item_store_dispose (GObject *object)
{
  GtkCrusaderVillageItemStore *self = GTK_CRUSADER_VILLAGE_ITEM_STORE (object);

  g_clear_object (&self->list_store);

  G_OBJECT_CLASS (gtk_crusader_village_item_store_parent_class)->dispose (object);
}

static void
gtk_crusader_village_item_store_class_init (GtkCrusaderVillageItemStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gtk_crusader_village_item_store_dispose;
}

static void
gtk_crusader_village_item_store_init (GtkCrusaderVillageItemStore *self)
{
  self->list_store = g_list_store_new (GTK_CRUSADER_VILLAGE_TYPE_ITEM);
}

static GType
list_model_get_item_type (GListModel *list)
{
  GtkCrusaderVillageItemStore *self = GTK_CRUSADER_VILLAGE_ITEM_STORE (list);

  return GTK_CRUSADER_VILLAGE_TYPE_ITEM;
}

static guint
list_model_get_n_items (GListModel *list)
{
  GtkCrusaderVillageItemStore *self = GTK_CRUSADER_VILLAGE_ITEM_STORE (list);

  return g_list_model_get_n_items (G_LIST_MODEL (self->list_store));
}

static gpointer
list_model_get_item (GListModel *list,
                     guint       position)
{
  GtkCrusaderVillageItemStore *self = GTK_CRUSADER_VILLAGE_ITEM_STORE (list);

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
gtk_crusader_village_item_store_read_resources (GtkCrusaderVillageItemStore *self)
{
  g_auto (GStrv) children = NULL;

#define ITEMS_PATH "/am/kolunmi/GtkCrusaderVillage/items/"
  children = g_resources_enumerate_children (ITEMS_PATH, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  if (children == NULL)
    return;

  for (char **path = children; *path != NULL; path++)
    {
      g_autofree char *full_path              = NULL;
      g_autoptr (GError) local_error          = NULL;
      g_autoptr (GtkCrusaderVillageItem) item = NULL;

      full_path = g_strdup_printf (ITEMS_PATH "%s", *path);

      item = gtk_crusader_village_item_new_for_resource (full_path, &local_error);
      if (item == NULL)
        {
          g_critical ("Could not load internal item spec at %s: %s", full_path, local_error->message);
          continue;
        }

      g_list_store_append (self->list_store, item);
    }
}
