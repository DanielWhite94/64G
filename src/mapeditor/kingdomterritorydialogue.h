#ifndef MAPEDITOR_KINGDOMTERRITORYDIALOGUE_H
#define MAPEDITOR_KINGDOMTERRITORYDIALOGUE_H

#include "../engine/util.h"

#include <gtk/gtk.h>

namespace MapEditor {

	class KingdomTerritoryDialogue {
	public:
		struct Params {
			unsigned threads;
		};

		KingdomTerritoryDialogue(GtkWidget *parentWindow);
		~KingdomTerritoryDialogue();

		bool run(Params *params); // params specify initial values and also returns outputs if dialogue is accepted
	private:
		GtkWidget *window;
		GtkWidget *otherThreadsSpinButton;
	};

};

#endif
