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

			double landCoverage; // in range [0,1]
			unsigned threads;
		};

		HeightTemperatureDialogue(GtkWidget *parentWindow);
		~HeightTemperatureDialogue();

		void getParams(Params *params);

		bool run(Params *params); // params specify initial values and also returns outputs if dialogue is accepted

		void heightNoiseMinSpinButtonValueChanged(GtkSpinButton *spinButton);
		void heightNoiseMaxSpinButtonValueChanged(GtkSpinButton *spinButton);
		void heightNoiseSeedSpinButtonValueChanged(GtkSpinButton *spinButton);
		void heightNoiseOctavesSpinButtonValueChanged(GtkSpinButton *spinButton);
		void heightNoiseFrequencySpinButtonValueChanged(GtkSpinButton *spinButton);
		void temperatureNoiseMinSpinButtonValueChanged(GtkSpinButton *spinButton);
		void temperatureNoiseMaxSpinButtonValueChanged(GtkSpinButton *spinButton);
		void temperatureNoiseSeedSpinButtonValueChanged(GtkSpinButton *spinButton);
		void temperatureNoiseOctavesSpinButtonValueChanged(GtkSpinButton *spinButton);
		void temperatureNoiseFrequencySpinButtonValueChanged(GtkSpinButton *spinButton);
		void temperatureLapseRateSpinButtonValueChanged(GtkSpinButton *spinButton);
		void temperatureLatitudeRangeSpinButtonValueChanged(GtkSpinButton *spinButton);
		void otherLandCoverageSpinButtonValueChanged(GtkSpinButton *spinButton);

		gboolean previewHeightDrawingAreaDraw(GtkWidget *widget, cairo_t *cr);
		gboolean previewTemperatureDrawingAreaDraw(GtkWidget *widget, cairo_t *cr);

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
		GtkWidget *otherLandCoverageSpinButton;
		GtkWidget *otherThreadsSpinButton;

		GtkWidget *previewHeightDrawingArea;
		GtkWidget *previewTemperatureDrawingArea;

		bool previewCacheDirty;
		double previewCacheHeightValues[256][256];
		double previewCacheTemperatureValues[256][256];
		double previewCacheMaxHeight;
		double previewCacheMinTemperature, previewCacheMaxTemperature;
		double previewCacheSeaLevel;
		double previewCacheHeightList[65536];

		void parametersChanged(void); // called when parameter values (except Threads) change

		void previewCalculateData(void);
	};

};

#endif
