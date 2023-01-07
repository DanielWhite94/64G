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

	// Check for arguments
	char *pathToOpen=NULL;
	if (argc>=2)
		pathToOpen=argv[1];

	// Init map editor
	try {
		MapEditor::Main m(pathToOpen);

		// Main loop.
		gtk_main();
	} catch (const std::exception& e) {
		std::cout << "Could not init map editor: " << e.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

namespace MapEditor {
	Main::Main(const char *pathToOpen) {
		mainWindow=new MainWindow(pathToOpen);
		mainWindow->show();
	}

	Main::~Main() {
		delete mainWindow;
	}
};

