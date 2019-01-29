#ifndef MAPEDITOR_MAINWINDOW_H
#define MAPEDITOR_MAINWINDOW_H

#include <gtk/gtk.h>

namespace MapEditor {
	class MainWindow {
	public:
		MainWindow();
		~MainWindow();

		void show();
		void hide();

		bool deleteEvent(GtkWidget *widget, GdkEvent *event);

		GtkWidget *window;
	private:
	};
};

#endif
