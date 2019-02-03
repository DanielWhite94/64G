#include <cassert>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <exception>
#include <iostream>

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
		map=NULL;
		zoomLevel=zoomLevelMin;
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

		// Redraw etc
		updateDrawingArea();
		updatePositionLabel();
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
		if (zoomLevel<zoomLevelMax) {
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
		// Special case if no map loaded
		if (map==NULL) {
			// Simply clear screen to black
			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			cairo_paint(cr);
			return false;
		}

		// Various parameters
		const double userTileSizeX=32.0;
		const double userTileSizeY=32.0;
		const double userRegionSizeX=MapRegion::tilesWide*userTileSizeX;
		const double userRegionSizeY=MapRegion::tilesHigh*userTileSizeY;
		const double userKmSizeX=4.0*userRegionSizeX;
		const double userKmSizeY=4.0*userRegionSizeY;
		const double userMapSizeX=Engine::Map::Map::regionsWide*MapRegion::tilesWide*userTileSizeX;
		const double userMapSizeY=Engine::Map::Map::regionsHigh*MapRegion::tilesHigh*userTileSizeY;

		//                                                zoom level = {  0   1   2   3   4   5   6   7   8   9  10  11}
		const double tileGridLineWidths[zoomLevelMax+1-zoomLevelMin]  ={0  ,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1};
		const double regionGridLineWidths[zoomLevelMax+1-zoomLevelMin]={0  ,128, 64, 32, 32, 32, 16, 16, 16,  8,  4,  4};
		const double kmGridLineWidths[zoomLevelMax+1-zoomLevelMin]    ={512,512,256,128,128,128, 64, 32, 16, 16,  8,  8};

		double deviceTopLeftX=0.0, deviceTopLeftY=0.0;
		double deviceBottomRightX=gtk_widget_get_allocated_width(drawingArea);
		double deviceBottomRightY=gtk_widget_get_allocated_height(drawingArea);

		// Clear screen to pink to make any undrawn portions clear
		cairo_set_source_rgb(cr, 1.0, 0.2, 0.6);
		cairo_paint(cr);

		// Transform cairo for given zoom and panning offset
		double zoomFactor=getZoomFactor();
		cairo_scale(cr, zoomFactor, zoomFactor);

		// Calculate extents of what is on screen in user space units.
		double userTopLeftX=deviceTopLeftX, userTopLeftY=deviceTopLeftY;
		cairo_device_to_user(cr, &userTopLeftX, &userTopLeftY);
		double userBottomRightX=deviceBottomRightX, userBottomRightY=deviceBottomRightY;
		cairo_device_to_user(cr, &userBottomRightX, &userBottomRightY);

		// Draw tile grid if needed
		if (menuViewShowTileGridIsActive() && zoomLevel>=8) {
			double userStartX=(floor(userTopLeftX/userTileSizeX)-1)*userTileSizeX;
			double userStartY=(floor(userTopLeftY/userTileSizeY)-1)*userTileSizeY;
			double userEndX=(ceil(userBottomRightX/userTileSizeX)+1)*userTileSizeX;
			double userEndY=(ceil(userBottomRightY/userTileSizeY)+1)*userTileSizeY;

			if (userStartX<0.0) userStartX=0.0;
			if (userStartY<0.0) userStartY=0.0;
			if (userEndX>=userMapSizeX) userEndX=userMapSizeX;
			if (userEndY>=userMapSizeY) userEndY=userMapSizeY;

			cairo_save(cr);
			cairo_set_line_width(cr, tileGridLineWidths[zoomLevel]);
			cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
			cairo_new_path(cr);

			for(double userCurrY=userStartY; userCurrY<=userEndY; userCurrY+=userTileSizeY) {
				cairo_new_sub_path(cr);
				cairo_move_to(cr, userStartX, userCurrY);
				cairo_line_to(cr, userEndX, userCurrY);
			}

			for(double userCurrX=userStartX; userCurrX<=userEndX; userCurrX+=userTileSizeX) {
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
		map=new class Map(filename);
		if (map==NULL || !map->initialized) {
			delete map;
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
		map=new class Map(filename);
		if (map==NULL || !map->initialized) {
			delete map;
			map=NULL;
			return false;
		}

		// Update various things
		char statusLabelStr[1024]; // TODO: better
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
		return pow(2.0, zoomLevel-zoomLevelMax);
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
		sprintf(str, "Zoom level %i (x%'.0f)", zoomLevel, getZoomFactorHuman());
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
