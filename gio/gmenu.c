/*
 * Copyright © 2011 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gmenu.h"

#include <string.h>

/**
 * SECTION:gmenu
 * @title: GMenu
 * @short_description: A simple implementation of GMenuModel
 *
 * #GMenu is a simple implementation of #GMenuModel.
 * You populate a #GMenu by adding #GMenuItem instances to it.
 *
 * There are some convenience functions to allow you to directly
 * add items (avoiding #GMenuItem) for the common cases. To add
 * a regular item, use g_menu_insert(). To add a section, use
 * g_menu_insert_section(). To add a submenu, use
 * g_menu_insert_submenu().
 */

/**
 * GMenu:
 *
 * #GMenu is an opaque structure type.  You must access it using the
 * functions below.
 *
 * Since: 2.32
 */

/**
 * GMenuItem:
 *
 * #GMenuItem is an opaque structure type.  You must access it using the
 * functions below.
 *
 * Since: 2.32
 */

struct _GMenuItem
{
  GObject parent_instance;

  GHashTable *attributes;
  GHashTable *links;
  gboolean    cow;
};

typedef GObjectClass GMenuItemClass;

struct _GMenu
{
  GMenuModel parent_instance;

  GArray   *items;
  gboolean  mutable;
};

typedef GMenuModelClass GMenuClass;

G_DEFINE_TYPE (GMenu, g_menu, G_TYPE_MENU_MODEL)
G_DEFINE_TYPE (GMenuItem, g_menu_item, G_TYPE_OBJECT)

struct item
{
  GHashTable *attributes;
  GHashTable *links;
};

static gboolean
g_menu_is_mutable (GMenuModel *model)
{
  GMenu *menu = G_MENU (model);

  return menu->mutable;
}

static gint
g_menu_get_n_items (GMenuModel *model)
{
  GMenu *menu = G_MENU (model);

  return menu->items->len;
}

static void
g_menu_get_item_attributes (GMenuModel  *model,
                            gint         position,
                            GHashTable **table)
{
  GMenu *menu = G_MENU (model);

  *table = g_hash_table_ref (g_array_index (menu->items, struct item, position).attributes);
}

static void
g_menu_get_item_links (GMenuModel  *model,
                       gint         position,
                       GHashTable **table)
{
  GMenu *menu = G_MENU (model);

  *table = g_hash_table_ref (g_array_index (menu->items, struct item, position).links);
}

/**
 * g_menu_insert_item:
 * @menu: a #GMenu
 * @position: the position at which to insert the item
 * @item: the #GMenuItem to insert
 *
 * Inserts @item into @menu.
 *
 * The "insertion" is actually done by copying all of the attribute and
 * link values of @item and using them to form a new item within @menu.
 * As such, @item itself is not really inserted, but rather, a menu item
 * that is exactly the same as the one presently described by @item.
 *
 * This means that @item is essentially useless after the insertion
 * occurs.  Any changes you make to it are ignored unless it is inserted
 * again (at which point its updated values will be copied).
 *
 * You should probably just free @item once you're done.
 *
 * There are many convenience functions to take care of common cases.
 * See g_menu_insert(), g_menu_insert_section() and
 * g_menu_insert_submenu() as well as "prepend" and "append" variants of
 * each of these functions.
 *
 * Since: 2.32
 */
void
g_menu_insert_item (GMenu     *menu,
                    gint       position,
                    GMenuItem *item)
{
  struct item new_item;

  g_return_if_fail (G_IS_MENU (menu));
  g_return_if_fail (G_IS_MENU_ITEM (item));

  if (position < 0 || position > menu->items->len)
    position = menu->items->len;

  new_item.attributes = g_hash_table_ref (item->attributes);
  new_item.links = g_hash_table_ref (item->links);
  item->cow = TRUE;

  g_array_insert_val (menu->items, position, new_item);
  g_menu_model_items_changed (G_MENU_MODEL (menu), position, 0, 1);
}

/**
 * g_menu_prepend_item:
 * @menu: a #GMenu
 * @item: a #GMenuItem to prepend
 *
 * Prepends @item to the start of @menu.
 *
 * See g_menu_insert_item() for more information.
 *
 * Since: 2.32
 */
void
g_menu_prepend_item (GMenu     *menu,
                     GMenuItem *item)
{
  g_menu_insert_item (menu, 0, item);
}

/**
 * g_menu_append_item:
 * @menu: a #GMenu
 * @item: a #GMenuItem to append
 *
 * Appends @item to the end of @menu.
 *
 * See g_menu_insert_item() for more information.
 *
 * Since: 2.32
 */
void
g_menu_append_item (GMenu     *menu,
                    GMenuItem *item)
{
  g_menu_insert_item (menu, -1, item);
}

/**
 * g_menu_freeze:
 * @menu: a #GMenu
 *
 * Marks @menu as frozen.
 *
 * After the menu is frozen, it is an error to attempt to make any
 * changes to it.  In effect this means that the #GMenu API must no
 * longer be used.
 *
 * This function causes g_menu_model_is_mutable() to begin returning
 * %FALSE, which has some positive performance implications.
 *
 * Since: 2.32
 */
void
g_menu_freeze (GMenu *menu)
{
  g_return_if_fail (G_IS_MENU (menu));

  menu->mutable = FALSE;
}

/**
 * g_menu_new:
 *
 * Creates a new #GMenu.
 *
 * The new menu has no items.
 *
 * Returns: a new #GMenu
 *
 * Since: 2.32
 */
GMenu *
g_menu_new (void)
{
  return g_object_new (G_TYPE_MENU, NULL);
}

/**
 * g_menu_insert:
 * @menu: a #GMenu
 * @position: the position at which to insert the item
 * @label: (allow-none): the section label, or %NULL
 * @detailed_action: (allow-none): the detailed action string, or %NULL
 *
 * Convenience function for inserting a normal menu item into @menu.
 * Combine g_menu_new() and g_menu_insert_item() for a more flexible
 * alternative.
 *
 * Since: 2.32
 */
void
g_menu_insert (GMenu       *menu,
               gint         position,
               const gchar *label,
               const gchar *detailed_action)
{
  GMenuItem *menu_item;

  menu_item = g_menu_item_new (label, detailed_action);
  g_menu_insert_item (menu, position, menu_item);
  g_object_unref (menu_item);
}

/**
 * g_menu_prepend:
 * @menu: a #GMenu
 * @label: (allow-none): the section label, or %NULL
 * @detailed_action: (allow-none): the detailed action string, or %NULL
 *
 * Convenience function for prepending a normal menu item to the start
 * of @menu.  Combine g_menu_new() and g_menu_insert_item() for a more
 * flexible alternative.
 *
 * Since: 2.32
 */
void
g_menu_prepend (GMenu       *menu,
                const gchar *label,
                const gchar *detailed_action)
{
  g_menu_insert (menu, 0, label, detailed_action);
}

/**
 * g_menu_append:
 * @menu: a #GMenu
 * @label: (allow-none): the section label, or %NULL
 * @detailed_action: (allow-none): the detailed action string, or %NULL
 *
 * Convenience function for appending a normal menu item to the end of
 * @menu.  Combine g_menu_new() and g_menu_insert_item() for a more
 * flexible alternative.
 *
 * Since: 2.32
 */
void
g_menu_append (GMenu       *menu,
               const gchar *label,
               const gchar *detailed_action)
{
  g_menu_insert (menu, -1, label, detailed_action);
}

/**
 * g_menu_insert_section:
 * @menu: a #GMenu
 * @position: the position at which to insert the item
 * @label: (allow-none): the section label, or %NULL
 * @section: a #GMenuModel with the items of the section
 *
 * Convenience function for inserting a section menu item into @menu.
 * Combine g_menu_item_new_section() and g_menu_insert_item() for a more
 * flexible alternative.
 *
 * Since: 2.32
 */
void
g_menu_insert_section (GMenu       *menu,
                       gint         position,
                       const gchar *label,
                       GMenuModel  *section)
{
  GMenuItem *menu_item;

  menu_item = g_menu_item_new_section (label, section);
  g_menu_insert_item (menu, position, menu_item);
  g_object_unref (menu_item);
}


/**
 * g_menu_prepend_section:
 * @menu: a #GMenu
 * @label: (allow-none): the section label, or %NULL
 * @section: a #GMenuModel with the items of the section
 *
 * Convenience function for prepending a section menu item to the start
 * of @menu.  Combine g_menu_item_new_section() and g_menu_insert_item() for
 * a more flexible alternative.
 *
 * Since: 2.32
 */
void
g_menu_prepend_section (GMenu       *menu,
                        const gchar *label,
                        GMenuModel  *section)
{
  g_menu_insert_section (menu, 0, label, section);
}

/**
 * g_menu_append_section:
 * @menu: a #GMenu
 * @label: (allow-none): the section label, or %NULL
 * @section: a #GMenuModel with the items of the section
 *
 * Convenience function for appending a section menu item to the end of
 * @menu.  Combine g_menu_item_new_section() and g_menu_insert_item() for a
 * more flexible alternative.
 *
 * Since: 2.32
 */
void
g_menu_append_section (GMenu       *menu,
                       const gchar *label,
                       GMenuModel  *section)
{
  g_menu_insert_section (menu, -1, label, section);
}

/**
 * g_menu_insert_submenu:
 * @menu: a #GMenu
 * @position: the position at which to insert the item
 * @label: (allow-none): the section label, or %NULL
 * @submenu: a #GMenuModel with the items of the submenu
 *
 * Convenience function for inserting a submenu menu item into @menu.
 * Combine g_menu_item_new_submenu() and g_menu_insert_item() for a more
 * flexible alternative.
 *
 * Since: 2.32
 */
void
g_menu_insert_submenu (GMenu       *menu,
                       gint         position,
                       const gchar *label,
                       GMenuModel  *submenu)
{
  GMenuItem *menu_item;

  menu_item = g_menu_item_new_submenu (label, submenu);
  g_menu_insert_item (menu, position, menu_item);
  g_object_unref (menu_item);
}

/**
 * g_menu_prepend_submenu:
 * @menu: a #GMenu
 * @label: (allow-none): the section label, or %NULL
 * @submenu: a #GMenuModel with the items of the submenu
 *
 * Convenience function for prepending a submenu menu item to the start
 * of @menu.  Combine g_menu_item_new_submenu() and g_menu_insert_item() for
 * a more flexible alternative.
 *
 * Since: 2.32
 */
void
g_menu_prepend_submenu (GMenu       *menu,
                        const gchar *label,
                        GMenuModel  *submenu)
{
  g_menu_insert_submenu (menu, 0, label, submenu);
}

/**
 * g_menu_append_submenu:
 * @menu: a #GMenu
 * @label: (allow-none): the section label, or %NULL
 * @submenu: a #GMenuModel with the items of the submenu
 *
 * Convenience function for appending a submenu menu item to the end of
 * @menu.  Combine g_menu_item_new_submenu() and g_menu_insert_item() for a
 * more flexible alternative.
 *
 * Since: 2.32
 */
void
g_menu_append_submenu (GMenu       *menu,
                       const gchar *label,
                       GMenuModel  *submenu)
{
  g_menu_insert_submenu (menu, -1, label, submenu);
}

static void
g_menu_clear_item (struct item *item)
{
  if (item->attributes != NULL)
    g_hash_table_unref (item->attributes);
  if (item->links != NULL);
    g_hash_table_unref (item->links);
}

/**
 * g_menu_remove:
 * @menu: a #GMenu
 * @position: the position of the item to remove
 *
 * Removes an item from the menu.
 *
 * @position gives the index of the item to remove.
 *
 * It is an error if position is not in range the range from 0 to one
 * less than the number of items in the menu.
 *
 * It is not possible to remove items by identity since items are added
 * to the menu simply by copying their links and attributes (ie:
 * identity of the item itself is not preserved).
 *
 * Since: 2.32
 */
void
g_menu_remove (GMenu *menu,
               gint   position)
{
  g_return_if_fail (G_IS_MENU (menu));
  g_return_if_fail (0 <= position && position < menu->items->len);

  g_menu_clear_item (&g_array_index (menu->items, struct item, position));
  g_array_remove_index (menu->items, position);
  g_menu_model_items_changed (G_MENU_MODEL (menu), position, 1, 0);
}

static void
g_menu_finalize (GObject *object)
{
  GMenu *menu = G_MENU (object);
  struct item *items;
  gint n_items;
  gint i;

  n_items = menu->items->len;
  items = (struct item *) g_array_free (menu->items, FALSE);
  for (i = 0; i < n_items; i++)
    g_menu_clear_item (&items[i]);
  g_free (items);

  G_OBJECT_CLASS (g_menu_parent_class)
    ->finalize (object);
}

static void
g_menu_init (GMenu *menu)
{
  menu->items = g_array_new (FALSE, FALSE, sizeof (struct item));
  menu->mutable = TRUE;
}

static void
g_menu_class_init (GMenuClass *class)
{
  GMenuModelClass *model_class = G_MENU_MODEL_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = g_menu_finalize;

  model_class->is_mutable = g_menu_is_mutable;
  model_class->get_n_items = g_menu_get_n_items;
  model_class->get_item_attributes = g_menu_get_item_attributes;
  model_class->get_item_links = g_menu_get_item_links;
}


static void
g_menu_item_clear_cow (GMenuItem *menu_item)
{
  if (menu_item->cow)
    {
      GHashTableIter iter;
      GHashTable *new;
      gpointer key;
      gpointer val;

      new = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);
      g_hash_table_iter_init (&iter, menu_item->attributes);
      while (g_hash_table_iter_next (&iter, &key, &val))
        g_hash_table_insert (new, g_strdup (key), g_variant_ref (val));
      g_hash_table_unref (menu_item->attributes);
      menu_item->attributes = new;

      new = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_object_unref);
      g_hash_table_iter_init (&iter, menu_item->links);
      while (g_hash_table_iter_next (&iter, &key, &val))
        g_hash_table_insert (new, g_strdup (key), g_object_ref (val));
      g_hash_table_unref (menu_item->links);
      menu_item->links = new;

      menu_item->cow = FALSE;
    }
}

static void
g_menu_item_finalize (GObject *object)
{
  GMenuItem *menu_item = G_MENU_ITEM (object);

  g_hash_table_unref (menu_item->attributes);
  g_hash_table_unref (menu_item->links);

  G_OBJECT_CLASS (g_menu_item_parent_class)
    ->finalize (object);
}

static void
g_menu_item_init (GMenuItem *menu_item)
{
  menu_item->attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);
  menu_item->links = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  menu_item->cow = FALSE;
}

static void
g_menu_item_class_init (GMenuItemClass *class)
{
  class->finalize = g_menu_item_finalize;
}

/* We treat attribute names the same as GSettings keys:
 * - only lowercase ascii, digits and '-'
 * - must start with lowercase
 * - must not end with '-'
 * - no consecutive '-'
 * - not longer than 1024 chars
 */
static gboolean
valid_attribute_name (const gchar *name)
{
  gint i;

  if (!g_ascii_islower (name[0]))
    return FALSE;

  for (i = 1; name[i]; i++)
    {
      if (name[i] != '-' &&
          !g_ascii_islower (name[i]) &&
          !g_ascii_isdigit (name[i]))
        return FALSE;

      if (name[i] == '-' && name[i + 1] == '-')
        return FALSE;
    }

  if (name[i - 1] == '-')
    return FALSE;

  if (i > 1024)
    return FALSE;

  return TRUE;
}

/**
 * g_menu_item_set_attribute_value:
 * @menu_item: a #GMenuItem
 * @attribute: the attribute to set
 * @value: (allow-none): a #GVariant to use as the value, or %NULL
 *
 * Sets or unsets an attribute on @menu_item.
 *
 * The attribute to set or unset is specified by @attribute. This
 * can be one of the standard attribute names %G_MENU_ATTRIBUTE_LABEL,
 * %G_MENU_ATTRIBUTE_ACTION, %G_MENU_ATTRIBUTE_TARGET, or a custom
 * attribute name.
 * Attribute names are restricted to lowercase characters, numbers
 * and '-'. Furthermore, the names must begin with a lowercase character,
 * must not end with a '-', and must not contain consecutive dashes.
 *
 * must consist only of lowercase
 * ASCII characters, digits and '-'.
 *
 * If @value is non-%NULL then it is used as the new value for the
 * attribute.  If @value is %NULL then the attribute is unset. If
 * the @value #GVariant is floating, it is consumed.
 *
 * See also g_menu_item_set_attribute() for a more convenient way to do
 * the same.
 *
 * Since: 2.32
 */
void
g_menu_item_set_attribute_value (GMenuItem   *menu_item,
                                 const gchar *attribute,
                                 GVariant    *value)
{
  g_return_if_fail (G_IS_MENU_ITEM (menu_item));
  g_return_if_fail (attribute != NULL);
  g_return_if_fail (valid_attribute_name (attribute));

  g_menu_item_clear_cow (menu_item);

  if (value != NULL)
    g_hash_table_insert (menu_item->attributes, g_strdup (attribute), g_variant_ref_sink (value));
  else
    g_hash_table_remove (menu_item->attributes, attribute);
}

/**
 * g_menu_item_set_attribute:
 * @menu_item: a #GMenuItem
 * @attribute: the attribute to set
 * @format_string: (allow-none): a #GVariant format string, or %NULL
 * @...: positional parameters, as per @format_string
 *
 * Sets or unsets an attribute on @menu_item.
 *
 * The attribute to set or unset is specified by @attribute. This
 * can be one of the standard attribute names %G_MENU_ATTRIBUTE_LABEL,
 * %G_MENU_ATTRIBUTE_ACTION, %G_MENU_ATTRIBUTE_TARGET, or a custom
 * attribute name.
 * Attribute names are restricted to lowercase characters, numbers
 * and '-'. Furthermore, the names must begin with a lowercase character,
 * must not end with a '-', and must not contain consecutive dashes.
 *
 * If @format_string is non-%NULL then the proper position parameters
 * are collected to create a #GVariant instance to use as the attribute
 * value.  If it is %NULL then the positional parameterrs are ignored
 * and the named attribute is unset.
 *
 * See also g_menu_item_set_attribute_value() for an equivalent call
 * that directly accepts a #GVariant.
 *
 * Since: 2.32
 */
void
g_menu_item_set_attribute (GMenuItem   *menu_item,
                           const gchar *attribute,
                           const gchar *format_string,
                           ...)
{
  GVariant *value;

  if (format_string != NULL)
    {
      va_list ap;

      va_start (ap, format_string);
      value = g_variant_new_va (format_string, NULL, &ap);
      va_end (ap);
    }
  else
    value = NULL;

  g_menu_item_set_attribute_value (menu_item, attribute, value);
}

/**
 * g_menu_item_set_link:
 * @menu_item: a #GMenuItem
 * @link: type of link to establish or unset
 * @model: (allow-none): the #GMenuModel to link to (or %NULL to unset)
 *
 * Creates a link from @menu_item to @model if non-%NULL, or unsets it.
 *
 * Links are used to establish a relationship between a particular menu
 * item and another menu.  For example, %G_MENU_LINK_SUBMENU is used to
 * associate a submenu with a particular menu item, and %G_MENU_LINK_SECTION
 * is used to create a section. Other types of link can be used, but there
 * is no guarantee that clients will be able to make sense of them.
 * Link types are restricted to lowercase characters, numbers
 * and '-'. Furthermore, the names must begin with a lowercase character,
 * must not end with a '-', and must not contain consecutive dashes.
 *
 * Since: 2.32
 */
void
g_menu_item_set_link (GMenuItem   *menu_item,
                      const gchar *link,
                      GMenuModel  *model)
{
  g_return_if_fail (G_IS_MENU_ITEM (menu_item));
  g_return_if_fail (link != NULL);
  g_return_if_fail (valid_attribute_name (link));

  g_menu_item_clear_cow (menu_item);

  if (model != NULL)
    g_hash_table_insert (menu_item->links, g_strdup (link), g_object_ref (model));
  else
    g_hash_table_remove (menu_item->links, link);
}

/**
 * g_menu_item_set_label:
 * @menu_item: a #GMenuItem
 * @label: (allow-none): the label to set, or %NULL to unset
 *
 * Sets or unsets the "label" attribute of @menu_item.
 *
 * If @label is non-%NULL it is used as the label for the menu item.  If
 * it is %NULL then the label attribute is unset.
 *
 * Since: 2.32
 */
void
g_menu_item_set_label (GMenuItem   *menu_item,
                       const gchar *label)
{
  GVariant *value;

  if (label != NULL)
    value = g_variant_new_string (label);
  else
    value = NULL;

  g_menu_item_set_attribute_value (menu_item, G_MENU_ATTRIBUTE_LABEL, value);
}

/**
 * g_menu_item_set_submenu:
 * @menu_item: a #GMenuItem
 * @submenu: (allow-none): a #GMenuModel, or %NULL
 *
 * Sets or unsets the "submenu" link of @menu_item to @submenu.
 *
 * If @submenu is non-%NULL, it is linked to.  If it is %NULL then the
 * link is unset.
 *
 * The effect of having one menu appear as a submenu of another is
 * exactly as it sounds.
 *
 * Since: 2.32
 */
void
g_menu_item_set_submenu (GMenuItem  *menu_item,
                         GMenuModel *submenu)
{
  g_menu_item_set_link (menu_item, G_MENU_LINK_SUBMENU, submenu);
}

/**
 * g_menu_item_set_section:
 * @menu_item: a #GMenuItem
 * @section: (allow-none): a #GMenuModel, or %NULL
 *
 * Sets or unsets the "section" link of @menu_item to @section.
 *
 * The effect of having one menu appear as a section of another is
 * exactly as it sounds: the items from @section become a direct part of
 * the menu that @menu_item is added to.  See g_menu_item_new_section()
 * for more information about what it means for a menu item to be a
 * section.
 *
 * Since: 2.32
 */
void
g_menu_item_set_section (GMenuItem  *menu_item,
                         GMenuModel *section)
{
  g_menu_item_set_link (menu_item, G_MENU_LINK_SECTION, section);
}

/**
 * g_menu_item_set_action_and_target_value:
 * @menu_item: a #GMenuItem
 * @action: (allow-none): the name of the action for this item
 * @target_value: (allow-none): a #GVariant to use as the action target
 *
 * Sets or unsets the "action" and "target" attributes of @menu_item.
 *
 * If @action is %NULL then both the "action" and "target" attributes
 * are unset (and @target_value is ignored).
 *
 * If @action is non-%NULL then the "action" attribute is set.  The
 * "target" attribute is then set to the value of @target_value if it is
 * non-%NULL or unset otherwise.
 *
 * Normal menu items (ie: not submenu, section or other custom item
 * types) are expected to have the "action" attribute set to identify
 * the action that they are associated with.  The state type of the
 * action help to determine the disposition of the menu item.  See
 * #GAction and #GActionGroup for an overview of actions.
 *
 * In general, clicking on the menu item will result in activation of
 * the named action with the "target" attribute given as the parameter
 * to the action invocation.  If the "target" attribute is not set then
 * the action is invoked with no parameter.
 *
 * If the action has no state then the menu item is usually drawn as a
 * plain menu item (ie: with no additional decoration).
 *
 * If the action has a boolean state then the menu item is usually drawn
 * as a toggle menu item (ie: with a checkmark or equivalent
 * indication).  The item should be marked as 'toggled' or 'checked'
 * when the boolean state is %TRUE.
 *
 * If the action has a string state then the menu item is usually drawn
 * as a radio menu item (ie: with a radio bullet or equivalent
 * indication).  The item should be marked as 'selected' when the string
 * state is equal to the value of the @target property.
 *
 * See g_menu_item_set_action_and_target() or
 * g_menu_item_set_detailed_action() for two equivalent calls that are
 * probably more convenient for most uses.
 *
 * Since: 2.32
 */
void
g_menu_item_set_action_and_target_value (GMenuItem   *menu_item,
                                         const gchar *action,
                                         GVariant    *target_value)
{
  GVariant *action_value;

  if (action != NULL)
    {
      action_value = g_variant_new_string (action);
    }
  else
    {
      action_value = NULL;
      target_value = NULL;
    }

  g_menu_item_set_attribute_value (menu_item, G_MENU_ATTRIBUTE_ACTION, action_value);
  g_menu_item_set_attribute_value (menu_item, G_MENU_ATTRIBUTE_TARGET, target_value);
}

/**
 * g_menu_item_set_action_and_target:
 * @menu_item: a #GMenuItem
 * @action: (allow-none): the name of the action for this item
 * @format_string: (allow-none): a GVariant format string
 * @...: positional parameters, as per @format_string
 *
 * Sets or unsets the "action" and "target" attributes of @menu_item.
 *
 * If @action is %NULL then both the "action" and "target" attributes
 * are unset (and @format_string is ignored along with the positional
 * parameters).
 *
 * If @action is non-%NULL then the "action" attribute is set.
 * @format_string is then inspected.  If it is non-%NULL then the proper
 * position parameters are collected to create a #GVariant instance to
 * use as the target value.  If it is %NULL then the positional
 * parameters are ignored and the "target" attribute is unset.
 *
 * See also g_menu_item_set_action_and_target_value() for an equivalent
 * call that directly accepts a #GVariant.  See
 * g_menu_item_set_detailed_action() for a more convenient version that
 * works with string-typed targets.
 *
 * See also g_menu_item_set_action_and_target_value() for a
 * description of the semantics of the action and target attributes.
 *
 * Since: 2.32
 */
void
g_menu_item_set_action_and_target (GMenuItem   *menu_item,
                                   const gchar *action,
                                   const gchar *format_string,
                                   ...)
{
  GVariant *value;

  if (format_string != NULL)
    {
      va_list ap;

      va_start (ap, format_string);
      value = g_variant_new_va (format_string, NULL, &ap);
      va_end (ap);
    }
  else
    value = NULL;

  g_menu_item_set_action_and_target_value (menu_item, action, value);
}

/**
 * g_menu_item_set_detailed_action:
 * @menu_item: a #GMenuItem
 * @detailed_action: the "detailed" action string
 *
 * Sets the "action" and possibly the "target" attribute of @menu_item.
 *
 * If @detailed_action contains a double colon ("::") then it is used as
 * a separator between an action name and a target string.  In this
 * case, this call is equivalent to calling
 * g_menu_item_set_action_and_target() with the part before the "::" and
 * with a string-type #GVariant containing the part following the "::".
 *
 * If @detailed_action doesn't contain "::" then the action is set to
 * the given string (verbatim) and the target value is unset.
 *
 * See g_menu_item_set_action_and_target() or
 * g_menu_item_set_action_and_target_value() for more flexible (but
 * slightly less convenient) alternatives.
 *
 * See also g_menu_item_set_action_and_target_value() for a description of
 * the semantics of the action and target attributes.
 *
 * Since: 2.32
 */
void
g_menu_item_set_detailed_action (GMenuItem   *menu_item,
                                 const gchar *detailed_action)
{
  const gchar *sep;

  sep = strstr (detailed_action, "::");

  if (sep != NULL)
    {
      gchar *action;

      action = g_strndup (detailed_action, sep - detailed_action);
      g_menu_item_set_action_and_target (menu_item, action, "s", sep + 2);
      g_free (action);
    }

  else
    g_menu_item_set_action_and_target_value (menu_item, detailed_action, NULL);
}

/**
 * g_menu_item_new:
 * @label: (allow-none): the section label, or %NULL
 * @detailed_action: (allow-none): the detailed action string, or %NULL
 *
 * Creates a new #GMenuItem.
 *
 * If @label is non-%NULL it is used to set the "label" attribute of the
 * new item.
 *
 * If @detailed_action is non-%NULL it is used to set the "action" and
 * possibly the "target" attribute of the new item.  See
 * g_menu_item_set_detailed_action() for more information.
 *
 * Returns: a new #GMenuItem
 *
 * Since: 2.32
 */
GMenuItem *
g_menu_item_new (const gchar *label,
                 const gchar *detailed_action)
{
  GMenuItem *menu_item;

  menu_item = g_object_new (G_TYPE_MENU_ITEM, NULL);

  if (label != NULL)
    g_menu_item_set_label (menu_item, label);

  if (detailed_action != NULL)
    g_menu_item_set_detailed_action (menu_item, detailed_action);

  return menu_item;
}

/**
 * g_menu_item_new_submenu:
 * @label: (allow-none): the section label, or %NULL
 * @submenu: a #GMenuModel with the items of the submenu
 *
 * Creates a new #GMenuItem representing a submenu.
 *
 * This is a convenience API around g_menu_item_new() and
 * g_menu_item_set_submenu().
 *
 * Returns: a new #GMenuItem
 *
 * Since: 2.32
 */
GMenuItem *
g_menu_item_new_submenu (const gchar *label,
                         GMenuModel  *submenu)
{
  GMenuItem *menu_item;

  menu_item = g_object_new (G_TYPE_MENU_ITEM, NULL);

  if (label != NULL)
    g_menu_item_set_label (menu_item, label);

  g_menu_item_set_submenu (menu_item, submenu);

  return menu_item;
}

/**
 * g_menu_item_new_section:
 * @label: (allow-none): the section label, or %NULL
 * @section: a #GMenuModel with the items of the section
 *
 * Creates a new #GMenuItem representing a section.
 *
 * This is a convenience API around g_menu_item_new() and
 * g_menu_item_set_section().
 *
 * The effect of having one menu appear as a section of another is
 * exactly as it sounds: the items from @section become a direct part of
 * the menu that @menu_item is added to.
 *
 * Visual separation is typically displayed between two non-empty
 * sections.  If @label is non-%NULL then it will be encorporated into
 * this visual indication.  This allows for labeled subsections of a
 * menu.
 *
 * As a simple example, consider a typical "Edit" menu from a simple
 * program.  It probably contains an "Undo" and "Redo" item, followed by
 * a separator, followed by "Cut", "Copy" and "Paste".
 *
 * This would be accomplished by creating three #GMenu instances.  The
 * first would be populated with the "Undo" and "Redo" items, and the
 * second with the "Cut", "Copy" and "Paste" items.  The first and
 * second menus would then be added as submenus of the third.  In XML
 * format, this would look something like the following:
 *
 * <informalexample><programlisting><![CDATA[
 * <menu id='edit-menu'>
 *   <section>
 *     <item label='Undo'/>
 *     <item label='Redo'/>
 *   </section>
 *   <section>
 *     <item label='Cut'/>
 *     <item label='Copy'/>
 *     <item label='Paste'/>
 *   </section>
 * </menu>
 * ]]></programlisting></informalexample>
 *
 * The following example is exactly equivalent.  It is more illustrative
 * of the exact relationship between the menus and items (keeping in
 * mind that the 'link' element defines a new menu that is linked to the
 * containing one).  The style of the second example is more verbose and
 * difficult to read (and therefore not recommended except for the
 * purpose of understanding what is really going on).
 *
 * <informalexample><programlisting><![CDATA[
 * <menu id='edit-menu'>
 *   <item>
 *     <link name='section'>
 *       <item label='Undo'/>
 *       <item label='Redo'/>
 *     </link>
 *   </item>
 *   <item>
 *     <link name='section'>
 *       <item label='Cut'/>
 *       <item label='Copy'/>
 *       <item label='Paste'/>
 *     </link>
 *   </item>
 * </menu>
 * ]]></programlisting></informalexample>
 *
 * Returns: a new #GMenuItem
 *
 * Since: 2.32
 */
GMenuItem *
g_menu_item_new_section (const gchar *label,
                         GMenuModel  *section)
{
  GMenuItem *menu_item;

  menu_item = g_object_new (G_TYPE_MENU_ITEM, NULL);

  if (label != NULL)
    g_menu_item_set_label (menu_item, label);

  g_menu_item_set_section (menu_item, section);

  return menu_item;
}
