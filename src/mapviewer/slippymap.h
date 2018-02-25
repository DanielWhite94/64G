#ifndef MAPVIEWER_SLIPPYMAP_H
#define MAPVIEWER_SLIPPYMAP_H

#include <sys/stat.h>

#include "../engine/map/map.h"

namespace MapViewer {
	class SlippyMap {
	public:
		SlippyMap(const class Map *map, const char *imageDir);
		~SlippyMap();

		char *getImagePath(unsigned tileX, unsigned tileY, int tilesPerPixel) const ; // tilesPerPixel should be a power of two. Result should be passed to free
		char *getImage(unsigned tileX, unsigned tileY, int tilesPerPixel); // tilesPerPixel should be a power of two. Result should be passed to free

		void invalidateTile(unsigned tileX, unsigned tileY);
		void invalidateRegion(unsigned regionX, unsigned regionY);
		void invalidateMap(void);

		static bool tilesPerPixelIsValid(int tilesPerPixel);
	private:
		static int imageSize;
		static int tilesPerPixelMax;
		char *imageDir;

		static int tileXToOffsetX(unsigned tileX, int tilesPerPixel);
		static int tileYToOffsetY(unsigned tileY, int tilesPerPixel);
		static int offsetXToTileX(unsigned offsetX, int tilesPerPixel);
		static int offsetYToTileY(unsigned offsetY, int tilesPerPixel);

		static bool fileExists(const char *name) {
			struct stat buffer;
			return (stat(name, &buffer)==0);
		}
	};

};

#endif
