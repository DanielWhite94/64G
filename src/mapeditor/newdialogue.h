#ifndef MAPEDITOR_NEWDIALOGUE_H
#define MAPEDITOR_NEWDIALOGUE_H

#include "../engine/util.h"

#include <gtk/gtk.h>

namespace MapEditor {

	class NewDialogue {
	public:
		NewDialogue(GtkWidget *parentWindow);
		~NewDialogue();

		bool run(unsigned *width, unsigned *height);
	private:
		GtkWidget *window;
		GtkWidget *sizeWidthSpinButton;
		GtkWidget *sizeHeightSpinButton;
	};

};

#endif
