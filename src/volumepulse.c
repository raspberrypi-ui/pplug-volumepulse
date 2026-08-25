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
#include "commongui.h"
#include "pulse.h"
#include "bluetooth.h"

/*----------------------------------------------------------------------------*/
/* Typedefs and macros                                                        */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Global data                                                                */
/*----------------------------------------------------------------------------*/

conf_table_t conf_table[1] = {
    {CONF_TYPE_NONE, NULL, NULL, NULL, NULL}
};

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/

static int get_value (const char *fmt, ...);
static void hdmi_init (VolumePulsePlugin *vol);
static void button_clicked (VolumePulsePlugin *vol, gboolean input);
static void vol_button_clicked (GtkWidget *, gpointer data);
static void mic_button_clicked (GtkWidget *, gpointer data);
static gboolean button_pressed (GdkEventButton *event, VolumePulsePlugin *vol, gboolean input);
static gboolean vol_button_pressed (GtkWidget *, GdkEventButton *event, gpointer data);
static gboolean mic_button_pressed (GtkWidget *, GdkEventButton *event, gpointer data);
static void vol_gesture_end (GtkGestureLongPress *, GdkEventSequence *, gpointer data);
static void mic_gesture_end (GtkGestureLongPress *, GdkEventSequence *, gpointer data);

/*----------------------------------------------------------------------------*/
/* Function definitions                                                       */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Generic helper functions                                                   */
/*----------------------------------------------------------------------------*/

/* Call the supplied system command and parse the result for an integer value */

static int get_value (const char *fmt, ...)
{
    char *res;
    int n, m;

    res = get_string (fmt);
    n = sscanf (res, "%d", &m);
    g_free (res);

    if (n != 1) return -1;
    else return m;
}

/* Find number of HDMI devices and device names */

static void hdmi_init (VolumePulsePlugin *vol)
{
    int i, m;

    /* check for connected monitors */
    m = get_value (HDMI_NUM_DEVICES);
    if (m < 0) m = 1; /* couldn't read, so assume 1... */
    if (m > 2) m = 2;

    for (i = 0; i < 2; i++)
    {
        if (vol->hdmi_names[i]) g_free (vol->hdmi_names[i]);
        vol->hdmi_names[i] = NULL;
    }

    /* get the names */
    if (m == 2)
    {
        vol->hdmi_names[0] = get_string (HDMI_DEVICE_0);
        vol->hdmi_names[1] = get_string (HDMI_DEVICE_1);

        /* check both devices are HDMI */
        if (vol->hdmi_names[0] && !strncmp (vol->hdmi_names[0], "HDMI", 4)
            && vol->hdmi_names[1] && !strncmp (vol->hdmi_names[1], "HDMI", 4))
                return;
    }

    /* only one device, just name it "HDMI" */
    for (i = 0; i < 2; i++)
    {
        if (vol->hdmi_names[i]) g_free (vol->hdmi_names[i]);
        vol->hdmi_names[i] = g_strdup (_("HDMI"));
    }
}

/* Check for pipewire or pulseaudio */

gboolean check_pipewire (gpointer data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) data;
    if (!system ("systemctl --user -q is-active pipewire-pulse.service")) vol->pipewire = 1;
    else vol->pipewire = 0;

    if (vol->pipewire)
    {
        DEBUG ("using pipewire");
    }
    else
    {
        DEBUG ("using pulseaudio");
    }

    return FALSE;
}

/*----------------------------------------------------------------------------*/
/* wf-panel plugin functions                                                  */
/*----------------------------------------------------------------------------*/

/* Handler for button click */
static void button_clicked (VolumePulsePlugin *vol, gboolean input)
{
    CHECK_LONGPRESS
    if (vol->popup_window[0] || vol->popup_window[1]) close_popup ();
    else popup_window_show (vol, input);

    update_display (vol, input);
}

static void vol_button_clicked (GtkWidget *, gpointer data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) data;
    button_clicked (vol, FALSE);
}

static void mic_button_clicked (GtkWidget *, gpointer data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) data;
    button_clicked (vol, TRUE);
}

/* Handler for button press */
static gboolean button_pressed (GdkEventButton *event, VolumePulsePlugin *vol, gboolean input)
{
    switch (event->button)
    {
        case 1: /* handled as a click - ignore here */
                return FALSE;

        case 2: /* middle-click - toggle mute */
                pulse_set_mute (vol, pulse_get_mute (vol, input) ? 0 : 1, input);
                break;

        case 3: /* right-click - show device list */
                menu_show (vol, input);
                break;
    }

    update_display (vol, input);
    return TRUE;
}

static gboolean vol_button_pressed (GtkWidget *, GdkEventButton *event, gpointer data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) data;
    return button_pressed (event, vol, FALSE);
}

static gboolean mic_button_pressed (GtkWidget *, GdkEventButton *event, gpointer data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) data;
    return button_pressed (event, vol, TRUE);
}

/* Handler for long-press gesture */
static void vol_gesture_end (GtkGestureLongPress *, GdkEventSequence *, gpointer data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) data;
    NOTLONG_EXIT
    menu_show (vol, FALSE);
}

static void mic_gesture_end (GtkGestureLongPress *, GdkEventSequence *, gpointer data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) data;
    NOTLONG_EXIT
    menu_show (vol, TRUE);
}

/* Handler for system config changed message from panel */
void volumepulse_update_display (VolumePulsePlugin *vol)
{
    update_display (vol, FALSE);
    update_display (vol, TRUE);
}

/* Handler for control message */
gboolean volumepulse_control_msg (VolumePulsePlugin *vol, const char *cmd)
{
    if (!gtk_widget_is_visible (vol->button[0])) return TRUE;

    if (!strncmp (cmd, "mute", 4))
    {
        pulse_set_mute (vol, pulse_get_mute (vol, FALSE) ? 0 : 1, FALSE);
        update_display (vol, FALSE);
        popup_window_show_timed (vol);
        return TRUE;
    }

    if (!strncmp (cmd, "volu", 4))
    {
        if (pulse_get_mute (vol, FALSE)) pulse_set_mute (vol, 0, FALSE);
        else
        {
            int volume = pulse_get_volume (vol, FALSE);
            if (volume < 100)
            {
                volume += 9;  // some hardware rounds volumes, so make sure we are going as far as possible up before we round....
                volume /= 5;
                volume *= 5;
            }
            pulse_set_volume (vol, volume, FALSE);
        }
        update_display (vol, FALSE);
        popup_window_show_timed (vol);
        return TRUE;
    }

    if (!strncmp (cmd, "vold", 4))
    {
        if (pulse_get_mute (vol, FALSE)) pulse_set_mute (vol, 0, FALSE);
        else
        {
            int volume = pulse_get_volume (vol, FALSE);
            if (volume > 0)
            {
                volume -= 4; // ... and the same for going down
                volume /= 5;
                volume *= 5;
            }
            pulse_set_volume (vol, volume, FALSE);
        }
        update_display (vol, FALSE);
        popup_window_show_timed (vol);
        return TRUE;
    }

    if (!strncmp (cmd, "stop", 5))
    {
        pulse_terminate (vol);
    }

    if (!strncmp (cmd, "start", 5))
    {
        hdmi_init (vol);
        pulse_init (vol);
    }

    return FALSE;
}


void volumepulse_init (VolumePulsePlugin *vol)
{
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    if (!g_strcmp0 (getenv ("USER"), "rpi-first-boot-wizard")) vol->wizard = TRUE;
    else vol->wizard = FALSE;

    /* Allocate icon as a child of top level */
    gtk_widget_show (vol->plugin);
    vol->button[0] = gtk_button_new ();
    gtk_box_pack_start (GTK_BOX (vol->plugin), vol->button[0], TRUE, TRUE, 0);
    vol->button[1] = gtk_button_new ();
    gtk_box_pack_start (GTK_BOX (vol->plugin), vol->button[1], TRUE, TRUE, 0);

    vol->tray_icon[0] = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (vol->button[0]), vol->tray_icon[0]);
    vol->tray_icon[1] = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (vol->button[1]), vol->tray_icon[1]);

    /* Set up button */
    gtk_button_set_relief (GTK_BUTTON (vol->button[0]), GTK_RELIEF_NONE);
    g_signal_connect (vol->button[0], "scroll-event", G_CALLBACK (volumepulse_mouse_scrolled), vol);
    gtk_widget_add_events (vol->button[0], GDK_SCROLL_MASK);

    gtk_button_set_relief (GTK_BUTTON (vol->button[1]), GTK_RELIEF_NONE);
    g_signal_connect (vol->button[1], "scroll-event", G_CALLBACK (micpulse_mouse_scrolled), vol);
    gtk_widget_add_events (vol->button[1], GDK_SCROLL_MASK);

    g_signal_connect (vol->button[0], "clicked", G_CALLBACK (vol_button_clicked), vol);
    g_signal_connect (vol->button[1], "clicked", G_CALLBACK (mic_button_clicked), vol);
    g_signal_connect (vol->button[0], "button-press-event", G_CALLBACK (vol_button_pressed), vol);
    g_signal_connect (vol->button[1], "button-press-event", G_CALLBACK (mic_button_pressed), vol);

    wrap_add_longpress (vol->gesture[0], vol->button[0], G_CALLBACK (vol_gesture_end), vol);
    wrap_add_longpress (vol->gesture[1], vol->button[1], G_CALLBACK (mic_gesture_end), vol);

    /* Set up variables */
    vol->menu_devices[0] = NULL;
    vol->menu_devices[1] = NULL;
    vol->popup_window[0] = NULL;
    vol->popup_window[1] = NULL;
    vol->profiles_dialog = NULL;
    vol->conn_dialog = NULL;
    vol->hdmi_names[0] = NULL;
    vol->hdmi_names[1] = NULL;

    vol->pipewire = -1;
    g_idle_add (check_pipewire, vol);

    /* Delete any old ALSA config */
    char *asf = g_strdup_printf ("%s/.asoundrc", getenv ("HOME"));
    remove (asf);
    g_free (asf);

    /* Find HDMIs */
    hdmi_init (vol);

    /* Set up PulseAudio */
    pulse_init (vol);

    /* Set up Bluez D-Bus interface */
    bluetooth_init (vol);

    /* Show the initial state of the widget */
    volumepulse_update_display (vol);
}

void volumepulse_destructor (gpointer user_data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) user_data;

    close_popup ();

    close_widget (&vol->profiles_dialog);
    close_widget (&vol->conn_dialog);
    close_widget (&vol->menu_devices[0]);
    close_widget (&vol->menu_devices[1]);

    bluetooth_terminate (vol);
    pulse_terminate (vol);

    wrap_free_gesture (vol->gesture[0]);
    wrap_free_gesture (vol->gesture[1]);

    g_free (vol->pa_default_sink);
    g_free (vol->pa_default_source);
    g_free (vol->pa_profile);
    g_free (vol->pa_error_msg);
    g_free (vol->hdmi_names[0]);
    g_free (vol->hdmi_names[1]);

    g_free (vol);
}

/* End of file */
/*----------------------------------------------------------------------------*/
