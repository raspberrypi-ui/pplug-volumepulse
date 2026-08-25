/*============================================================================
Copyright (c) 2020-2025 Raspberry Pi
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================*/

#include <locale.h>
#include <glib/gi18n.h>
#include <pulse/pulseaudio.h>

#include "plugin.h"

#include "volumepulse.h"

/*----------------------------------------------------------------------------*/
/* LXPanel plugin functions                                                   */
/*----------------------------------------------------------------------------*/

/* Constructor */
static GtkWidget *volumepulse_constructor (LXPanel *panel, config_setting_t *settings)
{
    /* Allocate and initialize plugin context */
    VolumePulsePlugin *vol = g_new0 (VolumePulsePlugin, 1);

    /* Allocate top level widget and set into plugin widget pointer */
    vol->panel = panel;
    vol->settings = settings;
    vol->plugin = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    lxpanel_plugin_set_data (vol->plugin, vol, volumepulse_destructor);

    volumepulse_init (vol);

    return vol->plugin;
}

/* Handler for system config changed message from panel */
static void volumepulse_configuration_changed (LXPanel *, GtkWidget *plugin)
{
    VolumePulsePlugin *vol = lxpanel_plugin_get_data (plugin);
    volumepulse_update_display (vol);
}

/* Handler for control message */
static gboolean volumepulse_control (GtkWidget *plugin, const char *cmd)
{
    VolumePulsePlugin *vol = lxpanel_plugin_get_data (plugin);
    return volumepulse_control_msg (vol, cmd);
}

int module_lxpanel_gtk_version = 1;
char module_name[] = PLUGIN_NAME;

/* Plugin descriptor */
LXPanelPluginInit fm_module_init_lxpanel_gtk =
{
    .name = PLUGIN_TITLE,
    .description = N_("Display and control volume for PulseAudio"),
    .new_instance = volumepulse_constructor,
    .reconfigure = volumepulse_configuration_changed,
    .control = volumepulse_control,
    .gettext_package = GETTEXT_PACKAGE
};

/* End of file */
/*----------------------------------------------------------------------------*/
