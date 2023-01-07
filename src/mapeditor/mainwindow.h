#ifndef MAPEDITOR_MAINWINDOW_H
#define MAPEDITOR_MAINWINDOW_H

#include <cairo.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <vector>

#include "../engine/map/map.h"
#include "../engine/map/maptiled.h"

namespace MapEditor {
	class MainWindow {
	public:
		MainWindow(const char *filename); // pass NULL for no initial map
		~MainWindow();

		void show();
		void hide();

		// Performs regular tasks such as rendering
		void timerTick(void);

		// Called when possible to do long running background tasks such as generating MapTiled images
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
		bool menuViewZoomFitActivate(GtkWidget *widget);
		void setZoom(int level);
		void zoomFit(void); // set zoom and centre x/y such that the entire map is visible and centered

		bool menuToolsClearActivate(GtkWidget *widget);
		bool menuToolsHeightTemperatureActivate(GtkWidget *widget);

		bool drawingAreaDraw(GtkWidget *widget, cairo_t *cr);
		gboolean drawingAreaKeyPressEvent(GtkWidget *widget, GdkEventKey *event);
		gboolean drawingAreaKeyReleaseEvent(GtkWidget *widget, GdkEventKey *event);
		gboolean drawingAreaScrollEvent(GtkWidget *widget, GdkEventScroll *event);
		void drawingAreaDragBegin(double startX, double startY);
		void drawingAreaDragUpdate(double startX, double startY, double offsetX, double offsetY);

		bool menuViewShowRegionGridIsActive(void);
		bool menuViewShowRegionGridToggled(GtkWidget *widget);
		bool menuViewShowTileGridIsActive(void);
		bool menuViewShowTileGridToggled(GtkWidget *widget);
		bool menuViewShowKmGridIsActive(void);
		bool menuViewShowKmGridToggled(GtkWidget *widget);

		MapTiled::ImageLayer menuViewLayersGetActiveLayer(void);
		bool menuViewLayersToggled(GtkWidget *widget);
		bool menuViewLayersHeightContoursIsActive(void);
		bool menuViewLayersHeightContoursToggled(GtkWidget *widget);

		GtkWidget *window;
		GtkWidget *menuFileNew;
		GtkWidget *menuFileOpen;
		GtkWidget *menuFileSave;
		GtkWidget *menuFileSaveAs;
		GtkWidget *menuFileClose;
		GtkWidget *menuFileQuit;
		GtkWidget *menuViewZoomIn;
		GtkWidget *menuViewZoomOut;
		GtkWidget *menuViewZoomFit;
		GtkWidget *menuToolsClear;
		GtkWidget *menuToolsHeightTemperature;
		GtkWidget *statusLabel;
		GtkWidget *positionLabel;
		GtkWidget *drawingArea;
		GtkWidget *menuViewShowRegionGrid;
		GtkWidget *menuViewShowTileGrid;
		GtkWidget *menuViewShowKmGrid;
		GtkWidget *menuViewLayersBase;
		GtkWidget *menuViewLayersTemperature;
		GtkWidget *menuViewLayersHeight;
		GtkWidget *menuViewLayersMoisture;
		GtkWidget *menuViewLayersPolitical;
		GtkWidget *menuViewLayersHeightContours;

		class Map *map;

		// Zoom limits based on available MapTiled images
		static const int zoomExtra=5; // At zoom=MapTiled::maxZoom MapTiled images are 1:1 with the screen, this is how many levels we can scale them to zoom in further.
		static const int zoomLevelMin=0, zoomLevelMax=MapTiled::maxZoom+zoomExtra;

		int zoomLevel;
		double userCentreX, userCentreY;
		double dragBeginUserCentreX, dragBeginUserCentreY;

		bool keyPanningLeft, keyPanningRight, keyPanningUp, keyPanningDown;
		gint64 lastTickTimeMs;

		// If this is set then whenever we are idle we will work towards generating this image, otherwise we work towards generating any missing images.
		int mapTileToGenX, mapTileToGenY, mapTileToGenZoom;
		MapTiled::ImageLayerSet mapTileToGenLayerSet;
	private:
		const char *initialMapFilenameToOpen;

		int timerTickSource;
		int idleTickSource;

		unsigned operationCounter; // if 0 then no operation

		bool mapNew(void);
		bool mapOpen(const char *filename); // Returns true if successfully opened, false if choosen folder is not a valid map.
		bool mapSave(void); // Returns false on failure to save
		bool mapSaveAs(void); // Returns false on failure to save or user choosing cancel in file chooser dialogue
		bool mapClose(void); // Can return false if map has unsaved changes and they choose to cancel. Returns true if no map even loaded.
		void mapQuit(void);
		bool mapGetUnsavedChanges(void);

		double getZoomFactor(void);
		int getZoomLevelHuman(void);
		double getZoomFactorHuman(void);

		void updateFileMenuSensitivity(void);
		void updateTitle(void);
		void updateDrawingArea(void);
		void updatePositionLabel(void);

		cairo_surface_t *getMapTiledImageSurface(unsigned z, unsigned x, unsigned y, MapTiled::ImageLayer layer);

		// These are used to pause timer and idle tick functions during intensive operations.
		// operationBegin can be called while an operation is already in progress - the operation is only considered complete when operationEnd has been called a matching number of times.
		void operationBegin(void);
		void operationEnd(void);
	};
};

#endif
