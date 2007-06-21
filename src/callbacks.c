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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "grandr.h"

void
on_ok_btn_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	set_hotkeys();
	
	if (!apply (screen_info)) {
		return;
	}
	
	free_screen_info (screen_info);
	
	gtk_widget_destroy (root_window);
	
	gtk_main_quit ();
}


void
on_apply_btn_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	set_hotkeys();
	
	if (!apply (screen_info)) {
		return;
	}
	
	printf("apply\n");
}


void
on_rotation0_rbtn_pressed              (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkImage *rotation_img = GTK_IMAGE (lookup_widget (root_window, "rotation_img"));
	
	gtk_image_set_from_icon_name (rotation_img, "gtk-go-up", GTK_ICON_SIZE_BUTTON);
	
	screen_info->cur_crtc->cur_rotation = RR_Rotate_0;
}


void
on_rotation90_rbtn_pressed             (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkImage *rotation_img = GTK_IMAGE (lookup_widget (root_window, "rotation_img"));
	
	gtk_image_set_from_icon_name (rotation_img, "gtk-go-back", GTK_ICON_SIZE_BUTTON);
	
	screen_info->cur_crtc->cur_rotation = RR_Rotate_90;
}


void
on_rotation180_rbtn_pressed            (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkImage *rotation_img = GTK_IMAGE (lookup_widget (root_window, "rotation_img"));
	
	gtk_image_set_from_icon_name (rotation_img, "gtk-go-down", GTK_ICON_SIZE_BUTTON);
	
	screen_info->cur_crtc->cur_rotation = RR_Rotate_180;
}


void
on_rotation270_rbtn_pressed            (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkImage *rotation_img = GTK_IMAGE (lookup_widget (root_window, "rotation_img"));
	
	gtk_image_set_from_icon_name (rotation_img, "gtk-go-forward", GTK_ICON_SIZE_BUTTON);
	
	screen_info->cur_crtc->cur_rotation = RR_Rotate_270;
}



void
on_output_iview_selection_changed      (GtkIconView     *iconview,
                                        gpointer         user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *path_list;
	GtkTreePath *tree_path;
	int output_id;
	int i;
	
	path_list = gtk_icon_view_get_selected_items (iconview);
	if (g_list_length (path_list) == 0) {
		return;
	}
	
	tree_path = (GtkTreePath *) path_list[0].data;
	
	model = gtk_icon_view_get_model (iconview);

	gtk_tree_model_get_iter (model,
			   &iter, tree_path);
  	gtk_tree_model_get (model, &iter,
		      COL_OUTPUT_ID, &output_id,
		      -1);
		      
	for (i = 0; i < screen_info->n_output; i++) {
		if (output_id == screen_info->outputs[i]->id) {
			screen_info->cur_crtc = screen_info->outputs[i]->cur_crtc;
			screen_info->cur_output = screen_info->outputs[i];
			
			set_basic_views (screen_info->cur_output);
			set_rotation_views (screen_info->cur_crtc);
			
			break;
		}
	}
}


void
on_modes_combo_changed                 (GtkComboBox     *combobox,
                                        gpointer         user_data)
{	
	GtkTreeModel *model;
	GtkTreeIter iter;
	int mode_id;
	
	model = gtk_combo_box_get_model (combobox);
	gtk_combo_box_get_active_iter (combobox, &iter);
  	gtk_tree_model_get (model, &iter,
		      COL_MODE_ID, &mode_id,
		      -1);
      
	if (screen_info->cur_crtc) {
		screen_info->cur_crtc->cur_mode_id = mode_id;
		screen_info->cur_crtc->changed = 1;
	} else {
		struct CrtcInfo *crtc_info;
		
		crtc_info = auto_find_crtc (screen_info, screen_info->cur_output);
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
on_clone_rbtn_pressed                  (GtkButton       *button,
                                        gpointer         user_data)
{
	screen_info->clone = 1;
	//set_output_layout (screen_info);
}


void
on_panorama_rbtn_pressed               (GtkButton       *button,
                                        gpointer         user_data)
{
	screen_info->clone = 0;
	//set_output_layout (screen_info);
}


void
on_output_rbtn_pressed                 (GtkButton       *button,
                                        gpointer         user_data)
{
	struct CrtcInfo *crtc_info = (struct CrtcInfo *)user_data;
	
	screen_info->primary_crtc = crtc_info;
	
	
}


void
on_iview_drag_data_get                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *path_list;
	GtkTreePath *tree_path;
	int output_id;
	int i;
	GtkIconView *iconview = (GtkIconView *) widget;
	
	if (data->format >= 8 && data->format % 8 == 0) {
		//return;
	}
	
	path_list = gtk_icon_view_get_selected_items (iconview);
	if (g_list_length (path_list) == 0) {
		return;
	}
	
	tree_path = (GtkTreePath *) path_list[0].data;
	
	model = gtk_icon_view_get_model (iconview);

	gtk_tree_model_get_iter (model,
			   &iter, tree_path);
  	gtk_tree_model_get (model, &iter,
		      COL_OUTPUT_ID, &output_id,
		      -1);

	gtk_selection_data_set(
				data,
				GDK_SELECTION_TYPE_STRING,
				sizeof (int),	
				&output_id, sizeof (int)
			);

	gtk_list_store_remove (model, &iter);
}


void
on_iview_drag_data_received            (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	GdkPixbuf *output_pixbuf;
	int output_id;
	char *output_name;
	
	if (data->format >= 8 && data->format % 8 == 0) {
		//return;
	}
	
	output_pixbuf = randr_create_pixbuf (small_pixbuf);
	output_id = (int) *data->data;
	output_name = get_output_name (screen_info, output_id);
	
	store = gtk_icon_view_get_model (widget);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 
									COL_OUTPUT_ID, output_id,
									COL_OUTPUT_NAME, output_name,
									COL_OUTPUT_PIXBUF, output_pixbuf,
									-1);
}


void
on_auto_cbtn_toggled                   (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	GtkWidget *mode_combo;
	GtkWidget *off_cbtn;
	
	mode_combo = lookup_widget (root_window, MODE_COMBO_NAME);
	off_cbtn = lookup_widget (root_window, OFF_CHECKBUTTON_NAME);
	
	if (gtk_toggle_button_get_active (togglebutton)) {
		gtk_widget_set_sensitive (mode_combo, FALSE);
		gtk_toggle_button_set_active (off_cbtn, FALSE);
		
		screen_info->cur_output->auto_set = 1;
		screen_info->cur_output->off_set = 0;
		
		output_auto (screen_info, screen_info->cur_output);
		//screen_info->cur_crtc->changed = 1;
		
	} else {
		if (!gtk_toggle_button_get_active (off_cbtn)) {
			gtk_widget_set_sensitive (mode_combo, TRUE);
		}
	}
}


void
on_off_cbtn_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	GtkWidget *mode_combo;
	GtkWidget *auto_cbtn;
	
	mode_combo = lookup_widget (root_window, MODE_COMBO_NAME);
	auto_cbtn = lookup_widget (root_window, AUTO_CHECKBUTTON_NAME);
	
	if (gtk_toggle_button_get_active (togglebutton)) {
		gtk_widget_set_sensitive (mode_combo, FALSE);
		gtk_toggle_button_set_active (auto_cbtn, FALSE);
		
		screen_info->cur_output->auto_set = 0;
		screen_info->cur_output->off_set = 1;
		
		output_off (screen_info, screen_info->cur_output); 
		//screen_info->cur_crtc->changed = 1;
		
	} else {
		if (!gtk_toggle_button_get_active (auto_cbtn)) {
			gtk_widget_set_sensitive (mode_combo, TRUE);
		}
	}
}


void
on_hotkey_cbtn_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	GtkWidget *hotkey_tview;
	
	hotkey_tview = lookup_widget (root_window, HOTKEY_TREEVIEW_NAME);

	if (gtk_toggle_button_get_active (togglebutton)) {
		gtk_widget_set_sensitive (hotkey_tview, TRUE);
	} else {
		gtk_widget_set_sensitive (hotkey_tview, FALSE);
	}
}


void
on_about_btn_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	gchar *authors[] = {"Ming Lin (ming.m.lin@intel.com)", NULL};
	gchar *comments = "This GUI is for RandR 1.2 setting\nming.m.lin@intel.com";
	gchar *license =
"Copyright © 2007 Intel Corporation\n"
"\n"
"Permission is hereby granted, free of charge, to any person obtaining a copy\n"
"of this software and associated documentation files (the “Software”), to deal\n"
"in the Software without restriction, including without limitation the rights\n"
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
"copies of the Software, and to permit persons to whom the Software is\n"
"furnished to do so, subject to the following conditions:\n"
"\n"
"The above copyright notice and this permission notice shall be included in\n"
"all copies or substantial portions of the Software.\n"
"\n"
"THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE\n"
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\n"
"THE SOFTWARE.";
	
	gtk_show_about_dialog (root_window, 
								"authors", authors, 
								"comments", comments,
								"name", "RandR GUI",
								"license", license,
								NULL);
}

