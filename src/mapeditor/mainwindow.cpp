#include <algorithm>
#include <cassert>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <exception>
#include <iostream>
#include <new>

#include "heighttemperaturedialogue.h"
#include "mainwindow.h"
#include "newdialogue.h"
#include "progressdialogue.h"
#include "util.h"

#include "../engine/gen/modifytiles.h"
#include "../engine/fbnnoise.h"

const char mainWindowXmlFile[]="mainwindow.glade";

struct MainWindowToolsHeightTemperatureModifyTilesData {
	MapEditor::HeightTemperatureDialogue::Params *params;
	FbnNoise *heightNoise;
	FbnNoise *temperatureNoise;
};

gboolean mapEditorMainWindowWrapperWindowDeleteEvent(GtkWidget *widget, GdkEvent *event, void *userData);

gboolean mapEditorMainWindowWrapperMenuFileNewActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileOpenActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileSaveActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileSaveAsActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileCloseActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileQuitActivate(GtkWidget *widget, gpointer userData);

gboolean mapEditorMainWindowWrapperMenuViewZoomInActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuViewZoomOutActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuViewZoomFitActivate(GtkWidget *widget, gpointer userData);

gboolean mapEditorMainWindowWrapperMenuToolsClearActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuToolsHeightTemperatureActivate(GtkWidget *widget, gpointer userData);

gboolean mapEditorMainWindowWrapperDrawingAreaDraw(GtkWidget *widget, cairo_t *cr, gpointer userData);
gboolean mapEditorMainWindowWrapperDrawingAreaKeyPressEvent(GtkWidget *widget, GdkEventKey *event, gpointer userData);
gboolean mapEditorMainWindowWrapperDrawingAreaKeyReleaseEvent(GtkWidget *widget, GdkEventKey *event, gpointer userData);
gboolean mapEditorMainWindowWrapperDrawingAreaScrollEvent(GtkWidget *widget, GdkEventScroll *event, gpointer userData);
void mapEditorMainWindowWrapperDrawingAreaDragBegin(GtkGestureDrag *gesture, double x, double y, gpointer userData);
void mapEditorMainWindowWrapperDrawingAreaDragUpdate(GtkGestureDrag *gesture, double x, double y, gpointer userData);

gboolean mapEditorMainWindowWrapperMenuFileQuitActivate(GtkWidget *widget, gpointer userData);

gboolean mapEditorMainWindowWrapperMenuViewShowRegionGridToggled(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuViewShowTileGridToggled(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuViewShowKmGridToggled(GtkWidget *widget, gpointer userData);

gboolean mapEditorMainWindowWrapperMenuLayersToggled(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuLayersHeightContoursToggled(GtkWidget *widget, gpointer userData);

void mainWindowToolsClearModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);

void mainWindowToolsHeightTemperatureModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData);

namespace MapEditor {
	MainWindow::MainWindow(const char *filename) {
		// Clear basic fields
		map=NULL;
		zoomLevel=zoomLevelMin;
		userCentreX=0;
		userCentreY=0;
		lastTickTimeMs=0;

		keyPanningLeft=false;
		keyPanningRight=false;
		keyPanningUp=false;
		keyPanningDown=false;

		mapTileToGenX=0;
		mapTileToGenY=0;
		mapTileToGenZoom=0;
		mapTileToGenLayerSet=MapTiled::ImageLayerSetAll;

		initialMapFilenameToOpen=filename;

		// Use GtkBuilder to build our interface from the XML file.
		GtkBuilder *builder=gtk_builder_new();
		GError *err=NULL;
		if (gtk_builder_add_from_file(builder, mainWindowXmlFile, &err)==0) {
			g_error_free(err);
			throw std::runtime_error("could not init main window glade file"); // TODO: use 'err' for more info
		}

		// Connect glade signals
		gtk_builder_connect_signals(builder, NULL);

		// Extract widgets we will need.
		bool error=false;
		error|=(window=GTK_WIDGET(gtk_builder_get_object(builder, "window")))==NULL;
		error|=(menuFileNew=GTK_WIDGET(gtk_builder_get_object(builder, "menuFileNew")))==NULL;
		error|=(menuFileOpen=GTK_WIDGET(gtk_builder_get_object(builder, "menuFileOpen")))==NULL;
		error|=(menuFileSave=GTK_WIDGET(gtk_builder_get_object(builder, "menuFileSave")))==NULL;
		error|=(menuFileSaveAs=GTK_WIDGET(gtk_builder_get_object(builder, "menuFileSaveAs")))==NULL;
		error|=(menuFileClose=GTK_WIDGET(gtk_builder_get_object(builder, "menuFileClose")))==NULL;
		error|=(menuFileQuit=GTK_WIDGET(gtk_builder_get_object(builder, "menuFileQuit")))==NULL;
		error|=(statusLabel=GTK_WIDGET(gtk_builder_get_object(builder, "statusLabel")))==NULL;
		error|=(positionLabel=GTK_WIDGET(gtk_builder_get_object(builder, "positionLabel")))==NULL;
		error|=(drawingArea=GTK_WIDGET(gtk_builder_get_object(builder, "drawingArea")))==NULL;
		error|=(menuViewShowRegionGrid=GTK_WIDGET(gtk_builder_get_object(builder, "menuViewShowRegionGrid")))==NULL;
		error|=(menuViewShowTileGrid=GTK_WIDGET(gtk_builder_get_object(builder, "menuViewShowTileGrid")))==NULL;
		error|=(menuViewShowKmGrid=GTK_WIDGET(gtk_builder_get_object(builder, "menuViewShowKmGrid")))==NULL;
		error|=(menuViewLayersBase=GTK_WIDGET(gtk_builder_get_object(builder, "menuViewLayersBase")))==NULL;
		error|=(menuViewLayersTemperature=GTK_WIDGET(gtk_builder_get_object(builder, "menuViewLayersTemperature")))==NULL;
		error|=(menuViewLayersHeight=GTK_WIDGET(gtk_builder_get_object(builder, "menuViewLayersHeight")))==NULL;
		error|=(menuViewLayersMoisture=GTK_WIDGET(gtk_builder_get_object(builder, "menuViewLayersMoisture")))==NULL;
		error|=(menuViewLayersPolitical=GTK_WIDGET(gtk_builder_get_object(builder, "menuViewLayersPolitical")))==NULL;
		error|=(menuViewLayersHeightContours=GTK_WIDGET(gtk_builder_get_object(builder, "menuViewLayersHeightContours")))==NULL;
		error|=(menuViewZoomIn=GTK_WIDGET(gtk_builder_get_object(builder, "menuViewZoomIn")))==NULL;
		error|=(menuViewZoomOut=GTK_WIDGET(gtk_builder_get_object(builder, "menuViewZoomOut")))==NULL;
		error|=(menuViewZoomFit=GTK_WIDGET(gtk_builder_get_object(builder, "menuViewZoomFit")))==NULL;
		error|=(menuToolsClear=GTK_WIDGET(gtk_builder_get_object(builder, "menuToolsClear")))==NULL;
		error|=(menuToolsHeightTemperature=GTK_WIDGET(gtk_builder_get_object(builder, "menuToolsHeightTemperature")))==NULL;
		if (error)
			throw std::runtime_error("could not grab main window widgets");

		// Connect manual signals
		g_signal_connect(window, "delete-event", G_CALLBACK(mapEditorMainWindowWrapperWindowDeleteEvent), (void *)this);
		g_signal_connect(menuFileNew, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuFileNewActivate), (void *)this);
		g_signal_connect(menuFileOpen, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuFileOpenActivate), (void *)this);
		g_signal_connect(menuFileSave, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuFileSaveActivate), (void *)this);
		g_signal_connect(menuFileSaveAs, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuFileSaveAsActivate), (void *)this);
		g_signal_connect(menuFileClose, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuFileCloseActivate), (void *)this);
		g_signal_connect(menuFileQuit, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuFileQuitActivate), (void *)this);
		g_signal_connect(menuViewZoomIn, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuViewZoomInActivate), (void *)this);
		g_signal_connect(menuViewZoomOut, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuViewZoomOutActivate), (void *)this);
		g_signal_connect(menuViewZoomFit, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuViewZoomFitActivate), (void *)this);
		g_signal_connect(menuToolsClear, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuToolsClearActivate), (void *)this);
		g_signal_connect(menuToolsHeightTemperature, "activate", G_CALLBACK(mapEditorMainWindowWrapperMenuToolsHeightTemperatureActivate), (void *)this);
		g_signal_connect(drawingArea, "draw", G_CALLBACK(mapEditorMainWindowWrapperDrawingAreaDraw), (void *)this);
		g_signal_connect(drawingArea, "key-press-event", G_CALLBACK(mapEditorMainWindowWrapperDrawingAreaKeyPressEvent), (void *)this);
		g_signal_connect(drawingArea, "key-release-event", G_CALLBACK(mapEditorMainWindowWrapperDrawingAreaKeyReleaseEvent), (void *)this);
		g_signal_connect(drawingArea, "scroll-event", G_CALLBACK(mapEditorMainWindowWrapperDrawingAreaScrollEvent), (void *)this);
		g_signal_connect(menuViewShowRegionGrid, "toggled", G_CALLBACK(mapEditorMainWindowWrapperMenuViewShowRegionGridToggled), (void *)this);
		g_signal_connect(menuViewShowTileGrid, "toggled", G_CALLBACK(mapEditorMainWindowWrapperMenuViewShowTileGridToggled), (void *)this);
		g_signal_connect(menuViewShowKmGrid, "toggled", G_CALLBACK(mapEditorMainWindowWrapperMenuViewShowKmGridToggled), (void *)this);
		g_signal_connect(menuViewLayersBase, "toggled", G_CALLBACK(mapEditorMainWindowWrapperMenuLayersToggled), (void *)this);
		g_signal_connect(menuViewLayersTemperature, "toggled", G_CALLBACK(mapEditorMainWindowWrapperMenuLayersToggled), (void *)this);
		g_signal_connect(menuViewLayersHeight, "toggled", G_CALLBACK(mapEditorMainWindowWrapperMenuLayersToggled), (void *)this);
		g_signal_connect(menuViewLayersMoisture, "toggled", G_CALLBACK(mapEditorMainWindowWrapperMenuLayersToggled), (void *)this);
		g_signal_connect(menuViewLayersPolitical, "toggled", G_CALLBACK(mapEditorMainWindowWrapperMenuLayersToggled), (void *)this);
		g_signal_connect(menuViewLayersHeightContours, "toggled", G_CALLBACK(mapEditorMainWindowWrapperMenuLayersHeightContoursToggled), (void *)this);

		GtkGesture *drag=gtk_gesture_drag_new(drawingArea); // TODO: does this need freed at some point?
		gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(drag), GDK_BUTTON_PRIMARY);
		g_signal_connect(drag, "drag-begin", G_CALLBACK(mapEditorMainWindowWrapperDrawingAreaDragBegin), (void *)this);
		g_signal_connect(drag, "drag-update", G_CALLBACK(mapEditorMainWindowWrapperDrawingAreaDragUpdate), (void *)this);

		// Free memory used by GtkBuilder object.
		g_object_unref(G_OBJECT(builder));

		// Try to ensure the window is maximised on startup
		gtk_window_maximize(GTK_WINDOW(window));

		// Ensure widgets are setup correctly before showing the window
		updateFileMenuSensitivity();
		updateTitle();
		updatePositionLabel();
		gtk_label_set_text(GTK_LABEL(statusLabel), "No map open");
	}

	MainWindow::~MainWindow() {
		// Destroy window (and all other child widgets)
		if (window!=NULL)
			gtk_widget_destroy(window);
	}

	void MainWindow::show() {
		// Ensure window is on top and has focus
		gtk_widget_show_all(window);
		gtk_window_present(GTK_WINDOW(window));
	}

	void MainWindow::hide() {
		gtk_widget_hide(window);
	}

	void MainWindow::tick(void) {
		// If this is the first tick, we may need to open a map
		if (lastTickTimeMs==0 && initialMapFilenameToOpen!=NULL)
			mapOpen(initialMapFilenameToOpen);

		// Compute time since last tick
		gint64 tickTimeMs=g_get_monotonic_time()/1000.0;
		double timeDeltaMs=(lastTickTimeMs>0 ? tickTimeMs-lastTickTimeMs : 1);
		lastTickTimeMs=tickTimeMs;

		// No panning?
		if (!keyPanningLeft && !keyPanningRight && !keyPanningUp && !keyPanningDown)
			return;

		// Calculate device delta
		double speedFactor=0.25*(timeDeltaMs/1000.0);
		double deviceW=gtk_widget_get_allocated_width(drawingArea);
		double deviceH=gtk_widget_get_allocated_height(drawingArea);
		double deviceSpeedX=deviceW*speedFactor;
		double deviceSpeedY=deviceH*speedFactor;
		double deviceDeltaX=deviceSpeedX*((keyPanningLeft ? -1.0 : 0.0)+(keyPanningRight ? 1.0 : 0.0));
		double deviceDeltaY=deviceSpeedY*((keyPanningUp ? -1.0 : 0.0)+(keyPanningDown ? 1.0 : 0.0));

		// Convert to user space delta
		double zoomFactorInverse=1.0/getZoomFactor();
		double userDeltaX=deviceDeltaX*zoomFactorInverse;
		double userDeltaY=deviceDeltaY*zoomFactorInverse;

		// Update offset
		userCentreX+=userDeltaX;
		userCentreY+=userDeltaY;

		// Redraw etc
		updateDrawingArea();
		updatePositionLabel();
	}

	void MainWindow::idleTick(void) {
		// No map?
		if (map==NULL)
			return;

		// Generate a needed image (or work towards it by generating a child image)
		assert(mapTileToGenZoom<zoomLevelMax-zoomExtra);
		if (!MapTiled::generateImage(map, mapTileToGenZoom, mapTileToGenX, mapTileToGenY, mapTileToGenLayerSet, 100, NULL, NULL))
			return;

		// Reset to-gen variables
		mapTileToGenX=0;
		mapTileToGenY=0;
		mapTileToGenZoom=0;
		mapTileToGenLayerSet=MapTiled::ImageLayerSetAll;

		// Show newly generated image
		updateDrawingArea();
	}

	bool MainWindow::deleteEvent(GtkWidget *widget, GdkEvent *event) {
		mapQuit();
		return TRUE;
	}

	bool MainWindow::menuFileNewActivate(GtkWidget *widget) {
		mapNew();
		return false;
	}

	bool MainWindow::menuFileOpenActivate(GtkWidget *widget) {
		// Prompt user for filename
		GtkWidget *dialog=gtk_file_chooser_dialog_new("Open Map", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		    "Cancel", GTK_RESPONSE_CANCEL,
		    "Open", GTK_RESPONSE_ACCEPT,
		NULL);

		if (gtk_dialog_run(GTK_DIALOG(dialog))!=GTK_RESPONSE_ACCEPT) {
			gtk_widget_destroy(dialog);
			return false;
		}

		// Extract filename
		char *filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_widget_destroy(dialog);

		// Call mapOpen to do the rest of the work
		mapOpen(filename);

		// Tidy up
		g_free(filename);

		return false;
	}

	bool MainWindow::menuFileSaveActivate(GtkWidget *widget) {
		mapSave();
		return false;
	}

	bool MainWindow::menuFileSaveAsActivate(GtkWidget *widget) {
		mapSaveAs();
		return false;
	}

	bool MainWindow::menuFileCloseActivate(GtkWidget *widget) {
		mapClose();
		return false;
	}

	bool MainWindow::menuFileQuitActivate(GtkWidget *widget) {
		mapQuit();
		return false;
	}

	bool MainWindow::menuViewZoomInActivate(GtkWidget *widget) {
		setZoom(zoomLevel+1);
		return false;
	}

	bool MainWindow::menuViewZoomOutActivate(GtkWidget *widget) {
		setZoom(zoomLevel-1);
		return false;
	}

	bool MainWindow::menuViewZoomFitActivate(GtkWidget *widget) {
		zoomFit();
		return false;
	}

	void MainWindow::setZoom(int level) {
		if (level<zoomLevelMin)
			level=zoomLevelMin;
		if (level>=zoomLevelMax)
			level=zoomLevelMax-1;

		if (level==zoomLevel)
			return;

		zoomLevel=level;

		updateDrawingArea();
		updatePositionLabel();
	}

	void MainWindow::zoomFit(void) {
		if (map==NULL)
			return;

		// Find centre of the map
		userCentreX=map->getWidth()/2.0;
		userCentreY=map->getHeight()/2.0;

		// Calculate zoom level which shows entire map
		double ratioW=gtk_widget_get_allocated_width(drawingArea)/((double)map->getWidth());
		double ratioH=gtk_widget_get_allocated_height(drawingArea)/((double)map->getHeight());
		setZoom(floor(log2(std::min(ratioW, ratioH)))+8);

		// Call these manually here in case setZoom didn't because there was no change in the zoom level
		updateDrawingArea();
		updatePositionLabel();
	}

	bool MainWindow::menuToolsClearActivate(GtkWidget *widget) {
		// Sanity check
		if (map==NULL)
			return false;

		// Create progress dialogue to provide updates
		ProgressDialogue *prog=new ProgressDialogue("Clearing tile data...", window);

		// Run main operation via modifyTiles
		Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), 1, &mainWindowToolsClearModifyTilesFunctor, NULL, &progressDialogueProgressFunctor, prog);

		// Tidy up
		delete prog;
	bool MainWindow::menuToolsHeightTemperatureActivate(GtkWidget *widget) {
		// Sanity check
		if (map==NULL)
			return false;

		// Prompt user for parameters
		HeightTemperatureDialogue *heightTempDialogue;
		try {
			heightTempDialogue=new HeightTemperatureDialogue(window);
		} catch (std::exception& e) {
			heightTempDialogue=NULL;

			// Update status label
			char statusLabelStr[1024]; // TODO: better
			sprintf(statusLabelStr, "Could not create height temperature dialogue: %s", e.what());
			gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);

			return false;
		}

		HeightTemperatureDialogue::Params params={
			.heightNoiseMin=-6000.0,
			.heightNoiseMax=6000.0,
			.heightNoiseSeed=18,
			.heightNoiseOctaves=8,
			.heightNoiseFrequency=8.0,
			.temperatureNoiseMin=0.0,
			.temperatureNoiseMax=20.0,
			.temperatureNoiseSeed=20,
			.temperatureNoiseOctaves=8,
			.temperatureNoiseFrequency=1.0,
			.temperatureLapseRate=5.0,
			.temperatureLatitudeRange=60.0,
		};

		bool promptResult=heightTempDialogue->run(&params);

		delete heightTempDialogue;

		if (!promptResult)
			return false;

		// Create progress dialogue to provide updates
		ProgressDialogue *prog=new ProgressDialogue("Generating height and temperature tile data...", window);

		// Initialise map values so we can calculate these as we go
		map->minHeight=DBL_MAX;
		map->maxHeight=DBL_MIN;
		map->minTemperature=DBL_MAX;
		map->maxTemperature=DBL_MIN;

		// Run main operation via modifyTiles
		MainWindowToolsHeightTemperatureModifyTilesData modifyTilesData={
			.params=&params,
			.heightNoise=new FbnNoise(params.heightNoiseSeed, params.heightNoiseOctaves, params.heightNoiseFrequency),
			.temperatureNoise=new FbnNoise(params.temperatureNoiseSeed, params.temperatureNoiseOctaves, params.temperatureNoiseFrequency),
		};
		Gen::modifyTiles(map, 0, 0, map->getWidth(), map->getHeight(), 1, &mainWindowToolsHeightTemperatureModifyTilesFunctor, &modifyTilesData, &progressDialogueProgressFunctor, prog);

		// Tidy up
		delete modifyTilesData.heightNoise;
		delete modifyTilesData.temperatureNoise;
		delete prog;

		return false;
	}

	bool MainWindow::drawingAreaDraw(GtkWidget *widget, cairo_t *cr) {
		// Special case if no map loaded
		if (map==NULL) {
			// Simply clear screen to black
			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			cairo_paint(cr);
			return false;
		}

		// Clear any existing to-gen image
		mapTileToGenX=0;
		mapTileToGenY=0;
		mapTileToGenZoom=0;
		mapTileToGenLayerSet=MapTiled::ImageLayerSetAll;

		// Various parameters
		const double userKmSizeX=4.0*MapRegion::tilesSize;
		const double userKmSizeY=4.0*MapRegion::tilesSize;
		const double userMapSizeX=Engine::Map::Map::regionsSize*MapRegion::tilesSize;
		const double userMapSizeY=Engine::Map::Map::regionsSize*MapRegion::tilesSize;

		//                                              zoom level = {  0   1   2   3   4   5   6   7   8   9  10    11,    12,     13}
		// TODO: this will need adjusting after changing MapTiled image size parameters
		const double tileGridLineWidths[zoomLevelMax-zoomLevelMin]  ={  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1/32.0,1/64.0,1/128.0};
		const double regionGridLineWidths[zoomLevelMax-zoomLevelMin]={ 64, 32, 32, 16,  8,  8,  4,  2,  1,  1,0.5,  0.25, 0.125,  0.125};
		const double kmGridLineWidths[zoomLevelMax-zoomLevelMin]    ={128, 64, 32, 32, 32, 16,  8,  4,  2,  2,  1,   0.5,  0.25,   0.25};
		assert(zoomLevelMax-zoomLevelMin==9+5);

		double deviceTopLeftX=0.0, deviceTopLeftY=0.0;
		double deviceBottomRightX=gtk_widget_get_allocated_width(drawingArea);
		double deviceBottomRightY=gtk_widget_get_allocated_height(drawingArea);

		// Clear screen to black
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_paint(cr);

		// Transform cairo for given zoom and panning offset
		double zoomFactor=getZoomFactor();

		cairo_translate(cr, deviceBottomRightX/2, deviceBottomRightY/2);
		cairo_scale(cr, zoomFactor, zoomFactor);
		cairo_translate(cr, -userCentreX, -userCentreY);

		// Draw grey box to represent maximum possible map extents
		cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
		cairo_rectangle(cr, 0, 0, Engine::Map::Map::regionsSize*MapRegion::tilesSize, Engine::Map::Map::regionsSize*MapRegion::tilesSize);
		cairo_fill(cr);

		// Calculate extents of what is on screen in user space units.
		double userTopLeftX=deviceTopLeftX, userTopLeftY=deviceTopLeftY;
		cairo_device_to_user(cr, &userTopLeftX, &userTopLeftY);
		double userBottomRightX=deviceBottomRightX, userBottomRightY=deviceBottomRightY;
		cairo_device_to_user(cr, &userBottomRightX, &userBottomRightY);

		// Draw regions
		if (1) {
			// Calculate which set of map tile images to use (i.e. which zoom level)
			if (1) {
				double userDevicePixelSize=pow(2.0, zoomLevelMax-zoomExtra-1-zoomLevel); // how many user space units are represent by a single pixel (in either X or Y direction, they are equal)
				double userMapTileImageSize=userDevicePixelSize*MapTiled::imageSize;

				int mapTileStartX=floor(userTopLeftX/userMapTileImageSize);
				int mapTileStartY=floor(userTopLeftY/userMapTileImageSize);
				int mapTileEndX=ceil(userBottomRightX/userMapTileImageSize);
				int mapTileEndY=ceil(userBottomRightY/userMapTileImageSize);
				int mapTileMax=((1u)<<zoomLevel);

				if (mapTileStartX<0)
					mapTileStartX=0;
				if (mapTileStartX>=mapTileMax)
					mapTileStartX=mapTileMax-1;
				if (mapTileStartY<0)
					mapTileStartY=0;
				if (mapTileStartY>=mapTileMax)
					mapTileStartY=mapTileMax-1;

				if (mapTileEndX<0)
					mapTileEndX=0;
				if (mapTileEndX>=mapTileMax)
					mapTileEndX=mapTileMax-1;
				if (mapTileEndY<0)
					mapTileEndY=0;
				if (mapTileEndY>=mapTileMax)
					mapTileEndY=mapTileMax-1;

				MapTiled::ImageLayer activeLayer=menuViewLayersGetActiveLayer();

				for(int mapTileY=mapTileStartY; mapTileY<=mapTileEndY; ++mapTileY) {
					double userMapTileTopLeftY=mapTileY*userMapTileImageSize;
					for(int mapTileX=mapTileStartX; mapTileX<=mapTileEndX; ++mapTileX) {
						double userMapTileTopLeftX=mapTileX*userMapTileImageSize;

						// Attempt to load png image as a cairo surface
						cairo_surface_t *mapTileSurface=getMapTiledImageSurface(zoomLevel, mapTileX, mapTileY, activeLayer);

						// Draw surface
						if (cairo_surface_status(mapTileSurface)==CAIRO_STATUS_SUCCESS) {
							// To blit the png surface we will reset the cairo transformation matrix.
							// So before we do that work out where we should be drawing said surface.
							double deviceMapTileTopLeftX=userMapTileTopLeftX;
							double deviceMapTileTopLeftY=userMapTileTopLeftY;
							cairo_user_to_device(cr, &deviceMapTileTopLeftX, &deviceMapTileTopLeftY);

							// Reset transformation matrix and blit
							cairo_save(cr);
							cairo_identity_matrix(cr);
							cairo_set_source_surface(cr, mapTileSurface, deviceMapTileTopLeftX, deviceMapTileTopLeftY);
							cairo_paint(cr);
							cairo_surface_destroy(mapTileSurface);
							cairo_restore(cr);
						}

						// Do we need to draw any other layers on top?
						if (menuViewLayersHeightContoursIsActive()) {
							cairo_surface_t *mapTileSurface=getMapTiledImageSurface(zoomLevel, mapTileX, mapTileY, MapTiled::ImageLayerHeightContour);
							if (cairo_surface_status(mapTileSurface)==CAIRO_STATUS_SUCCESS) {
								// To blit the png surface we will reset the cairo transformation matrix.
								// So before we do that work out where we should be drawing said surface.
								double deviceMapTileTopLeftX=userMapTileTopLeftX;
								double deviceMapTileTopLeftY=userMapTileTopLeftY;
								cairo_user_to_device(cr, &deviceMapTileTopLeftX, &deviceMapTileTopLeftY);

								// Reset transformation matrix and blit
								cairo_save(cr);
								cairo_identity_matrix(cr);
								cairo_set_source_surface(cr, mapTileSurface, deviceMapTileTopLeftX, deviceMapTileTopLeftY);
								cairo_paint(cr);
								cairo_surface_destroy(mapTileSurface);
								cairo_restore(cr);
							}
						}
					}
				}
			}
		}

		// Draw tile grid if needed
		if (menuViewShowTileGridIsActive() && zoomLevel>=10) {
			double userStartX=floor(userTopLeftX)-1;
			double userStartY=floor(userTopLeftY)-1;
			double userEndX=ceil(userBottomRightX)+1;
			double userEndY=ceil(userBottomRightY)+1;

			if (userStartX<0.0) userStartX=0.0;
			if (userStartY<0.0) userStartY=0.0;
			if (userEndX>=userMapSizeX) userEndX=userMapSizeX;
			if (userEndY>=userMapSizeY) userEndY=userMapSizeY;

			cairo_save(cr);
			cairo_set_line_width(cr, tileGridLineWidths[zoomLevel]);
			cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
			cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
			cairo_new_path(cr);

			for(double userCurrY=userStartY; userCurrY<=userEndY; ++userCurrY) {
				cairo_new_sub_path(cr);
				cairo_move_to(cr, userStartX, userCurrY);
				cairo_line_to(cr, userEndX, userCurrY);
			}

			for(double userCurrX=userStartX; userCurrX<=userEndX; ++userCurrX) {
				cairo_new_sub_path(cr);
				cairo_move_to(cr, userCurrX, userStartY);
				cairo_line_to(cr, userCurrX, userEndY);
			}

			cairo_stroke(cr);
			cairo_restore(cr);
		}

		// Draw region grid if needed
		if (menuViewShowRegionGridIsActive() && zoomLevel>=2) {
			double userStartX=(floor(userTopLeftX/MapRegion::tilesSize)-1)*MapRegion::tilesSize;
			double userStartY=(floor(userTopLeftY/MapRegion::tilesSize)-1)*MapRegion::tilesSize;
			double userEndX=(ceil(userBottomRightX/MapRegion::tilesSize)+1)*MapRegion::tilesSize;
			double userEndY=(ceil(userBottomRightY/MapRegion::tilesSize)+1)*MapRegion::tilesSize;

			if (userStartX<0.0) userStartX=0.0;
			if (userStartY<0.0) userStartY=0.0;
			if (userEndX>=userMapSizeX) userEndX=userMapSizeX;
			if (userEndY>=userMapSizeY) userEndY=userMapSizeY;

			cairo_save(cr);
			cairo_set_line_width(cr, regionGridLineWidths[zoomLevel]);
			cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
			cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
			cairo_new_path(cr);

			for(double userCurrY=userStartY; userCurrY<=userEndY; userCurrY+=MapRegion::tilesSize) {
				cairo_new_sub_path(cr);
				cairo_move_to(cr, userStartX, userCurrY);
				cairo_line_to(cr, userEndX, userCurrY);
			}

			for(double userCurrX=userStartX; userCurrX<=userEndX; userCurrX+=MapRegion::tilesSize) {
				cairo_new_sub_path(cr);
				cairo_move_to(cr, userCurrX, userStartY);
				cairo_line_to(cr, userCurrX, userEndY);
			}

			cairo_stroke(cr);
			cairo_restore(cr);
		}

		// Draw km grid if needed
		if (menuViewShowKmGridIsActive()) {
			double userStartX=(floor(userTopLeftX/userKmSizeX)-1)*userKmSizeX;
			double userStartY=(floor(userTopLeftY/userKmSizeY)-1)*userKmSizeY;
			double userEndX=(ceil(userBottomRightX/userKmSizeX)+1)*userKmSizeX;
			double userEndY=(ceil(userBottomRightY/userKmSizeY)+1)*userKmSizeY;

			if (userStartX<0.0) userStartX=0.0;
			if (userStartY<0.0) userStartY=0.0;
			if (userEndX>=userMapSizeX) userEndX=userMapSizeX;
			if (userEndY>=userMapSizeY) userEndY=userMapSizeY;

			cairo_save(cr);
			cairo_set_line_width(cr, kmGridLineWidths[zoomLevel]);
			cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
			cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
			cairo_new_path(cr);

			for(double userCurrY=userStartY; userCurrY<=userEndY; userCurrY+=userKmSizeY) {
				cairo_new_sub_path(cr);
				cairo_move_to(cr, userStartX, userCurrY);
				cairo_line_to(cr, userEndX, userCurrY);
			}

			for(double userCurrX=userStartX; userCurrX<=userEndX; userCurrX+=userKmSizeX) {
				cairo_new_sub_path(cr);
				cairo_move_to(cr, userCurrX, userStartY);
				cairo_line_to(cr, userCurrX, userEndY);
			}

			cairo_stroke(cr);
			cairo_restore(cr);
		}

		return false;
	}

	gboolean MainWindow::drawingAreaKeyPressEvent(GtkWidget *widget, GdkEventKey *event) {
		if (event->keyval==GDK_KEY_Left) {
			keyPanningLeft=true;
			return true;
		}
		if (event->keyval==GDK_KEY_Right) {
			keyPanningRight=true;
			return true;
		}
		if (event->keyval==GDK_KEY_Up) {
			keyPanningUp=true;
			return true;
		}
		if (event->keyval==GDK_KEY_Down) {
			keyPanningDown=true;
			return true;
		}
		return false;
	}

	gboolean MainWindow::drawingAreaKeyReleaseEvent(GtkWidget *widget, GdkEventKey *event) {
		if (event->keyval==GDK_KEY_Left) {
			keyPanningLeft=false;
			return true;
		}
		if (event->keyval==GDK_KEY_Right) {
			keyPanningRight=false;
			return true;
		}
		if (event->keyval==GDK_KEY_Up) {
			keyPanningUp=false;
			return true;
		}
		if (event->keyval==GDK_KEY_Down) {
			keyPanningDown=false;
			return true;
		}
		return false;
	}

	gboolean MainWindow::drawingAreaScrollEvent(GtkWidget *widget, GdkEventScroll *event) {
		// Ensure we have correct event
		if (event->type!=GDK_SCROLL)
			return false;

		// Zoom in/out based on direction
		if (event->direction==GDK_SCROLL_UP)
			setZoom(zoomLevel+1);
		else if (event->direction==GDK_SCROLL_DOWN)
			setZoom(zoomLevel-1);

		return false;
	}

	void MainWindow::drawingAreaDragBegin(double startX, double startY) {
		dragBeginUserCentreX=userCentreX;
		dragBeginUserCentreY=userCentreY;
	}

	void MainWindow::drawingAreaDragUpdate(double startX, double startY, double offsetX, double offsetY) {
		double zoomFactor=getZoomFactor();
		userCentreX=dragBeginUserCentreX-offsetX/zoomFactor;
		userCentreY=dragBeginUserCentreY-offsetY/zoomFactor;

		updateDrawingArea();
		updatePositionLabel();
	}

	bool MainWindow::menuViewShowRegionGridIsActive(void) {
		return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuViewShowRegionGrid));
	}

	bool MainWindow::menuViewShowRegionGridToggled(GtkWidget *widget) {
		updateDrawingArea();
		return false;
	}

	bool MainWindow::menuViewShowTileGridIsActive(void) {
		return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuViewShowTileGrid));
	}

	bool MainWindow::menuViewShowTileGridToggled(GtkWidget *widget) {
		updateDrawingArea();
		return false;
	}

	bool MainWindow::menuViewShowKmGridIsActive(void) {
		return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuViewShowKmGrid));
	}

	bool MainWindow::menuViewShowKmGridToggled(GtkWidget *widget) {
		updateDrawingArea();
		return false;
	}

	MapTiled::ImageLayer MainWindow::menuViewLayersGetActiveLayer(void) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuViewLayersTemperature)))
			return MapTiled::ImageLayerTemperature;
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuViewLayersHeight)))
			return MapTiled::ImageLayerHeight;
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuViewLayersMoisture)))
			return MapTiled::ImageLayerMoisture;
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuViewLayersPolitical)))
			return MapTiled::ImageLayerPolitical;
		return MapTiled::ImageLayerBase;
	}

	bool MainWindow::menuViewLayersToggled(GtkWidget *widget) {
		updateDrawingArea();
		return false;
	}

	bool MainWindow::menuViewLayersHeightContoursIsActive(void) {
		return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuViewLayersHeightContours));
	}

	bool MainWindow::menuViewLayersHeightContoursToggled(GtkWidget *widget) {
		updateDrawingArea();
		return false;
	}

	bool MainWindow::mapNew(void) {
		// Close the current map (if any)
		if (!mapClose())
			return false;

		// Prompt user for parameters
		NewDialogue *newDialogue;
		try {
			newDialogue=new NewDialogue(window);
		} catch (std::exception& e) {
			newDialogue=NULL;

			// Update status label
			char statusLabelStr[1024]; // TODO: better
			sprintf(statusLabelStr, "Could not create new dialogue: %s", e.what());
			gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);

			return false;
		}

		unsigned mapW=4096;
		unsigned mapH=4096;

		bool newResult=newDialogue->run(&mapW, &mapH);

		delete newDialogue;

		if (!newResult)
			return false;

		// Prompt user for filename
		GtkWidget *dialog=gtk_file_chooser_dialog_new("New Map", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
		    "Cancel", GTK_RESPONSE_CANCEL,
		    "Create", GTK_RESPONSE_ACCEPT,
		NULL);

		if (gtk_dialog_run(GTK_DIALOG(dialog))!=GTK_RESPONSE_ACCEPT) {
			gtk_widget_destroy(dialog);
			return false;
		}

		// Extract filename
		char *filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_widget_destroy(dialog);

		// Attempt to load map
		Util::unlinkDir(filename); // use of file chooser dialogue above seems to create an empty folder which causes Map constructor to think it is already in use
		try {
			map=new class Map(filename, mapW, mapH);
		} catch (std::exception& e) {
			map=NULL;

			// Update status label
			char statusLabelStr[1024]; // TODO: better
			sprintf(statusLabelStr, "Could not create new map at: %s: %s", filename, e.what());
			gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);

			return false;
		}

		// Reset to-gen variables
		mapTileToGenX=0;
		mapTileToGenY=0;
		mapTileToGenZoom=0;
		mapTileToGenLayerSet=MapTiled::ImageLayerSetAll;

		// Update various things
		char statusLabelStr[1024]; // TODO: better
		sprintf(statusLabelStr, "Created new map at: %s", filename);
		gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);

		updateFileMenuSensitivity();
		updateTitle();

		// Centre map within the drawing area with the zoom set such that the whole map is visible
		// This also updates the drawing area and position label
		zoomFit();

		// Tidy up
		g_free(filename);

		return true;
	}

	bool MainWindow::mapOpen(const char *filename) {
		char statusLabelStr[1024]; // TODO: better

		// Close the current map (if any)
		if (!mapClose())
			return false;

		// Attempt to load map
		try {
			map=new class Map(filename, false);
		} catch (std::exception& e) {
			// Clear (invalid) map handle
			map=NULL;

			// Update status message
			sprintf(statusLabelStr, "Could not open map at '%s': %s", filename, e.what());
			gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);

			// If due to being locked, ask user if should override
			if (strcmp(e.what(), "locked")==0) {
				// Prompt user
				sprintf(statusLabelStr, "Could not open map at '%s': %s.\n\nIgnore lock and try again?", filename, e.what());
				if (!utilPromptForYesNo("Could not open map", statusLabelStr, window))
					return false;

				// Try loading again but ignoring the lock file
				try {
					map=new class Map(filename, true);
				} catch (std::exception& e2) {
					// Clear (invalid) map handle
					map=NULL;

					// Update status message
					sprintf(statusLabelStr, "Could not open map at '%s': %s", filename, e2.what());
					gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);

					return false;
				}
			} else
				return false;
		}

		// Reset to-gen variables
		mapTileToGenX=0;
		mapTileToGenY=0;
		mapTileToGenZoom=0;
		mapTileToGenLayerSet=MapTiled::ImageLayerSetAll;

		// Update various things
		sprintf(statusLabelStr, "Opened map at: %s", filename);
		gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);

		updateFileMenuSensitivity();
		updateTitle();

		// Centre map within the drawing area with the zoom set such that the whole map is visible
		// This also updates the drawing area and position label
		zoomFit();

		return true;
	}

	bool MainWindow::mapSave(void) {
		// No map loaded?
		if (map==NULL)
			return false;

		// Attempt to save
		if (!map->save())
			return false;

		// Update various things
		gtk_label_set_text(GTK_LABEL(statusLabel), "Map saved");

		updateFileMenuSensitivity();
		updateTitle();

		return true;
	}

	bool MainWindow::mapSaveAs(void) {
		// No map loaded?
		if (map==NULL)
			return false;

		// Prompt user for new filename
		GtkWidget *dialog=gtk_file_chooser_dialog_new("Save Map As", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
		    "Cancel", GTK_RESPONSE_CANCEL,
		    "Save As", GTK_RESPONSE_ACCEPT,
		NULL);

		if (gtk_dialog_run(GTK_DIALOG(dialog))!=GTK_RESPONSE_ACCEPT)
			return false;

		// Extract filename
		char *filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_widget_destroy(dialog);

		// Change path and save
		// TODO: this
		return false;

		// Update various things
		char statusLabelStr[1024]; // TODO: better
		sprintf(statusLabelStr, "Map now saved to: %s", filename);
		gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);

		updateFileMenuSensitivity();
		updateTitle();

		// Tidy up
		g_free(filename);

		return true;
	}

	bool MainWindow::mapClose(void) {
		// No map even loaded?
		if (map==NULL)
			return true;

		// Unload the current map
		delete map;
		map=NULL;

		// Update various things
		gtk_label_set_text(GTK_LABEL(statusLabel), "Closed map");

		updateFileMenuSensitivity();
		updateTitle();
		updateDrawingArea();
		updatePositionLabel();

		return true;
	}

	void MainWindow::mapQuit(void) {
		// Close the current map (if any)
		if (!mapClose())
			return;

		// Call gtk_main_quit function to cause gtk_main call in our main function to return.
		gtk_main_quit();
	}

	bool MainWindow::mapGetUnsavedChanges(void) {
		// TODO: this
		return false;
	}

	double MainWindow::getZoomFactor(void) {
		return pow(2.0, zoomLevel-(zoomLevelMax-zoomExtra-1));
	}

	int MainWindow::getZoomLevelHuman(void) {
		return zoomLevel;
	}

	double MainWindow::getZoomFactorHuman(void) {
		return pow(2.0, getZoomLevelHuman());
	}

	void MainWindow::updateFileMenuSensitivity(void) {
		bool mapOpen=(map!=NULL);
		bool mapChanges=mapGetUnsavedChanges();

		gtk_widget_set_sensitive(menuFileSave, (mapOpen && mapChanges));
		gtk_widget_set_sensitive(menuFileSaveAs, mapOpen);
		gtk_widget_set_sensitive(menuFileClose, mapOpen);
	}

	void MainWindow::updateTitle(void) {
		char str[2048]; // TODO: better
		if (map!=NULL)
			sprintf(str, "%s%s - Map Editor", map->getBaseDir(), (mapGetUnsavedChanges() ? "*" : ""));
		else
			sprintf(str, "Map Editor");
		gtk_window_set_title(GTK_WINDOW(window), str);
	}

	void MainWindow::updateDrawingArea(void) {
		gtk_widget_queue_draw(drawingArea);
	}

	void MainWindow::updatePositionLabel(void) {
		char str[1024]; // TODO: better

		char *oldLocale = setlocale(LC_NUMERIC, NULL);
		setlocale(LC_NUMERIC, "");
		sprintf(str, "Centre (%.0f,%.0f), Zoom level %i (x%.0f)", userCentreX, userCentreY, getZoomLevelHuman(), getZoomFactorHuman());
		setlocale(LC_NUMERIC, oldLocale);

		gtk_label_set_text(GTK_LABEL(positionLabel), str);
	}

	cairo_surface_t *MainWindow::getMapTiledImageSurface(unsigned z, unsigned x, unsigned y, MapTiled::ImageLayer layer) {
		// Separate case if need to generate/adjust the image due to being beyond MapTiled max zoom
		if (z>=zoomLevelMax-zoomExtra) {
			// Find and attempt to grab 'parent' image (more zoomed out which we can sample from)
			unsigned aZ=z, aX=x, aY=y;
			unsigned subSize=256;
			unsigned zoomMultiplier=1;
			while(aZ>=zoomLevelMax-zoomExtra) {
				--aZ;
				aX/=2;
				aY/=2;
				subSize/=2;
				zoomMultiplier*=2;
			}

			cairo_surface_t *parentSurface=getMapTiledImageSurface(aZ, aX, aY, layer);

			// Determine which part of parent image we need and scale as required
			int subOffsetX=256*(x-aX*zoomMultiplier)/zoomMultiplier;
			int subOffsetY=256*(y-aY*zoomMultiplier)/zoomMultiplier;

			cairo_surface_t *surface=cairo_surface_create_for_rectangle(parentSurface, subOffsetX, subOffsetY, subSize, subSize);
			cairo_surface_destroy(parentSurface);

			cairo_surface_set_device_scale(surface, 1.0/zoomMultiplier, 1.0/zoomMultiplier);

			return surface;
		}

		// Calculate image path
		char path[1024];
		MapTiled::getZoomXYPath(map, z, x, y, layer, path);

		// Attempt to load image
		cairo_surface_t *surface=cairo_image_surface_create_from_png(path);

		// Unsuccessful?
		if (cairo_surface_status(surface)!=CAIRO_STATUS_SUCCESS) {
			// No image avaiable - use standard blank one until it has been generated
			cairo_surface_destroy(surface);
			MapTiled::getBlankImagePath(map, path);
			surface=cairo_image_surface_create_from_png(path);

			// Set to-gen variables to indicate this image is missing and needs generating
			if (mapTileToGenZoom==0) {
				mapTileToGenX=x;
				mapTileToGenY=y;
				mapTileToGenZoom=z;
				mapTileToGenLayerSet=((1u)<<layer);
			}
		}

		return surface;
	}
};

gboolean mapEditorMainWindowWrapperWindowDeleteEvent(GtkWidget *widget, GdkEvent *event, void *userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->deleteEvent(widget, event);
}

gboolean mapEditorMainWindowWrapperMenuFileNewActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuFileNewActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuFileOpenActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuFileOpenActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuFileSaveActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuFileSaveActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuFileSaveAsActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuFileSaveAsActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuFileCloseActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuFileCloseActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuFileQuitActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuFileQuitActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuViewZoomInActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuViewZoomInActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuViewZoomOutActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuViewZoomOutActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuViewZoomFitActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuViewZoomFitActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuToolsClearActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuToolsClearActivate(widget);
}

gboolean mapEditorMainWindowWrapperMenuToolsHeightTemperatureActivate(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuToolsHeightTemperatureActivate(widget);
}

gboolean mapEditorMainWindowWrapperDrawingAreaDraw(GtkWidget *widget, cairo_t *cr, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->drawingAreaDraw(widget, cr);
}

gboolean mapEditorMainWindowWrapperDrawingAreaKeyPressEvent(GtkWidget *widget, GdkEventKey *event, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->drawingAreaKeyPressEvent(widget, event);
}

gboolean mapEditorMainWindowWrapperDrawingAreaKeyReleaseEvent(GtkWidget *widget, GdkEventKey *event, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->drawingAreaKeyReleaseEvent(widget, event);
}

gboolean mapEditorMainWindowWrapperDrawingAreaScrollEvent(GtkWidget *widget, GdkEventScroll *event, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->drawingAreaScrollEvent(widget, event);
}

void mapEditorMainWindowWrapperDrawingAreaDragBegin(GtkGestureDrag *gesture, double x, double y, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	mainWindow->drawingAreaDragBegin(x, y);
}

void mapEditorMainWindowWrapperDrawingAreaDragUpdate(GtkGestureDrag *gesture, double x, double y, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;

	double startX, startY;
	if (gtk_gesture_drag_get_start_point(gesture, &startX, &startY))
		mainWindow->drawingAreaDragUpdate(startX, startY, x, y);
}

gboolean mapEditorMainWindowWrapperMenuViewShowRegionGridToggled(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuViewShowRegionGridToggled(widget);
}

gboolean mapEditorMainWindowWrapperMenuViewShowTileGridToggled(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuViewShowTileGridToggled(widget);
}

gboolean mapEditorMainWindowWrapperMenuViewShowKmGridToggled(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuViewShowKmGridToggled(widget);
}

gboolean mapEditorMainWindowWrapperMenuLayersToggled(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuViewLayersToggled(widget);
}

gboolean mapEditorMainWindowWrapperMenuLayersHeightContoursToggled(GtkWidget *widget, gpointer userData) {
	MapEditor::MainWindow *mainWindow=(MapEditor::MainWindow *)userData;
	return mainWindow->menuViewLayersHeightContoursToggled(widget);
}

void mainWindowToolsClearModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData==NULL);

	// Grab tile
	MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::CreateDirty);
	if (tile==NULL)
		return;

	// Clear tile fields
	tile->setHeight(0.0);
	tile->setMoisture(0.0);
	tile->setTemperature(0.0);
	tile->setLandmassId(0);
	for(unsigned i=0; i<MapTile::layersMax; ++i) {
		MapTile::Layer layer={.textureId=MapTexture::IdMax, .hitmask=HitMask()};
		tile->setLayer(i, layer);
	}
}

void mainWindowToolsHeightTemperatureModifyTilesFunctor(unsigned threadId, class Map *map, unsigned x, unsigned y, void *userData) {
	assert(map!=NULL);
	assert(userData!=NULL);

	MainWindowToolsHeightTemperatureModifyTilesData *data=(MainWindowToolsHeightTemperatureModifyTilesData *)userData;

	// Grab tile
	MapTile *tile=map->getTileAtOffset(x, y, Engine::Map::Map::GetTileFlag::CreateDirty);
	if (tile==NULL)
		return;

	// Calculate height
	double normalisedHeight=(1.0+data->heightNoise->eval(x/((double)map->getWidth()), y/((double)map->getHeight())))/2.0; // [0.0,1.0]
	const double height=data->params->heightNoiseMin+(data->params->heightNoiseMax-data->params->heightNoiseMin)*normalisedHeight;

	// Calculate temperature
	double temperature=0.0;

	const double temperatureRandomOffset=(1.0+data->temperatureNoise->eval(x, y))/2.0;
	temperature+=data->params->temperatureNoiseMin+temperatureRandomOffset*(data->params->temperatureNoiseMax-data->params->temperatureNoiseMin);

	const double latitude=2.0*((double)y)/map->getHeight()-1.0;
	assert(latitude>=-1.0 && latitude<=1.0);
	const double poleDistance=1.0-fabs(latitude);
	assert(poleDistance>=0.0 && poleDistance<=1.0);
	double adjustedPoleDistance=2*poleDistance-1; // -1..1
	assert(adjustedPoleDistance>=-1.0 && adjustedPoleDistance<=1.0);
	temperature+=adjustedPoleDistance*data->params->temperatureLatitudeRange/2.0;

	temperature-=std::max(0.0, height/1000.0)*data->params->temperatureLapseRate; // /1000 because lapse rate is in degrees/km

	// Update tile
	tile->setHeight(height);
	tile->setTemperature(temperature);

	// See if we need to update min/max values
	map->minHeight=std::min(map->minHeight, height);
	map->maxHeight=std::max(map->maxHeight, height);
	map->minTemperature=std::min(map->minTemperature, temperature);
	map->maxTemperature=std::max(map->maxTemperature, temperature);
}
