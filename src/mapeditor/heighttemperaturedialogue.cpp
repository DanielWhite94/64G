#include <cassert>
#include <exception>
#include <stdexcept>

#include "heighttemperaturedialogue.h"

using namespace MapEditor;

namespace MapEditor {
	HeightTemperatureDialogue::HeightTemperatureDialogue(GtkWidget *parentWindow) {
		const char xmlFile[]="heighttemperaturedialogue.glade";

		// Init
		window=NULL;

		// Use GtkBuilder to build our interface from the XML file.
		GtkBuilder *builder=gtk_builder_new();
		GError *err=NULL;
		if (gtk_builder_add_from_file(builder, xmlFile, &err)==0) {
			g_error_free(err);
			throw std::runtime_error("could not init height temperature dialogue glade file"); // TODO: use 'err->message' for more info
		}

		// Connect glade signals
		gtk_builder_connect_signals(builder, NULL);

		// Extract widgets we will need.
		bool error=false;
		error|=(window=GTK_WIDGET(gtk_builder_get_object(builder, "window")))==NULL;
		error|=(heightNoiseMinSpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "heightNoiseMinSpinButton")))==NULL;
		error|=(heightNoiseMaxSpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "heightNoiseMaxSpinButton")))==NULL;
		error|=(heightNoiseSeedSpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "heightNoiseSeedSpinButton")))==NULL;
		error|=(heightNoiseOctavesSpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "heightNoiseOctavesSpinButton")))==NULL;
		error|=(heightNoiseFrequencySpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "heightNoiseFrequencySpinButton")))==NULL;
		error|=(temperatureNoiseMinSpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "temperatureNoiseMinSpinButton")))==NULL;
		error|=(temperatureNoiseMaxSpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "temperatureNoiseMaxSpinButton")))==NULL;
		error|=(temperatureNoiseSeedSpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "temperatureNoiseSeedSpinButton")))==NULL;
		error|=(temperatureNoiseOctavesSpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "temperatureNoiseOctavesSpinButton")))==NULL;
		error|=(temperatureNoiseFrequencySpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "temperatureNoiseFrequencySpinButton")))==NULL;
		error|=(temperatureLapseRateSpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "temperatureLapseRateSpinButton")))==NULL;
		error|=(temperatureLatitudeRangeSpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "temperatureLatitudeRangeSpinButton")))==NULL;
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

	HeightTemperatureDialogue::~HeightTemperatureDialogue() {
		gtk_widget_destroy(window);
	}

	bool HeightTemperatureDialogue::run(Params *params) {
		// Update spin buttons based on given values
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(heightNoiseMinSpinButton), params->heightNoiseMin);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(heightNoiseMaxSpinButton), params->heightNoiseMax);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(heightNoiseSeedSpinButton), params->heightNoiseSeed);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(heightNoiseOctavesSpinButton), params->heightNoiseOctaves);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(heightNoiseFrequencySpinButton), params->heightNoiseFrequency);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(temperatureNoiseMinSpinButton), params->temperatureNoiseMin);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(temperatureNoiseMaxSpinButton), params->temperatureNoiseMax);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(temperatureNoiseSeedSpinButton), params->temperatureNoiseSeed);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(temperatureNoiseOctavesSpinButton), params->temperatureNoiseOctaves);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(temperatureNoiseFrequencySpinButton), params->temperatureNoiseFrequency);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(temperatureLapseRateSpinButton), params->temperatureLapseRate);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(temperatureLatitudeRangeSpinButton), params->temperatureLatitudeRange);

		// Show the dialogue and wait for response
		gtk_widget_show_all(window);
		gint result=gtk_dialog_run(GTK_DIALOG(window));

		// Handle result
		if (result!=GTK_RESPONSE_OK)
			return false;

		// Grab values to return
		params->heightNoiseMin=gtk_spin_button_get_value(GTK_SPIN_BUTTON(heightNoiseMinSpinButton));
		params->heightNoiseMax=gtk_spin_button_get_value(GTK_SPIN_BUTTON(heightNoiseMaxSpinButton));
		params->heightNoiseSeed=gtk_spin_button_get_value(GTK_SPIN_BUTTON(heightNoiseSeedSpinButton));
		params->heightNoiseOctaves=gtk_spin_button_get_value(GTK_SPIN_BUTTON(heightNoiseOctavesSpinButton));
		params->heightNoiseFrequency=gtk_spin_button_get_value(GTK_SPIN_BUTTON(heightNoiseFrequencySpinButton));
		params->temperatureNoiseMin=gtk_spin_button_get_value(GTK_SPIN_BUTTON(temperatureNoiseMinSpinButton));
		params->temperatureNoiseMax=gtk_spin_button_get_value(GTK_SPIN_BUTTON(temperatureNoiseMaxSpinButton));
		params->temperatureNoiseSeed=gtk_spin_button_get_value(GTK_SPIN_BUTTON(temperatureNoiseSeedSpinButton));
		params->temperatureNoiseOctaves=gtk_spin_button_get_value(GTK_SPIN_BUTTON(temperatureNoiseOctavesSpinButton));
		params->temperatureNoiseFrequency=gtk_spin_button_get_value(GTK_SPIN_BUTTON(temperatureNoiseFrequencySpinButton));
		params->temperatureLapseRate=gtk_spin_button_get_value(GTK_SPIN_BUTTON(temperatureLapseRateSpinButton));
		params->temperatureLatitudeRange=gtk_spin_button_get_value(GTK_SPIN_BUTTON(temperatureLatitudeRangeSpinButton));

		return true;
	}

};
