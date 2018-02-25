#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "slippymap.h"

namespace MapViewer {
	int SlippyMap::imageSize=1024;
	int SlippyMap::tilesPerPixelMax=(Engine::Map::Map::regionsWide*MapRegion::tilesWide)/imageSize; // TODO: Think about how Y comes into this.

	SlippyMap::SlippyMap(const class Map *map, const char *gImageDir) {
		imageDir=(char *)malloc(strlen(gImageDir)+1); // TODO: Improve this
		strcpy(imageDir, gImageDir);
	}

	SlippyMap::~SlippyMap() {
		free(imageDir);
	}

	char *SlippyMap::getImagePath(unsigned tileX, unsigned tileY, int tilesPerPixel) const {
		// Ensure tileX/Y are in range.
		if (tileX>=Engine::Map::Map::regionsWide*MapRegion::tilesWide || tileY>=Engine::Map::Map::regionsHigh*MapRegion::tilesHigh)
			return NULL;

		// Ensure tilesPerPixel is valid.
		if (!tilesPerPixelIsValid(tilesPerPixel))
			return NULL;

		// Convert tileX/Y into offsets.
		unsigned offsetX=tileXToOffsetX(tileX, tilesPerPixel);
		unsigned offsetY=tileYToOffsetY(tileY, tilesPerPixel);

		// Generate the image path.
		char *imagePath=(char *)malloc(128); // TODO: Improve this.
		sprintf(imagePath, "%s/%ux%u,%u.png", imageDir, tilesPerPixel, offsetX, offsetY);

		return imagePath;
	}

	char *SlippyMap::getImage(unsigned tileX, unsigned tileY, int tilesPerPixel) {
		// Generate image path.
		char *imagePath=getImagePath(tileX, tileY, tilesPerPixel);
		if (imagePath==NULL)
			return NULL;

		// Do we need to (re)generate the image?
		/*
		TODO: this
		if (..... file does not exist) {
			if (tilesPerPixel==1) {
				..... most zoomed in - simply call mappng with the right arguments
			} else {
				.... call getImage on 4 child images and then stitch them together
			}
		}
		*/

		return imagePath;
	}

	void SlippyMap::invalidateTile(unsigned tileX, unsigned tileY) {
		// Loop over every zoom level, clearing any images which contain the given tileX/Y/
		for(int tilesPerPixel=1; tilesPerPixel<=tilesPerPixelMax; ++tilesPerPixel) {
			char *imagePath=getImagePath(tileX, tileY, tilesPerPixel);
			if (imagePath==NULL)
				continue;
			unlink(imagePath);
			free(imagePath);
		}
	}

	void SlippyMap::invalidateRegion(unsigned regionX, unsigned regionY) {
		// TODO: Call invalidateTile on each tile in this region.
	}

	void SlippyMap::invalidateMap(void) {
		// TODO: Simply clear whole directory.
	}

	bool SlippyMap::tilesPerPixelIsValid(int tilesPerPixel) {
		// Ensure range is OK.
		if (tilesPerPixel<1 || tilesPerPixel>tilesPerPixelMax)
			return false;

		// Ensure it is a power of two.
		if ((tilesPerPixel & (tilesPerPixel-1)))
			return false;

		return true;
	}

	int SlippyMap::tileXToOffsetX(unsigned tileX, int tilesPerPixel) {
		assert(tilesPerPixelIsValid(tilesPerPixel));

		return tileX/(tilesPerPixel*imageSize);
	}
	int SlippyMap::tileYToOffsetY(unsigned tileY, int tilesPerPixel) {
		assert(tilesPerPixelIsValid(tilesPerPixel));

		return tileY/(tilesPerPixel*imageSize);
	}
	int SlippyMap::offsetXToTileX(unsigned offsetX, int tilesPerPixel) {
		assert(tilesPerPixelIsValid(tilesPerPixel));

		return offsetX*(tilesPerPixel*imageSize);
	}
	int SlippyMap::offsetYToTileY(unsigned offsetY, int tilesPerPixel) {
		assert(tilesPerPixelIsValid(tilesPerPixel));

		return offsetY*(tilesPerPixel*imageSize);
	}
};
