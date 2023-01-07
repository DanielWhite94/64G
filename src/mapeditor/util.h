#ifndef MAPEDITOR_UTIL_H
#define MAPEDITOR_UTIL_H

#include <cairo.h>
#include <cstdarg>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

namespace MapEditor {
	bool utilPromptForYesNo(const char *title, const char *prompt, GtkWidget *parentWindow);
	void utilPromptForOk(const char *title, const char *prompt, GtkWidget *parentWindow);
	int utilPromptCustom(const char *title, const char *prompt, GtkWidget *parentWindow, const char *firstButton, ...);

	void utilDoGtkEvents(void); // process a finite (small) number of GTK events
};

#endif
