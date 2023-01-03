#ifndef MAPEDITOR_NEWDIALOGUE_H
#define MAPEDITOR_NEWDIALOGUE_H

#include "../engine/util.h"

#include <gtk/gtk.h>

namespace MapEditor {

	class NewDialogue {
	public:
		NewDialogue(GtkWidget *parentWindow);
		~NewDialogue();

		bool run(unsigned *width, unsigned *height); // width and height specify initial values and also return output if dialogue is accepted
	private:
		GtkWidget *window;
		GtkWidget *sizeWidthSpinButton;
		GtkWidget *sizeHeightSpinButton;
	};

};

#endif
