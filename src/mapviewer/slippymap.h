#ifndef MAPVIEWER_SLIPPYMAP_H
#define MAPVIEWER_SLIPPYMAP_H

#include <sys/stat.h>

#include "../engine/map/map.h"

namespace MapViewer {
	class SlippyMap {
	public:
		SlippyMap(const class Map *map, unsigned mapSize, const char *imageDir);
		~SlippyMap();

		char *getImagePath(unsigned tileX, unsigned tileY, int tilesPerPixel) const ; // tilesPerPixel should be a power of two. Result should be passed to free
		char *getImage(unsigned tileX, unsigned tileY, int tilesPerPixel); // tilesPerPixel should be a power of two. Result should be passed to free

		void invalidateTile(unsigned tileX, unsigned tileY);
		void invalidateRegion(unsigned regionX, unsigned regionY);
		void invalidateMap(void);

		bool tilesPerPixelIsValid(int tilesPerPixel) const;
		int getTilesPerPixelMax(void) const;
		int getMaxZoom(void) const;
		int getZoomForTilesPerPixel(int tilesPerPixel) const;
		int getTilesPerPixelForZoom(int zoom) const;
	private:
		static int imageSize;
		char *imageDir;

		const class Map *map;
		int mapSize;

		int tileXToOffsetX(unsigned tileX, int tilesPerPixel) const;
		int tileYToOffsetY(unsigned tileY, int tilesPerPixel) const;
		int offsetXToTileX(unsigned offsetX, int tilesPerPixel) const;
		int offsetYToTileY(unsigned offsetY, int tilesPerPixel) const;

		static bool fileExists(const char *name) {
			struct stat buffer;
			return (stat(name, &buffer)==0);
		}
	};

};

#endif
