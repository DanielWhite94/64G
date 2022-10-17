#include <cassert>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <exception>
#include <iostream>
#include <new>

#include "mainwindow.h"

const char mainWindowXmlFile[]="mainwindow.glade";

gboolean mapEditorMainWindowWrapperWindowDeleteEvent(GtkWidget *widget, GdkEvent *event, void *userData);

gboolean mapEditorMainWindowWrapperMenuFileNewActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileOpenActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileSaveActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileSaveAsActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileCloseActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuFileQuitActivate(GtkWidget *widget, gpointer userData);

gboolean mapEditorMainWindowWrapperMenuViewZoomInActivate(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuViewZoomOutActivate(GtkWidget *widget, gpointer userData);

gboolean mapEditorMainWindowWrapperDrawingAreaDraw(GtkWidget *widget, cairo_t *cr, gpointer userData);
gboolean mapEditorMainWindowWrapperDrawingAreaKeyPressEvent(GtkWidget *widget, GdkEventKey *event, gpointer userData);
gboolean mapEditorMainWindowWrapperDrawingAreaKeyReleaseEvent(GtkWidget *widget, GdkEventKey *event, gpointer userData);

gboolean mapEditorMainWindowWrapperMenuFileQuitActivate(GtkWidget *widget, gpointer userData);

gboolean mapEditorMainWindowWrapperMenuViewShowRegionGridToggled(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuViewShowTileGridToggled(GtkWidget *widget, gpointer userData);
gboolean mapEditorMainWindowWrapperMenuViewShowKmGridToggled(GtkWidget *widget, gpointer userData);

namespace MapEditor {
	MainWindow::MainWindow() {
		// Clear basic fields
		mapTilesToGen.clear();
		map=NULL;
		zoomLevel=zoomLevelMin;
		userCentreX=Engine::Map::Map::regionsSize*MapRegion::tilesSize*userTileSize/2.0;
		userCentreY=Engine::Map::Map::regionsSize*MapRegion::tilesSize*userTileSize/2.0;
		lastTickTimeMs=0;

		keyPanningLeft=false;
		keyPanningRight=false;
		keyPanningUp=false;
		keyPanningDown=false;

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
		error|=(menuViewZoomIn=GTK_WIDGET(gtk_builder_get_object(builder, "menuViewZoomIn")))==NULL;
		error|=(menuViewZoomOut=GTK_WIDGET(gtk_builder_get_object(builder, "menuViewZoomOut")))==NULL;
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
		g_signal_connect(drawingArea, "draw", G_CALLBACK(mapEditorMainWindowWrapperDrawingAreaDraw), (void *)this);
		g_signal_connect(drawingArea, "key-press-event", G_CALLBACK(mapEditorMainWindowWrapperDrawingAreaKeyPressEvent), (void *)this);
		g_signal_connect(drawingArea, "key-release-event", G_CALLBACK(mapEditorMainWindowWrapperDrawingAreaKeyReleaseEvent), (void *)this);
		g_signal_connect(menuViewShowRegionGrid, "toggled", G_CALLBACK(mapEditorMainWindowWrapperMenuViewShowRegionGridToggled), (void *)this);
		g_signal_connect(menuViewShowTileGrid, "toggled", G_CALLBACK(mapEditorMainWindowWrapperMenuViewShowTileGridToggled), (void *)this);
		g_signal_connect(menuViewShowKmGrid, "toggled", G_CALLBACK(mapEditorMainWindowWrapperMenuViewShowKmGridToggled), (void *)this);

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
		// If there is a map loaded, we may need to generate more images.
		if (map!=NULL && !mapTilesToGen.empty()) {
			if (!mapTilesToGen.empty()) {
				// Take most recently marked 'maptile' and generate it, then remove it from the queue.
				const DrawMapTileEntry &mapTileEntry=mapTilesToGen.back();
				// TODO: fix this call - previously have genOnce=true so that only a single image would be generated before returning
				// However, we retired this functionality from generateImage as it was inefficient and complex.
				// Will need an alternative to use here
				if (MapTiled::generateImage(map, mapTileEntry.zoom, mapTileEntry.x, mapTileEntry.y, MapTiled::ImageLayerSetAll, NULL, NULL))
					mapTilesToGen.pop_back();

				// Force redraw to show new image
				updateDrawingArea();
			}

			// Update status bar
			char statusLabelStr[128];
			if (!mapTilesToGen.empty())
				sprintf(statusLabelStr, "%llu images still to render", (unsigned long long)mapTilesToGen.size());
			else
				sprintf(statusLabelStr, "image rendering complete");
			gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);
		}
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
		mapOpen();
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
		if (zoomLevel+1<zoomLevelMax) {
			++zoomLevel;
			updateDrawingArea();
			updatePositionLabel();
		}
		return false;
	}

	bool MainWindow::menuViewZoomOutActivate(GtkWidget *widget) {
		if (zoomLevel>zoomLevelMin) {
			--zoomLevel;
			updateDrawingArea();
			updatePositionLabel();
		}
		return false;
	}

	bool MainWindow::drawingAreaDraw(GtkWidget *widget, cairo_t *cr) {
		// Clear 'to gen' list (ready to populate during this function)
		mapTilesToGen.clear();

		// Special case if no map loaded
		if (map==NULL) {
			// Simply clear screen to black
			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			cairo_paint(cr);
			return false;
		}

		// Various parameters
		const double userRegionSizeX=MapRegion::tilesSize*userTileSize;
		const double userRegionSizeY=MapRegion::tilesSize*userTileSize;
		const double userKmSizeX=4.0*userRegionSizeX;
		const double userKmSizeY=4.0*userRegionSizeY;
		const double userMapSizeX=Engine::Map::Map::regionsSize*MapRegion::tilesSize*userTileSize;
		const double userMapSizeY=Engine::Map::Map::regionsSize*MapRegion::tilesSize*userTileSize;

		//                                              zoom level = {   0    1    2    3    4   5   6   7   8   9  10  11, 12, 13, 14, 15}
		// TODO: this will need adjusting after changing MapTiled image size parameters
		const double tileGridLineWidths[zoomLevelMax-zoomLevelMin]  ={/*   0,   0,   0,   0,   0,  0,  0,*/  0,  0,  0,  1,  1,  1,  1,  1,  1};
		const double regionGridLineWidths[zoomLevelMax-zoomLevelMin]={/*   0,   0,   0,   0,   0,128,128,*/ 64, 32, 32, 16,  8,  8,  8,  4,  4};
		const double kmGridLineWidths[zoomLevelMax-zoomLevelMin]    ={/*4096,4096,2048,2048,1024,256,128,*/128, 64, 32, 32, 32,  16, 16, 8,  8};
		assert(zoomLevelMax-zoomLevelMin==9);

		double deviceTopLeftX=0.0, deviceTopLeftY=0.0;
		double deviceBottomRightX=gtk_widget_get_allocated_width(drawingArea);
		double deviceBottomRightY=gtk_widget_get_allocated_height(drawingArea);

		// Clear screen to pink to make any undrawn portions clear
		cairo_set_source_rgb(cr, 1.0, 0.2, 0.6);
		cairo_paint(cr);

		// Transform cairo for given zoom and panning offset
		double zoomFactor=getZoomFactor();

		cairo_translate(cr, deviceBottomRightX/2, deviceBottomRightY/2);
		cairo_scale(cr, zoomFactor, zoomFactor);
		cairo_translate(cr, -userCentreX, -userCentreY);

		// Calculate extents of what is on screen in user space units.
		double userTopLeftX=deviceTopLeftX, userTopLeftY=deviceTopLeftY;
		cairo_device_to_user(cr, &userTopLeftX, &userTopLeftY);
		double userBottomRightX=deviceBottomRightX, userBottomRightY=deviceBottomRightY;
		cairo_device_to_user(cr, &userBottomRightX, &userBottomRightY);

		// Draw regions
		if (1) {
			// Calculate which set of map tile images to use (i.e. which zoom level)
			int mapTileZl=lrint(log2(userTileSize/MapTiled::pixelsPerTileAtMaxZoom))+zoomLevel; // as generated map tile images may use a different resolution for the same zoom level, we may need to make an adjustment
			if (mapTileZl>=0 && mapTileZl<MapTiled::maxZoom) {
				double userDevicePixelSize=pow(2.0, zoomLevelMax-1-zoomLevel); // how many user space units are represent by a single pixel (in either X or Y direction, they are equal)
				double userMapTileImageSize=userDevicePixelSize*MapTiled::imageSize;

				int mapTileStartX=floor(userTopLeftX/userMapTileImageSize);
				int mapTileStartY=floor(userTopLeftY/userMapTileImageSize);
				int mapTileEndX=ceil(userBottomRightX/userMapTileImageSize);
				int mapTileEndY=ceil(userBottomRightY/userMapTileImageSize);
				int mapTileMax=((1u)<<mapTileZl);

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

				for(int mapTileY=mapTileStartY; mapTileY<=mapTileEndY; ++mapTileY) {
					double userMapTileTopLeftY=mapTileY*userMapTileImageSize;
					for(int mapTileX=mapTileStartX; mapTileX<=mapTileEndX; ++mapTileX) {
						double userMapTileTopLeftX=mapTileX*userMapTileImageSize;

						// Attempt to load png image as a cairo surfaace
						char mapTileFilename[1024];
						MapTiled::getZoomXYPath(map, mapTileZl, mapTileX, mapTileY, MapTiled::ImageLayerBase, mapTileFilename);
						cairo_surface_t *mapTileSurface=cairo_image_surface_create_from_png(mapTileFilename);
						if (cairo_surface_status(mapTileSurface)!=CAIRO_STATUS_SUCCESS) {
							// No image avaiable - use standard blank one until it has been generated
							cairo_surface_destroy(mapTileSurface);
							MapTiled::getBlankImagePath(map, mapTileFilename);
							mapTileSurface=cairo_image_surface_create_from_png(mapTileFilename);

							// Add to list of map tiles that need generating for the current scene
							DrawMapTileEntry mapTileEntry={.zoom=mapTileZl, .x=mapTileX, .y=mapTileY};
							mapTilesToGen.push_back(mapTileEntry);
						}

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

		// Draw tile grid if needed
		if (menuViewShowTileGridIsActive() && zoomLevel>=8) {
			double userStartX=(floor(userTopLeftX/userTileSize)-1)*userTileSize;
			double userStartY=(floor(userTopLeftY/userTileSize)-1)*userTileSize;
			double userEndX=(ceil(userBottomRightX/userTileSize)+1)*userTileSize;
			double userEndY=(ceil(userBottomRightY/userTileSize)+1)*userTileSize;

			if (userStartX<0.0) userStartX=0.0;
			if (userStartY<0.0) userStartY=0.0;
			if (userEndX>=userMapSizeX) userEndX=userMapSizeX;
			if (userEndY>=userMapSizeY) userEndY=userMapSizeY;

			cairo_save(cr);
			cairo_set_line_width(cr, tileGridLineWidths[zoomLevel]);
			cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
			cairo_new_path(cr);

			for(double userCurrY=userStartY; userCurrY<=userEndY; userCurrY+=userTileSize) {
				cairo_new_sub_path(cr);
				cairo_move_to(cr, userStartX, userCurrY);
				cairo_line_to(cr, userEndX, userCurrY);
			}

			for(double userCurrX=userStartX; userCurrX<=userEndX; userCurrX+=userTileSize) {
				cairo_new_sub_path(cr);
				cairo_move_to(cr, userCurrX, userStartY);
				cairo_line_to(cr, userCurrX, userEndY);
			}

			cairo_stroke(cr);
			cairo_restore(cr);
		}

		// Draw region grid if needed
		if (menuViewShowRegionGridIsActive() && zoomLevel>=1) {
			double userStartX=(floor(userTopLeftX/userRegionSizeX)-1)*userRegionSizeX;
			double userStartY=(floor(userTopLeftY/userRegionSizeY)-1)*userRegionSizeY;
			double userEndX=(ceil(userBottomRightX/userRegionSizeX)+1)*userRegionSizeX;
			double userEndY=(ceil(userBottomRightY/userRegionSizeY)+1)*userRegionSizeY;

			if (userStartX<0.0) userStartX=0.0;
			if (userStartY<0.0) userStartY=0.0;
			if (userEndX>=userMapSizeX) userEndX=userMapSizeX;
			if (userEndY>=userMapSizeY) userEndY=userMapSizeY;

			cairo_save(cr);
			cairo_set_line_width(cr, regionGridLineWidths[zoomLevel]);
			cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
			cairo_new_path(cr);

			for(double userCurrY=userStartY; userCurrY<=userEndY; userCurrY+=userRegionSizeY) {
				cairo_new_sub_path(cr);
				cairo_move_to(cr, userStartX, userCurrY);
				cairo_line_to(cr, userEndX, userCurrY);
			}

			for(double userCurrX=userStartX; userCurrX<=userEndX; userCurrX+=userRegionSizeX) {
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

	bool MainWindow::mapNew(void) {
		// Close the current map (if any)
		if (!mapClose())
			return false;

		// Prompt user for filename
		GtkWidget *dialog=gtk_file_chooser_dialog_new("New Map", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
		    "Cancel", GTK_RESPONSE_CANCEL,
		    "Create", GTK_RESPONSE_ACCEPT,
		NULL);

		if (gtk_dialog_run(GTK_DIALOG(dialog))!=GTK_RESPONSE_ACCEPT)
			return false;

		// Extract filename
		char *filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_widget_destroy(dialog);

		// Attempt to load map
		try {
			map=new class Map(filename, 2048, 2048); // TODO: sizes are just a hack - needs a way for user to dictate
		} catch (std::exception& e) {
			map=NULL;
			return false;
		}

		// Update various things
		char statusLabelStr[1024]; // TODO: better
		sprintf(statusLabelStr, "Created new map at: %s", filename);
		gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);

		updateFileMenuSensitivity();
		updateTitle();
		updateDrawingArea();
		updatePositionLabel();

		// Tidy up
		g_free(filename);

		return true;
	}

	bool MainWindow::mapOpen(void) {
		char statusLabelStr[1024]; // TODO: better

		// Close the current map (if any)
		if (!mapClose())
			return false;

		// Prompt user for filename
		GtkWidget *dialog=gtk_file_chooser_dialog_new("Open Map", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		    "Cancel", GTK_RESPONSE_CANCEL,
		    "Open", GTK_RESPONSE_ACCEPT,
		NULL);

		if (gtk_dialog_run(GTK_DIALOG(dialog))!=GTK_RESPONSE_ACCEPT)
			return false;

		// Extract filename
		char *filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_widget_destroy(dialog);

		// Attempt to load map
		class Map *map;
		try {
			map=new class Map(filename);
		} catch (std::exception& e) {
			map=NULL;

			sprintf(statusLabelStr, "Could not open map at '%s': %s", filename, e.what());
			gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);

			return false;
		}

		// Update various things
		sprintf(statusLabelStr, "Opened map at: %s", filename);
		gtk_label_set_text(GTK_LABEL(statusLabel), statusLabelStr);

		updateFileMenuSensitivity();
		updateTitle();
		updateDrawingArea();
		updatePositionLabel();

		// Tidy up
		g_free(filename);

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

		// If there are unsaved changes prompt the user for confirmation.
		/*
		TODO: this
		if (....unsaved changes) {
			switch(.....response) {
				case .....save:
					if (!mapSave())
						return false;
				break;
				case .....saveas:
					if (!mapSaveAs())
						return false;
				break;
				case .....discard:
					// Nothing to do - simply avoid saving
				break;
				case .....cancel:
					return false;
				break;
			}
		}
		*/

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
		return pow(2.0, zoomLevel-(zoomLevelMax-1));
	}

	double MainWindow::getZoomFactorHuman(void) {
		return pow(2.0, zoomLevel);
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
		sprintf(str, "Centre (%.0f,%.0f), Zoom level %i (x%'.0f)", userCentreX, userCentreY, zoomLevel, getZoomFactorHuman());
		setlocale(LC_NUMERIC, oldLocale);

		gtk_label_set_text(GTK_LABEL(positionLabel), str);
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
