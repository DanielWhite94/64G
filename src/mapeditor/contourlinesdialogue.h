#ifndef MAPEDITOR_CONTOURLINESDIALOGUE_H
#define MAPEDITOR_CONTOURLINESDIALOGUE_H

#include "../engine/util.h"

#include <gtk/gtk.h>

namespace MapEditor {

	class ContourLinesDialogue {
	public:
		struct Params {
			unsigned lineCount;
			unsigned threads;
		};

		ContourLinesDialogue(GtkWidget *parentWindow);
		~ContourLinesDialogue();

		bool run(Params *params); // params specify initial values and also returns outputs if dialogue is accepted
	private:
		GtkWidget *window;
		GtkWidget *otherLineCountSpinButton;
		GtkWidget *otherThreadsSpinButton;
	};

};

#endif
