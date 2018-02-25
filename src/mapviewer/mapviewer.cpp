#include <gtkmm/application.h>

#include "mainwindow.h"

int main (int argc, char *argv[]) {
	// Create window.
	auto app=Gtk::Application::create(argc, argv, "sixtyfourg.mapviewer", Gio::APPLICATION_HANDLES_OPEN);
	MainWindow mainWindow;

	// Show window.
	app->run(mainWindow);

	return 0;
}
