#include <cstdlib>
#include <cstdio>
#include <exception>
#include <iostream>

#include "main.h"

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
		gtk_main();
	} catch (const std::exception& e) {
		std::cout << "Could not init map editor: " << e.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

namespace MapEditor {
	Main::Main() {
		mainWindow=new MainWindow();
		mainWindow->show();
	}

	Main::~Main() {
		delete mainWindow;
	}
};

