/* gtk-crusader-village-item.c
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

#include "gtk-crusader-village-item.h"

G_DEFINE_ENUM_TYPE (
    GcvItemKind,
    gcv_item_kind,
    G_DEFINE_ENUM_VALUE (GCV_ITEM_KIND_BUILDING, "building"),
    G_DEFINE_ENUM_VALUE (GCV_ITEM_KIND_UNIT, "unit"),
    G_DEFINE_ENUM_VALUE (GCV_ITEM_KIND_WALL, "wall"),
    G_DEFINE_ENUM_VALUE (GCV_ITEM_KIND_MOAT, "moat"))

struct _GcvItem
{
  GObject parent_instance;

  int         id;
  char       *name;
  char       *description;
  char       *thumbnail_resource;
  char       *section_icon_resource;
  char       *tile_resource;
  GcvItemKind kind;
  int         tile_width;
  int         tile_height;
  int         tile_offset_x;
  int         tile_offset_y;
  int         tile_impassable_rect_x;
  int         tile_impassable_rect_y;
  int         tile_impassable_rect_w;
  int         tile_impassable_rect_h;

  guint tile_resource_hash;
};

G_DEFINE_FINAL_TYPE (GcvItem, gcv_item, G_TYPE_OBJECT)

enum
{
  PROP_0,

  PROP_ID,
  PROP_NAME,
  PROP_DESCRIPTION,
  PROP_THUMBNAIL_RESOURCE,
  PROP_SECTION_ICON_RESOURCE,
  PROP_TILE_RESOURCE,
  PROP_KIND,
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
  PROP_TILE_OFFSET_X,
  PROP_TILE_OFFSET_Y,
  PROP_TILE_IMPASSABLE_RECT_X,
  PROP_TILE_IMPASSABLE_RECT_Y,
  PROP_TILE_IMPASSABLE_RECT_W,
  PROP_TILE_IMPASSABLE_RECT_H,

  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { 0 };

static void
gcv_item_dispose (GObject *object)
{
  GcvItem *self = GCV_ITEM (object);

  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->description, g_free);
  g_clear_pointer (&self->thumbnail_resource, g_free);
  g_clear_pointer (&self->section_icon_resource, g_free);
  g_clear_pointer (&self->tile_resource, g_free);

  G_OBJECT_CLASS (gcv_item_parent_class)->dispose (object);
}

static void
gcv_item_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  GcvItem *self = GCV_ITEM (object);

  switch (prop_id)
    {
    case PROP_ID:
      g_value_set_int (value, self->id);
      break;
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;
    case PROP_DESCRIPTION:
      g_value_set_string (value, self->description);
      break;
    case PROP_THUMBNAIL_RESOURCE:
      g_value_set_string (value, self->thumbnail_resource);
      break;
    case PROP_SECTION_ICON_RESOURCE:
      g_value_set_string (value, self->section_icon_resource);
      break;
    case PROP_TILE_RESOURCE:
      g_value_set_string (value, self->tile_resource);
      break;
    case PROP_KIND:
      g_value_set_enum (value, self->kind);
      break;
    case PROP_TILE_WIDTH:
      g_value_set_int (value, self->tile_width);
      break;
    case PROP_TILE_HEIGHT:
      g_value_set_int (value, self->tile_height);
      break;
    case PROP_TILE_OFFSET_X:
      g_value_set_int (value, self->tile_offset_x);
      break;
    case PROP_TILE_OFFSET_Y:
      g_value_set_int (value, self->tile_offset_y);
      break;
    case PROP_TILE_IMPASSABLE_RECT_X:
      g_value_set_int (value, self->tile_impassable_rect_x);
      break;
    case PROP_TILE_IMPASSABLE_RECT_Y:
      g_value_set_int (value, self->tile_impassable_rect_y);
      break;
    case PROP_TILE_IMPASSABLE_RECT_W:
      g_value_set_int (value, self->tile_impassable_rect_w);
      break;
    case PROP_TILE_IMPASSABLE_RECT_H:
      g_value_set_int (value, self->tile_impassable_rect_h);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gcv_item_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  GcvItem *self = GCV_ITEM (object);

  switch (prop_id)
    {
    case PROP_ID:
      self->id = g_value_get_int (value);
      break;
    case PROP_NAME:
      g_clear_pointer (&self->name, g_free);
      self->name = g_value_dup_string (value);
      break;
    case PROP_DESCRIPTION:
      g_clear_pointer (&self->description, g_free);
      self->description = g_value_dup_string (value);
      break;
    case PROP_THUMBNAIL_RESOURCE:
      g_clear_pointer (&self->thumbnail_resource, g_free);
      self->thumbnail_resource = g_value_dup_string (value);
      break;
    case PROP_SECTION_ICON_RESOURCE:
      g_clear_pointer (&self->section_icon_resource, g_free);
      self->section_icon_resource = g_value_dup_string (value);
      break;
    case PROP_TILE_RESOURCE:
      g_clear_pointer (&self->tile_resource, g_free);
      self->tile_resource = g_value_dup_string (value);
      if (self->tile_resource != NULL)
        self->tile_resource_hash = g_str_hash (self->tile_resource);
      else
        self->tile_resource_hash = 0;
      break;
    case PROP_KIND:
      self->kind = g_value_get_enum (value);
      break;
    case PROP_TILE_WIDTH:
      self->tile_width = g_value_get_int (value);
      break;
    case PROP_TILE_HEIGHT:
      self->tile_height = g_value_get_int (value);
      break;
    case PROP_TILE_OFFSET_X:
      self->tile_offset_x = g_value_get_int (value);
      break;
    case PROP_TILE_OFFSET_Y:
      self->tile_offset_y = g_value_get_int (value);
      break;
    case PROP_TILE_IMPASSABLE_RECT_X:
      self->tile_impassable_rect_x = g_value_get_int (value);
      break;
    case PROP_TILE_IMPASSABLE_RECT_Y:
      self->tile_impassable_rect_y = g_value_get_int (value);
      break;
    case PROP_TILE_IMPASSABLE_RECT_W:
      self->tile_impassable_rect_w = g_value_get_int (value);
      break;
    case PROP_TILE_IMPASSABLE_RECT_H:
      self->tile_impassable_rect_h = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gcv_item_class_init (GcvItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = gcv_item_dispose;
  object_class->get_property = gcv_item_get_property;
  object_class->set_property = gcv_item_set_property;

  props[PROP_ID] =
      g_param_spec_int (
          "id",
          "ID",
          "The ID of this item",
          0, G_MAXINT, 0,
          G_PARAM_READWRITE);

  props[PROP_NAME] =
      g_param_spec_string (
          "name",
          "Name",
          "The item class name",
          NULL,
          G_PARAM_READWRITE);

  props[PROP_DESCRIPTION] =
      g_param_spec_string (
          "description",
          "Description",
          "Flavor text for the item class",
          NULL,
          G_PARAM_READWRITE);

  props[PROP_THUMBNAIL_RESOURCE] =
      g_param_spec_string (
          "thumbnail-resource",
          "Thumbnail Resource",
          "The resource path for a displayable thumbnail image",
          NULL,
          G_PARAM_READWRITE);

  props[PROP_SECTION_ICON_RESOURCE] =
      g_param_spec_string (
          "section-icon-resource",
          "Section Icon Resource",
          "The resource path for a displayable section icon image",
          NULL,
          G_PARAM_READWRITE);

  props[PROP_TILE_RESOURCE] =
      g_param_spec_string (
          "tile-resource",
          "Tile Resource",
          "The resource path for a repeatable tile image to be used in the map view",
          NULL,
          G_PARAM_READWRITE);

  props[PROP_KIND] =
      g_param_spec_enum (
          "kind",
          "Kind",
          "The item type",
          GCV_TYPE_ITEM_KIND,
          GCV_ITEM_KIND_BUILDING,
          G_PARAM_READWRITE);

  props[PROP_TILE_WIDTH] =
      g_param_spec_int (
          "tile-width",
          "Tile Width",
          "The width of the item in tiles, if applicable",
          1, G_MAXINT, 1,
          G_PARAM_READWRITE);

  props[PROP_TILE_HEIGHT] =
      g_param_spec_int (
          "tile-height",
          "Tile Height",
          "The height of the item in tiles, if applicable",
          1, G_MAXINT, 1,
          G_PARAM_READWRITE);

  props[PROP_TILE_OFFSET_X] =
      g_param_spec_int (
          "tile-offset-x",
          "Tile Offset X",
          "A x offset to applied to this item when loading and exporting",
          G_MININT, G_MAXINT, 0,
          G_PARAM_READWRITE);

  props[PROP_TILE_OFFSET_Y] =
      g_param_spec_int (
          "tile-offset-y",
          "Tile Offset Y",
          "A y offset to applied to this item when loading and exporting",
          G_MININT, G_MAXINT, 0,
          G_PARAM_READWRITE);

  props[PROP_TILE_IMPASSABLE_RECT_X] =
      g_param_spec_int (
          "tile-impassable-rect-x",
          "Tile Impassable Rect X",
          "If this item is a building, the x component of "
          "impassable rect within the building's footprint",
          0, G_MAXINT, 0,
          G_PARAM_READWRITE);

  props[PROP_TILE_IMPASSABLE_RECT_Y] =
      g_param_spec_int (
          "tile-impassable-rect-y",
          "Tile Impassable Rect Y",
          "If this item is a building, the y component of "
          "impassable rect within the building's footprint",
          0, G_MAXINT, 0,
          G_PARAM_READWRITE);

  props[PROP_TILE_IMPASSABLE_RECT_W] =
      g_param_spec_int (
          "tile-impassable-rect-w",
          "Tile Impassable Rect Width",
          "If this item is a building, the width component of "
          "impassable rect within the building's footprint",
          0, G_MAXINT, 0,
          G_PARAM_READWRITE);

  props[PROP_TILE_IMPASSABLE_RECT_H] =
      g_param_spec_int (
          "tile-impassable-rect-h",
          "Tile Impassable Rect Height",
          "If this item is a building, the height component of "
          "impassable rect within the building's footprint",
          0, G_MAXINT, 0,
          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
gcv_item_init (GcvItem *self)
{
  self->kind        = GCV_ITEM_KIND_BUILDING;
  self->tile_width  = 1;
  self->tile_height = 1;
}

static void
gcv_item_set_property_from_variant_inner (GcvItem  *self,
                                          int       prop_id,
                                          GVariant *variant)
{
#define WARN_INVALID_TYPE(need)                                                \
  g_critical ("%s: property \"%s\" needs type " G_STRINGIFY (need) ", got %s", \
              G_STRFUNC, props[prop_id]->name, g_variant_get_type_string (variant))

#define RECEIVE_BASIC_VARIANT(type, type_def, ...)                                         \
  G_STMT_START                                                                             \
  {                                                                                        \
    if (g_variant_is_of_type (variant, type_def))                                          \
      g_object_set (self, props[prop_id]->name, g_variant_get_##type (__VA_ARGS__), NULL); \
    else                                                                                   \
      WARN_INVALID_TYPE (type);                                                            \
  }                                                                                        \
  G_STMT_END

  switch (prop_id)
    {
    case PROP_ID:
    case PROP_TILE_WIDTH:
    case PROP_TILE_HEIGHT:
    case PROP_TILE_OFFSET_X:
    case PROP_TILE_OFFSET_Y:
    case PROP_TILE_IMPASSABLE_RECT_X:
    case PROP_TILE_IMPASSABLE_RECT_Y:
    case PROP_TILE_IMPASSABLE_RECT_W:
    case PROP_TILE_IMPASSABLE_RECT_H:
      RECEIVE_BASIC_VARIANT (int32, G_VARIANT_TYPE_INT32, variant);
      break;
    case PROP_NAME:
    case PROP_DESCRIPTION:
    case PROP_THUMBNAIL_RESOURCE:
    case PROP_SECTION_ICON_RESOURCE:
    case PROP_TILE_RESOURCE:
      RECEIVE_BASIC_VARIANT (string, G_VARIANT_TYPE_STRING, variant, NULL);
      break;
    case PROP_KIND:
      if (g_variant_is_of_type (variant, G_VARIANT_TYPE_STRING))
        {
          const char *variant_string        = NULL;
          g_autoptr (GEnumClass) enum_class = NULL;
          GEnumValue *enum_value            = NULL;

          variant_string = g_variant_get_string (variant, NULL);
          enum_class     = g_type_class_ref (GCV_TYPE_ITEM_KIND);
          enum_value     = g_enum_get_value_by_nick (enum_class, variant_string);

          if (enum_value != NULL)
            g_object_set (
                self,
                props[prop_id]->name, enum_value->value,
                NULL);
          else
            g_critical ("%s: property \"%s\" does not accept the enum value '%s'",
                        G_STRFUNC, props[prop_id]->name, variant_string);
        }
      else
        WARN_INVALID_TYPE (string);
      break;
    default:
      g_assert_not_reached ();
      break;
    }

#undef RECEIVE_VARIANT
}

GcvItem *
gcv_item_new_for_resource (const char *resource_path,
                           GError    **error)
{
  GError *local_error               = NULL;
  g_autoptr (GBytes) bytes          = NULL;
  gsize       local_size            = 0;
  const char *resource_data         = NULL;
  g_autoptr (GVariant) data_variant = NULL;
  g_autoptr (GcvItem) item          = NULL;

  g_return_val_if_fail (resource_path != NULL, NULL);

  bytes = g_resources_lookup_data (resource_path, G_RESOURCE_LOOKUP_FLAGS_NONE, &local_error);
  if (bytes == NULL)
    {
      g_propagate_error (error, local_error);
      return NULL;
    }

  resource_data = g_bytes_get_data (bytes, &local_size);

  data_variant = g_variant_parse (NULL, resource_data, NULL, NULL, &local_error);
  if (data_variant == NULL)
    {
      g_propagate_error (error, local_error);
      return NULL;
    }

  item = g_object_new (GCV_TYPE_ITEM, NULL);

  /* TODO: maybe add real errors and improve feedback
   * through gui if custom items are ever a thing.
   */
  if (g_variant_is_of_type (data_variant, G_VARIANT_TYPE_DICTIONARY))
    {
      GVariantIter iter = { 0 };

      g_variant_iter_init (&iter, data_variant);
      for (;;)
        {
          g_autofree char *key       = NULL;
          g_autoptr (GVariant) value = NULL;

          if (!g_variant_iter_next (&iter, "{sv}", &key, &value))
            break;

          gcv_item_set_property_from_variant (item, key, value);
        }
    }

  return g_steal_pointer (&item);
}

void
gcv_item_set_property_from_variant (GcvItem    *self,
                                    const char *property,
                                    GVariant   *variant)
{
  gboolean found = FALSE;

  for (int prop_id = PROP_0 + 1; prop_id < LAST_PROP; prop_id++)
    {
      if (g_strcmp0 (property, props[prop_id]->name) == 0)
        {
          gcv_item_set_property_from_variant_inner (self, prop_id, variant);
          found = TRUE;
          break;
        }
    }

  if (!found)
    g_critical ("%s: property '%s' not found", G_STRFUNC, property);
}

const char *
gcv_item_get_name (GcvItem *self)
{
  g_return_val_if_fail (GCV_IS_ITEM (self), NULL);

  return self->name;
}

gpointer
gcv_item_get_tile_resource_hash (GcvItem *self)
{
  g_return_val_if_fail (GCV_IS_ITEM (self), NULL);

  return GUINT_TO_POINTER (self->tile_resource_hash);
}
