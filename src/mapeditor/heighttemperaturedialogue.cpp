#include <cassert>
#include <cmath>
#include <exception>
#include <stdexcept>

#include "heighttemperaturedialogue.h"
#include "../engine/fbnnoise.h"

using namespace MapEditor;

namespace MapEditor {
	void heightTemperatureDialogueHeightNoiseMinSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData);
	void heightTemperatureDialogueHeightNoiseMaxSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData);
	void heightTemperatureDialogueHeightNoiseSeedSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData);
	void heightTemperatureDialogueHeightNoiseOctavesSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData);
	void heightTemperatureDialogueHeightNoiseFrequencySpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData);
	void heightTemperatureDialogueTemperatureNoiseMinSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData);
	void heightTemperatureDialogueTemperatureNoiseMaxSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData);
	void heightTemperatureDialogueTemperatureNoiseSeedSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData);
	void heightTemperatureDialogueTemperatureNoiseOctavesSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData);
	void heightTemperatureDialogueTemperatureNoiseFrequencySpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData);
	void heightTemperatureDialogueTemperatureLapseRateSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData);
	void heightTemperatureDialogueTemperatureLatitudeRangeSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData);
	void heightTemperatureDialogueOtherLandCoverageSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData);

	gboolean heightTemperatureDialoguePreviewHeightDrawingAreaDraw(GtkWidget *widget, cairo_t *cr, gpointer userData);
	gboolean heightTemperatureDialoguePreviewTemperatureDrawingAreaDraw(GtkWidget *widget, cairo_t *cr, gpointer userData);

	HeightTemperatureDialogue::HeightTemperatureDialogue(GtkWidget *parentWindow) {
		const char xmlFile[]="heighttemperaturedialogue.glade";

		// Init
		window=NULL;
		previewCacheDirty=true;

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
		error|=(otherLandCoverageSpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "otherLandCoverageSpinButton")))==NULL;
		error|=(otherThreadsSpinButton=GTK_WIDGET(gtk_builder_get_object(builder, "otherThreadsSpinButton")))==NULL;
		error|=(previewHeightDrawingArea=GTK_WIDGET(gtk_builder_get_object(builder, "previewHeightDrawingArea")))==NULL;
		error|=(previewTemperatureDrawingArea=GTK_WIDGET(gtk_builder_get_object(builder, "previewTemperatureDrawingArea")))==NULL;
		if (error)
			throw std::runtime_error("could not grab main window widgets");

		// Connect manual signals
		g_signal_connect(heightNoiseMinSpinButton, "value-changed", G_CALLBACK(heightTemperatureDialogueHeightNoiseMinSpinButtonValueChanged), (void *)this);
		g_signal_connect(heightNoiseMaxSpinButton, "value-changed", G_CALLBACK(heightTemperatureDialogueHeightNoiseMaxSpinButtonValueChanged), (void *)this);
		g_signal_connect(heightNoiseSeedSpinButton, "value-changed", G_CALLBACK(heightTemperatureDialogueHeightNoiseSeedSpinButtonValueChanged), (void *)this);
		g_signal_connect(heightNoiseOctavesSpinButton, "value-changed", G_CALLBACK(heightTemperatureDialogueHeightNoiseOctavesSpinButtonValueChanged), (void *)this);
		g_signal_connect(heightNoiseFrequencySpinButton, "value-changed", G_CALLBACK(heightTemperatureDialogueHeightNoiseFrequencySpinButtonValueChanged), (void *)this);
		g_signal_connect(temperatureNoiseMinSpinButton, "value-changed", G_CALLBACK(heightTemperatureDialogueTemperatureNoiseMinSpinButtonValueChanged), (void *)this);
		g_signal_connect(temperatureNoiseMaxSpinButton, "value-changed", G_CALLBACK(heightTemperatureDialogueTemperatureNoiseMaxSpinButtonValueChanged), (void *)this);
		g_signal_connect(temperatureNoiseSeedSpinButton, "value-changed", G_CALLBACK(heightTemperatureDialogueTemperatureNoiseSeedSpinButtonValueChanged), (void *)this);
		g_signal_connect(temperatureNoiseOctavesSpinButton, "value-changed", G_CALLBACK(heightTemperatureDialogueTemperatureNoiseOctavesSpinButtonValueChanged), (void *)this);
		g_signal_connect(temperatureNoiseFrequencySpinButton, "value-changed", G_CALLBACK(heightTemperatureDialogueTemperatureNoiseFrequencySpinButtonValueChanged), (void *)this);
		g_signal_connect(temperatureLapseRateSpinButton, "value-changed", G_CALLBACK(heightTemperatureDialogueTemperatureLapseRateSpinButtonValueChanged), (void *)this);
		g_signal_connect(temperatureLatitudeRangeSpinButton, "value-changed", G_CALLBACK(heightTemperatureDialogueTemperatureLatitudeRangeSpinButtonValueChanged), (void *)this);
		g_signal_connect(otherLandCoverageSpinButton, "value-changed", G_CALLBACK(heightTemperatureDialogueOtherLandCoverageSpinButtonValueChanged), (void *)this);

		g_signal_connect(previewHeightDrawingArea, "draw", G_CALLBACK(heightTemperatureDialoguePreviewHeightDrawingAreaDraw), (void *)this);
		g_signal_connect(previewTemperatureDrawingArea, "draw", G_CALLBACK(heightTemperatureDialoguePreviewTemperatureDrawingAreaDraw), (void *)this);

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

	void HeightTemperatureDialogue::getParams(Params *params) {
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
		params->landCoverage=gtk_spin_button_get_value(GTK_SPIN_BUTTON(otherLandCoverageSpinButton))/100.0;
		params->threads=gtk_spin_button_get_value(GTK_SPIN_BUTTON(otherThreadsSpinButton));
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
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(otherLandCoverageSpinButton), params->landCoverage*100.0);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(otherThreadsSpinButton), params->threads);

		// Show the dialogue and wait for response
		gtk_widget_show_all(window);
		gint result=gtk_dialog_run(GTK_DIALOG(window));

		// Handle result
		if (result!=GTK_RESPONSE_OK)
			return false;

		// Grab values to return
		getParams(params);

		return true;
	}

	void HeightTemperatureDialogue::heightNoiseMinSpinButtonValueChanged(GtkSpinButton *spinButton) {
		parametersChanged();
	}

	void HeightTemperatureDialogue::heightNoiseMaxSpinButtonValueChanged(GtkSpinButton *spinButton) {
		parametersChanged();
	}

	void HeightTemperatureDialogue::heightNoiseSeedSpinButtonValueChanged(GtkSpinButton *spinButton) {
		parametersChanged();
	}

	void HeightTemperatureDialogue::heightNoiseOctavesSpinButtonValueChanged(GtkSpinButton *spinButton) {
		parametersChanged();
	}

	void HeightTemperatureDialogue::heightNoiseFrequencySpinButtonValueChanged(GtkSpinButton *spinButton) {
		parametersChanged();
	}

	void HeightTemperatureDialogue::temperatureNoiseMinSpinButtonValueChanged(GtkSpinButton *spinButton) {
		parametersChanged();
	}

	void HeightTemperatureDialogue::temperatureNoiseMaxSpinButtonValueChanged(GtkSpinButton *spinButton) {
		parametersChanged();
	}

	void HeightTemperatureDialogue::temperatureNoiseSeedSpinButtonValueChanged(GtkSpinButton *spinButton) {
		parametersChanged();
	}

	void HeightTemperatureDialogue::temperatureNoiseOctavesSpinButtonValueChanged(GtkSpinButton *spinButton) {
		parametersChanged();
	}

	void HeightTemperatureDialogue::temperatureNoiseFrequencySpinButtonValueChanged(GtkSpinButton *spinButton) {
		parametersChanged();
	}

	void HeightTemperatureDialogue::temperatureLapseRateSpinButtonValueChanged(GtkSpinButton *spinButton) {
		parametersChanged();
	}

	void HeightTemperatureDialogue::temperatureLatitudeRangeSpinButtonValueChanged(GtkSpinButton *spinButton) {
		parametersChanged();
	}

	void HeightTemperatureDialogue::otherLandCoverageSpinButtonValueChanged(GtkSpinButton *spinButton) {
		parametersChanged();
	}

	void HeightTemperatureDialogue::parametersChanged(void) {
		previewCacheDirty=true;

		gtk_widget_queue_draw(previewHeightDrawingArea);
		gtk_widget_queue_draw(previewTemperatureDrawingArea);
	}

	void HeightTemperatureDialogue::previewCalculateData(void) {
		// If no changes then nothing to do
		if (!previewCacheDirty)
			return;

		// Grab current parameters
		Params params;
		getParams(&params);

		// Create noise
		Engine::FbnNoise *heightNoise=new Engine::FbnNoise(params.heightNoiseSeed, params.heightNoiseOctaves, params.heightNoiseFrequency);

		// Generate height data
		previewCacheMaxHeight=DBL_MIN;
		for(unsigned y=0; y<256; ++y) {
			for(unsigned x=0; x<256; ++x) {
				double heightNoiseValue=(1.0+heightNoise->eval(x/256.0, y/256.0))/2.0; // [0.0,1.0]
				previewCacheHeightValues[y][x]=params.heightNoiseMin+(params.heightNoiseMax-params.heightNoiseMin)*heightNoiseValue;

				if (previewCacheHeightValues[y][x]>previewCacheMaxHeight)
					previewCacheMaxHeight=previewCacheHeightValues[y][x];
			}
		}

		// For now assume sea level of 0
		previewCacheSeaLevel=0.0;

		// Tidy up
		delete heightNoise;

		// Clear dirty flag
		previewCacheDirty=false;
	}

	gboolean HeightTemperatureDialogue::previewHeightDrawingAreaDraw(GtkWidget *widget, cairo_t *cr) {
		// Ensure data is up to date
		previewCalculateData();

		// Loop over pixels of the drawing area
		for(unsigned y=0; y<256; ++y) {
			for(unsigned x=0; x<256; ++x) {
				// Grab height
				const double height=previewCacheHeightValues[y][x];

				// Choose colour
				double r=0.0;
				double g=0.0;
				double b=1.0;

				if (height>previewCacheSeaLevel) {
					double heightNormalised=(height-previewCacheSeaLevel)/(previewCacheMaxHeight-previewCacheSeaLevel);
					unsigned heightScaled=floor(heightNormalised*(2*256-1));

					if (heightScaled<128) {
						// 0x008800 -> 0x888800
						r=heightScaled/256.0;
						g=0.5;
						b=0.0;
					} else if (heightScaled<256) {
						// 0x888800 -> 0x880000
						heightScaled-=128;
						r=0.5;
						g=0.5-heightScaled/256.0;
						b=0.0;
					} else if (heightScaled<384) {
						// 0x880000 -> 0x888888
						heightScaled-=256;
						r=0.5;
						g=heightScaled/256.0;
						b=heightScaled/256.0;
					} else {
						// 0x888888 -> 0xFFFFFF
						heightScaled-=384;
						r=0.5+heightScaled/256.0;
						g=0.5+heightScaled/256.0;
						b=0.5+heightScaled/256.0;
					}
				}

				assert(r>=0.0 && r<=1.0);
				assert(g>=0.0 && g<=1.0);
				assert(b>=0.0 && b<=1.0);

				// Draw pixel
				cairo_set_source_rgb(cr, r, g, b);
				cairo_rectangle(cr, x, y, 1, 1);
				cairo_fill(cr);
			}
		}

		return false;
	}

	gboolean HeightTemperatureDialogue::previewTemperatureDrawingAreaDraw(GtkWidget *widget, cairo_t *cr) {
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_paint(cr);

		return false;
	}

	void heightTemperatureDialogueHeightNoiseMinSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData) {
		assert(userData!=NULL);

		HeightTemperatureDialogue *dialogue=(HeightTemperatureDialogue *)userData;
		dialogue->heightNoiseMinSpinButtonValueChanged(spinButton);
	}

	void heightTemperatureDialogueHeightNoiseMaxSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData) {
		assert(userData!=NULL);

		HeightTemperatureDialogue *dialogue=(HeightTemperatureDialogue *)userData;
		dialogue->heightNoiseMaxSpinButtonValueChanged(spinButton);
	}

	void heightTemperatureDialogueHeightNoiseSeedSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData) {
		assert(userData!=NULL);

		HeightTemperatureDialogue *dialogue=(HeightTemperatureDialogue *)userData;
		dialogue->heightNoiseSeedSpinButtonValueChanged(spinButton);
	}

	void heightTemperatureDialogueHeightNoiseOctavesSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData) {
		assert(userData!=NULL);

		HeightTemperatureDialogue *dialogue=(HeightTemperatureDialogue *)userData;
		dialogue->heightNoiseOctavesSpinButtonValueChanged(spinButton);
	}

	void heightTemperatureDialogueHeightNoiseFrequencySpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData) {
		assert(userData!=NULL);

		HeightTemperatureDialogue *dialogue=(HeightTemperatureDialogue *)userData;
		dialogue->heightNoiseFrequencySpinButtonValueChanged(spinButton);
	}

	void heightTemperatureDialogueTemperatureNoiseMinSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData) {
		assert(userData!=NULL);

		HeightTemperatureDialogue *dialogue=(HeightTemperatureDialogue *)userData;
		dialogue->temperatureNoiseMinSpinButtonValueChanged(spinButton);
	}

	void heightTemperatureDialogueTemperatureNoiseMaxSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData) {
		assert(userData!=NULL);

		HeightTemperatureDialogue *dialogue=(HeightTemperatureDialogue *)userData;
		dialogue->temperatureNoiseMaxSpinButtonValueChanged(spinButton);
	}

	void heightTemperatureDialogueTemperatureNoiseSeedSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData) {
		assert(userData!=NULL);

		HeightTemperatureDialogue *dialogue=(HeightTemperatureDialogue *)userData;
		dialogue->temperatureNoiseSeedSpinButtonValueChanged(spinButton);
	}

	void heightTemperatureDialogueTemperatureNoiseOctavesSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData) {
		assert(userData!=NULL);

		HeightTemperatureDialogue *dialogue=(HeightTemperatureDialogue *)userData;
		dialogue->temperatureNoiseOctavesSpinButtonValueChanged(spinButton);
	}

	void heightTemperatureDialogueTemperatureNoiseFrequencySpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData) {
		assert(userData!=NULL);

		HeightTemperatureDialogue *dialogue=(HeightTemperatureDialogue *)userData;
		dialogue->temperatureNoiseFrequencySpinButtonValueChanged(spinButton);
	}

	void heightTemperatureDialogueTemperatureLapseRateSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData) {
		assert(userData!=NULL);

		HeightTemperatureDialogue *dialogue=(HeightTemperatureDialogue *)userData;
		dialogue->temperatureLapseRateSpinButtonValueChanged(spinButton);
	}

	void heightTemperatureDialogueTemperatureLatitudeRangeSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData) {
		assert(userData!=NULL);

		HeightTemperatureDialogue *dialogue=(HeightTemperatureDialogue *)userData;
		dialogue->temperatureLatitudeRangeSpinButtonValueChanged(spinButton);
	}

	void heightTemperatureDialogueOtherLandCoverageSpinButtonValueChanged(GtkSpinButton *spinButton, gpointer userData) {
		assert(userData!=NULL);

		HeightTemperatureDialogue *dialogue=(HeightTemperatureDialogue *)userData;
		dialogue->otherLandCoverageSpinButtonValueChanged(spinButton);
	}

	gboolean heightTemperatureDialoguePreviewHeightDrawingAreaDraw(GtkWidget *widget, cairo_t *cr, gpointer userData) {
		MapEditor::HeightTemperatureDialogue *dialogue=(MapEditor::HeightTemperatureDialogue *)userData;
		return dialogue->previewHeightDrawingAreaDraw(widget, cr);
	}

	gboolean heightTemperatureDialoguePreviewTemperatureDrawingAreaDraw(GtkWidget *widget, cairo_t *cr, gpointer userData) {
		MapEditor::HeightTemperatureDialogue *dialogue=(MapEditor::HeightTemperatureDialogue *)userData;
		return dialogue->previewTemperatureDrawingAreaDraw(widget, cr);
	}

};
