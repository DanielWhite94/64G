#include <cassert>
#include <exception>
#include <stdexcept>

#include "newdialogue.h"

using namespace MapEditor;

namespace MapEditor {
	NewDialogue::NewDialogue(GtkWidget *parentWindow) {
		const char xmlFile[]="newdialogue.glade";

		// Init
		window=NULL;

		// Use GtkBuilder to build our interface from the XML file.
		GtkBuilder *builder=gtk_builder_new();
		GError *err=NULL;
		if (gtk_builder_add_from_file(builder, xmlFile, &err)==0) {
			g_error_free(err);
			throw std::runtime_error("could not init new dialogue glade file"); // TODO: use 'err->message' for more info
		}

		// Connect glade signals
		gtk_builder_connect_signals(builder, NULL);

		// Extract widgets we will need.
		bool error=false;
		error|=(window=GTK_WIDGET(gtk_builder_get_object(builder, "window")))==NULL;
		error|=(sizeWidthSpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "sizeWidthSpinButton")))==NULL;
		error|=(sizeHeightSpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "sizeHeightSpinButton")))==NULL;
		if (error)
			throw std::runtime_error("could not grab main window widgets");

		// Connect manual signals

		// Add OK and Cancel buttons
		gtk_dialog_add_button(GTK_DIALOG(window), "Cancel", GTK_RESPONSE_CANCEL);
		gtk_dialog_add_button(GTK_DIALOG(window), "OK", GTK_RESPONSE_OK);

		// Free memory used by GtkBuilder object.
		g_object_unref(G_OBJECT(builder));

		// Set transient for main window
		if (parentWindow!=NULL) {
			gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ON_PARENT);
			gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(parentWindow));
		} else
			gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);
	}

	NewDialogue::~NewDialogue() {
		gtk_widget_destroy(window);
	}

	bool NewDialogue::run(unsigned *width, unsigned *height) {
		// Update spin buttons based on given values
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(sizeWidthSpinButton), *width);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(sizeHeightSpinButton), *height);

		// Show the dialogue and wait for response
		gtk_widget_show_all(window);
		gint result=gtk_dialog_run(GTK_DIALOG(window));

		// Handle result
		if (result!=GTK_RESPONSE_OK)
			return false;

		// Grab values to return
		*width=gtk_spin_button_get_value(GTK_SPIN_BUTTON(sizeWidthSpinButton));
		*height=gtk_spin_button_get_value(GTK_SPIN_BUTTON(sizeHeightSpinButton));

		return true;
	}

};
