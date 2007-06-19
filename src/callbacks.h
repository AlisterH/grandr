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
#include <gtk/gtk.h>


void
on_ok_btn_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_apply_btn_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_rotation0_rbtn_pressed              (GtkButton       *button,
                                        gpointer         user_data);

void
on_rotation90_rbtn_pressed             (GtkButton       *button,
                                        gpointer         user_data);

void
on_rotation180_rbtn_pressed            (GtkButton       *button,
                                        gpointer         user_data);

void
on_rotation270_rbtn_pressed            (GtkButton       *button,
                                        gpointer         user_data);

void
on_output_iview_selection_changed      (GtkIconView     *iconview,
                                        gpointer         user_data);

void
on_modes_combo_changed                 (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_clone_rbtn_pressed                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_panorama_rbtn_pressed               (GtkButton       *button,
                                        gpointer         user_data);

void
on_output_rbtn_pressed                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_relation_combo_changed              (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_iview_drag_data_get                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_iview_drag_data_received            (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);


void
on_auto_cbtn_toggled                   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_off_cbtn_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_hotkey_cbtn_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_about_btn_clicked                   (GtkButton       *button,
                                        gpointer         user_data);
