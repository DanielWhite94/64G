#include <cassert>
#include <cstdio>
#include <exception>
#include <iostream>

#include "mainwindow.h"

const char mainWindowXmlFile[]="mainwindow.glade";

gboolean mapEditorMainWindowWrapperWindowDeleteEvent(GtkWidget *widget, GdkEvent *event, void *userData);

gboolean mapEditorMainWindowWrapperMenuFileNewActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileOpenActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileSaveActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileSaveAsActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileCloseActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileQuitActivate(GtkWidget *widget, gpointer userData);

namespace MapEditor {
	MainWindow::MainWindow() {
		// Clear basic fields
		map=NULL;

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
		error|=(menuFileNew=GTK_WIDGET(gtk_builder_get_object(builder, "menuFileNew")))==NULL;
		error|=(menuFileOpen=GTK_WIDGET(gtk_builder_get_object(builder, "menuFileOpen")))==NULL;
		error|=(menuFileSave=GTK_WIDGET(gtk_builder_get_object(builder, "menuFileSave")))==NULL;
		error|=(menuFileSaveAs=GTK_WIDGET(gtk_builder_get_object(builder, "menuFileSaveAs")))==NULL;
		error|=(menuFileClose=GTK_WIDGET(gtk_builder_get_object(builder, "menuFileClose")))==NULL;
		error|=(menuFileQuit=GTK_WIDGET(gtk_builder_get_object(builder, "menuFileQuit")))==NULL;
		error|=(statusLabel=GTK_WIDGET(gtk_builder_get_object(builder, "statusLabel")))==NULL;
		if (error)
			throw std::runtime_error("could not grab main window widgets");

		// Connect manual signals
		g_signal_connect(window, "delete-event", G_CALLBACK(mapEditorMainWindowWrapperWindowDeleteEvent), (void *)this);
		g_signal_connect(menuFileNew, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuFileNewActivate), (void *)this);
		g_signal_connect(menuFileOpen, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuFileOpenActivate), (void *)this);
		g_signal_connect(menuFileSave, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuFileSaveActivate), (void *)this);
		g_signal_connect(menuFileSaveAs, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuFileSaveAsActivate), (void *)this);
		g_signal_connect(menuFileClose, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuFileCloseActivate), (void *)this);
		g_signal_connect(menuFileQuit, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuFileQuitActivate), (void *)this);

		// Free memory used by GtkBuilder object.
		g_object_unref(G_OBJECT(builder));

		// Ensure widgets are setup correctly before showing the window
		updateFileMenuSensitivity();
		updateTitle();
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
		mapQuit();
		return TRUE;
	}

	bool MainWindow::menuFileNewActivate(GtkWidget *widget) {
		mapNew();
		return false;
	}

	bool MainWindow::menuFileOpenActivate(GtkWidget *widget) {
		mapOpen();
		return false;
	}

	bool MainWindow::menuFileSaveActivate(GtkWidget *widget) {
		mapSave();
		return false;
	}

	bool MainWindow::menuFileSaveAsActivate(GtkWidget *widget) {
		mapSaveAs();
		return false;
	}

	bool MainWindow::menuFileCloseActivate(GtkWidget *widget) {
		mapClose();
		return false;
	}

	bool MainWindow::menuFileQuitActivate(GtkWidget *widget) {
		mapQuit();
		return false;
	}

	bool MainWindow::mapNew(void) {
		// Close the current map (if any)
		if (!mapClose())
			return false;

		// Prompt user for filename
		GtkWidget *dialog=gtk_file_chooser_dialog_new("New Map", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
		    "Cancel", GTK_RESPONSE_CANCEL,
		    "Create", GTK_RESPONSE_ACCEPT,
		NULL);

		if (gtk_dialog_run(GTK_DIALOG(dialog))!=GTK_RESPONSE_ACCEPT)
			return false;

		// Extract filename
		char *filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_widget_destroy(dialog);

		// Attempt to load map
		map=new class Map(filename);
		if (map==NULL || !map->initialized) {
			delete map;
			map=NULL;
			return false;
		}

		// Update various things
		char statusLabelStr[1024]; // TODO: better
		sprintf(statusLabelStr, "Created new map at: %s", filename);
		gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);

		updateFileMenuSensitivity();
		updateTitle();

		// Tidy up
		g_free(filename);

		return true;
	}

	bool MainWindow::mapOpen(void) {
		// Close the current map (if any)
		if (!mapClose())
			return false;

		// Prompt user for filename
		GtkWidget *dialog=gtk_file_chooser_dialog_new("Open Map", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		    "Cancel", GTK_RESPONSE_CANCEL,
		    "Open", GTK_RESPONSE_ACCEPT,
		NULL);

		if (gtk_dialog_run(GTK_DIALOG(dialog))!=GTK_RESPONSE_ACCEPT)
			return false;

		// Extract filename
		char *filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_widget_destroy(dialog);

		// Attempt to load map
		map=new class Map(filename);
		if (map==NULL || !map->initialized) {
			delete map;
			map=NULL;
			return false;
		}

		// Update various things
		char statusLabelStr[1024]; // TODO: better
		sprintf(statusLabelStr, "Opened map at: %s", filename);
		gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);

		updateFileMenuSensitivity();
		updateTitle();

		// Tidy up
		g_free(filename);

		return true;
	}

	bool MainWindow::mapSave(void) {
		// No map loaded?
		if (map==NULL)
			return false;

		// Attempt to save
		if (!map->save())
			return false;

		// Update various things
		gtk_label_set_text(GTK_LABEL(statusLabel), "Map saved");

		updateFileMenuSensitivity();
		updateTitle();

		return true;
	}

	bool MainWindow::mapSaveAs(void) {
		// No map loaded?
		if (map==NULL)
			return false;

		// Prompt user for new filename
		GtkWidget *dialog=gtk_file_chooser_dialog_new("Save Map As", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
		    "Cancel", GTK_RESPONSE_CANCEL,
		    "Save As", GTK_RESPONSE_ACCEPT,
		NULL);

		if (gtk_dialog_run(GTK_DIALOG(dialog))!=GTK_RESPONSE_ACCEPT)
			return false;

		// Extract filename
		char *filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_widget_destroy(dialog);

		// Change path and save
		// TODO: this
		return false;

		// Update various things
		char statusLabelStr[1024]; // TODO: better
		sprintf(statusLabelStr, "Map now saved to: %s", filename);
		gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);

		updateFileMenuSensitivity();
		updateTitle();

		// Tidy up
		g_free(filename);

		return true;
	}

	bool MainWindow::mapClose(void) {
		// No map even loaded?
		if (map==NULL)
			return true;

		// If there are unsaved changes prompt the user for confirmation.
		/*
		TODO: this
		if (....unsaved changes) {
			switch(.....response) {
				case .....save:
					if (!mapSave())
						return false;
				break;
				case .....saveas:
					if (!mapSaveAs())
						return false;
				break;
				case .....discard:
					// Nothing to do - simply avoid saving
				break;
				case .....cancel:
					return false;
				break;
			}
		}
		*/

		// Unload the current map
		delete map;
		map=NULL;

		// Update various things
		gtk_label_set_text(GTK_LABEL(statusLabel), "Closed map");

		updateFileMenuSensitivity();
		updateTitle();

		return true;
	}

	void MainWindow::mapQuit(void) {
		// Close the current map (if any)
		if (!mapClose())
			return;

		// Call gtk_main_quit function to cause gtk_main call in our main function to return.
		gtk_main_quit();
	}

	bool MainWindow::mapGetUnsavedChanges(void) {
		// TODO: this
		return false;
	}

	void MainWindow::updateFileMenuSensitivity(void) {
		bool mapOpen=(map!=NULL);
		bool mapChanges=mapGetUnsavedChanges();

		gtk_widget_set_sensitive(menuFileSave, (mapOpen && mapChanges));
		gtk_widget_set_sensitive(menuFileSaveAs, mapOpen);
		gtk_widget_set_sensitive(menuFileClose, mapOpen);
	}

	void MainWindow::updateTitle(void) {
		char str[2048]; // TODO: better
		if (map!=NULL)
			sprintf(str, "%s%s - Map Editor", map->getBaseDir(), (mapGetUnsavedChanges() ? "*" : ""));
		else
			sprintf(str, "Map Editor");
		gtk_window_set_title(GTK_WINDOW(window), str);
	}
};

gboolean mapEditorMainWindowWrapperWindowDeleteEvent(GtkWidget *widget, GdkEvent *event, void *userData) {
	assert(widget!=NULL);
	assert(userData!=NULL);

	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->deleteEvent(widget, event);
}

gboolean mapEditorMainWindowWrapperMenuFileNewActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuFileNewActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuFileOpenActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuFileOpenActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuFileSaveActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuFileSaveActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuFileSaveAsActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuFileSaveAsActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuFileCloseActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuFileCloseActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuFileQuitActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuFileQuitActivate(widget);
}
