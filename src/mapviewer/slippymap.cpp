#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "slippymap.h"

namespace MapViewer {
	int SlippyMap::imageSize=256;

	SlippyMap::SlippyMap(const class Map *map, unsigned mapSize, const char *gImageDir): map(map), mapSize(mapSize) {
		// Copy imageDir.
		imageDir=(char *)malloc(strlen(gImageDir)+1); // TODO: Improve this
		strcpy(imageDir, gImageDir);

		// Create sub-dirs.
		for(int zoom=0; zoom<=getMaxZoom(); ++zoom) {
			// Create zoom-level dir.
			char dirName[1024];
			sprintf(dirName, "%s/%i", imageDir, zoom);
			mkdir(dirName, S_IRWXU);

			// Loop over y coordinates/
			int imageCount=((1llu)<<zoom);
			for(int y=0; y<imageCount; ++y) {
				// Create y-level dir.
				char dirName[1024];
				sprintf(dirName, "%s/%i/%i", imageDir, zoom, y);
				mkdir(dirName, S_IRWXU);
			}
		}
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

		// Calculate equivalent zoom factor for the given tilesPerPixel.
		int zoom=getZoomForTilesPerPixel(tilesPerPixel);

		// Generate the image path.
		char *imagePath=(char *)malloc(128); // TODO: Improve this.
		sprintf(imagePath, "%s/%u/%u/%u.png", imageDir, zoom, offsetX, offsetY);

		return imagePath;
	}

	char *SlippyMap::getImage(unsigned tileX, unsigned tileY, int tilesPerPixel) {
		// Generate image path.
		char *imagePath=getImagePath(tileX, tileY, tilesPerPixel);
		if (imagePath==NULL)
			return NULL;

		// Do we need to (re)generate the image?
		if (!fileExists(imagePath)) {
			if (tilesPerPixel==1) {
				// Compute mappng arguments.
				int mapX=offsetXToTileX(tileXToOffsetX(tileX, tilesPerPixel), tilesPerPixel);
				int mapY=offsetYToTileY(tileYToOffsetY(tileY, tilesPerPixel), tilesPerPixel);
				int mapW=imageSize*tilesPerPixel;
				int mapH=imageSize*tilesPerPixel;

				// Create mappng command.
				char command[4*1024];
				sprintf(command, "./mappng --quiet %s %u %u %u %u %u %u %s", map->getBaseDir(), mapX, mapY, mapW, mapH, imageSize, imageSize, imagePath);

				// Run mappng command.
				system(command); // TODO: This better (silence output, check for errors etc).
			} else {
				int offsetX=tileXToOffsetX(tileX, tilesPerPixel);
				int offsetY=tileYToOffsetY(tileY, tilesPerPixel);

				int childOffsetX=offsetX*2;
				int childOffsetY=offsetY*2;
				int childTilesPerPixel=tilesPerPixel/2;

				// Generate images for 4 children.
				char *childPaths[2][2];
				for(int ty=0; ty<2; ++ty)
					for(int tx=0; tx<2; ++tx) {
						childPaths[tx][ty]=getImage(offsetXToTileX(childOffsetX+tx, childTilesPerPixel), offsetYToTileY(childOffsetY+ty, childTilesPerPixel), childTilesPerPixel);
					}

				// Shrink 4 child images in half and stitch them together.
				char command[4*1024];
				sprintf(command, "montage -geometry 50%%x50%%+0+0 %s %s %s %s %s", childPaths[0][0], childPaths[1][0], childPaths[0][1], childPaths[1][1], imagePath);
				system(command);

				// Free child paths.
				for(int ty=0; ty<2; ++ty)
					for(int tx=0; tx<2; ++tx)
						free(childPaths[tx][ty]);
			}
		}

		return imagePath;
	}

	char *SlippyMap::getImageByZoom(unsigned tileX, unsigned tileY, int zoom) {
		return getImage(tileX, tileY, getTilesPerPixelForZoom(zoom));
	}

	void SlippyMap::invalidateTile(unsigned tileX, unsigned tileY) {
		// Loop over every zoom level, clearing any images which contain the given tileX/Y/
		for(int tilesPerPixel=1; tilesPerPixel<=getTilesPerPixelMax(); ++tilesPerPixel) {
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

	bool SlippyMap::tilesPerPixelIsValid(int tilesPerPixel) const {
		// Ensure range is OK.
		if (tilesPerPixel<1 || tilesPerPixel>getTilesPerPixelMax())
			return false;

		// Ensure it is a power of two.
		if ((tilesPerPixel & (tilesPerPixel-1)))
			return false;

		return true;
	}

	int SlippyMap::getTilesPerPixelMax(void) const {
		return mapSize/imageSize;
	}

	int SlippyMap::getMaxZoom(void) const  {
		return getZoomForTilesPerPixel(1);
	}

	int SlippyMap::getZoomForTilesPerPixel(int tilesPerPixel) const {
		return log2(getTilesPerPixelMax()/tilesPerPixel);
	}

	int SlippyMap::getTilesPerPixelForZoom(int zoom) const {
		return ((unsigned)getTilesPerPixelMax())>>((unsigned)zoom);
	}

	int SlippyMap::tileXToOffsetX(unsigned tileX, int tilesPerPixel) const {
		assert(tilesPerPixelIsValid(tilesPerPixel));

		return tileX/(tilesPerPixel*imageSize);
	}
	int SlippyMap::tileYToOffsetY(unsigned tileY, int tilesPerPixel) const {
		assert(tilesPerPixelIsValid(tilesPerPixel));

		return tileY/(tilesPerPixel*imageSize);
	}
	int SlippyMap::offsetXToTileX(unsigned offsetX, int tilesPerPixel) const {
		assert(tilesPerPixelIsValid(tilesPerPixel));

		return offsetX*(tilesPerPixel*imageSize);
	}
	int SlippyMap::offsetYToTileY(unsigned offsetY, int tilesPerPixel) const {
		assert(tilesPerPixelIsValid(tilesPerPixel));

		return offsetY*(tilesPerPixel*imageSize);
	}
};
