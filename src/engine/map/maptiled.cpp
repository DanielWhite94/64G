#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <png.h>

#include "mappnglib.h"
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

		bool MapTiled::createMetadata(const class Map *map) {
			char path[2048]; // TODO: better

			// Create directories for each zoom level
			for(unsigned zoom=0; zoom<maxZoom; ++zoom) {
				getZoomPath(map, zoom, path);
				if (!Util::isDir(path) && !Util::makeDir(path))
					return false;

				// Create directories for each x coordinate
				unsigned maxX=(1u<<zoom);
				for(unsigned x=0; x<maxX; ++x) {
					getZoomXPath(map, zoom, x, path);
					if (!Util::isDir(path) && !Util::makeDir(path))
						return false;
				}
			}

			// Create blank image (can be used if an image has not yet been generated)
			char imagePath[2048]; // TODO: better
			getBlankImagePath(map, imagePath);
			FILE *imageFile=fopen(imagePath, "wb");
			png_structp pngPtr=png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
			png_init_io(pngPtr, imageFile);
			png_infop infoPtr=png_create_info_struct(pngPtr);
			png_set_IHDR(pngPtr, infoPtr, imageSize, imageSize, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
			png_write_info(pngPtr, infoPtr);
			png_set_compression_level(pngPtr, 3);

			png_bytep pngRows=(png_bytep)malloc(imageSize*imageSize*3*sizeof(png_byte));
			uint8_t r=255, g=0, b=255;
			unsigned imageY;
			for(imageY=0; imageY<imageSize; ++imageY) {
				unsigned imageX;
				for(imageX=0; imageX<imageSize; ++imageX) {
					pngRows[(imageY*imageSize+imageX)*3+0]=r;
					pngRows[(imageY*imageSize+imageX)*3+1]=g;
					pngRows[(imageY*imageSize+imageX)*3+2]=b;
				}
				png_write_row(pngPtr, &pngRows[(imageY*imageSize+0)*3*sizeof(png_byte)]);
			}
			free(pngRows);

			png_write_end(pngPtr, NULL);
			fclose(imageFile);
			png_free_data(pngPtr, infoPtr, PNG_FREE_ALL, -1);
			png_destroy_write_struct(&pngPtr, (png_infopp)NULL);

			return true;
		}

		bool MapTiled::generateImage(class Map *map, unsigned zoom, unsigned x, unsigned y, unsigned minZoomToGen, bool genOnce, bool *haveGen) {
			char path[1024];
			getZoomXYPath(map, zoom, x, y, path);

			if (haveGen!=NULL)
				*haveGen=false;

			// Bad zoom value?
			if (zoom>=maxZoom)
				return false;

			// Does this map tile already exist?
			if (Util::isFile(path))
				return true;

			// If we are at the maximum zoom level then this is a 'leaf node' that needs rendering from scratch.
			if (zoom==maxZoom-1) {
				// Weird edge case - why would the caller request this...
				if (zoom<minZoomToGen)
					return false;

				// Compute mappng arguments.
				unsigned mapSize=imageSize/pixelsPerTileAtMaxZoom;
				unsigned mapX=x*mapSize;
				unsigned mapY=y*mapSize;

				// Generate image
				bool res=MapPngLib::generatePng(map, path, mapX, mapY, mapSize, mapSize, imageSize, imageSize, true);
				if (haveGen!=NULL)
					*haveGen=res;
				return res;
			}

			// If not a leaf node then need to compose from 4 children. First generate their paths and check if they exist.
			bool allchildrenExist=true;
			char childPaths[2][2][1024]; // TODO: Improve this.
			unsigned childZoom=zoom+1;
			unsigned childBaseX=x*2;
			unsigned childBaseY=y*2;
			for(unsigned tx=0; tx<2; ++tx)
				for(unsigned ty=0; ty<2; ++ty) {
					unsigned childX=childBaseX+tx;
					unsigned childY=childBaseY+ty;
					getZoomXYPath(map, childZoom, childX, childY, childPaths[tx][ty]);

					if (!Util::isFile(childPaths[tx][ty]))
						allchildrenExist=false;
				}

			// If all children exist, can simply stitch together regardless of depth option.
			if (allchildrenExist) {
				// Shrink 4 child images in half and stitch them together
				char stitchCommand[4096]; // TODO: better
				sprintf(stitchCommand, "montage %s %s %s %s -geometry 50%%x50%% -mode concatenate -tile 2x2 %s", childPaths[0][0], childPaths[1][0], childPaths[0][1], childPaths[1][1], path);
				system(stitchCommand);

				return true;
			}

			// Neither image nor children already exist, so zoom level is large enough then simply generate desired image.
			if (zoom>=minZoomToGen) {
				// Compute mappng arguments.
				unsigned mapSize=(imageSize*(1u<<(maxZoom-1-zoom)))/pixelsPerTileAtMaxZoom;
				unsigned mapX=x*mapSize;
				unsigned mapY=y*mapSize;

				// Generate image
				bool res=MapPngLib::generatePng(map, path, mapX, mapY, mapSize, mapSize, imageSize, imageSize, true);
				if (haveGen!=NULL)
					*haveGen=res;
				return res;
			}

			// Otherwise recurse to generate children and stitch together
			for(unsigned tx=0; tx<2; ++tx)
				for(unsigned ty=0; ty<2; ++ty) {
					unsigned childX=childBaseX+tx;
					unsigned childY=childBaseY+ty;

					bool childHaveGen=false;
					if (!generateImage(map, childZoom, childX, childY, minZoomToGen, genOnce, &childHaveGen))
						return false;
					if (genOnce && childHaveGen)
						return false;
				}

			char stitchCommand[4096]; // TODO: better
			sprintf(stitchCommand, "montage %s %s %s %s -geometry 50%%x50%% -mode concatenate -tile 2x2 %s", childPaths[0][0], childPaths[1][0], childPaths[0][1], childPaths[1][1], path);
			system(stitchCommand);

			return true;
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

		void MapTiled::getBlankImagePath(const class Map *map, char path[1024]) {
			sprintf(path, "%s/blank.png", map->getMapTiledDir());
		}
	};
};
