#ifndef MAPEDITOR_MAIN_H
#define MAPEDITOR_MAIN_H

#include "mainwindow.h"

namespace MapEditor {
	class Main {
	public:
		Main();
		~Main();

		void tick(void);
		void idleTick(void);

	private:
		MainWindow *mainWindow;
	};
};

#endif
