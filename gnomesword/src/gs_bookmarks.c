/***************************************************************************
                                    gs_bookmarks.c
                             -------------------
    begin                : Thu July 05 2001
    copyright            : (C) 2001 by tbiggs
    email                : tbiggs@users.sf.net
 ***************************************************************************/
 /*
    *  This program is free software; you can redistribute it and/or modify
    *  it under the terms of the GNU General Public License as published by
    *  the Free Software Foundation; either version 2 of the License, or
    *  (at your option) any later version.
    *
    *  This program is distributed in the hope that it will be useful,
    *  but WITHOUT ANY WARRANTY; without even the implied warranty of
    *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    *  GNU Library General Public License for more details.
    *
    *  You should have received a copy of the GNU General Public License
    *  along with this program; if not, write to the Free Software
    *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
  */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include  <gal/shortcut-bar/e-shortcut-bar.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "gs_bookmarks.h"
#include "sw_bookmarks.h"
#include "gs_sword.h"
#include "gs_gnomesword.h"
#include "gs_listeditor.h"
#include "support.h"


GdkPixmap *pixmap1;
GdkPixmap *pixmap2;
GdkPixmap *pixmap3;
GdkBitmap *mask1;
GdkBitmap *mask2;
GdkBitmap *mask3;
GtkStyle *style;
extern GtkWidget *MainFrm;
BM_TREE bmtree;
BM_TREE *p_bmtree;
extern GS_APP gs;
extern GtkCTreeNode *personal_node;
extern gchar *fnbookmarksnew;

static char *book_open_xpm[] = {
	"16 16 4 1",
	"       c None s None",
	".      c black",
	"X      c #808080",
	"o      c white",
	"                ",
	"  ..            ",
	" .Xo.    ...    ",
	" .Xoo. ..oo.    ",
	" .Xooo.Xooo...  ",
	" .Xooo.oooo.X.  ",
	" .Xooo.Xooo.X.  ",
	" .Xooo.oooo.X.  ",
	" .Xooo.Xooo.X.  ",
	" .Xooo.oooo.X.  ",
	"  .Xoo.Xoo..X.  ",
	"   .Xo.o..ooX.  ",
	"    .X..XXXXX.  ",
	"    ..X.......  ",
	"     ..         ",
	"                "
};

static char *book_closed_xpm[] = {
	"16 16 6 1",
	"       c None s None",
	".      c black",
	"X      c red",
	"o      c yellow",
	"O      c #808080",
	"#      c white",
	"                ",
	"       ..       ",
	"     ..XX.      ",
	"   ..XXXXX.     ",
	" ..XXXXXXXX.    ",
	".ooXXXXXXXXX.   ",
	"..ooXXXXXXXXX.  ",
	".X.ooXXXXXXXXX. ",
	".XX.ooXXXXXX..  ",
	" .XX.ooXXX..#O  ",
	"  .XX.oo..##OO. ",
	"   .XX..##OO..  ",
	"    .X.#OO..    ",
	"     ..O..      ",
	"      ..        ",
	"                "
};

static char *mini_page_xpm[] = {
	"16 16 4 1",
	"       c None s None",
	".      c black",
	"X      c white",
	"o      c #808080",
	"                ",
	"   .......      ",
	"   .XXXXX..     ",
	"   .XoooX.X.    ",
	"   .XXXXX....   ",
	"   .XooooXoo.o  ",
	"   .XXXXXXXX.o  ",
	"   .XooooooX.o  ",
	"   .XXXXXXXX.o  ",
	"   .XooooooX.o  ",
	"   .XXXXXXXX.o  ",
	"   .XooooooX.o  ",
	"   .XXXXXXXX.o  ",
	"   ..........o  ",
	"    oooooooooo  ",
	"                "
};


static GnomeUIInfo pmBookmarkTree_uiinfo[] = {
	GNOMEUIINFO_MENU_NEW_ITEM(N_("_New SubGroup"),
				  N_("Add new SubGroup to selected gourp"),
				  on_new_activate, NULL),
	{
	 GNOME_APP_UI_ITEM, N_("Add New Group"),
	 N_("Add a new root group and file"),
	 (gpointer) on_add_new_group1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
	 0, (GdkModifierType) 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Save Bookmarks"),
	 N_("Save all bookmark files"),
	 (gpointer) on_save_bookmarks1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
	 0, (GdkModifierType) 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Eidt Item"),
	 N_("Edit bookmark item"),
	 (gpointer) on_edit_item_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PROP,
	 0, (GdkModifierType) 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Delete Item(s)"),
	 N_("Delete item and it's siblings"),
	 (gpointer) on_delete_item_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT,
	 0, (GdkModifierType) 0, NULL},
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("Allow Reordering"),
	 N_("Toggle Reording - allow items to be moved by draging"),
	 (gpointer) on_allow_reordering_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, (GdkModifierType) 0, NULL},
	GNOMEUIINFO_END
};


/******************************************************************************
 * local functions
 ******************************************************************************/

static void after_press(GtkCTree * ctree, gpointer data)
{


}

static GtkCTreeNode *node2;
static void
setleaf(GtkWidget * ctree_widget)
{
	GtkCTree *ctree;
	int i;
	
	ctree = GTK_CTREE(ctree_widget);
	gtk_ctree_expand_recursive (ctree, NULL); 
	//g_warning("setleaf rows=%d",GTK_CLIST(ctree)->rows);
	for(i=0; i< GTK_CLIST(ctree)->rows; i++){
		node2 = gtk_ctree_node_nth(ctree, i);
		if(GTK_CTREE_ROW(node2)->children == NULL){
			//g_warning("no children");
			GTK_CTREE_ROW(node2)->pixmap_closed = pixmap3;
			GTK_CTREE_ROW(node2)->mask_closed = mask3;
			GTK_CTREE_ROW(node2)->pixmap_opened = pixmap3;
			GTK_CTREE_ROW(node2)->mask_opened = mask3;
			GTK_CTREE_ROW(node2)->is_leaf = TRUE;
			GTK_CTREE_ROW(node2)->expanded = FALSE;
		}
	}
	gtk_ctree_collapse_recursive (ctree, NULL); 
}
static GtkCTreeNode *selected_node;
static void
on_ctree_select_row(GtkCList * clist,
		    gint row,
		    gint column, GdkEvent * event, gpointer user_data)
{
	//GtkCTreeNode *node;
	gchar *label, *modName, *key;
	GtkCTree *ctree;

	ctree = user_data;
	selected_node = gtk_ctree_node_nth(p_bmtree->ctree, row);
	//if(GTK_CTREE_ROW(selected_node)->children == NULL){
		//g_warning("no children");
	if (GTK_CTREE_ROW(selected_node)->is_leaf) {	/* if node is leaf we need to change mod and key */
		
		label =
		    GTK_CELL_PIXTEXT(GTK_CTREE_ROW(selected_node)->row.
				     cell[0])->text;
		 key=
		    GTK_CELL_PIXTEXT(GTK_CTREE_ROW(selected_node)->row.
				     cell[1])->text;
		
		modName =
		    GTK_CELL_PIXTEXT(GTK_CTREE_ROW(selected_node)->row.
				     cell[2])->text;
		gtk_widget_set_sensitive(GTK_WIDGET
					 (pmBookmarkTree_uiinfo[0].widget),
					 FALSE);
		gotoBookmarkSWORD(modName, key);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET
					 (pmBookmarkTree_uiinfo[0].widget),
					 TRUE);
	}
}

static
void after_move(GtkCTree * ctree, GtkCTreeNode * child,
		GtkCTreeNode * parent, GtkCTreeNode * sibling,
		gpointer data)
{
	char *source;
	char *target1;
	char *target2;

	gtk_ctree_get_node_info(ctree, child, &source,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	if (parent)
		gtk_ctree_get_node_info(ctree, parent, &target1,
					NULL, NULL, NULL, NULL, NULL, NULL,
					NULL);
	if (sibling)
		gtk_ctree_get_node_info(ctree, sibling, &target2,
					NULL, NULL, NULL, NULL, NULL, NULL,
					NULL);

	g_print("Moving \"%s\" to \"%s\" with sibling \"%s\".\n", source,
		(parent) ? target1 : "nil", (sibling) ? target2 : "nil");
}

static void count_items(GtkCTree * ctree, GtkCTreeNode * list)
{
	/* if (GTK_CTREE_ROW(list)->is_leaf)
		pages--;
	else
		books--;
	*/
}
/*
static void expand_all(GtkWidget * widget, GtkCTree * ctree)
{
	gtk_ctree_expand_recursive(ctree, NULL);
	after_press(ctree, NULL);
}

static void collapse_all(GtkWidget * widget, GtkCTree * ctree)
{
	gtk_ctree_collapse_recursive(ctree, NULL);
	after_press(ctree, NULL);
}

static void change_style(GtkWidget * widget, GtkCTree * ctree)
{
	static GtkStyle *style1 = NULL;
	static GtkStyle *style2 = NULL;

	GtkCTreeNode *node;
	GdkColor col1;
	GdkColor col2;

	if (GTK_CLIST(ctree)->focus_row >= 0)
		node = GTK_CTREE_NODE
		    (g_list_nth
		     (GTK_CLIST(ctree)->row_list,
		      GTK_CLIST(ctree)->focus_row));
	else
		node = GTK_CTREE_NODE(GTK_CLIST(ctree)->row_list);

	if (!node)
		return;

	if (!style1) {
		col1.red = 0;
		col1.green = 56000;
		col1.blue = 0;
		col2.red = 32000;
		col2.green = 0;
		col2.blue = 56000;

		style1 = gtk_style_new();
		style1->base[GTK_STATE_NORMAL] = col1;
		style1->fg[GTK_STATE_SELECTED] = col2;

		style2 = gtk_style_new();
		style2->base[GTK_STATE_SELECTED] = col2;
		style2->fg[GTK_STATE_NORMAL] = col1;
		style2->base[GTK_STATE_NORMAL] = col2;
		gdk_font_unref(style2->font);
		style2->font =
		    gdk_font_load
		    ("-*-courier-medium-*-*-*-*-300-*-*-*-*-*-*");
	}

	gtk_ctree_node_set_cell_style(ctree, node, 1, style1);
	gtk_ctree_node_set_cell_style(ctree, node, 0, style2);

	if (GTK_CTREE_ROW(node)->children)
		gtk_ctree_node_set_row_style(ctree,
					     GTK_CTREE_ROW(node)->children,
					     style2);
}
*/
static void 
remove_selection(GtkWidget * widget, GtkCTree * ctree)
{
	GtkCList *clist;
	GtkCTreeNode *node;

	clist = GTK_CLIST(ctree);

	gtk_clist_freeze(clist);

	while (clist->selection) {
		node = clist->selection->data;

		if (GTK_CTREE_ROW(node)->is_leaf){
			//do nothing
		}
		else
			gtk_ctree_post_recursive(ctree, node,
						 (GtkCTreeFunc)
						 count_items, NULL);

		gtk_ctree_remove_node(ctree, node);

		if (clist->selection_mode == GTK_SELECTION_BROWSE)
			break;
	}

	if (clist->selection_mode == GTK_SELECTION_EXTENDED
	    && !clist->selection && clist->focus_row >= 0) {
		node = gtk_ctree_node_nth(ctree, clist->focus_row);

		if (node)
			gtk_ctree_select(ctree, node);
	}

	gtk_clist_thaw(clist);
	after_press(ctree, NULL);
}
/*
struct _ExportStruct {
	gchar *tree;
	gchar *info;
	gboolean is_leaf;
};

typedef struct _ExportStruct ExportStruct;

static gboolean
gnode2ctree(GtkCTree * ctree,
	    guint depth,
	    GNode * gnode, GtkCTreeNode * cnode, gpointer data)
{
	ExportStruct *es;
	GdkPixmap *pixmap_closed;
	GdkBitmap *mask_closed;
	GdkPixmap *pixmap_opened;
	GdkBitmap *mask_opened;

	if (!cnode || !gnode || (!(es = gnode->data)))
		return FALSE;

	if (es->is_leaf) {
		pixmap_closed = pixmap3;
		mask_closed = mask3;
		pixmap_opened = NULL;
		mask_opened = NULL;
	} else {
		pixmap_closed = pixmap1;
		mask_closed = mask1;
		pixmap_opened = pixmap2;
		mask_opened = mask2;
	}

	gtk_ctree_set_node_info(ctree, cnode, es->tree, 2, pixmap_closed,
				mask_closed, pixmap_opened, mask_opened,
				es->is_leaf, (depth < 3));
	gtk_ctree_node_set_text(ctree, cnode, 1, es->info);
	g_free(es);
	gnode->data = NULL;

	return TRUE;
}

static gboolean
ctree2gnode(GtkCTree * ctree,
	    guint depth,
	    GNode * gnode, GtkCTreeNode * cnode, gpointer data)
{
	ExportStruct *es;

	if (!cnode || !gnode)
		return FALSE;

	es = g_new(ExportStruct, 1);
	gnode->data = es;
	es->is_leaf = GTK_CTREE_ROW(cnode)->is_leaf;
	es->tree =
	    GTK_CELL_PIXTEXT(GTK_CTREE_ROW(cnode)->row.cell[0])->text;
	es->info =
	    GTK_CELL_PIXTEXT(GTK_CTREE_ROW(cnode)->row.cell[1])->text;
	return TRUE;
}

static void export_ctree(GtkWidget * widget, GtkCTree * ctree)
{
	char *title[] = { "Tree", "Info" };
	static GtkWidget *export_window = NULL;
	static GtkCTree *export_ctree;
	GtkWidget *vbox;
	GtkWidget *scrolled_win;
	GtkWidget *button;
	GtkWidget *sep;
	GNode *gnode;
	GtkCTreeNode *node;

	if (!export_window) {
		export_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

		gtk_signal_connect(GTK_OBJECT(export_window), "destroy",
				   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
				   &export_window);

		gtk_window_set_title(GTK_WINDOW(export_window),
				     "exported ctree");
		gtk_container_set_border_width(GTK_CONTAINER
					       (export_window), 5);

		vbox = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(export_window), vbox);

		button = gtk_button_new_with_label("Close");
		gtk_box_pack_end(GTK_BOX(vbox), button, FALSE, TRUE, 0);

		gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
					  (GtkSignalFunc)
					  gtk_widget_destroy,
					  GTK_OBJECT(export_window));

		sep = gtk_hseparator_new();
		gtk_box_pack_end(GTK_BOX(vbox), sep, FALSE, TRUE, 10);

		export_ctree =
		    GTK_CTREE(gtk_ctree_new_with_titles(2, 0, title));
		gtk_ctree_set_line_style(export_ctree,
					 GTK_CTREE_LINES_DOTTED);

		scrolled_win = gtk_scrolled_window_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(scrolled_win),
				  GTK_WIDGET(export_ctree));
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW
					       (scrolled_win),
					       GTK_POLICY_AUTOMATIC,
					       GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start(GTK_BOX(vbox), scrolled_win, TRUE, TRUE,
				   0);
		gtk_clist_set_selection_mode(GTK_CLIST(export_ctree),
					     GTK_SELECTION_EXTENDED);
		gtk_clist_set_column_width(GTK_CLIST(export_ctree), 0,
					   200);
		gtk_clist_set_column_width(GTK_CLIST(export_ctree), 1,
					   200);
		gtk_widget_set_usize(GTK_WIDGET(export_ctree), 300, 200);
	}

	if (!GTK_WIDGET_VISIBLE(export_window))
		gtk_widget_show_all(export_window);

	gtk_clist_clear(GTK_CLIST(export_ctree));

	node = GTK_CTREE_NODE(g_list_nth(GTK_CLIST(ctree)->row_list,
					 GTK_CLIST(ctree)->focus_row));
	if (!node)
		return;

	gnode = gtk_ctree_export_to_gnode(ctree, NULL, NULL, node,
					  ctree2gnode, NULL);
	if (gnode) {
		gtk_ctree_insert_gnode(export_ctree, NULL, NULL, gnode,
				       gnode2ctree, NULL);
		g_node_destroy(gnode);
	}
}

static void change_indent(GtkWidget * widget, GtkCTree * ctree)
{
	gtk_ctree_set_indent(ctree, GTK_ADJUSTMENT(widget)->value);
}

static void change_spacing(GtkWidget * widget, GtkCTree * ctree)
{
	gtk_ctree_set_spacing(ctree, GTK_ADJUSTMENT(widget)->value);
}

static void change_row_height(GtkWidget * widget, GtkCList * clist)
{
	gtk_clist_set_row_height(clist, GTK_ADJUSTMENT(widget)->value);
}

static void set_background(GtkCTree * ctree, GtkCTreeNode * node,
			   gpointer data)
{
	GtkStyle *style = NULL;

	if (!node)
		return;

	if (ctree->line_style != GTK_CTREE_LINES_TABBED) {
		if (!GTK_CTREE_ROW(node)->is_leaf)
			style = GTK_CTREE_ROW(node)->row.data;
		else if (GTK_CTREE_ROW(node)->parent)
			style =
			    GTK_CTREE_ROW(GTK_CTREE_ROW(node)->parent)->
			    row.data;
	}

	gtk_ctree_node_set_row_style(ctree, node, style);
}
*/
/******************************************************************************
 * pmBookmarkTree call backs
 ******************************************************************************/
static void stringCallback(gchar * string, gpointer data)
{
	gchar *text[3];
	gchar buf[80], buf2[80];
	gint length, i = 0, j = 0;

	if ((string == NULL) || (strlen(string) == 0)) {
		(gchar *) data = NULL;
	} else {
		switch (GPOINTER_TO_INT(data)) {
		case 0:
			text[0] = string;
			text[1] = "GROUP";
			text[2] = "GROUP";
			gtk_ctree_insert_node(p_bmtree->ctree,
					      selected_node, NULL, text, 3,
					      pixmap1, mask1, pixmap2,
					      mask2, FALSE, FALSE);
			break;
		case 1:
			sprintf(buf, "%s", string);	  /*** make file name ***/
			text[0] = string;
			length = strlen(buf);
			if (length > 29)
				length = 29;
			while (i < length) {
				if (isalpha(buf[i])) {
					buf2[j] = buf[i];
					++j;
				}
				buf2[j] = '\0';
				++i;
			}
			sprintf(buf, "%s%s", buf2, ".conf");
			text[1] = buf;
			text[2] = "ROOT";
			g_warning(text[1]);
			gtk_ctree_insert_node(p_bmtree->ctree, NULL, NULL,
					      text, 3, pixmap1, mask1,
					      pixmap2, mask2, FALSE,
					      FALSE);
			break;
		}
	}

}

/*
 * add new sub group to selected group
 */
void on_new_activate(GtkMenuItem * menuitem, gpointer user_data)
{
	GtkWidget *dialog;
	gchar *buf;

	buf = "0";
	dialog =
	    gnome_request_dialog(FALSE,
				 "Enter SubGroup Name - use no commas",
				 NULL, 79,
				 (GnomeStringCallback) stringCallback,
				 GINT_TO_POINTER(0), GTK_WINDOW(MainFrm));
}

/*
 * add new root group to tree
 */
void on_add_new_group1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
	GtkWidget *dialog;

	dialog =
	    gnome_request_dialog(FALSE,
				 "Enter Group Name - use no commas",
				 NULL, 79,
				 (GnomeStringCallback) stringCallback,
				 GINT_TO_POINTER(1), GTK_WINDOW(MainFrm));
}


void
on_save_bookmarks1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
	savebookmarks(p_bmtree->ctree_widget);  /*** sw_bookmarks.cpp ***/
}

static gboolean applychangestobookmark;

void on_edit_item_activate(GtkMenuItem * menuitem, gpointer user_data)
{
	GtkCList 
		*clist;
	GtkWidget 
		*dlg;
	GtkCTreeNode 
		*node;
	gchar 
		*text[3];
	
	applychangestobookmark = FALSE;
	clist = GTK_CLIST(p_bmtree->ctree);
	gtk_clist_freeze(clist);

	node = clist->selection->data;
	text[0] = GTK_CELL_PIXTEXT(GTK_CTREE_ROW(node)->row.
				     cell[0])->text;
	text[1] = GTK_CELL_PIXTEXT(GTK_CTREE_ROW(node)->row.
				     cell[1])->text;		
	text[2] = GTK_CELL_PIXTEXT(GTK_CTREE_ROW(node)->row.
				     cell[2])->text;
	
	dlg = create_dlgEditBookMark (text,FALSE);
  		gnome_dialog_set_default(GNOME_DIALOG(dlg), 2);
		gnome_dialog_run_and_close(GNOME_DIALOG(dlg));
	if(applychangestobookmark){
		GTK_CELL_PIXTEXT(GTK_CTREE_ROW(node)->row.
				     cell[0])->text = g_strdup(text[0]);
		GTK_CELL_PIXTEXT(GTK_CTREE_ROW(node)->row.
				     cell[1])->text = g_strdup(text[1]);
		GTK_CELL_PIXTEXT(GTK_CTREE_ROW(node)->row.
				     cell[2])->text = g_strdup(text[2]);			      
		g_free(text[0]); /*** we used g_strdup() in  on_btnBMok_clicked() ***/	
		g_free(text[1]);
		g_free(text[2]);
		applychangestobookmark = FALSE;
	}	
	gtk_clist_thaw(clist);
	after_press(p_bmtree->ctree, NULL);
}


void on_delete_item_activate(GtkMenuItem * menuitem, gpointer user_data)
{
	remove_selection(p_bmtree->ctree_widget, p_bmtree->ctree);
}


void
on_allow_reordering_activate(GtkMenuItem * menuitem, gpointer user_data)
{
	gtk_clist_set_reorderable(GTK_CLIST(p_bmtree->ctree),
				  GTK_CHECK_MENU_ITEM(menuitem)->active);
}

/******************************************************************************
 * add leaf node to personal root
 ******************************************************************************/
void addbookmarktotree(gchar * modName, gchar * verse)
{
	GtkWidget *dlg;
	gchar 
		*text[3],
		buf[256];
	sprintf(buf,"%s %s",verse,modName);
	text[0] = buf;
	text[1] = verse;
	text[2] = modName;
	applychangestobookmark = FALSE;
	dlg = create_dlgEditBookMark (text,TRUE);
  		gnome_dialog_set_default(GNOME_DIALOG(dlg), 2);
		gnome_dialog_run_and_close(GNOME_DIALOG(dlg));
	if(applychangestobookmark){
		gtk_ctree_insert_node(p_bmtree->ctree, 
						personal_node, 
						NULL, text,
			      			3, 
						pixmap3, 
						mask3, 
						NULL, 
						NULL, 
						TRUE, 
						FALSE);
		g_free(text[0]); /*** we used g_strdup() in  on_btnBMok_clicked() ***/
		g_free(text[1]);
		g_free(text[2]);		
		applychangestobookmark = FALSE;
	}
}
 
void loadtree(GtkWidget * ctree1)
{
	GtkWidget *menu;
	GdkColor transparent = { 0 };
	bmtree.ctree = GTK_CTREE(ctree1);
	bmtree.ctree_widget = ctree1;
	p_bmtree = &bmtree;

	pixmap1 =
	    gdk_pixmap_create_from_xpm_d(ctree1->window, &mask1,
					 &transparent, book_closed_xpm);
	pixmap2 =
	    gdk_pixmap_create_from_xpm_d(MainFrm->window, &mask2,
					 &transparent, book_open_xpm);
	pixmap3 =
	    gdk_pixmap_create_from_xpm_d(MainFrm->window, &mask3,
					 &transparent, mini_page_xpm);

	gtk_signal_connect_after(GTK_OBJECT(p_bmtree->ctree_widget),
				 "button_press_event",
				 GTK_SIGNAL_FUNC(after_press), NULL);
	gtk_signal_connect_after(GTK_OBJECT(p_bmtree->ctree_widget),
				 "button_release_event",
				 GTK_SIGNAL_FUNC(after_press), NULL);
	gtk_signal_connect_after(GTK_OBJECT(p_bmtree->ctree_widget),
				 "tree_move", GTK_SIGNAL_FUNC(after_move),
				 NULL);
	gtk_signal_connect_after(GTK_OBJECT(p_bmtree->ctree_widget),
				 "end_selection",
				 GTK_SIGNAL_FUNC(after_press), NULL);
	gtk_signal_connect_after(GTK_OBJECT(p_bmtree->ctree_widget),
				 "toggle_focus_row",
				 GTK_SIGNAL_FUNC(after_press), NULL);
	gtk_signal_connect_after(GTK_OBJECT(p_bmtree->ctree_widget),
				 "select_all",
				 GTK_SIGNAL_FUNC(after_press), NULL);
	gtk_signal_connect_after(GTK_OBJECT(p_bmtree->ctree_widget),
				 "unselect_all",
				 GTK_SIGNAL_FUNC(after_press), NULL);
	gtk_signal_connect_after(GTK_OBJECT(p_bmtree->ctree_widget),
				 "scroll_vertical",
				 GTK_SIGNAL_FUNC(after_press), NULL);

	loadbookmarks(gs.ctree_widget);
	setleaf(ctree1);
	gtk_ctree_sort_recursive(p_bmtree->ctree, personal_node);
	
	gtk_signal_connect(GTK_OBJECT(gs.ctree_widget), "select_row",
			   GTK_SIGNAL_FUNC(on_ctree_select_row),
			   bmtree.ctree);
	gtk_clist_set_row_height(GTK_CLIST(p_bmtree->ctree), 15);
	gtk_ctree_set_spacing(p_bmtree->ctree, 3);
	gtk_ctree_set_indent(p_bmtree->ctree, 8);
	menu = create_pmBookmarkTree();
	gnome_popup_menu_attach(menu, p_bmtree->ctree_widget,
				(gchar *) "1");
	gtk_widget_set_sensitive(GTK_WIDGET
				 (pmBookmarkTree_uiinfo[0].widget), FALSE);
	//gtk_widget_set_sensitive(GTK_WIDGET(pmBookmarkTree_uiinfo[1].widget),FALSE);
}

GtkWidget *create_pmBookmarkTree(void)
{
	GtkWidget *pmBookmarkTree;

	pmBookmarkTree = gtk_menu_new();
	gtk_object_set_data(GTK_OBJECT(pmBookmarkTree), "pmBookmarkTree",
			    pmBookmarkTree);
	gnome_app_fill_menu(GTK_MENU_SHELL(pmBookmarkTree),
			    pmBookmarkTree_uiinfo, NULL, FALSE, 0);

	gtk_widget_ref(pmBookmarkTree_uiinfo[0].widget);
	gtk_object_set_data_full(GTK_OBJECT(pmBookmarkTree), "new",
				 pmBookmarkTree_uiinfo[0].widget,
				 (GtkDestroyNotify) gtk_widget_unref);

	gtk_widget_ref(pmBookmarkTree_uiinfo[1].widget);
	gtk_object_set_data_full(GTK_OBJECT(pmBookmarkTree),
				 "add_new_group1",
				 pmBookmarkTree_uiinfo[1].widget,
				 (GtkDestroyNotify) gtk_widget_unref);

	gtk_widget_ref(pmBookmarkTree_uiinfo[2].widget);
	gtk_object_set_data_full(GTK_OBJECT(pmBookmarkTree),
				 "save_bookmarks1",
				 pmBookmarkTree_uiinfo[2].widget,
				 (GtkDestroyNotify) gtk_widget_unref);

	gtk_widget_ref(pmBookmarkTree_uiinfo[3].widget);
	gtk_object_set_data_full(GTK_OBJECT(pmBookmarkTree), "delete_item",
				 pmBookmarkTree_uiinfo[3].widget,
				 (GtkDestroyNotify) gtk_widget_unref);

	gtk_widget_ref(pmBookmarkTree_uiinfo[4].widget);
	gtk_object_set_data_full(GTK_OBJECT(pmBookmarkTree),
				 "allow_reordering",
				 pmBookmarkTree_uiinfo[4].widget,
				 (GtkDestroyNotify) gtk_widget_unref);

	return pmBookmarkTree;
}

/*****************************************************************************
 * load bookmarks program start
 *****************************************************************************/
gint 
loadoldbookmarks(void)
{
	LISTITEM mylist;
	LISTITEM *p_mylist;
	int flbookmarksnew;
	gchar 
		*buf[3],
		tmpbuf[256];
	gint i = 0;
	long filesize;
	struct stat stat_p;
	GtkCTreeNode 
			*rootnode = NULL,
			*node = NULL,
			*parent = NULL;
	gint ibookmarks;
	
	stat(fnbookmarksnew, &stat_p);
	filesize = stat_p.st_size;
	ibookmarks = (filesize / (sizeof(mylist)));
	p_mylist = &mylist;
        /* try to open file */ 
	if ((flbookmarksnew = open(fnbookmarksnew, O_RDONLY)) == -1) {	
	       return 0;	
	}else{
		buf[0] = "Personal";
		buf[1] = "personal.conf";
		buf[2] = "ROOT";
		rootnode = gtk_ctree_insert_node(p_bmtree->ctree,NULL,NULL,buf, 3, pixmap1,mask1,pixmap2, mask2, FALSE, FALSE);
		while (i < ibookmarks) {
			read(flbookmarksnew, (char *) &mylist, sizeof(mylist));		
			if (p_mylist->type == 1) {  /* if type is 1 it is a subtree (submenu) */
				buf[0] = p_mylist->item;
				buf[1] = "GROUP";
				buf[2] = "GROUP";
				if (p_mylist->level == 0) {
					parent = gtk_ctree_insert_node(p_bmtree->ctree,rootnode,NULL,buf, 3, pixmap1,mask1,pixmap2, mask2, FALSE, FALSE);
				}else{
					parent = gtk_ctree_insert_node(p_bmtree->ctree,parent,NULL,buf, 3, pixmap1,mask1,pixmap2, mask2, FALSE, FALSE);
				}
			} else { 
				sprintf(tmpbuf,"%s %s",p_mylist->item,"KJV");
				buf[0] = tmpbuf;
				buf[1] = p_mylist->item; 
				buf[2] = "KJV"; 
				if (p_mylist->level == 0) {
					node = gtk_ctree_insert_node(p_bmtree->ctree,rootnode,NULL,buf, 3, pixmap1,mask1,pixmap2, mask2, FALSE, FALSE);
				}else{
					node = gtk_ctree_insert_node(p_bmtree->ctree,parent,NULL,buf, 3, pixmap1,mask1,pixmap2, mask2, FALSE, FALSE);
				}
			}
			++i;
		} 
		close(flbookmarksnew);
	}
	setleaf(p_bmtree->ctree_widget);
	return 1;
}


static void
on_entryBM_changed                (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkWidget *dlg, *btnok, *btnapply;

	dlg = gtk_widget_get_toplevel(GTK_WIDGET(editable));
	btnok = lookup_widget(dlg, "btnBMok");
	btnapply = lookup_widget(dlg, "btnBMapply");
	gtk_widget_set_sensitive(btnok, TRUE);
	gtk_widget_set_sensitive(btnapply, TRUE);
	
}


static void
on_btnBMok_clicked      (GtkButton       *button,
                                        gchar *buf[3])
{
	GtkWidget 
		*dlg, 
		*labelentry, 
		*keyentry, 
		*modentry;
	
	dlg = gtk_widget_get_toplevel(GTK_WIDGET(button));	
	labelentry = lookup_widget(dlg, "entryBMLabel");
	keyentry = lookup_widget(dlg, "entryBMKey");
	modentry= lookup_widget(dlg, "entryBMMod");
	/*** freed in addbookmarktotree() ***/
	buf[0] = g_strdup(gtk_entry_get_text(GTK_ENTRY(labelentry)));
	buf[1] = g_strdup(gtk_entry_get_text(GTK_ENTRY(keyentry)));
	buf[2] = g_strdup(gtk_entry_get_text(GTK_ENTRY(modentry)));
	applychangestobookmark = TRUE;
	gtk_widget_destroy(dlg);
	//g_warning("buf1 = %s, buf2 = %s, buf3 = %s",buf[0],buf[1],buf[2]);
}


static void
on_btnBMapply_clicked  (GtkButton       *button,
                                        gchar *buf[3])
{
	GtkWidget 
		*dlg, 
		*btnok, 
		*btnapply,
		*labelentry, 
		*keyentry, 
		*modentry;

	dlg = gtk_widget_get_toplevel(GTK_WIDGET(button));
	btnok = lookup_widget(dlg, "btnBMok");
	btnapply = lookup_widget(dlg, "btnBMapply");	
	labelentry = lookup_widget(dlg, "entryBMLabel");
	keyentry = lookup_widget(dlg, "entryBMKey");
	modentry= lookup_widget(dlg, "entryBMMod");
	/*** freed in addbookmarktotree() ***/
	buf[0] = g_strdup(gtk_entry_get_text(GTK_ENTRY(labelentry)));
	buf[1] = g_strdup(gtk_entry_get_text(GTK_ENTRY(keyentry)));
	buf[2] = g_strdup(gtk_entry_get_text(GTK_ENTRY(modentry)));	
	gtk_widget_set_sensitive(btnok, FALSE);
	gtk_widget_set_sensitive(btnapply, FALSE);
	applychangestobookmark = TRUE;
}


static void
on_btnEBMcancel_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget 
		*dlg;
	
	applychangestobookmark = FALSE;
	dlg = gtk_widget_get_toplevel(GTK_WIDGET(button));
	gtk_widget_destroy(dlg);
}

/*** open dialog to allow editing of a bookmark ***/
GtkWidget*
create_dlgEditBookMark (gchar *text[3], gboolean newbookmark)
{
  GtkWidget *dlgEditBookMark;
  GtkWidget *dialog_vbox14;
  GtkWidget *table11;
  GtkWidget *label170;
  GtkWidget *label171;
  GtkWidget *label172;
  GtkWidget *entryBMLabel;
  GtkWidget *entryBMKey;
  GtkWidget *entryBMMod;
  GtkWidget *dialog_action_area14;
  GtkWidget *btnBMok;
  GtkWidget *btnBMapply;
  GtkWidget *btnEBMcancel;

  dlgEditBookMark = gnome_dialog_new (_("GnomeSword - Edit Bookmark"), NULL);
  gtk_object_set_data (GTK_OBJECT (dlgEditBookMark), "dlgEditBookMark", dlgEditBookMark);
  gtk_window_set_policy (GTK_WINDOW (dlgEditBookMark), FALSE, FALSE, FALSE);

  dialog_vbox14 = GNOME_DIALOG (dlgEditBookMark)->vbox;
  gtk_object_set_data (GTK_OBJECT (dlgEditBookMark), "dialog_vbox14", dialog_vbox14);
  gtk_widget_show (dialog_vbox14);

  table11 = gtk_table_new (3, 2, FALSE);
  gtk_widget_ref (table11);
  gtk_object_set_data_full (GTK_OBJECT (dlgEditBookMark), "table11", table11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table11);
  gtk_box_pack_start (GTK_BOX (dialog_vbox14), table11, TRUE, TRUE, 0);

  label170 = gtk_label_new (_("Label"));
  gtk_widget_ref (label170);
  gtk_object_set_data_full (GTK_OBJECT (dlgEditBookMark), "label170", label170,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label170);
  gtk_table_attach (GTK_TABLE (table11), label170, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label170), 0, 0.5);

  label171 = gtk_label_new (_("Reference  "));
  gtk_widget_ref (label171);
  gtk_object_set_data_full (GTK_OBJECT (dlgEditBookMark), "label171", label171,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label171);
  gtk_table_attach (GTK_TABLE (table11), label171, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label171), 0, 0.5);

  label172 = gtk_label_new (_("Module"));
  gtk_widget_ref (label172);
  gtk_object_set_data_full (GTK_OBJECT (dlgEditBookMark), "label172", label172,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label172);
  gtk_table_attach (GTK_TABLE (table11), label172, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label172), 0, 0.5);

  entryBMLabel = gtk_entry_new ();
  gtk_widget_ref (entryBMLabel);
  gtk_object_set_data_full (GTK_OBJECT (dlgEditBookMark), "entryBMLabel", entryBMLabel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entryBMLabel);
  gtk_table_attach (GTK_TABLE (table11), entryBMLabel, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_entry_set_text(GTK_ENTRY(entryBMLabel), text[0]);

  entryBMKey = gtk_entry_new ();
  gtk_widget_ref (entryBMKey);
  gtk_object_set_data_full (GTK_OBJECT (dlgEditBookMark), "entryBMKey", entryBMKey,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entryBMKey);
  gtk_table_attach (GTK_TABLE (table11), entryBMKey, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_entry_set_text(GTK_ENTRY(entryBMKey), text[1]);
  
  entryBMMod = gtk_entry_new ();
  gtk_widget_ref (entryBMMod);
  gtk_object_set_data_full (GTK_OBJECT (dlgEditBookMark), "entryBMMod", entryBMMod,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entryBMMod);
  gtk_table_attach (GTK_TABLE (table11), entryBMMod, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_entry_set_text(GTK_ENTRY(entryBMMod), text[2]);

  dialog_action_area14 = GNOME_DIALOG (dlgEditBookMark)->action_area;
  gtk_object_set_data (GTK_OBJECT (dlgEditBookMark), "dialog_action_area14", dialog_action_area14);
  gtk_widget_show (dialog_action_area14);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area14), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area14), 8);

  gnome_dialog_append_button (GNOME_DIALOG (dlgEditBookMark), GNOME_STOCK_BUTTON_OK);
  btnBMok = GTK_WIDGET (g_list_last (GNOME_DIALOG (dlgEditBookMark)->buttons)->data);
  gtk_widget_ref (btnBMok);
  gtk_object_set_data_full (GTK_OBJECT (dlgEditBookMark), "btnBMok", btnBMok,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (btnBMok);
  if(newbookmark)
  	gtk_widget_set_sensitive (btnBMok, TRUE);
  else
	gtk_widget_set_sensitive (btnBMok, FALSE);
  
  GTK_WIDGET_SET_FLAGS (btnBMok, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dlgEditBookMark), GNOME_STOCK_BUTTON_APPLY);
  btnBMapply = GTK_WIDGET (g_list_last (GNOME_DIALOG (dlgEditBookMark)->buttons)->data);
  gtk_widget_ref (btnBMapply);
  gtk_object_set_data_full (GTK_OBJECT (dlgEditBookMark), "btnBMapply", btnBMapply,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (btnBMapply);
  gtk_widget_set_sensitive (btnBMapply, FALSE);
  GTK_WIDGET_SET_FLAGS (btnBMapply, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dlgEditBookMark), GNOME_STOCK_BUTTON_CANCEL);
  btnEBMcancel = GTK_WIDGET (g_list_last (GNOME_DIALOG (dlgEditBookMark)->buttons)->data);
  gtk_widget_ref (btnEBMcancel);
  gtk_object_set_data_full (GTK_OBJECT (dlgEditBookMark), "btnEBMcancel", btnEBMcancel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (btnEBMcancel);
  GTK_WIDGET_SET_FLAGS (btnEBMcancel, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (entryBMLabel), "changed",
                      GTK_SIGNAL_FUNC (on_entryBM_changed),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (entryBMKey), "changed",
                      GTK_SIGNAL_FUNC (on_entryBM_changed),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (entryBMMod), "changed",
                      GTK_SIGNAL_FUNC (on_entryBM_changed),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (btnBMok), "clicked",
                      GTK_SIGNAL_FUNC (on_btnBMok_clicked),
                      text);
  gtk_signal_connect (GTK_OBJECT (btnBMapply), "clicked",
                      GTK_SIGNAL_FUNC (on_btnBMapply_clicked),
                      text);
  gtk_signal_connect (GTK_OBJECT (btnEBMcancel), "clicked",
                      GTK_SIGNAL_FUNC (on_btnEBMcancel_clicked),
                      NULL);

  return dlgEditBookMark;
}



