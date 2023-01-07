#ifndef MAPEDITOR_CLEARDIALOGUE_H
#define MAPEDITOR_CLEARDIALOGUE_H

#include "../engine/util.h"

#include <gtk/gtk.h>

namespace MapEditor {

	class ClearDialogue {
	public:
		struct Params {
			unsigned threads;
		};

		ClearDialogue(GtkWidget *parentWindow);
		~ClearDialogue();

		bool run(Params *params); // params specify initial values and also returns outputs if dialogue is accepted
	private:
		GtkWidget *window;
		GtkWidget *otherThreadsSpinButton;
	};

};

#endif
