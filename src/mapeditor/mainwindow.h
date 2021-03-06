#ifndef MAPEDITOR_MAINWINDOW_H
#define MAPEDITOR_MAINWINDOW_H

#include <cairo.h>
#include <gtk/gtk.h>
#include <vector>

#include "../engine/map/map.h"
#include "../engine/map/maptiled.h"

namespace MapEditor {
	class MainWindow {
	public:
		struct DrawMapTileEntry {
			int zoom, x, y;
		};

		MainWindow();
		~MainWindow();

		void show();
		void hide();

		void tick(void);
		void idleTick(void);

		bool deleteEvent(GtkWidget *widget, GdkEvent *event);

		bool menuFileNewActivate(GtkWidget *widget);
		bool menuFileOpenActivate(GtkWidget *widget);
		bool menuFileSaveActivate(GtkWidget *widget);
		bool menuFileSaveAsActivate(GtkWidget *widget);
		bool menuFileCloseActivate(GtkWidget *widget);
		bool menuFileQuitActivate(GtkWidget *widget);

		bool menuViewZoomInActivate(GtkWidget *widget);
		bool menuViewZoomOutActivate(GtkWidget *widget);

		bool drawingAreaDraw(GtkWidget *widget, cairo_t *cr);
		gboolean drawingAreaKeyPressEvent(GtkWidget *widget, GdkEventKey *event);
		gboolean drawingAreaKeyReleaseEvent(GtkWidget *widget, GdkEventKey *event);

		bool menuViewShowRegionGridIsActive(void);
		bool menuViewShowRegionGridToggled(GtkWidget *widget);
		bool menuViewShowTileGridIsActive(void);
		bool menuViewShowTileGridToggled(GtkWidget *widget);
		bool menuViewShowKmGridIsActive(void);
		bool menuViewShowKmGridToggled(GtkWidget *widget);

		GtkWidget *window;
		GtkWidget *menuFileNew;
		GtkWidget *menuFileOpen;
		GtkWidget *menuFileSave;
		GtkWidget *menuFileSaveAs;
		GtkWidget *menuFileClose;
		GtkWidget *menuFileQuit;
		GtkWidget *menuViewZoomIn;
		GtkWidget *menuViewZoomOut;
		GtkWidget *statusLabel;
		GtkWidget *positionLabel;
		GtkWidget *drawingArea;
		GtkWidget *menuViewShowRegionGrid;
		GtkWidget *menuViewShowTileGrid;
		GtkWidget *menuViewShowKmGrid;

		class Map *map;

		static const int zoomLevelMin=0, zoomLevelMax=MapTiled::maxZoom;
		int zoomLevel;
		double userCentreX, userCentreY;

		bool keyPanningLeft, keyPanningRight, keyPanningUp, keyPanningDown;
		gint64 lastTickTimeMs;

		vector<DrawMapTileEntry> mapTilesToGen;
	private:
		bool mapNew(void);
		bool mapOpen(void); // Returns true if successfully opened, false if user clicks cancel of the choosen folder is not a valid map.
		bool mapSave(void); // Returns false on failure to save
		bool mapSaveAs(void); // Returns false on failure to save or user choosing cancel in file chooser dialogue
		bool mapClose(void); // Can return false if map has unsaved changes and they choose to cancel. Returns true if no map even loaded.
		void mapQuit(void);
		bool mapGetUnsavedChanges(void);

		double getZoomFactor(void);
		double getZoomFactorHuman(void);

		void updateFileMenuSensitivity(void);
		void updateTitle(void);
		void updateDrawingArea(void);
		void updatePositionLabel(void);
	};
};

#endif
