/*
 * Copyright © 2007 Intel Corporation
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "grandr.h"
#include "support.h"
#include "callbacks.h"
#include <stdlib.h>
#include <string.h>
#include <gconf/gconf-client.h>

static Status crtc_disable (struct CrtcInfo *crtc);

#define RANDR_GUI_DEBUG 1

char *
get_output_name (struct ScreenInfo *screen_info, RROutput id)
{
	char *output_name = NULL;
	int i;
	
	for (i = 0; i < screen_info->n_output; i++) {
		if (id == screen_info->outputs[i]->id) {
			output_name = screen_info->outputs[i]->info->name;
		}
	}
	
	if (!output_name) {
		output_name = "Unknown";
	}
	
	return output_name;
}

XRRModeInfo *
find_mode_by_xid (struct ScreenInfo *screen_info, RRMode mode_id)
{
	XRRModeInfo *mode_info = NULL;
	XRRScreenResources *res;
	int i;
	
	res = screen_info->res;
	for (i = 0; i < res->nmode; i++) {
		if (mode_id == res->modes[i].id) {
			mode_info = &res->modes[i];
			break;
		}
	}
	
	return mode_info;
}

static XRRCrtcInfo *
find_crtc_by_xid (struct ScreenInfo *screen_info, RRCrtc crtc_id)
{
	XRRCrtcInfo *crtc_info;
	Display *dpy;
	XRRScreenResources *res;
	
	dpy = screen_info->dpy;
	res = screen_info->res;
	
	crtc_info = XRRGetCrtcInfo (dpy, res, crtc_id);
	
	return crtc_info;
}

int
get_width_by_output_id (struct ScreenInfo *screen_info, RROutput output_id)
{
	struct OutputInfo *output_info;
	struct CrtcInfo *crtc_info;
	RRMode mode_id;
	XRRModeInfo *mode_info;
	int i;
	int width = -1;
	
	for (i = 0; i < screen_info->n_output; i++) {
		if (output_id == screen_info->outputs[i]->id) {
			crtc_info = screen_info->outputs[i]->cur_crtc;
			if (!crtc_info) {
				width = 0;
				break;
			}
			mode_id = crtc_info->cur_mode_id;
			mode_info = find_mode_by_xid (screen_info, mode_id);
			
			width = mode_width (mode_info, crtc_info->cur_rotation);
			
			break;
		}
	}
	
	return width;
}

int
get_height_by_output_id (struct ScreenInfo *screen_info, RROutput output_id)
{
	struct OutputInfo *output_info;
	struct CrtcInfo *crtc_info;
	RRMode mode_id;
	XRRModeInfo *mode_info;
	int i;
	int height = -1;
	
	for (i = 0; i < screen_info->n_output; i++) {
		if (output_id == screen_info->outputs[i]->id) {
			crtc_info = screen_info->outputs[i]->cur_crtc;
			if (!crtc_info) {
				height = 0;
				break;
			}
			mode_id = crtc_info->cur_mode_id;
			mode_info = find_mode_by_xid (screen_info, mode_id);
			
			height = mode_height (mode_info, crtc_info->cur_rotation);
			
			break;
		}
	}
	
	return height;
}

int
mode_height (XRRModeInfo *mode_info, Rotation rotation)
{
    switch (rotation & 0xf) {
    case RR_Rotate_0:
    case RR_Rotate_180:
        return mode_info->height;
    case RR_Rotate_90:
    case RR_Rotate_270:
        return mode_info->width;
    default:
        return 0;
    }
}

int
mode_width (XRRModeInfo *mode_info, Rotation rotation)
{
    switch (rotation & 0xf) {
    case RR_Rotate_0:
    case RR_Rotate_180:
        return mode_info->width;
    case RR_Rotate_90:
    case RR_Rotate_270:
        return mode_info->height;
    default:
        return 0;
    }
}


static struct CrtcInfo * 
find_crtc (struct ScreenInfo *screen_info, XRROutputInfo *output)
{
	struct CrtcInfo *crtc_info = NULL;
	int i;
	
	for (i = 0; i < screen_info->n_crtc; i++) {
		if (screen_info->crtcs[i]->id == output->crtc) {
			crtc_info = screen_info->crtcs[i];
			break;
		}
	}
	
	return crtc_info;
}

struct CrtcInfo *
auto_find_crtc (struct ScreenInfo *screen_info, struct OutputInfo *output_info)
{
	struct CrtcInfo *crtc_info = NULL;
	int i;
	
	for (i = 0; i < screen_info->n_crtc; i++) {
		if (0 == screen_info->crtcs[i]->cur_noutput) {
			crtc_info = screen_info->crtcs[i];
			break;
		}
	}
	
	if (NULL == crtc_info) {
		crtc_info = screen_info->crtcs[0];
	}
	
	return crtc_info;
}

int
set_screen_size (struct ScreenInfo *screen_info)
{
	Display *dpy;
	int screen;
	struct CrtcInfo *crtc;
	XRRModeInfo *mode_info;
	int cur_x = 0, cur_y = 0;
	int w = 0, h = 0;
	int mmW, mmH;
	int max_width = 0, max_height = 0;
	int i;
	
	dpy = screen_info->dpy;
	screen = DefaultScreen (dpy);
	
	for (i = 0; i < screen_info->n_crtc; i++) {
		crtc = screen_info->crtcs[i];
		if (!crtc->cur_mode_id) {
			continue;
		}
		mode_info = find_mode_by_xid (screen_info, crtc->cur_mode_id);
		cur_x = crtc->cur_x;
		cur_y = crtc->cur_y;
		
		w = mode_width (mode_info, crtc->cur_rotation);
		h = mode_height (mode_info, crtc->cur_rotation);
		
		if (cur_x + w > max_width) {
			max_width = cur_x + w;
		}
		if (cur_y + h > max_height) {
			max_height = cur_y + h;
		}
	}
	
		if (max_width > screen_info->max_width) {
	#if RANDR_GUI_DEBUG
			fprintf (stderr, "user set screen width %d, larger than max width %d, set to max width\n", 
						cur_x + w, screen_info->max_width);
	#endif
			return 0;
		} else if (max_width < screen_info->min_width) {
			screen_info->cur_width = screen_info->min_width;
		} else {
			screen_info->cur_width = max_width;
		} 
	
		if (max_height > screen_info->max_height) {
	#if RANDR_GUI_DEBUG
			fprintf (stderr, "user set screen height %d, larger than max height %d, set to max height\n", 
						cur_y + h, screen_info->max_height);
	#endif	
			return 0;
		} else if (max_height < screen_info->min_height) {
			screen_info->cur_height = screen_info->min_height;
		} else {
			screen_info->cur_height = max_height;
		}
	
	
	//calculate mmWidth, mmHeight
	if (screen_info->cur_width != DisplayWidth (dpy, screen) ||
		 screen_info->cur_height != DisplayHeight (dpy, screen) ) {
		double dpi; 
		
		dpi = (25.4 * DisplayHeight (dpy, screen)) / DisplayHeightMM(dpy, screen);
		mmW = (25.4 * screen_info->cur_width) / dpi;
		mmH = (25.4 * screen_info->cur_height) / dpi;
	} else {
		mmW = DisplayWidthMM (dpy, screen);
		mmH = DisplayHeightMM (dpy, screen);
	}

	screen_info->cur_mmWidth = mmW;
	screen_info->cur_mmHeight = mmH;
	
	return 1;
}

void
screen_apply (struct ScreenInfo *screen_info)
{
	int width, height;
	int mmWidth, mmHeight;
	Display *dpy, *cur_dpy;
	Window window;
	int screen;
	static int first = 1;
	
	width = screen_info->cur_width;
	height = screen_info->cur_height;
	mmWidth = screen_info->cur_mmWidth;
	mmHeight = screen_info->cur_mmHeight;
	dpy = screen_info->dpy;
	window = screen_info->window;
	screen = DefaultScreen (dpy);
	
	cur_dpy = XOpenDisplay (NULL);

	if (width == DisplayWidth (cur_dpy, screen) &&
			height == DisplayHeight (cur_dpy, screen) &&
			mmWidth == DisplayWidthMM (cur_dpy, screen) &&
			mmHeight == DisplayHeightMM (cur_dpy, screen) ) {
		return;
	} else {
		XRRSetScreenSize (dpy, window, width, height, mmWidth, mmHeight);
	}
}

static Status 
crtc_apply (struct CrtcInfo *crtc_info)
{
	struct ScreenInfo *screen_info;
	XRRCrtcInfo *rr_crtc_info;
	Display *dpy;
	XRRScreenResources *res;
	RRCrtc crtc_id;
	int x, y;
	RRMode mode_id;
	Rotation rotation;
	RROutput *outputs;
	int noutput;
	Status s;
	int i;
	
	/*if (!crtc_info->changed) {
		return RRSetConfigSuccess;
	}*/
	
	screen_info = crtc_info->screen_info;
	dpy = screen_info->dpy;
	res = screen_info->res;
	crtc_id = crtc_info->id;
	x = crtc_info->cur_x;
	y = crtc_info->cur_y;
	
	mode_id = crtc_info->cur_mode_id;
	rotation = crtc_info->cur_rotation;

	noutput = crtc_info->cur_noutput;
	/*for (i = 0; i < screen_info->n_output; i++) {
		struct OutputInfo *output_info = screen_info->outputs[i];
		
		if (output_info->cur_crtc && crtc_id == output_info->cur_crtc->id) {
			noutput++;
		}
	}*/
	
	if (0 == noutput) {
		return crtc_disable (crtc_info);
	}
	
	outputs = malloc (sizeof (RROutput) * noutput);
	noutput = 0;
	for (i = 0; i < screen_info->n_output; i++) {
		struct OutputInfo *output_info = screen_info->outputs[i];
		
		if (output_info->cur_crtc && crtc_id == output_info->cur_crtc->id) {
			outputs[noutput++] = output_info->id;
		}
	}

	
	s = XRRSetCrtcConfig (dpy, res, crtc_id, CurrentTime,
                              x, y, mode_id, rotation,
                              outputs, noutput);

	if (RRSetConfigSuccess == s) {
		crtc_info->changed = 0;
	} 
	
	free (outputs);
	
	return s;
}

static Status
crtc_disable (struct CrtcInfo *crtc)
{
	struct ScreenInfo *screen_info;
	
	screen_info = crtc->screen_info;
	
	return XRRSetCrtcConfig (screen_info->dpy, screen_info->res, crtc->id, CurrentTime,
                             0, 0, None, RR_Rotate_0, NULL, 0);
}

int
apply (struct ScreenInfo *screen_info)
{
	int i;
	struct CrtcInfo *crtc_info;
	GtkWidget *dialog;

	//XGrabServer (screen_info->dpy);
	set_positions (screen_info);
	
	if (!set_screen_size (screen_info)) {
		dialog = gtk_message_dialog_new (GTK_WINDOW(root_window),
			  	  GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
				  GTK_MESSAGE_WARNING,
				  GTK_BUTTONS_CANCEL,
				  _("User set screen size larger than max screen size\n")
				  );
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return 0;
	}
	
	for (i = 0; i < screen_info->n_crtc; i++) {
		int old_x, old_y, old_w, old_h;
		
		XRRCrtcInfo *crtc_info = XRRGetCrtcInfo (screen_info->dpy, screen_info->res, screen_info->crtcs[i]->id);
		XRRModeInfo *old_mode = find_mode_by_xid (screen_info, crtc_info->mode);
		
		if (crtc_info->mode == None) {
			continue;
		}
		
		old_x = crtc_info->x;
		old_y = crtc_info->y;
		old_w = mode_width (old_mode, crtc_info->rotation);
		old_h = mode_height (old_mode, crtc_info->rotation);

		if (old_x + old_w <= screen_info->cur_width &&
			 old_y + old_h <= screen_info->cur_height ) {
			continue;	
		} else {
			crtc_disable (screen_info->crtcs[i]);
		}
	}
	
	screen_apply (screen_info);
	
	for (i = 0; i < screen_info->n_crtc; i++) {
		Status s;
		crtc_info = screen_info->crtcs[i];
		
		//if (crtc_info->changed) {
			s = crtc_apply (crtc_info);
			if (RRSetConfigSuccess != s) {
				fprintf (stderr, "crtc apply error\n");
			}
		//}
	}
	
	return 1;
	
	//XUngrabServer (screen_info->dpy);
}

struct ScreenInfo*
read_screen_info (Display *display)
{
	struct ScreenInfo *screen_info;
	int screen_num;
	Window root_window;
	XRRScreenResources *sr;
	int i;
	
	screen_num = DefaultScreen (display);
	root_window = RootWindow (display, screen_num);
	
	sr = XRRGetScreenResources (display, root_window);
	
	screen_info = malloc (sizeof (struct ScreenInfo));
	screen_info->dpy = display;
	screen_info->window = root_window;
	screen_info->res = sr;
	screen_info->cur_width = DisplayWidth (display, screen_num);
	screen_info->cur_height = DisplayHeight (display, screen_num);
	screen_info->cur_mmWidth = DisplayWidthMM (display, screen_num);
	screen_info->cur_mmHeight = DisplayHeightMM (display, screen_num);
	screen_info->n_output = sr->noutput;
	screen_info->n_crtc = sr->ncrtc;
	screen_info->outputs = malloc (sizeof (struct OutputInfo *) * sr->noutput);
	screen_info->crtcs = malloc (sizeof (struct CrtcInfo *) * sr->ncrtc);
	screen_info->clone = 0;
	
	//get min max width height
	XRRGetScreenSizeRange (display, root_window, 
					&screen_info->min_width, &screen_info->min_height,
					&screen_info->max_width, &screen_info->max_height);
	
	//get crtc
	for (i = 0; i < sr->ncrtc; i++) {
		struct CrtcInfo *crtc_info;
		screen_info->crtcs[i] = malloc (sizeof (struct CrtcInfo));
		crtc_info = screen_info->crtcs[i];
		XRRCrtcInfo *xrr_crtc_info = XRRGetCrtcInfo (display, sr, sr->crtcs[i]);
		
		crtc_info->id = sr->crtcs[i];
		crtc_info->info = xrr_crtc_info;
		crtc_info->cur_x = xrr_crtc_info->x;
		crtc_info->cur_y = xrr_crtc_info->y;
		crtc_info->cur_mode_id = xrr_crtc_info->mode;
		crtc_info->cur_rotation = xrr_crtc_info->rotation;
		crtc_info->rotations = xrr_crtc_info->rotations;
		crtc_info->cur_noutput = xrr_crtc_info->noutput;
	
		crtc_info->changed = 0;
		crtc_info->screen_info = screen_info;
	}
	
	
	//get output
	for (i = 0; i < sr->noutput; i++) {
		struct OutputInfo *output;
		screen_info->outputs[i] = malloc (sizeof (struct OutputInfo));
		output = screen_info->outputs[i];
		
		output->id = sr->outputs[i];
		output->info = XRRGetOutputInfo (display, sr, sr->outputs[i]);
		output->cur_crtc = find_crtc (screen_info, output->info);
		output->auto_set = 0;
		if (output->cur_crtc) {
			output->off_set = 0;
		} else {
			output->off_set = 1;
		}
		
	}
	
	//set current crtc
	screen_info->cur_crtc = screen_info->outputs[0]->cur_crtc;
	screen_info->primary_crtc = screen_info->cur_crtc;
	screen_info->cur_output = screen_info->outputs[0];
	
	return screen_info;	
}

void 
free_screen_info (struct ScreenInfo *screen_info)
{
	free (screen_info->outputs);
	free (screen_info->crtcs);
	free (screen_info);
}



GdkPixbuf*
randr_create_pixbuf (const guint8 *data)
{
	GdkPixbuf *output_pixbuf;
	
	output_pixbuf = gdk_pixbuf_new_from_inline(9240, data, TRUE, NULL);
	
	return output_pixbuf;
}

GtkListStore *
create_output_store ()
{
	GtkListStore *store;
	
	store = gtk_list_store_new (N_OUTPUT_COLS, G_TYPE_INT, G_TYPE_STRING, GDK_TYPE_PIXBUF);
	
	return store;
}

GtkListStore *
create_mode_store ()
{
	GtkListStore *store;
	
	store = gtk_list_store_new (N_MODE_COLS, G_TYPE_STRING, G_TYPE_INT);
	
	return store;
}

GtkListStore *
create_hotkey_store ()
{
	GtkListStore *store;
	
	store = gtk_list_store_new (N_HOTKEY_COLS, G_TYPE_STRING, G_TYPE_STRING);
	
	return store;
}

void
fill_hotkey_store (GtkListStore *store)
{
	GConfClient *client;
	GtkTreeIter iter;
	
	gtk_list_store_clear (store);
	
	client = gconf_client_get_default();
	
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 
									COL_HOTKEY_ACTION, "Invoke randr gui",
									COL_HOTKEY_COMBINATION, HOTKEY_STR,
									-1);
}

void
fill_output_store (GtkListStore *store, struct ScreenInfo *screen_info, int big_pic, int output_type)
{
	GtkTreeIter iter;
	
	XRROutputInfo *output_info;
	char *output_name;
	GdkPixbuf *output_pixbuf;
	RROutput output_id;
	
	int i;
	
	gtk_list_store_clear (store);
	
	for (i = 0; i < screen_info->n_output; i++) {
		output_info = screen_info->outputs[i]->info;
		switch (output_type) {
			case OUTPUT_ALL:
				break;
			case OUTPUT_ON:
				if (!screen_info->outputs[i]->cur_crtc) {
					continue;
				}
			case OUTPUT_CONNECTED:
				if (RR_Disconnected == screen_info->outputs[i]->info->connection) {
					continue;
				}
			default:
				break;
		}
		
		output_name = output_info->name;
		output_pixbuf = randr_create_pixbuf (big_pic ? big_pixbuf : small_pixbuf);
		output_id = screen_info->outputs[i]->id;
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 
									COL_OUTPUT_ID, output_id,
									COL_OUTPUT_NAME, output_name,
									COL_OUTPUT_PIXBUF, output_pixbuf,
									-1);
	}
}


static gchar *
get_mode_name (struct ScreenInfo *screen_info, RRMode mode_id)
{
	XRRScreenResources *sr;
	gchar *mode_name = NULL;
	int i;
	
	sr = screen_info->res;
	
	for (i = 0; i < sr->nmode; i++) {
		if (sr->modes[i].id == mode_id) {
			break;
		}
	}
	
	if (i == sr->nmode) {
		mode_name = g_strdup ("Unknown mode"); 
	} else {
		double rate;
		if (sr->modes[i].hTotal && sr->modes[i].vTotal) {
			rate = ((double) sr->modes[i].dotClock / 
					 ((double) sr->modes[i].hTotal * (double) sr->modes[i].vTotal));
		} else {
			rate = 0;
		}
		mode_name = g_strdup_printf ("%s%6.1fHz", sr->modes[i].name, rate);
	}
	
	return mode_name;
}

/*check if other outputs that connected to the same crtc support this mode*/
static int
check_mode (struct ScreenInfo *screen_info, struct OutputInfo *output, RRMode mode_id)
{
	XRRCrtcInfo *crtc_info;
	//XRR
	int i, j;
	int mode_ok = 1;
	
	if (!output->cur_crtc) {
		return 1;
	}
	
	crtc_info = output->cur_crtc->info;
	for (i = 0; i < crtc_info->noutput; i++) {
		XRROutputInfo *output_info;
		int nmode;
		
		if (output->id == crtc_info->outputs[i]) {
			continue;
		}
		
		mode_ok = 0;
		output_info = XRRGetOutputInfo (screen_info->dpy, screen_info->res, crtc_info->outputs[i]);
		nmode = output_info->nmode;
		for (j = 0; j < nmode; j++) {
			if (mode_id == output_info->modes[j]) {
				mode_ok = 1;
				break;
			}
		}
		if (!mode_ok) {
			break;
		}
	}
	
	return mode_ok;
} 

void
fill_mode_store (GtkListStore *store, struct OutputInfo *output)
{
	GtkComboBox *modes_combo = GTK_COMBO_BOX (lookup_widget (root_window, "modes_combo"));
	int active_num = -1;
	
	GtkTreeIter iter;
	XRROutputInfo *output_info;
	gchar *mode_name;
	
	int i;
	int mode_index = -1;
	
	gtk_list_store_clear (store);
	output_info = output->info;
	
	for (i = 0; i < output_info->nmode; i++) {
		if (!check_mode (screen_info, output, output_info->modes[i])) {
			continue;
		}
		
		mode_name = get_mode_name (screen_info, output_info->modes[i]);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
									COL_MODE_ID, output_info->modes[i],
									COL_MODE_NAME, mode_name,
									-1);
		mode_index++;
		g_free (mode_name);
		
		if (output->cur_crtc && output->cur_crtc->cur_mode_id == output_info->modes[i]) {
			active_num = mode_index;
		}
	} 
	
	if (active_num > -1) {
		gtk_combo_box_set_active (modes_combo, active_num);
	}
}

void
set_output_store (GtkListStore *store, const char *widget_name)
{
	GtkIconView *output_iview = GTK_ICON_VIEW (lookup_widget (root_window, widget_name));
															
	gtk_icon_view_set_model (output_iview, GTK_TREE_MODEL (store));
	
	g_object_unref (store);
	
	gtk_icon_view_set_text_column (output_iview, COL_OUTPUT_NAME);
	gtk_icon_view_set_pixbuf_column (output_iview, COL_OUTPUT_PIXBUF);
}

void
set_mode_store (GtkListStore *store, const char *widget_name)
{
	GtkComboBox *modes_combo = GTK_COMBO_BOX (lookup_widget (root_window, widget_name));
														
	gtk_combo_box_set_model (modes_combo, GTK_TREE_MODEL (store));
	
	g_object_unref (store);
}

void 
set_hotkey_store (GtkListStore *store, const char *widget_name)
{
	GtkTreeView *hotkeys_tview = GTK_TREE_VIEW (lookup_widget (root_window, widget_name));
														
	gtk_tree_view_set_model (hotkeys_tview, GTK_TREE_MODEL (store));
	
	g_object_unref (store);
}

void 
set_basic_views (struct OutputInfo *output_info)
{
	int auto_set;
	int off_set;
	GtkWidget *mode_combo;
	GtkWidget *auto_cbtn, *off_cbtn;
	
	mode_combo = lookup_widget (root_window, MODE_COMBO_NAME);
	auto_cbtn = lookup_widget (root_window, AUTO_CHECKBUTTON_NAME);
	off_cbtn = lookup_widget (root_window, OFF_CHECKBUTTON_NAME);

	auto_set = output_info->auto_set;
	off_set = output_info->off_set;
	fill_mode_store (mode_store, output_info);
	
	gtk_widget_set_sensitive (mode_combo, FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (auto_cbtn), FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (off_cbtn), FALSE);
	
	if (auto_set) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (auto_cbtn), TRUE);
	} else if (off_set) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (off_cbtn), TRUE);
	} else {
		gtk_widget_set_sensitive (mode_combo, TRUE);
	}
	
}

void 
set_rotation_views (struct CrtcInfo* crtc_info)
{
	GtkRadioButton *rotation_rbtn = GTK_RADIO_BUTTON (lookup_widget (root_window, "rotation0_rbtn"));
	
	GSList *rotation_rbtn_group = gtk_radio_button_get_group (rotation_rbtn);
	int len = g_slist_length (rotation_rbtn_group);
	Rotation cur_rotation;
	Rotation rotations;
	GtkRadioButton *cur_rbtn;
	GtkWidget *rotation_page;
	GtkWidget *setting_notebook;
	int i;
	
	setting_notebook = lookup_widget (root_window, SETTING_NOTEBOOK_NAME);
	rotation_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (setting_notebook), ROTATION_PAGE);
		
	/* first check crtc_info NULL */
	if (!crtc_info) {
		gtk_widget_set_sensitive (rotation_page, FALSE);
		return;
	} else {
		gtk_widget_set_sensitive (rotation_page, TRUE);
	}
	
	cur_rotation = crtc_info->cur_rotation;
	rotations = crtc_info->rotations & 0xf;
	
	for (i = 0; i < len; i++) {
		cur_rbtn = GTK_RADIO_BUTTON (g_slist_nth_data (rotation_rbtn_group, len-1-i));
		
		if ((cur_rotation >> i) & 1) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cur_rbtn), TRUE);	
		} 
		
		if ((rotations >> i) & 1) {
			gtk_widget_set_sensitive (GTK_WIDGET (cur_rbtn), TRUE);
		}
	}

}

void
set_output_layout (struct ScreenInfo *screen_info)
{
	GtkTargetEntry target_table[] = {{ "text/uri-list", 0, 0 }};
	GtkWidget *center_iview, *left_iview, *right_iview, *above_iview, *below_iview;
	
	fill_output_store (center_store, screen_info, 0, OUTPUT_ON);
	
	center_iview = lookup_widget (root_window, CENTER_ICONVIEW_NAME);
	left_iview = lookup_widget (root_window, LEFT_ICONVIEW_NAME);
	right_iview = lookup_widget (root_window, RIGHT_ICONVIEW_NAME);
	above_iview = lookup_widget (root_window, ABOVE_ICONVIEW_NAME);
	below_iview = lookup_widget (root_window, BELOW_ICONVIEW_NAME);
	
	gtk_drag_source_set (center_iview, GDK_MODIFIER_MASK, target_table, 1, GDK_ACTION_COPY);
	gtk_drag_source_set (left_iview, GDK_MODIFIER_MASK, target_table, 1, GDK_ACTION_COPY);
	gtk_drag_source_set (right_iview, GDK_MODIFIER_MASK, target_table, 1, GDK_ACTION_COPY);
	gtk_drag_source_set (above_iview, GDK_MODIFIER_MASK, target_table, 1, GDK_ACTION_COPY);
	gtk_drag_source_set (below_iview, GDK_MODIFIER_MASK, target_table, 1, GDK_ACTION_COPY);
          
	gtk_drag_dest_set (center_iview, GTK_DEST_DEFAULT_ALL, target_table, 1, GDK_ACTION_COPY);
	gtk_drag_dest_set (left_iview, GTK_DEST_DEFAULT_ALL, target_table, 1, GDK_ACTION_COPY);
	gtk_drag_dest_set (right_iview, GTK_DEST_DEFAULT_ALL, target_table, 1, GDK_ACTION_COPY);
	gtk_drag_dest_set (above_iview, GTK_DEST_DEFAULT_ALL, target_table, 1, GDK_ACTION_COPY);
	gtk_drag_dest_set (below_iview, GTK_DEST_DEFAULT_ALL, target_table, 1, GDK_ACTION_COPY);
	
	g_signal_connect ((gpointer) center_iview, "drag_data_get",
                    G_CALLBACK (on_iview_drag_data_get),
                    root_window);
  	g_signal_connect ((gpointer) center_iview, "drag_data_received",
                    G_CALLBACK (on_iview_drag_data_received),
                    root_window);
        
	g_signal_connect ((gpointer) left_iview, "drag_data_get",
                    G_CALLBACK (on_iview_drag_data_get),
                    root_window);
  	g_signal_connect ((gpointer) left_iview, "drag_data_received",
                    G_CALLBACK (on_iview_drag_data_received),
                    root_window);
                                        
	g_signal_connect ((gpointer) right_iview, "drag_data_get",
                    G_CALLBACK (on_iview_drag_data_get),
                    root_window);
  	g_signal_connect ((gpointer) right_iview, "drag_data_received",
                    G_CALLBACK (on_iview_drag_data_received),
                    root_window);
                    
   g_signal_connect ((gpointer) above_iview, "drag_data_get",
                    G_CALLBACK (on_iview_drag_data_get),
                    root_window);
  	g_signal_connect ((gpointer) above_iview, "drag_data_received",
                    G_CALLBACK (on_iview_drag_data_received),
                    root_window);
                                        
	g_signal_connect ((gpointer) below_iview, "drag_data_get",
                    G_CALLBACK (on_iview_drag_data_get),
                    root_window);
  	g_signal_connect ((gpointer) below_iview, "drag_data_received",
                    G_CALLBACK (on_iview_drag_data_received),
                    root_window);
}

int
get_iconview_child_count (GtkIconView *iconview)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	int count = 0;
	
	model = gtk_icon_view_get_model (iconview);
	if (NULL == model) {
		return -1;
	}
	
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		count = 1;
		
		while (gtk_tree_model_iter_next (model, &iter)) {
			count++;
		}
	}
	
	return count;
}

int 
get_iconview_child_max_width (GtkIconView *iconview)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	RROutput output_id;
	int max_width = 0;
	int width;
	
	model = gtk_icon_view_get_model (iconview);
	if (NULL == model) {
		max_width = 0;
	}
	
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		gtk_tree_model_get (model, &iter, COL_OUTPUT_ID, &output_id, -1);
		
		max_width = get_width_by_output_id (screen_info, output_id);
		
		while (gtk_tree_model_iter_next (model, &iter)) {
			gtk_tree_model_get (model, &iter, COL_OUTPUT_ID, &output_id, -1);
		   
		   width = get_width_by_output_id (screen_info, output_id);
		   if (width > max_width) {
		   	max_width = width;
		   	}
		}
	}
	
	return max_width;
}

int 
get_iconview_child_max_height (GtkIconView *iconview)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	RROutput output_id;
	int max_height = 0;
	int height;
	
	model = gtk_icon_view_get_model (iconview);
	if (NULL == model) {
		max_height = 0;
	}
	
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		gtk_tree_model_get (model, &iter, COL_OUTPUT_ID, &output_id, -1);
		
		max_height = get_height_by_output_id (screen_info, output_id);
		
		while (gtk_tree_model_iter_next (model, &iter)) {
			gtk_tree_model_get (model, &iter, COL_OUTPUT_ID, &output_id, -1);
		   
		   height = get_height_by_output_id (screen_info, output_id);
		   if (height > max_height) {
		   	max_height = height;
		   	}
		}
	}
	
	return max_height;
}

int
get_x (int position)
{
	int x;
	int left_width, above_width, center_width, below_width;
	GtkWidget *center_iview, *left_iview, *above_iview, *below_iview;
	
	center_iview = lookup_widget (root_window, CENTER_ICONVIEW_NAME);
	left_iview = lookup_widget (root_window, LEFT_ICONVIEW_NAME);
	above_iview = lookup_widget (root_window, ABOVE_ICONVIEW_NAME);
	below_iview = lookup_widget (root_window, BELOW_ICONVIEW_NAME);
	
	left_width = get_iconview_child_max_width (GTK_ICON_VIEW (left_iview));
	above_width = get_iconview_child_max_width (GTK_ICON_VIEW (above_iview));
	center_width = get_iconview_child_max_width (GTK_ICON_VIEW (center_iview));
	below_width = get_iconview_child_max_width (GTK_ICON_VIEW (below_iview));
	
	switch (position) {
		case LEFT_POS:
			x = 0;
			break;
		case CENTER_POS:
			x = left_width;
			break;
		case RIGHT_POS:
			x = above_width;
			if (center_width > x) {
				x = center_width;
			}
			if (below_width > x) {
				x = below_width;
			}
			x = x + left_width;
			break;
		default:
#if RANDR_GUI_DEBUG
			fprintf (stderr, "No such x position\n");
#endif
			x = 0;
			break;
	}
	
	return x;
}

int
get_y (int position)
{
	int y;
	int left_height, above_height, center_height, right_height;
	GtkWidget *center_iview, *left_iview, *above_iview, *right_iview;
	
	center_iview = lookup_widget (root_window, CENTER_ICONVIEW_NAME);
	left_iview = lookup_widget (root_window, LEFT_ICONVIEW_NAME);
	above_iview = lookup_widget (root_window, ABOVE_ICONVIEW_NAME);
	right_iview = lookup_widget (root_window, RIGHT_ICONVIEW_NAME);
	
	left_height = get_iconview_child_max_height (GTK_ICON_VIEW (left_iview));
	above_height = get_iconview_child_max_height (GTK_ICON_VIEW (above_iview));
	center_height = get_iconview_child_max_height (GTK_ICON_VIEW (center_iview));
	right_height = get_iconview_child_max_height (GTK_ICON_VIEW (right_iview));
	
	switch (position) {
		case ABOVE_POS:
			y = 0;
			break;
		case CENTER_POS:
			y = above_height;
			break;
		case BELOW_POS:
			y = left_height;
			if (center_height > y) {
				y = center_height;
			}
			if (right_height > y) {
				y = right_height;
			}
			y = above_height + y;
			break;
		default:
#if RANDR_GUI_DEBUG
			fprintf (stderr, "No such y position\n");
#endif
			y = 0;
			break;
	}
	
	return y;
}

static RRCrtc
get_crtc_id_by_output_id (struct ScreenInfo *screen_info, RROutput output_id)
{
	int i;
	RRCrtc crtc_id = -1;
	
	for (i = 0; i < screen_info->n_output; i++) {
		if (output_id == screen_info->outputs[i]->id) {
			if (screen_info->outputs[i]->cur_crtc) {
				crtc_id = screen_info->outputs[i]->cur_crtc->id;
			} else {
				crtc_id = 0;	//this output is off
			}
			break;
		}
	}
	
	return crtc_id;
}

static RRCrtc *
get_crtc_id_list (GtkIconView *iconview, int *list_len)
{
	int count;
	RRCrtc *crtc_list;
	RRCrtc crtc_id;
	RROutput output_id;
	GtkTreeModel *model;
	GtkTreeIter iter;
	int i = 0;
	
	count = get_iconview_child_count (iconview);
	if (0 == count) {
		return NULL;
	}
	
	model = gtk_icon_view_get_model (iconview);
	crtc_list = malloc (sizeof (RRCrtc) * count);
	
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		gtk_tree_model_get (model, &iter, COL_OUTPUT_ID, &output_id, -1);
		
		crtc_id = get_crtc_id_by_output_id (screen_info, output_id);
		if (crtc_id) {
			crtc_list[i++] = crtc_id;
		}
		
		while (gtk_tree_model_iter_next (model, &iter)) {
			gtk_tree_model_get (model, &iter, COL_OUTPUT_ID, &output_id, -1);
		   
		   crtc_id = get_crtc_id_by_output_id (screen_info, output_id);
			if (crtc_id) {
				crtc_list[i++] = crtc_id;
			}
		}
	}
	
	*list_len = i;
	
	return crtc_list;
}

static struct CrtcInfo *
get_crtc_info_by_xid (struct ScreenInfo *screen_info, RRCrtc crtc_id)
{
	struct CrtcInfo *crtc_info = NULL;
	int i;
	
	for (i = 0; i < screen_info->n_crtc; i++) {
		if (crtc_id == screen_info->crtcs[i]->id) {
			crtc_info = screen_info->crtcs[i];
			break;
		}
	}
	
	return crtc_info;
}

void 
set_positions (struct ScreenInfo *screen_info)
{
	GtkWidget *pos_iview[N_POSITIONS];
	struct CrtcInfo *crtc_info;
	RRCrtc *crtc_list;
	int list_len = 0;
	int x, y;
	int center_x, right_x;
	int center_y, below_y;
	int i, j;
	
	pos_iview[CENTER_POS] = lookup_widget (root_window, CENTER_ICONVIEW_NAME);
	pos_iview[LEFT_POS] = lookup_widget (root_window, LEFT_ICONVIEW_NAME);
	pos_iview[RIGHT_POS] = lookup_widget (root_window, RIGHT_ICONVIEW_NAME);
	pos_iview[ABOVE_POS] = lookup_widget (root_window, ABOVE_ICONVIEW_NAME);
	pos_iview[BELOW_POS] = lookup_widget (root_window, BELOW_ICONVIEW_NAME);
	
	center_x = get_x (CENTER_POS);
	right_x = get_x (RIGHT_POS);
	center_y = get_y (CENTER_POS);
	below_y = get_y (BELOW_POS);
	
	for (i = 0; i < N_POSITIONS; i++) {
		if (0 == get_iconview_child_count (GTK_ICON_VIEW (pos_iview[i]))) {
			continue;
		}
		crtc_list = get_crtc_id_list (GTK_ICON_VIEW (pos_iview[i]), &list_len);
		if (0 == list_len) {
			continue;
		}

		for (j = 0; j < list_len; j++) {
			crtc_info = get_crtc_info_by_xid (screen_info, crtc_list[j]);
			if (NULL == crtc_info) {
				continue;
			}
			
			switch (i) {
				case LEFT_POS:
					x = 0;
					y = center_y;
					break;
				case RIGHT_POS:
					x = right_x;
					y = center_y;
					break;
				case CENTER_POS:
					x = center_x;
					y = center_y;
					break;
				case ABOVE_POS:
					x = center_x;
					y = 0;
					break;
				case BELOW_POS:
					x = center_x;
					y = below_y;
					break;
				default:
					x = 0;
					y = 0;
			}
			crtc_info->cur_x = x;
			crtc_info->cur_y = y;
		}
		
		free (crtc_list);
	}
}

static XRRModeInfo *
preferred_mode (struct ScreenInfo *screen_info, struct OutputInfo *output)
{
    XRROutputInfo   *output_info = output->info;
    Display *dpy;
    int screen;
    int             m;
    XRRModeInfo     *best;
    int             bestDist;

	 dpy = screen_info->dpy;
	 screen = DefaultScreen (dpy);
    best = NULL;
    bestDist = 0;
    for (m = 0; m < output_info->nmode; m++) {
        XRRModeInfo *mode_info = find_mode_by_xid (screen_info, output_info->modes[m]);
        int         dist;

        if (m < output_info->npreferred)
            dist = 0;
        else if (output_info->mm_height)
            dist = (1000 * DisplayHeight(dpy, screen) / DisplayHeightMM(dpy, screen) -
                    1000 * mode_info->height / output_info->mm_height);
        else
            dist = DisplayHeight(dpy, screen) - mode_info->height;

        if (dist < 0) dist = -dist;
        if (!best || dist < bestDist) {
            best = mode_info;
            bestDist = dist;
        	   }
    	}
    return best;
}


void 
output_auto (struct ScreenInfo *screen_info, struct OutputInfo *output_info)
{
	XRRModeInfo *mode_info;
	RRMode mode_id;
	struct CrtcInfo *crtc_info;
	XRROutputInfo *probe_output_info;
	
	if (RR_Disconnected == output_info->info->connection) {
		XRRScreenResources *cur_res;
		
		cur_res = XRRGetScreenResources (screen_info->dpy, screen_info->window);
		probe_output_info = XRRGetOutputInfo (screen_info->dpy, cur_res, output_info->id);
		if (RR_Disconnected != probe_output_info->connection) {
			output_info->info = probe_output_info;
			output_info->cur_crtc = auto_find_crtc (screen_info, output_info);
		}
	}
	
	mode_info = preferred_mode (screen_info, output_info);
	if (!mode_info) {
		return;
	}
	mode_id = mode_info->id;
	
	crtc_info = output_info->cur_crtc;
	if (crtc_info) {
		crtc_info->cur_mode_id = mode_id;
	} else {
		crtc_info = auto_find_crtc (screen_info, output_info);
		if (!crtc_info) {
#if RANDR_GUI_DEBUG
			fprintf (stderr, "Can not find usable CRTC\n");
#endif
			return;
		} else {
			screen_info->cur_output->cur_crtc = crtc_info;
			screen_info->cur_crtc = crtc_info;
			screen_info->cur_crtc->cur_noutput++;
			fprintf (stderr, "n output: %d\n", screen_info->cur_crtc->cur_noutput);
			screen_info->cur_crtc->cur_mode_id = mode_id;
			screen_info->cur_crtc->changed = 1;
		}
	}
	
}

void 
output_off (struct ScreenInfo *screen_info, struct OutputInfo *output)
{
	if (output->cur_crtc) {
		output->cur_crtc->cur_noutput--;
	}
	output->cur_crtc = NULL;
	screen_info->cur_crtc = NULL;
	output->off_set = 1;
}

void
set_hotkeys_view (GtkListStore *hotkey_store)
{
	GtkTreeView *hotkey_tview;
	GtkTreeViewColumn *column;
	GtkToggleButton *hotkey_cbtn;
	GConfClient *client;
	gchar *key, *command;
	
	hotkey_tview = GTK_TREE_VIEW (lookup_widget (root_window, HOTKEY_TREEVIEW_NAME));
	hotkey_cbtn = GTK_TOGGLE_BUTTON (lookup_widget (root_window, HOTKEY_CHECKBUTTON_NAME));
	
	column = gtk_tree_view_column_new_with_attributes (_("Action"),
						     gtk_cell_renderer_text_new (),
						     "text", COL_HOTKEY_ACTION,
						     NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_append_column (hotkey_tview, column);
	
	
	column = gtk_tree_view_column_new_with_attributes (_("Hot Key Combination"),
						     gtk_cell_renderer_text_new (),
						     "text", COL_HOTKEY_COMBINATION,
						     NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_append_column (hotkey_tview, column);
	
	client = gconf_client_get_default();
	key = gconf_client_get_string(client, GCONF_KEY1,
											NULL);
	command = gconf_client_get_string(client, GCONF_KEY2,
											NULL);
	if (key && strcmp(key, HOTKEY) == 0 && command && strcmp(command, APP_NAME) == 0) {
		gtk_toggle_button_set_active (hotkey_cbtn, TRUE);
	} else {
		gtk_toggle_button_set_active (hotkey_cbtn, FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (hotkey_tview), FALSE);
	}
	
	if (key) {
		g_free(key);
	}
	if (command) {
		g_free(command);
	}
}

static void 
enable_hotkeys()
{
	GConfClient *client;
	
	client = gconf_client_get_default();
	gconf_client_set_string(client, GCONF_KEY1,
                          HOTKEY, NULL);
	gconf_client_set_string(client, GCONF_KEY2,
                          APP_NAME, NULL);
}

static void 
disable_hotkeys()
{
	GConfClient *client;
	
	client = gconf_client_get_default();
	gconf_client_set_string(client, GCONF_KEY1,
                          "disabled", NULL);
	gconf_client_set_string(client, GCONF_KEY2,
                          "", NULL);
}

void
set_hotkeys()
{
	GtkToggleButton *hotkey_cbtn;
	
	hotkey_cbtn = GTK_TOGGLE_BUTTON (lookup_widget (root_window, HOTKEY_CHECKBUTTON_NAME));
	if (gtk_toggle_button_get_active (hotkey_cbtn)) {
		enable_hotkeys();
	} else {
		disable_hotkeys();
	}
}

