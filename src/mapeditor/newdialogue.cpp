#include <cassert>
#include <exception>
#include <stdexcept>

#include "../engine/map/map.h"
#include "newdialogue.h"
#include "util.h"

using namespace MapEditor;

void mapEditorNewDialogueWrapperSizeWidthSpinButtonValuedChanged(GtkSpinButton *spinButton, gpointer userData);
void mapEditorNewDialogueWrapperSizeHeightSpinButtonValuedChanged(GtkSpinButton *spinButton, gpointer userData);
void mapEditorNewDialogueWrapperSizeUnitsComboChanged(GtkComboBox *combo, gpointer userData);

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
		error|=(sizeUnitsCombo=GTK_WIDGET(gtk_builder_get_object(builder, "sizeUnitsCombo")))==NULL;
		error|=(infoFileSizeLabel=GTK_WIDGET(gtk_builder_get_object(builder, "infoFileSizeLabel")))==NULL;
		if (error)
			throw std::runtime_error("could not grab main window widgets");

		// Connect manual signals
		g_signal_connect(sizeWidthSpinButton, "value-changed", G_CALLBACK(mapEditorNewDialogueWrapperSizeWidthSpinButtonValuedChanged), (void *)this);
		g_signal_connect(sizeHeightSpinButton, "value-changed", G_CALLBACK(mapEditorNewDialogueWrapperSizeHeightSpinButtonValuedChanged), (void *)this);
		g_signal_connect(sizeUnitsCombo, "changed", G_CALLBACK(mapEditorNewDialogueWrapperSizeUnitsComboChanged), (void *)this);

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
		// Update interface based on given values
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(sizeWidthSpinButton), *width);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(sizeHeightSpinButton), *height);
		gtk_combo_box_set_active_id(GTK_COMBO_BOX(sizeUnitsCombo), "tiles");

		updateInfo();

		// Show the dialogue and wait for response
		gtk_widget_show_all(window);
		gint result=gtk_dialog_run(GTK_DIALOG(window));

		// Handle result
		if (result!=GTK_RESPONSE_OK)
			return false;

		// Grab values to return
		*width=getSizeWidth();
		*height=getSizeHeight();

		return true;
	}

	unsigned NewDialogue::getSizeWidth(void) {
		unsigned width=gtk_spin_button_get_value(GTK_SPIN_BUTTON(sizeWidthSpinButton));
		return width*getSizeUnitsFactor();
	}

	unsigned NewDialogue::getSizeHeight(void) {
		unsigned height=gtk_spin_button_get_value(GTK_SPIN_BUTTON(sizeHeightSpinButton));
		return height*getSizeUnitsFactor();
	}

	unsigned NewDialogue::getSizeUnitsFactor(void) {
		if (strcmp(gtk_combo_box_get_active_id(GTK_COMBO_BOX(sizeUnitsCombo)), "regions")==0)
			return 256;
		if (strcmp(gtk_combo_box_get_active_id(GTK_COMBO_BOX(sizeUnitsCombo)), "km")==0)
			return 1024;
		return 1; // tiles
	}

	void NewDialogue::sizeWidthSpinButtonValuedChanged(GtkSpinButton *spinButton) {
		updateInfo();
	}

	void NewDialogue::sizeHeightSpinButtonValuedChanged(GtkSpinButton *spinButton) {
		updateInfo();
	}

	void NewDialogue::sizeUnitsComboChanged(GtkComboBox *combo) {
		// Adjust limits for width/height fields based on units selected
		unsigned factor=getSizeUnitsFactor();
		double lower=(factor==1 ? 256.0 : 1.0);
		double upper=65536.0/factor;
		double stepInc=(factor==1 ? 256.0/factor : 1024.0/factor);
		double pageInc=(factor==1 ? 1024.0/factor : 4096.0/factor);

		GtkAdjustment *adjustment=gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(sizeWidthSpinButton));
		double value=gtk_adjustment_get_value(adjustment);
		gtk_adjustment_configure(adjustment, std::max(std::min(value, upper), lower), lower, upper, stepInc, pageInc, 0.0);
		adjustment=gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(sizeHeightSpinButton));
		value=gtk_adjustment_get_value(adjustment);
		gtk_adjustment_configure(adjustment, std::max(std::min(value, upper), lower), lower, upper, stepInc, pageInc, 0.0);

		// Update info
		updateInfo();
	}

	void NewDialogue::updateInfo(void) {
		// Update file size estimation
		size_t fileSize=Engine::Map::Map::estimatedFileSize(getSizeWidth(), getSizeHeight());
		char fileSizeStr[128]; // TODO: better
		sizeToStr(fileSizeStr, fileSize);
		gtk_label_set_text(GTK_LABEL(infoFileSizeLabel), fileSizeStr);
	}

};

void mapEditorNewDialogueWrapperSizeWidthSpinButtonValuedChanged(GtkSpinButton *spinButton, gpointer userData) {
	NewDialogue *newDialogue=(NewDialogue *)userData;
	newDialogue->sizeWidthSpinButtonValuedChanged(spinButton);
}

void mapEditorNewDialogueWrapperSizeHeightSpinButtonValuedChanged(GtkSpinButton *spinButton, gpointer userData) {
	NewDialogue *newDialogue=(NewDialogue *)userData;
	newDialogue->sizeHeightSpinButtonValuedChanged(spinButton);
}

void mapEditorNewDialogueWrapperSizeUnitsComboChanged(GtkComboBox *combo, gpointer userData) {
	NewDialogue *newDialogue=(NewDialogue *)userData;
	newDialogue->sizeUnitsComboChanged(combo);
}
