#include <cstdlib>
#include <cstdio>
#include <exception>
#include <iostream>

#include "main.h"

const int mainTickIntervalMs=100;

gint mainTick(gpointer data);

int main(int argc, char **argv) {
	// Initialize gtk
	if (!gtk_init_check(&argc, &argv)) {
		printf("Could not initialize gtk.");
		return EXIT_FAILURE;
	}

	// Init map editor
	try {
		MapEditor::Main m;

		// Main loop.
		g_timeout_add(mainTickIntervalMs, &mainTick, (void *)&m);
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

namespace MapEditor {
	Main::Main() {
		mainWindow=new MainWindow();
		mainWindow->show();
	}

	Main::~Main() {
		delete mainWindow;
	}

	void Main::tick(void) {
		mainWindow->tick();
	}
};

