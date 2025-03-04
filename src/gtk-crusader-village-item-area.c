/* gtk-crusader-village-item-area.c
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

#include "gtk-crusader-village-item-area-item.h"
#include "gtk-crusader-village-item-area.h"
#include "gtk-crusader-village-item-store.h"
#include "gtk-crusader-village-item.h"

struct _GtkCrusaderVillageItemArea
{
  GtkCrusaderVillageUtilBin parent_instance;

  GtkCrusaderVillageItemStore *item_store;

  /* Template widgets */
  GtkEntry    *entry;
  GtkListView *list_view;
};

G_DEFINE_FINAL_TYPE (GtkCrusaderVillageItemArea, gtk_crusader_village_item_area, GTK_CRUSADER_VILLAGE_TYPE_UTIL_BIN)

enum
{
  PROP_0,

  PROP_ITEM_STORE,
  PROP_SELECTED_ITEM,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void setup_listitem (GtkListItemFactory *factory,
                            GtkListItem        *list_item,
                            gpointer            user_data);
static void bind_listitem (GtkListItemFactory *factory,
                           GtkListItem        *list_item,
                           gpointer            user_data);
static void search_changed (GtkEditable *self,
                            gpointer     user_data);

static int match (GtkCrusaderVillageItem     *item,
                  GtkCrusaderVillageItemArea *item_area);

static void
selected_item_changed (GtkSingleSelection         *selection,
                       GParamSpec                 *pspec,
                       GtkCrusaderVillageItemArea *item_area);

static void
gtk_crusader_village_item_area_dispose (GObject *object)
{
  GtkCrusaderVillageItemArea *self = GTK_CRUSADER_VILLAGE_ITEM_AREA (object);

  g_clear_object (&self->item_store);

  G_OBJECT_CLASS (gtk_crusader_village_item_area_parent_class)->dispose (object);
}

static void
gtk_crusader_village_item_area_get_property (GObject    *object,
                                             guint       prop_id,
                                             GValue     *value,
                                             GParamSpec *pspec)
{
  GtkCrusaderVillageItemArea *self = GTK_CRUSADER_VILLAGE_ITEM_AREA (object);

  switch (prop_id)
    {
    case PROP_ITEM_STORE:
      g_value_set_object (value, self->item_store);
      break;
    case PROP_SELECTED_ITEM:
      {
        GtkSingleSelection     *single_selection_model = NULL;
        GtkCrusaderVillageItem *item                   = NULL;

        single_selection_model = GTK_SINGLE_SELECTION (gtk_list_view_get_model (self->list_view));
        item                   = GTK_CRUSADER_VILLAGE_ITEM (gtk_single_selection_get_selected_item (single_selection_model));

        g_value_set_object (value, item);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_item_area_set_property (GObject      *object,
                                             guint         prop_id,
                                             const GValue *value,
                                             GParamSpec   *pspec)
{
  GtkCrusaderVillageItemArea *self = GTK_CRUSADER_VILLAGE_ITEM_AREA (object);

  switch (prop_id)
    {
    case PROP_ITEM_STORE:
      {
        GtkSingleSelection *single_selection_model = NULL;
        GtkFilterListModel *filter_list_model      = NULL;

        single_selection_model = GTK_SINGLE_SELECTION (gtk_list_view_get_model (self->list_view));
        filter_list_model      = GTK_FILTER_LIST_MODEL (gtk_single_selection_get_model (single_selection_model));
        gtk_filter_list_model_set_model (filter_list_model, NULL);

        g_clear_object (&self->item_store);

        self->item_store = g_value_dup_object (value);
        if (self->item_store != NULL)
          gtk_filter_list_model_set_model (filter_list_model, G_LIST_MODEL (self->item_store));
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_crusader_village_item_area_class_init (GtkCrusaderVillageItemAreaClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gtk_crusader_village_item_area_dispose;
  object_class->get_property = gtk_crusader_village_item_area_get_property;
  object_class->set_property = gtk_crusader_village_item_area_set_property;

  props[PROP_ITEM_STORE] =
      g_param_spec_object (
          "item-store",
          "Item Store",
          "The item store this widget should reflect",
          GTK_CRUSADER_VILLAGE_TYPE_ITEM_STORE,
          G_PARAM_READWRITE);

  props[PROP_SELECTED_ITEM] =
      g_param_spec_object (
          "selected-item",
          "Selected Item",
          "The currently selected item in the list",
          GTK_CRUSADER_VILLAGE_TYPE_ITEM,
          G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/GtkCrusaderVillage/gtk-crusader-village-item-area.ui");
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageItemArea, entry);
  gtk_widget_class_bind_template_child (widget_class, GtkCrusaderVillageItemArea, list_view);
}

static void
gtk_crusader_village_item_area_init (GtkCrusaderVillageItemArea *self)
{
  GtkListItemFactory *factory         = NULL;
  GtkCustomFilter    *custom_filter   = NULL;
  GtkFilterListModel *filter_model    = NULL;
  GtkSelectionModel  *selection_model = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  factory         = gtk_signal_list_item_factory_new ();
  custom_filter   = gtk_custom_filter_new ((GtkCustomFilterFunc) match, self, NULL);
  filter_model    = gtk_filter_list_model_new (NULL, GTK_FILTER (custom_filter));
  selection_model = GTK_SELECTION_MODEL (gtk_single_selection_new (G_LIST_MODEL (filter_model)));

  g_signal_connect (factory, "setup", G_CALLBACK (setup_listitem), self);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_listitem), self);
  gtk_list_view_set_factory (self->list_view, factory);
  gtk_list_view_set_model (self->list_view, selection_model);

  g_signal_connect (self->entry, "changed", G_CALLBACK (search_changed), self);

  g_signal_connect (selection_model, "notify::selected-item",
                    G_CALLBACK (selected_item_changed), self);
}

static void
setup_listitem (GtkListItemFactory *factory,
                GtkListItem        *list_item,
                gpointer            user_data)
{
  GtkCrusaderVillageItemAreaItem *area_item = NULL;

  area_item = g_object_new (GTK_CRUSADER_VILLAGE_TYPE_ITEM_AREA_ITEM, NULL);
  gtk_list_item_set_child (list_item, GTK_WIDGET (area_item));
}

static void
bind_listitem (GtkListItemFactory *factory,
               GtkListItem        *list_item,
               gpointer            user_data)
{
  GtkCrusaderVillageItemArea     *self      = user_data;
  GtkCrusaderVillageItem         *item      = NULL;
  GtkCrusaderVillageItemAreaItem *area_item = NULL;

  item      = GTK_CRUSADER_VILLAGE_ITEM (gtk_list_item_get_item (list_item));
  area_item = GTK_CRUSADER_VILLAGE_ITEM_AREA_ITEM (gtk_list_item_get_child (list_item));

  g_object_set (
      area_item,
      "item", item,
      NULL);
}

static void
search_changed (GtkEditable *editable,
                gpointer     user_data)
{
  GtkCrusaderVillageItemArea *self              = user_data;
  GtkSingleSelection         *selection_model   = NULL;
  GtkFilterListModel         *filter_list_model = NULL;
  GtkFilter                  *filter            = NULL;

  selection_model   = GTK_SINGLE_SELECTION (gtk_list_view_get_model (self->list_view));
  filter_list_model = GTK_FILTER_LIST_MODEL (gtk_single_selection_get_model (selection_model));
  filter            = gtk_filter_list_model_get_filter (filter_list_model);

  gtk_filter_changed (filter, GTK_FILTER_CHANGE_DIFFERENT);
}

static int
match (GtkCrusaderVillageItem     *item,
       GtkCrusaderVillageItemArea *item_area)
{
  const char      *search_text = NULL;
  g_autofree char *name        = NULL;

  search_text = gtk_editable_get_text (GTK_EDITABLE (item_area->entry));
  if (search_text == NULL)
    return 0;

  g_object_get (
      item,
      "name", &name,
      NULL);

  return strcasestr (name, search_text) != NULL ? 1 : 0;
}

static void
selected_item_changed (GtkSingleSelection         *selection,
                       GParamSpec                 *pspec,
                       GtkCrusaderVillageItemArea *item_area)
{
  g_object_notify_by_pspec (G_OBJECT (item_area), props[PROP_SELECTED_ITEM]);
}
