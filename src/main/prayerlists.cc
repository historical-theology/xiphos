/*
 * GnomeSword Bible Study Tool
 * prayerlists.cc - code to create several different prayer lists
 *
 * Copyright (C) 2008 GnomeSword Developer Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gnome.h>
#include <swmodule.h>
#include <swmgr.h>
#include <treekeyidx.h>
#include <rawgenbook.h>
#include <swconfig.h>
#include <iostream>
#include <string>
#include <stdio.h>


#include "main/mod_mgr.h"
#include "main/prayerlists.h"
#include "main/sword_treekey.h"
#include "main/settings.h"
#include "main/sidebar.h"
#include "main/sword.h"

#include "gui/dialog.h"
#include "gui/sidebar.h"

#include "backend/sword_main.hh"

#ifndef NO_SWORD_NAMESPACE
using sword::TreeKeyIdx;
using sword::RawGenBook;
using sword::SWKey;
using sword::SWConfig;
using sword::SWModule;
#endif


enum
{
	COL_OPEN_PIXBUF,
	COL_CLOSED_PIXBUF,
	COL_CAPTION,
	COL_MODULE,
	COL_OFFSET,
	N_COLUMNS
};


static int pl_month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static char *(pl_months[]) = {
    _("January"),
    _("February"),
    _("March"),
    _("April"),
    _("May"),
    _("June"),
    _("July"),
    _("August"),
    _("September"),
    _("October"),
    _("November"),
    _("December")
};

/*********************************    *************************************/

static
void setEntryText (RawGenBook * book, const gchar * text)
{
	TreeKeyIdx *treeKey = (TreeKeyIdx *) (SWKey *) (*book);
	if (treeKey->getOffset()) {
		(*book) << text;
	}
}


static
void appendChild (TreeKeyIdx * treeKey, const gchar * name)
{
	treeKey->appendChild();
	treeKey->setLocalName(name);
	treeKey->save();
	GS_message(("name: %s\nlocalName: %s", name,
		    treeKey->getLocalName ()));
}


static
void appendSibling (TreeKeyIdx * treeKey, const gchar * name)
{
	if (treeKey->getOffset()) {
		treeKey->append();
		treeKey->setLocalName(name);
		treeKey->save();
	}
}

static
void add_prayer_list_sections (RawGenBook * book, TreeKeyIdx * treeKey)
{
	appendChild(treeKey, _("Growth"));
	setEntryText(book, _("<b>For Growth</b><br>"));
	appendSibling(treeKey, _("Salvation"));
	setEntryText(book, _("<b>For Salvation</b><br>"));
	appendSibling(treeKey, _("Health"));
	setEntryText(book, _("<b>For Health</b><br>"));
	appendSibling(treeKey, _("Misc"));
	setEntryText(book, _("<b>Miscellaneous</b><br>"));

	treeKey->parent();
}


static
void add_prayer_list_days(RawGenBook * book,
			  TreeKeyIdx * treeKey,
			  int month)
{
	for (int day = 0; day < pl_month_days[month]; ++day) {
		char daynum[4], monthday[32];
		snprintf(daynum, 3, "%02d", day+1);
		snprintf(monthday, 31, "<b>%s %d</b><br>", pl_months[month], day+1);

		if (day == 0)
			appendChild(treeKey, daynum);
		else
			appendSibling(treeKey, daynum);

		setEntryText(book, monthday);
	}

	treeKey->parent();
}

/******************************************************************************
 * Name
 *   prayerlist_construct
 *
 * Synopsis
 *   #include "main/prayer_list.h"
 *   void prayerlist_fundamentals(gchar *listname, gchar *summary)
 *
 * Description
 *   manufacture prayer list configuration fundamentals.
 *
 */

void
prayerlist_construct(gchar *listname, gchar *summary)
{
	// configuration file generation.
	gchar *path = g_strdup_printf("%s/.sword/mods.d/%s.conf",
				      settings.homedir, listname);
	SWConfig config (path);
	g_free(path);

	// configuration file content.
	path = g_strdup_printf("./modules/genbook/rawgenbook/%s/%s",
			       listname, listname);
	config[listname]["DataPath"] = path;
	g_free(path);
	config[listname]["ModDrv"] = "RawGenBook";
	config[listname]["GSType"] = "PrayerList";
	config[listname]["Encoding"] = "UTF-8";
	config[listname]["Lang"] = sword_locale; // set at startup.
	config[listname]["Version"] = "0.1";
	config[listname]["MinimumVersion"] = "1.5.11";
	config[listname]["DisplayLevel"] = "2";	// ?
	config[listname]["Description"] = _("Prayer List/Journal");
	config[listname]["About"] = summary;	// as provided.
	config.Save();

	// fundamentals are complete.  update.
	main_update_module_lists();
	main_load_module_tree(sidebar.module_list);
}


/******************************************************************************
 * Name
 *   prayerlist_fundamentals
 *
 * Synopsis
 *   #include "main/prayer_list.h"
 *   gchar *prayerlist_fundamentals(gchar *summary)
 *
 * Description
 *   manufacture prayer list configuration fundamentals.
 *
 * Return value
 *   name of created module; NULL if failure.
 */

gchar *
prayerlist_fundamentals(gchar *summary)
{
	char *listname = NULL;	// assume failure.
	GS_DIALOG *info;
	gint test;
	gchar *path = NULL;

	// name selection dialog.
	info = gui_new_dialog();
	info->stock_icon = (gchar *)GTK_STOCK_DIALOG_QUESTION;
	info->title = _("Prayer List/Journal");
	info->label_top = _("Name for new prayer list or journal");
	info->label1 = _("Name: ");
	info->text1 = g_strdup(_("MyPrayerList"));
	info->ok = TRUE;
	info->cancel = TRUE;
	test = gui_gs_dialog(info);

	// result of name selection.
	if (test != GS_OK) {
		goto out;	// cancelled.
	}

	for (char *s = info->text1; *s; ++s) {
		if (!isalnum(*s)) {
			gui_generic_warning
			    (_("Module names must be letters+digits only."));
			goto out;
		}
	}

	if (main_is_module(info->text1)) {
		gui_generic_warning
		    (_("GnomeSword already knows a module by that name."));
		goto out;
	}

	// path creation based on name selection.
	path = g_strdup_printf("%s/.sword/modules/genbook/rawgenbook/%s",
				settings.homedir, info->text1);
	if (access(path, F_OK) == 0) {
		gui_generic_warning
		    (_("GnomeSword finds that prayer list already."));
		goto out;
	} else {
		if ((mkdir(path, S_IRWXU)) != 0) {
			gui_generic_warning
			    (_("GnomeSword cannot create module's path."));
			goto out;
		}
	}

	// wild, raving applause: we got through all the sanity checks.
	listname = g_strdup(info->text1);

	prayerlist_construct(listname, summary);

out:
	// both success and failure fall through here.
	if (path)
		g_free(path);
	g_free(info->text1);
	g_free(info);
	return listname;
}

/******************************************************************************
 * Name
 *   main_prayerlist_basic_create
 *
 * Synopsis
 *   #include "main/prayer_list.h"
 *   gint main_prayerlist_basic_create(void)
 *
 * Description
 *   create a simple prayer list module & add to sidebar module tree
 *
 * Return value
 *   int
 */

gboolean
main_prayerlist_basic_create(void)
{
	char *listname = prayerlist_fundamentals
	    (_("A simple prayer list \\par\\par Module created by GnomeSword"));
	if (listname == NULL)
		return FALSE;

	gchar *path = g_strdup_printf
	    ("%s/.sword/modules/genbook/rawgenbook/%s/%s",
	     settings.homedir, listname, listname);
	g_free(listname);
	RawGenBook::createModule (path);
	RawGenBook *book = new RawGenBook (path);

	TreeKeyIdx root = *((TreeKeyIdx *) ((SWKey *) (*book)));
	TreeKeyIdx *treeKey = (TreeKeyIdx *) (SWKey *) (*book);

	appendChild(treeKey, _("MyPrayerList"));
	setEntryText(book, _("<b>People:</b><br>Bob<br>Sam<br>Sue<br><br><b>Church:</b><br>pews<br>fellowship<br>Bibles for missionaries<br><br><br>"));

	delete treeKey;
	return TRUE;
}


/******************************************************************************
 * Name
 *   main_prayerlist_subject_create
 *
 * Synopsis
 *   #include "main/prayer_list.h"
 *   gint main_prayerlist_subject_create(void)
 *
 * Description
 *   create a subject prayer list module & add to sidebar module tree
 *
 * Return value
 *   int
 */

gboolean
main_prayerlist_subject_create(void)
{
	char *listname = prayerlist_fundamentals
	    (_("A subject-based prayer list \\par\\par Module created by GnomeSword"));
	if (listname == NULL)
		return FALSE;

	gchar *path = g_strdup_printf
	    ("%s/.sword/modules/genbook/rawgenbook/%s/%s",
	     settings.homedir, listname, listname);
	g_free(listname);
	RawGenBook::createModule (path);
	RawGenBook *book = new RawGenBook (path);

	TreeKeyIdx root = *((TreeKeyIdx *) ((SWKey *) (*book)));
	TreeKeyIdx *treeKey = (TreeKeyIdx *) (SWKey *) (*book);

	appendChild(treeKey, _("Salvation"));
	setEntryText(book, _("Bob<br>Sam<br>Sue<br>John<br>"));
	appendSibling(treeKey, _("Spiritual Growth"));
	setEntryText(book, _("Mike<br>Steve<br>"));
	appendSibling(treeKey, _("Health"));
	setEntryText(book, _("Sue<br>John<br>"));

	delete treeKey;
	return TRUE;
}


/******************************************************************************
 * Name
 *   main_prayerlist_wild_create
 *
 * Synopsis
 *   #include "main/prayer_list.h"
 *   gint main_prayerlist_wild_create(void)
 *
 * Description
 *   create a monthly prayer list module & add to sidebar module tree
 *
 * Return value
 *   int
 */

gboolean
main_prayerlist_monthly_create(void)
{
	char *listname = prayerlist_fundamentals
	    (_("An annual prayer list \\par\\par Module created by GnomeSword"));
	if (listname == NULL)
		return FALSE;

	gchar *path = g_strdup_printf
	    ("%s/.sword/modules/genbook/rawgenbook/%s/%s",
	     settings.homedir, listname, listname);
	g_free(listname);
	RawGenBook::createModule(path);
	RawGenBook *book = new RawGenBook (path);

	TreeKeyIdx root = *((TreeKeyIdx *) ((SWKey *) (*book)));
	TreeKeyIdx *treeKey = (TreeKeyIdx *) (SWKey *) (*book);

	for (int month = 0; month < 12; ++month)
	{
		char monthnum[4], monthstring[32];
		snprintf(monthnum, 3, "%02d", month+1);
		snprintf(monthstring, 31, "<b>%s</b><br>", pl_months[month]);

		if (month == 0)
			appendChild(treeKey, monthnum);
		else
			appendSibling(treeKey, monthnum);
		setEntryText(book, monthstring);
		add_prayer_list_sections(book, treeKey);
	}

	delete treeKey;
	return TRUE;
}

/******************************************************************************
 * Name
 *   main_prayerlist_journal_create
 *
 * Synopsis
 *   #include "main/prayer_list.h"
 *   gint main_prayerlist_journal_create(void)
 *
 * Description
 *   create a daily journal prayer list module & add to sidebar module tree
 *
 * Return value
 *   int
 */

gboolean
main_prayerlist_journal_create(void)
{
	char *listname = prayerlist_fundamentals
	    (_("An annual prayer list \\par\\par Module created by GnomeSword"));
	if (listname == NULL)
		return FALSE;

	gchar *path = g_strdup_printf
	    ("%s/.sword/modules/genbook/rawgenbook/%s/%s",
	     settings.homedir, listname, listname);
	g_free(listname);
	RawGenBook::createModule(path);
	RawGenBook *book = new RawGenBook (path);

	TreeKeyIdx root = *((TreeKeyIdx *) ((SWKey *) (*book)));
	TreeKeyIdx *treeKey = (TreeKeyIdx *) (SWKey *) (*book);

	for (int month = 0; month < 12; ++month)
	{
		char monthnum[4], monthstring[32];
		snprintf(monthnum, 3, "%02d", month+1);
		snprintf(monthstring, 31, "<b>%s</b><br>", pl_months[month]);

		if (month == 0)
			appendChild(treeKey, monthnum);
		else
			appendSibling(treeKey, monthnum);
		setEntryText(book, monthstring);
		add_prayer_list_days(book, treeKey, month);
	}

	delete treeKey;
	return TRUE;
}
