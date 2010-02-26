/* $Id$ */
/*
 * Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 * Copyright (c) 2007-2009 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4util/libxfce4util.h>
#include <common/panel-xfconf.h>
#include <xfconf/xfconf.h>
#include <exo/exo.h>

#include "separator.h"
#include "separator-dialog_glade.h"


#define SEPARATOR_OFFSET (0.15)
#define SEPARATOR_SIZE   (8)



static void     separator_plugin_get_property              (GObject               *object,
                                                            guint                  prop_id,
                                                            GValue                *value,
                                                            GParamSpec            *pspec);
static void     separator_plugin_set_property              (GObject               *object,
                                                            guint                  prop_id,
                                                            const GValue          *value,
                                                            GParamSpec            *pspec);
static gboolean separator_plugin_expose_event              (GtkWidget             *widget,
                                                            GdkEventExpose        *event);
static void     separator_plugin_construct                 (XfcePanelPlugin       *panel_plugin);
static void     separator_plugin_free_data                 (XfcePanelPlugin       *panel_plugin);
static gboolean separator_plugin_size_changed              (XfcePanelPlugin       *panel_plugin,
                                                            gint                   size);
static void     separator_plugin_configure_plugin          (XfcePanelPlugin       *panel_plugin);
static void     separator_plugin_orientation_changed       (XfcePanelPlugin       *panel_plugin,
                                                            GtkOrientation         orientation);



enum _SeparatorPluginStyle
{
  SEPARATOR_PLUGIN_STYLE_TRANSPARENT = 0,
  SEPARATOR_PLUGIN_STYLE_SEPARATOR,
  SEPARATOR_PLUGIN_STYLE_HANDLE,
  SEPARATOR_PLUGIN_STYLE_DOTS,
};

struct _SeparatorPluginClass
{
  /* parent class */
  XfcePanelPluginClass __parent__;
};

struct _SeparatorPlugin
{
  /* parent type */
  XfcePanelPlugin __parent__;

  /* xfconf channel */
  XfconfChannel        *channel;

  /* separator style */
  SeparatorPluginStyle  style;
};

enum
{
  PROP_0,
  PROP_STYLE,
  PROP_EXPAND
};



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (SeparatorPlugin, separator_plugin)



static void
separator_plugin_class_init (SeparatorPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;
  GObjectClass         *gobject_class;
  GtkWidgetClass       *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = separator_plugin_set_property;
  gobject_class->get_property = separator_plugin_get_property;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->expose_event = separator_plugin_expose_event;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = separator_plugin_construct;
  plugin_class->free_data = separator_plugin_free_data;
  plugin_class->size_changed = separator_plugin_size_changed;
  plugin_class->configure_plugin = separator_plugin_configure_plugin;
  plugin_class->orientation_changed = separator_plugin_orientation_changed;

  g_object_class_install_property (gobject_class,
                                   PROP_STYLE,
                                   g_param_spec_uint ("style",
                                                      NULL, NULL,
                                                      SEPARATOR_PLUGIN_STYLE_TRANSPARENT,
                                                      SEPARATOR_PLUGIN_STYLE_DOTS,
                                                      SEPARATOR_PLUGIN_STYLE_SEPARATOR,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_EXPAND,
                                   g_param_spec_boolean ("expand",
                                                         NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));
}



static void
separator_plugin_init (SeparatorPlugin *plugin)
{
  /* initialize */
  plugin->style = SEPARATOR_PLUGIN_STYLE_SEPARATOR;

  /* show the properties dialog */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* initialize xfconf */
  xfconf_init (NULL);
}



static void
separator_plugin_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  SeparatorPlugin *plugin = XFCE_SEPARATOR_PLUGIN (object);

  switch (prop_id)
    {
      case PROP_STYLE:
        g_value_set_uint (value, plugin->style);
        break;

      case PROP_EXPAND:
        g_value_set_boolean (value,
            xfce_panel_plugin_get_expand (XFCE_PANEL_PLUGIN (plugin)));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
separator_plugin_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  SeparatorPlugin *plugin = XFCE_SEPARATOR_PLUGIN (object);

  switch (prop_id)
    {
      case PROP_STYLE:
        plugin->style = g_value_get_uint (value);
        gtk_widget_queue_draw (GTK_WIDGET (object));
        break;

      case PROP_EXPAND:
        xfce_panel_plugin_set_expand (XFCE_PANEL_PLUGIN (plugin),
                                      g_value_get_boolean (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static gboolean
separator_plugin_expose_event (GtkWidget      *widget,
                               GdkEventExpose *event)
{
  SeparatorPlugin *plugin = XFCE_SEPARATOR_PLUGIN (widget);
  GtkAllocation   *alloc = &(widget->allocation);

  switch (plugin->style)
    {
      case SEPARATOR_PLUGIN_STYLE_TRANSPARENT:
        /* do nothing */
        break;

      case SEPARATOR_PLUGIN_STYLE_SEPARATOR:
        if (xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin)) ==
            GTK_ORIENTATION_HORIZONTAL)
          {
            /* paint vertical separator */
            gtk_paint_vline (widget->style,
                             widget->window,
                             GTK_WIDGET_STATE (widget),
                             &(event->area),
                             widget, "separator",
                             alloc->y + alloc->height * SEPARATOR_OFFSET,
                             alloc->y + alloc->height * (1.00 - SEPARATOR_OFFSET),
                             alloc->x + alloc->width / 2 - 1);
          }
        else
          {
            /* paint horizontal separator */
            gtk_paint_hline (widget->style,
                             widget->window,
                             GTK_WIDGET_STATE (widget),
                             &(event->area),
                             widget, "separator",
                             alloc->x + alloc->width * SEPARATOR_OFFSET,
                             alloc->x + alloc->width * (1.00 - SEPARATOR_OFFSET),
                             alloc->y + alloc->height / 2 - 1);
          }
        break;

      case SEPARATOR_PLUGIN_STYLE_HANDLE:
        /* paint handle box */
        gtk_paint_handle (widget->style,
                          widget->window,
                          GTK_WIDGET_STATE (widget),
                          GTK_SHADOW_NONE,
                          &(event->area),
                          widget, "handlebox",
                          alloc->x, alloc->y,
                          alloc->width, alloc->height,
                          xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin)));
        break;

      case SEPARATOR_PLUGIN_STYLE_DOTS:
        /* TODO */
        break;
    }

  return FALSE;
}



static void
separator_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  SeparatorPlugin     *plugin = XFCE_SEPARATOR_PLUGIN (panel_plugin);
  const PanelProperty  properties[] =
  {
    { "style", G_TYPE_UINT },
    { "expand", G_TYPE_BOOLEAN },
    { NULL, G_TYPE_NONE }
  };

  /* set the xfconf channel */
  plugin->channel = xfconf_channel_new (XFCE_PANEL_PLUGIN_CHANNEL_NAME);

  /* connect all properties */
  panel_properties_bind (plugin->channel, G_OBJECT (plugin),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         properties);

  /* make sure the plugin is drawn */
  gtk_widget_queue_draw (GTK_WIDGET (panel_plugin));
}



static void
separator_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  SeparatorPlugin *plugin = XFCE_SEPARATOR_PLUGIN (panel_plugin);

  panel_return_if_fail (XFCONF_IS_CHANNEL (plugin->channel));

  /* release the xfonf channel */
  if (G_LIKELY (plugin->channel != NULL))
    g_object_unref (G_OBJECT (plugin->channel));

  /* shutdown xfconf */
  xfconf_shutdown ();
}



static gboolean
separator_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                               gint             size)
{
  /* set the minimum separator size */
  if (xfce_panel_plugin_get_orientation (panel_plugin) ==
      GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (panel_plugin),
                                 SEPARATOR_SIZE, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (panel_plugin),
                                 size, SEPARATOR_SIZE);

  return TRUE;
}



static void
separator_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  SeparatorPlugin *plugin = XFCE_SEPARATOR_PLUGIN (panel_plugin);
  GtkBuilder      *builder;
  GObject         *dialog;
  GObject         *object;

  panel_return_if_fail (XFCE_IS_SEPARATOR_PLUGIN (plugin));
  panel_return_if_fail (XFCONF_IS_CHANNEL (plugin->channel));

  /* load the dialog from the glade file */
  builder = gtk_builder_new ();
  if (gtk_builder_add_from_string (builder, separator_dialog_glade, separator_dialog_glade_length, NULL))
    {
      dialog = gtk_builder_get_object (builder, "dialog");
      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) g_object_unref, builder);
      xfce_panel_plugin_take_window (panel_plugin, GTK_WINDOW (dialog));

      xfce_panel_plugin_block_menu (panel_plugin);
      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) xfce_panel_plugin_unblock_menu, panel_plugin);

      object = gtk_builder_get_object (builder, "close-button");
      g_signal_connect_swapped (G_OBJECT (object), "clicked", G_CALLBACK (gtk_widget_destroy), dialog);

      object = gtk_builder_get_object (builder, "style");
      exo_mutual_binding_new (G_OBJECT (plugin), "style",
                              G_OBJECT (object), "active");

      object = gtk_builder_get_object (builder, "expand");
      exo_mutual_binding_new (G_OBJECT (plugin), "expand",
                              G_OBJECT (object), "active");

      gtk_widget_show (GTK_WIDGET (dialog));
    }
  else
    {
      /* release the builder */
      g_object_unref (G_OBJECT (builder));
    }
}



static void
separator_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                      GtkOrientation   orientation)
{
  /* for a size change to set the widget size request properly */
  separator_plugin_size_changed (panel_plugin,
                                 xfce_panel_plugin_get_size (panel_plugin));
}
