/* gtk-crusader-village-item-area-item.c
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
#include "gtk-crusader-village-item.h"

struct _GcvItemAreaItem
{
  GcvUtilBin parent_instance;

  GcvItem *item;

  /* Template widgets */
  GtkLabel   *label;
  GtkPicture *picture;
  GtkPicture *icon;
  GtkLabel   *kind;
};

G_DEFINE_FINAL_TYPE (GcvItemAreaItem, gcv_item_area_item, GCV_TYPE_UTIL_BIN)

enum
{
  PROP_0,

  PROP_ITEM,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
gcv_item_area_item_dispose (GObject *object)
{
  GcvItemAreaItem *self = GCV_ITEM_AREA_ITEM (object);

  g_clear_object (&self->item);

  G_OBJECT_CLASS (gcv_item_area_item_parent_class)->dispose (object);
}

static void
gcv_item_area_item_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GcvItemAreaItem *self = GCV_ITEM_AREA_ITEM (object);

  switch (prop_id)
    {
    case PROP_ITEM:
      g_value_set_object (value, self->item);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gcv_item_area_item_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GcvItemAreaItem *self = GCV_ITEM_AREA_ITEM (object);

  switch (prop_id)
    {
    case PROP_ITEM:
      {
        g_clear_object (&self->item);
        self->item = g_value_dup_object (value);

        if (self->item != NULL)
          {
            g_autofree char *name                  = NULL;
            g_autofree char *description           = NULL;
            g_autofree char *thumbnail_resource    = NULL;
            g_autofree char *section_icon_resource = NULL;
            GcvItemKind      kind                  = GCV_ITEM_KIND_BUILDING;
            const char      *kind_string           = NULL;

            g_object_get (
                self->item,
                "name", &name,
                "description", &description,
                "thumbnail-resource", &thumbnail_resource,
                "section-icon-resource", &section_icon_resource,
                "kind", &kind,
                NULL);

            switch (kind)
              {
              case GCV_ITEM_KIND_BUILDING:
                kind_string = "Building";
                break;
              case GCV_ITEM_KIND_UNIT:
                kind_string = "Unit";
                break;
              case GCV_ITEM_KIND_WALL:
                kind_string = "Wall";
                break;
              case GCV_ITEM_KIND_MOAT:
                kind_string = "Moat";
                break;
              default:
                break;
              }

            gtk_label_set_label (self->label, name);
            gtk_widget_set_tooltip_text (GTK_WIDGET (self), description);
            gtk_picture_set_resource (self->picture, thumbnail_resource);
            gtk_picture_set_resource (self->icon, section_icon_resource);
            gtk_label_set_label (self->kind, kind_string);
          }
        else
          {
            gtk_label_set_label (self->label, NULL);
            gtk_widget_set_tooltip_text (GTK_WIDGET (self), NULL);
            gtk_picture_set_resource (self->picture, NULL);
            gtk_picture_set_resource (self->icon, NULL);
            gtk_label_set_label (self->kind, NULL);
          }
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gcv_item_area_item_class_init (GcvItemAreaItemClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gcv_item_area_item_dispose;
  object_class->get_property = gcv_item_area_item_get_property;
  object_class->set_property = gcv_item_area_item_set_property;

  props[PROP_ITEM] =
      g_param_spec_object (
          "item",
          "Item",
          "The item this widget represents",
          GCV_TYPE_ITEM,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/am/kolunmi/Gcv/gtk-crusader-village-item-area-item.ui");
  gtk_widget_class_bind_template_child (widget_class, GcvItemAreaItem, label);
  gtk_widget_class_bind_template_child (widget_class, GcvItemAreaItem, picture);
  gtk_widget_class_bind_template_child (widget_class, GcvItemAreaItem, icon);
  gtk_widget_class_bind_template_child (widget_class, GcvItemAreaItem, kind);
}

static void
gcv_item_area_item_init (GcvItemAreaItem *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
