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
#ifndef RANDR_GUI_CONSTANT_H
#define RANDR_GUI_CONSTANT_H

/* widget name */
#define CENTER_ICONVIEW_NAME		"center_iview"
#define LEFT_ICONVIEW_NAME		"left_iview"
#define RIGHT_ICONVIEW_NAME		"right_iview"
#define ABOVE_ICONVIEW_NAME		"above_iview"
#define BELOW_ICONVIEW_NAME		"below_iview"
#define MODE_COMBO_NAME 			"modes_combo"
#define AUTO_CHECKBUTTON_NAME	"auto_cbtn"
#define OFF_CHECKBUTTON_NAME		"off_cbtn"
#define SETTING_NOTEBOOK_NAME	"setting_notebook"
#define HOTKEY_CHECKBUTTON_NAME	"hotkey_cbtn"
#define HOTKEY_TREEVIEW_NAME		"hotkey_tview"

#define OUTPUT_CONNECTED			(1 << 0)
#define OUTPUT_UNKNOWN				(1 << 1)
#define OUTPUT_DISCONNECTED		(1 << 2)
#define OUTPUT_ON					(1 << 3)
#define OUTPUT_ALL					(0xf)

/*Hot Key*/
#define APP_NAME					"grandr"
#define GCONF_KEY1 				"/apps/metacity/global_keybindings/run_command_1"
#define GCONF_KEY2 				"/apps/metacity/keybinding_commands/command_1"
#define HOTKEY						"<Shift>F19"
#define HOTKEY_STR					"<Shift>F7"

enum {
	BASIC_PAGE,
	ROTATION_PAGE,
	LAYOUT_PAGE,
	HOTKEY_PAGE,
	N_PAGES
};

enum {
	LEFT_POS,
	RIGHT_POS,
	ABOVE_POS,
	BELOW_POS,
	CENTER_POS,
	N_POSITIONS
};

enum {
	COL_OUTPUT_ID,
	COL_OUTPUT_NAME,
	COL_OUTPUT_PIXBUF,
	N_OUTPUT_COLS	
}; 

enum {
	COL_MODE_NAME,
	COL_MODE_ID,
	N_MODE_COLS
};

enum
{
	COL_HOTKEY_ACTION,
	COL_HOTKEY_COMBINATION,
	N_HOTKEY_COLS
};

#endif
