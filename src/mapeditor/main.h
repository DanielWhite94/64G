#ifndef MAPEDITOR_MAIN_H
#define MAPEDITOR_MAIN_H

#include "mainwindow.h"

namespace MapEditor {
	class Main {
	public:
		Main(const char *pathToOpen);
		~Main();

	private:
		MainWindow *mainWindow;
	};
};

#endif
