#include <cassert>
#include <cstdio>
#include <exception>
#include <iostream>

#include "mainwindow.h"

const char mainWindowXmlFile[]="mainwindow.glade";

gboolean mapEditorMainWindowWrapperWindowDeleteEvent(GtkWidget *widget, GdkEvent *event, void *userData);

namespace MapEditor {
	MainWindow::MainWindow() {
		// Use GtkBuilder to build our interface from the XML file.
		GtkBuilder *builder=gtk_builder_new();
		GError *err=NULL;
		if (gtk_builder_add_from_file(builder, mainWindowXmlFile, &err)==0) {
			g_error_free(err);
			throw std::runtime_error("could not init main window glade file"); // TODO: use 'err' for more info
		}

		// Connect glade signals
		gtk_builder_connect_signals(builder, NULL);

		// Extract widgets we will need.
		bool error=false;
		error|=(window=GTK_WIDGET(gtk_builder_get_object(builder, "window")))==NULL;
		if (error)
			throw std::runtime_error("could not grab main window widgets");

		// Connect manual signals
		g_signal_connect(window, "delete-event", G_CALLBACK(mapEditorMainWindowWrapperWindowDeleteEvent), (void *)this);

		// Free memory used by GtkBuilder object.
		g_object_unref(G_OBJECT(builder));
	}

	MainWindow::~MainWindow() {
		// Destroy window (and all other child widgets)
		if (window!=NULL)
			gtk_widget_destroy(window);

	}

	void MainWindow::show() {
		// Ensure window is on top and has focus
		gtk_widget_show_all(window);
		gtk_window_present(GTK_WINDOW(window));
	}

	void MainWindow::hide() {
		gtk_widget_hide(window);
	}

	bool MainWindow::deleteEvent(GtkWidget *widget, GdkEvent *event) {
		// Call gtk_main_quit function to cause gtk_main call in our main function to return.
		gtk_main_quit();

		return TRUE;
	}
};

gboolean mapEditorMainWindowWrapperWindowDeleteEvent(GtkWidget *widget, GdkEvent *event, void *userData) {
	assert(widget!=NULL);
	assert(userData!=NULL);

	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->deleteEvent(widget, event);
}
