#ifndef MAPEDITOR_MAIN_H
#define MAPEDITOR_MAIN_H

#include "../engine/map/map.h"

#include "mainwindow.h"

namespace MapEditor {
	class Main {
	public:
		Main();
		~Main();

	private:
		class Map *map;
		MainWindow *mainWindow;
	};
};

#endif
