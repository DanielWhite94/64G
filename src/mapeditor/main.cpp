#include <cstdlib>
#include <cstdio>
#include <exception>
#include <iostream>

#include "main.h"

const int mainTickIntervalMs=100;

gint mainTick(gpointer data);
gint mainIdleTick(gpointer data);

int main(int argc, char **argv) {
	// Initialize gtk
	if (!gtk_init_check(&argc, &argv)) {
		printf("Could not initialize gtk.");
		return EXIT_FAILURE;
	}

	// Check for arguments
	char *pathToOpen=NULL;
	if (argc>=2)
		pathToOpen=argv[1];

	// Init map editor
	try {
		MapEditor::Main m(pathToOpen);

		// Main loop.
		g_timeout_add(mainTickIntervalMs, &mainTick, (void *)&m);
		g_idle_add(&mainIdleTick, (void *)&m);
		gtk_main();
	} catch (const std::exception& e) {
		std::cout << "Could not init map editor: " << e.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

gint mainTick(gpointer data) {
	MapEditor::Main *m=(MapEditor::Main *)data;
	m->tick();
	return TRUE;
}

gint mainIdleTick(gpointer data) {
	MapEditor::Main *m=(MapEditor::Main *)data;
	m->idleTick();
	return TRUE;
}

namespace MapEditor {
	Main::Main(const char *pathToOpen) {
		mainWindow=new MainWindow(pathToOpen);
		mainWindow->show();
	}

	Main::~Main() {
		delete mainWindow;
	}

	void Main::tick(void) {
		mainWindow->tick();
	}

	void Main::idleTick(void) {
		mainWindow->idleTick();
	}
};

