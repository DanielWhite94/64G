#ifndef MAPEDITOR_HEIGHTTEMPERATUREDIALOGUE_H
#define MAPEDITOR_HEIGHTTEMPERATUREDIALOGUE_H

#include "../engine/util.h"

#include <gtk/gtk.h>

namespace MapEditor {

	class HeightTemperatureDialogue {
	public:
		struct Params {
			double heightNoiseMin;
			double heightNoiseMax;
			double heightNoiseSeed;
			double heightNoiseOctaves;
			double heightNoiseFrequency;

			double temperatureNoiseMin;
			double temperatureNoiseMax;
			double temperatureNoiseSeed;
			double temperatureNoiseOctaves;
			double temperatureNoiseFrequency;
			double temperatureLapseRate;
			double temperatureLatitudeRange;

			unsigned threads;
		};

		HeightTemperatureDialogue(GtkWidget *parentWindow);
		~HeightTemperatureDialogue();

		bool run(Params *params); // params specify initial values and also returns outputs if dialogue is accepted
	private:
		GtkWidget *window;
		GtkWidget *heightNoiseMinSpinButton;
		GtkWidget *heightNoiseMaxSpinButton;
		GtkWidget *heightNoiseSeedSpinButton;
		GtkWidget *heightNoiseOctavesSpinButton;
		GtkWidget *heightNoiseFrequencySpinButton;
		GtkWidget *temperatureNoiseMinSpinButton;
		GtkWidget *temperatureNoiseMaxSpinButton;
		GtkWidget *temperatureNoiseSeedSpinButton;
		GtkWidget *temperatureNoiseOctavesSpinButton;
		GtkWidget *temperatureNoiseFrequencySpinButton;
		GtkWidget *temperatureLapseRateSpinButton;
		GtkWidget *temperatureLatitudeRangeSpinButton;
		GtkWidget *otherThreadsSpinButton;
	};

};

#endif
