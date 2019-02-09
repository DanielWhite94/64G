#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "maptiled.h"
#include "../physics/coord.h"
#include "../util.h"

using namespace Engine;

namespace Engine {
	namespace Map {
		MapTiled::MapTiled() {
		}

		MapTiled::~MapTiled() {
		};

		bool MapTiled::createDirs(const class Map *map) {
			char path[2048]; // TODO: better

			for(unsigned zoom=0; zoom<maxZoom; ++zoom) {
				getZoomPath(map, zoom, path);
				if (!Util::isDir(path) && !Util::makeDir(path))
					return false;

				unsigned maxX=(1u<<zoom);
				for(unsigned x=0; x<maxX; ++x) {
					getZoomXPath(map, zoom, x, path);
					if (!Util::isDir(path) && !Util::makeDir(path))
						return false;
				}
			}
			return true;
		}

		void MapTiled::generateTileMap(const class Map *map, unsigned zoom, unsigned x, unsigned y) {
			char path[1024];
			getZoomXYPath(map, zoom, x, y, path);

			// Does this map tile already exist?
			if (Util::isFile(path))
				return;

			// If we are at the maximum zoom level then this is a 'leaf node' that needs rendering from scratch.
			if (zoom==maxZoom-1) {
				// Compute mappng arguments.
				unsigned mapW=imageSize/pixelsPerTileAtMaxZoom;
				unsigned mapH=imageSize/pixelsPerTileAtMaxZoom;
				unsigned mapX=x*mapW;
				unsigned mapY=y*mapH;

				// Create mappng command.
				char baseCommand[4096];
				sprintf(baseCommand, "./mappng --quiet %s %u %u %u %u %u %u %s", map->getBaseDir(), mapX, mapY, mapW, mapH, imageSize, imageSize, path);

				// Run mappng command.
				system(baseCommand); // TODO: This better (silence output, check for errors etc).

				return;
			}

			// If not a leaf node then need to compose from 4 children. First ensure they exist and generate their paths
			char childPaths[2][2][1024]; // TODO: Improve this.
			unsigned childZoom=zoom+1;
			unsigned childBaseX=x*2;
			unsigned childBaseY=y*2;
			for(unsigned tx=0; tx<2; ++tx)
				for(unsigned ty=0; ty<2; ++ty) {
					unsigned childX=childBaseX+tx;
					unsigned childY=childBaseY+ty;
					getZoomXYPath(map, childZoom, childX, childY, childPaths[tx][ty]);
					generateTileMap(map, childZoom, childX, childY);
				}

			// Shrink 4 child images in half and stitch them together
			char stitchCommand[4096]; // TODO: better
			sprintf(stitchCommand, "montage -geometry 50%%x50%%+0+0 %s %s %s %s %s", childPaths[0][0], childPaths[1][0], childPaths[0][1], childPaths[1][1], path);
			system(stitchCommand);
		}
		void MapTiled::getZoomPath(const class Map *map, unsigned zoom, char path[1024]) {
			sprintf(path, "%s/%u", map->getMapTiledDir(), zoom);
		}

		void MapTiled::getZoomXPath(const class Map *map, unsigned zoom, unsigned x, char path[1024]) {
			sprintf(path, "%s/%u/%u", map->getMapTiledDir(), zoom, x);
		}

		void MapTiled::getZoomXYPath(const class Map *map, unsigned zoom, unsigned x, unsigned y, char path[1024]) {
			sprintf(path, "%s/%u/%u/%u.png", map->getMapTiledDir(), zoom, x, y);
		}

	};
};
