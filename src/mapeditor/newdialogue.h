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

		unsigned getSizeWidth(void); // in tiles
		unsigned getSizeHeight(void); // in tiles
		unsigned getSizeUnitsFactor(void); // 1 for tiles, 256 for regions, 1024 for km

		void sizeWidthSpinButtonValuedChanged(GtkSpinButton *spinButton);
		void sizeHeightSpinButtonValuedChanged(GtkSpinButton *spinButton);
		void sizeUnitsComboChanged(GtkComboBox *combo);
	private:
		GtkWidget *window;
		GtkWidget *sizeWidthSpinButton;
		GtkWidget *sizeHeightSpinButton;
		GtkWidget *sizeUnitsCombo;
		GtkWidget *infoFileSizeLabel;

		void updateInfo(void);
	};

};

#endif
