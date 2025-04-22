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

struct _GcvItemArea
{
  GcvUtilBin parent_instance;

  GSettings    *settings;
  GcvItemStore *item_store;

  GVariant *frequencies;

  /* Template widgets */
  GtkEntry    *entry;
  GtkListView *list_view;

  GtkToggleButton *all;
  GtkToggleButton *castle;
  GtkToggleButton *industry;
  GtkToggleButton *farm;
  GtkToggleButton *town;
  GtkToggleButton *weapons;
  GtkToggleButton *food;
  GtkToggleButton *euro;
  GtkToggleButton *arab;
};

G_DEFINE_FINAL_TYPE (GcvItemArea, gcv_item_area, GCV_TYPE_UTIL_BIN)

enum
{
  PROP_0,

  PROP_SETTINGS,
  PROP_ITEM_STORE,
  PROP_SELECTED_ITEM,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
item_frequencies_changed (GSettings   *self,
                          gchar       *key,
                          GcvItemArea *item_area);

static void
setup_listitem (GtkListItemFactory *factory,
                GtkListItem        *list_item,
                GcvItemArea        *item_area);

static void
bind_listitem (GtkListItemFactory *factory,
               GtkListItem        *list_item,
               GcvItemArea        *item_area);

static void
search_changed (GtkEditable *self,
                GcvItemArea *item_area);

static gint
cmp_item (GcvItem     *a,
          GcvItem     *b,
          GcvItemArea *item_area);

static int
match (GcvItem     *item,
       GcvItemArea *item_area);

static void
active_group_changed (GtkToggleButton *toggle_button,
                      GParamSpec      *pspec,
                      GcvItemArea     *item_area);

static void
selected_item_changed (GtkSingleSelection *selection,
                       GParamSpec         *pspec,
                       GcvItemArea        *item_area);

static void
read_frequencies (GcvItemArea *self);

static void
update_sorter (GcvItemArea *self);

static void
update_filter (GcvItemArea *self);

static void
gcv_item_area_dispose (GObject *object)
{
  GcvItemArea *self = GCV_ITEM_AREA (object);

  if (self->settings != NULL)
    g_signal_handlers_disconnect_by_func (
        self->settings, item_frequencies_changed, self);
  g_clear_object (&self->settings);
  g_clear_object (&self->item_store);

  g_clear_pointer (&self->frequencies, g_variant_unref);

  G_OBJECT_CLASS (gcv_item_area_parent_class)->dispose (object);
}

static void
gcv_item_area_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GcvItemArea *self = GCV_ITEM_AREA (object);

  switch (prop_id)
    {
    case PROP_SETTINGS:
      g_value_set_object (value, self->settings);
      break;
    case PROP_ITEM_STORE:
      g_value_set_object (value, self->item_store);
      break;
    case PROP_SELECTED_ITEM:
      {
        GtkSingleSelection *single_selection_model = NULL;
        GcvItem            *item                   = NULL;

        single_selection_model = GTK_SINGLE_SELECTION (gtk_list_view_get_model (self->list_view));
        item                   = GCV_ITEM (gtk_single_selection_get_selected_item (single_selection_model));

        g_value_set_object (value, item);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gcv_item_area_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GcvItemArea *self = GCV_ITEM_AREA (object);

  switch (prop_id)
    {
    case PROP_SETTINGS:
      if (self->settings != NULL)
        g_signal_handlers_disconnect_by_func (
            self->settings, item_frequencies_changed, self);
      g_clear_object (&self->settings);
      self->settings = g_value_dup_object (value);
      if (self->settings != NULL)
        g_signal_connect (self->settings, "changed::item-frequencies",
                          G_CALLBACK (item_frequencies_changed), self);
      read_frequencies (self);
      break;
    case PROP_ITEM_STORE:
      {
        GtkSingleSelection *selection_model   = NULL;
        GtkSortListModel   *sort_list_model   = NULL;
        GtkFilterListModel *filter_list_model = NULL;

        selection_model   = GTK_SINGLE_SELECTION (gtk_list_view_get_model (self->list_view));
        sort_list_model   = GTK_SORT_LIST_MODEL (gtk_single_selection_get_model (selection_model));
        filter_list_model = GTK_FILTER_LIST_MODEL (gtk_sort_list_model_get_model (sort_list_model));

        g_clear_object (&self->item_store);
        self->item_store = g_value_dup_object (value);
        gtk_filter_list_model_set_model (
            filter_list_model,
            self->item_store != NULL
                ? G_LIST_MODEL (self->item_store)
                : NULL);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gcv_item_area_class_init (GcvItemAreaClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gcv_item_area_dispose;
  object_class->get_property = gcv_item_area_get_property;
  object_class->set_property = gcv_item_area_set_property;

  props[PROP_SETTINGS] =
      g_param_spec_object (
          "settings",
          "Settings",
          "The settings object from which to retrieve recency sorting info",
          G_TYPE_SETTINGS,
          G_PARAM_READWRITE);

  props[PROP_ITEM_STORE] =
      g_param_spec_object (
          "item-store",
          "Item Store",
          "The item store this widget should reflect",
          GCV_TYPE_ITEM_STORE,
          G_PARAM_READWRITE);

  props[PROP_SELECTED_ITEM] =
      g_param_spec_object (
          "selected-item",
          "Selected Item",
          "The currently selected item in the list",
          GCV_TYPE_ITEM,
          G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/Gcv/gtk-crusader-village-item-area.ui");
  gtk_widget_class_bind_template_child (widget_class, GcvItemArea, entry);
  gtk_widget_class_bind_template_child (widget_class, GcvItemArea, list_view);

  gtk_widget_class_bind_template_child (widget_class, GcvItemArea, all);
  gtk_widget_class_bind_template_child (widget_class, GcvItemArea, castle);
  gtk_widget_class_bind_template_child (widget_class, GcvItemArea, industry);
  gtk_widget_class_bind_template_child (widget_class, GcvItemArea, farm);
  gtk_widget_class_bind_template_child (widget_class, GcvItemArea, town);
  gtk_widget_class_bind_template_child (widget_class, GcvItemArea, weapons);
  gtk_widget_class_bind_template_child (widget_class, GcvItemArea, food);
  gtk_widget_class_bind_template_child (widget_class, GcvItemArea, euro);
  gtk_widget_class_bind_template_child (widget_class, GcvItemArea, arab);
}

static void
gcv_item_area_init (GcvItemArea *self)
{
  GtkCustomFilter    *custom_filter   = NULL;
  GtkFilterListModel *filter_model    = NULL;
  GtkCustomSorter    *custom_sorter   = NULL;
  GtkSortListModel   *sort_model      = NULL;
  GtkSelectionModel  *selection_model = NULL;
  GtkListItemFactory *factory         = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  custom_filter = gtk_custom_filter_new ((GtkCustomFilterFunc) match, self, NULL);
  filter_model  = gtk_filter_list_model_new (NULL, GTK_FILTER (custom_filter));

  custom_sorter = gtk_custom_sorter_new ((GCompareDataFunc) cmp_item, self, NULL);
  sort_model    = gtk_sort_list_model_new (G_LIST_MODEL (filter_model), GTK_SORTER (custom_sorter));

  selection_model = GTK_SELECTION_MODEL (gtk_single_selection_new (G_LIST_MODEL (sort_model)));
  factory         = gtk_signal_list_item_factory_new ();

  g_signal_connect (factory, "setup", G_CALLBACK (setup_listitem), self);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_listitem), self);
  gtk_list_view_set_factory (self->list_view, factory);
  gtk_list_view_set_model (self->list_view, selection_model);

  g_signal_connect (self->entry, "changed", G_CALLBACK (search_changed), self);

  g_signal_connect (selection_model, "notify::selected-item",
                    G_CALLBACK (selected_item_changed), self);

  gtk_toggle_button_set_group (self->castle, self->all);
  gtk_toggle_button_set_group (self->industry, self->all);
  gtk_toggle_button_set_group (self->farm, self->all);
  gtk_toggle_button_set_group (self->town, self->all);
  gtk_toggle_button_set_group (self->weapons, self->all);
  gtk_toggle_button_set_group (self->food, self->all);
  gtk_toggle_button_set_group (self->euro, self->all);
  gtk_toggle_button_set_group (self->arab, self->all);

  gtk_toggle_button_set_active (self->all, TRUE);

  g_signal_connect (self->all, "notify::active", G_CALLBACK (active_group_changed), self);
  g_signal_connect (self->castle, "notify::active", G_CALLBACK (active_group_changed), self);
  g_signal_connect (self->industry, "notify::active", G_CALLBACK (active_group_changed), self);
  g_signal_connect (self->farm, "notify::active", G_CALLBACK (active_group_changed), self);
  g_signal_connect (self->town, "notify::active", G_CALLBACK (active_group_changed), self);
  g_signal_connect (self->weapons, "notify::active", G_CALLBACK (active_group_changed), self);
  g_signal_connect (self->food, "notify::active", G_CALLBACK (active_group_changed), self);
  g_signal_connect (self->euro, "notify::active", G_CALLBACK (active_group_changed), self);
  g_signal_connect (self->arab, "notify::active", G_CALLBACK (active_group_changed), self);
}

static void
item_frequencies_changed (GSettings   *self,
                          gchar       *key,
                          GcvItemArea *item_area)
{
  read_frequencies (item_area);
}

static void
setup_listitem (GtkListItemFactory *factory,
                GtkListItem        *list_item,
                GcvItemArea        *item_area)
{
  GcvItemAreaItem *area_item = NULL;

  area_item = g_object_new (GCV_TYPE_ITEM_AREA_ITEM, NULL);
  gtk_list_item_set_child (list_item, GTK_WIDGET (area_item));
}

static void
bind_listitem (GtkListItemFactory *factory,
               GtkListItem        *list_item,
               GcvItemArea        *item_area)
{
  GcvItem         *item      = NULL;
  GcvItemAreaItem *area_item = NULL;

  item      = GCV_ITEM (gtk_list_item_get_item (list_item));
  area_item = GCV_ITEM_AREA_ITEM (gtk_list_item_get_child (list_item));

  g_object_set (
      area_item,
      "item", item,
      NULL);
}

static void
search_changed (GtkEditable *editable,
                GcvItemArea *item_area)
{
  update_filter (item_area);
}

static gint
cmp_item (GcvItem     *a,
          GcvItem     *b,
          GcvItemArea *item_area)
{
  const char *a_name = NULL;
  const char *b_name = NULL;
  gboolean    a_res  = FALSE;
  gboolean    b_res  = FALSE;
  guint       a_cnt  = 0;
  guint       b_cnt  = 0;

  if (item_area->frequencies == NULL)
    return 0;

  a_name = gcv_item_get_name (a);
  b_name = gcv_item_get_name (b);

  a_res = g_variant_lookup (item_area->frequencies, a_name, "u", &a_cnt);
  b_res = g_variant_lookup (item_area->frequencies, b_name, "u", &b_cnt);

  if (!a_res && !b_res)
    return 0;
  else if (a_res && !b_res)
    return -1;
  else if (!a_res && b_res)
    return 1;
  else if (a_cnt == b_cnt)
    return 0;
  else if (a_cnt > b_cnt)
    return -1;
  else
    return 1;
}

static int
match (GcvItem     *item,
       GcvItemArea *item_area)
{
  const char      *search_text           = NULL;
  g_autofree char *name                  = NULL;
  g_autofree char *section_icon_resource = NULL;

  search_text = gtk_editable_get_text (GTK_EDITABLE (item_area->entry));
  if (search_text == NULL)
    return 0;

  g_object_get (
      item,
      "name", &name,
      "section-icon-resource", &section_icon_resource,
      NULL);

  if (!gtk_toggle_button_get_active (item_area->all) &&
      ((!gtk_toggle_button_get_active (item_area->castle) &&
        strstr (section_icon_resource, "tower.png") != NULL) ||
       (!gtk_toggle_button_get_active (item_area->industry) &&
        strstr (section_icon_resource, "hammer.png") != NULL) ||
       (!gtk_toggle_button_get_active (item_area->farm) &&
        strstr (section_icon_resource, "apple.png") != NULL) ||
       (!gtk_toggle_button_get_active (item_area->food) &&
        strstr (section_icon_resource, "sickle.png") != NULL) ||
       (!gtk_toggle_button_get_active (item_area->town) &&
        strstr (section_icon_resource, "house.png") != NULL) ||
       (!gtk_toggle_button_get_active (item_area->weapons) &&
        strstr (section_icon_resource, "shield.png") != NULL) ||
       (!gtk_toggle_button_get_active (item_area->euro) &&
        strstr (section_icon_resource, "euro_sword.png") != NULL) ||
       (!gtk_toggle_button_get_active (item_area->arab) &&
        strstr (section_icon_resource, "arab_sword.png") != NULL)))
    return 0;

  return g_str_match_string (search_text, name, TRUE) ? 1 : 0;
}

static void
selected_item_changed (GtkSingleSelection *selection,
                       GParamSpec         *pspec,
                       GcvItemArea        *item_area)
{
  g_object_notify_by_pspec (G_OBJECT (item_area), props[PROP_SELECTED_ITEM]);
}

static void
active_group_changed (GtkToggleButton *toggle_button,
                      GParamSpec      *pspec,
                      GcvItemArea     *item_area)
{
  if (gtk_toggle_button_get_active (toggle_button))
    update_filter (item_area);
}

static void
read_frequencies (GcvItemArea *self)
{
  g_clear_pointer (&self->frequencies, g_variant_unref);
  self->frequencies = g_settings_get_value (self->settings, "item-frequencies");
  update_sorter (self);
}

static void
update_sorter (GcvItemArea *self)
{
  GtkSingleSelection *selection_model = NULL;
  GtkSortListModel   *sort_list_model = NULL;
  GtkSorter          *sorter          = NULL;

  selection_model = GTK_SINGLE_SELECTION (gtk_list_view_get_model (self->list_view));
  sort_list_model = GTK_SORT_LIST_MODEL (gtk_single_selection_get_model (selection_model));
  sorter          = gtk_sort_list_model_get_sorter (sort_list_model);

  gtk_sorter_changed (sorter, GTK_SORTER_CHANGE_DIFFERENT);
}

static void
update_filter (GcvItemArea *self)
{
  GtkSingleSelection *selection_model   = NULL;
  GtkSortListModel   *sort_list_model   = NULL;
  GtkFilterListModel *filter_list_model = NULL;
  GtkFilter          *filter            = NULL;

  selection_model   = GTK_SINGLE_SELECTION (gtk_list_view_get_model (self->list_view));
  sort_list_model   = GTK_SORT_LIST_MODEL (gtk_single_selection_get_model (selection_model));
  filter_list_model = GTK_FILTER_LIST_MODEL (gtk_sort_list_model_get_model (sort_list_model));
  filter            = gtk_filter_list_model_get_filter (filter_list_model);

  gtk_filter_changed (filter, GTK_FILTER_CHANGE_DIFFERENT);
}

void
gcv_item_area_grab_search_focus (GcvItemArea *self)
{
  g_return_if_fail (GCV_IS_ITEM_AREA (self));

  gtk_widget_grab_focus (GTK_WIDGET (self->entry));
}
