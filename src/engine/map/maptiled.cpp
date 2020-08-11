#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

		bool MapTiled::generateImage(class Map *map, unsigned zoom, unsigned x, unsigned y, unsigned minZoomToGen, ImageLayerSet imageLayerSet, bool genOnce, bool *haveGen) {
			if (haveGen!=NULL)
				*haveGen=false;

			// Bad zoom value?
			if (zoom>=maxZoom)
				return false;

			// Loop over image layer set
			for(ImageLayer layer=0; layer<ImageLayerNB; ++layer) {
				// Check if we even need to generate this layer
				if (!(imageLayerSet & (1u<<layer)))
					continue;

				// Get path for this image
				char path[1024];
				getZoomXYPath(map, zoom, x, y, layer, path);

				// Does this image already exist?
				if (Util::isFile(path))
					continue;

				// If we are at the maximum zoom level then this is a 'leaf node' that needs rendering from scratch.
				if (zoom==maxZoom-1) {
					// Weird edge case - why would the caller request this...
					if (zoom<minZoomToGen)
						continue;

					// Compute mappng arguments.
					unsigned mapSize=imageSize/pixelsPerTileAtMaxZoom;
					unsigned mapX=x*mapSize;
					unsigned mapY=y*mapSize;

					// Generate image
					bool res=MapPngLib::generatePng(map, path, mapX, mapY, mapSize, mapSize, imageSize, imageSize, layer, true);
					if (haveGen!=NULL)
						*haveGen=res;
					if (!res)
						return false;
					continue;
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
						getZoomXYPath(map, childZoom, childX, childY, layer, childPaths[tx][ty]);

						if (!Util::isFile(childPaths[tx][ty]))
							allchildrenExist=false;
					}

				// If all children exist, can simply stitch together regardless of depth option.
				if (allchildrenExist) {
					// Shrink 4 child images in half and stitch them together
					char stitchCommand[4096]; // TODO: better
					sprintf(stitchCommand, "montage %s %s %s %s -geometry 50%%x50%% -mode concatenate -tile 2x2 %s", childPaths[0][0], childPaths[1][0], childPaths[0][1], childPaths[1][1], path);
					system(stitchCommand);

					continue;
				}

				// Neither image nor children already exist, so zoom level is large enough then simply generate desired image.
				if (zoom>=minZoomToGen) {
					// Compute mappng arguments.
					unsigned mapSize=(imageSize*(1u<<(maxZoom-1-zoom)))/pixelsPerTileAtMaxZoom;
					unsigned mapX=x*mapSize;
					unsigned mapY=y*mapSize;

					// Generate image
					bool res=MapPngLib::generatePng(map, path, mapX, mapY, mapSize, mapSize, imageSize, imageSize, layer, true);
					if (haveGen!=NULL)
						*haveGen=res;
					if (!res)
						return false;
					continue;
				}

				// Otherwise recurse to generate children and stitch together
				for(unsigned tx=0; tx<2; ++tx)
					for(unsigned ty=0; ty<2; ++ty) {
						unsigned childX=childBaseX+tx;
						unsigned childY=childBaseY+ty;

						bool childHaveGen=false;
						if (!generateImage(map, childZoom, childX, childY, minZoomToGen, imageLayerSet, genOnce, &childHaveGen))
							return false;
						if (genOnce && childHaveGen)
							return false;
					}

				char stitchCommand[4096]; // TODO: better
				sprintf(stitchCommand, "montage %s %s %s %s -geometry 50%%x50%% -mode concatenate -tile 2x2 %s", childPaths[0][0], childPaths[1][0], childPaths[0][1], childPaths[1][1], path);
				system(stitchCommand);

			}

			// Height contour map generation
			if (imageLayerSet & ImageLayerSetHeightGreyscale) {
				// TODO: improve these fixed sized buffers

				char contourInput[1024];
				getZoomXYPath(map, zoom, x, y, ImageLayerHeightGreyscale, contourInput);

				char dirPath[1024];
				getZoomXPath(map, zoom, x, dirPath);
				char contourOutput[1024];
				sprintf(contourOutput, "%s/%u-contour.png", dirPath, y);

				MapTiled::generateContourImage(contourInput, contourOutput); // TODO: check return?
			}

			return true;
		}

		void MapTiled::getZoomPath(const class Map *map, unsigned zoom, char path[1024]) {
			sprintf(path, "%s/%u", map->getMapTiledDir(), zoom);
		}

		void MapTiled::getZoomXPath(const class Map *map, unsigned zoom, unsigned x, char path[1024]) {
			sprintf(path, "%s/%u/%u", map->getMapTiledDir(), zoom, x);
		}

		void MapTiled::getZoomXYPath(const class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayer layer, char path[1024]) {
			sprintf(path, "%s/%u/%u/%u-%u.png", map->getMapTiledDir(), zoom, x, y, layer);
		}

		void MapTiled::getBlankImagePath(const class Map *map, char path[1024]) {
			sprintf(path, "%s/blank.png", map->getMapTiledDir());
		}

		bool MapTiled::generateContourImage(const char *input, const char *output) {
			// Choose parameters
			int contourCount=19;
			int contourStep=100/(contourCount+1);

			const char *tmpDirPath="/tmp"; // TODO: can probably improve this

			// Create temporary edge-detected images for each contour level
			char command[2048]; // TODO: improve this
			for(unsigned i=contourStep; i<100; i+=contourStep) {
				char edgePath[512];
				sprintf(edgePath, "%s/edge%u.png", tmpDirPath, i);

				// Extract black and white image for this height threshold,
				// then apply edge detection to find this particular contour.
				// The contour will be white with a black background
				sprintf(command, "convert %s -threshold %u%% -canny 0x1+10%%+30%% %s", input, i, edgePath);
				system(command);

				// Special case: check for fully white image implying no edges found
				if (Util::isImageWhite(edgePath)) {
					// Make all black to indicate 'no edges'
					sprintf(command, "convert %s -negate %s", edgePath, edgePath);
					system(command);
				}

				// Tidy up the image to produce a black edge on a transparent background
				sprintf(command, "convert %s -transparent black -alpha extract -threshold 0 -negate -transparent white %s", edgePath, edgePath);
				system(command);
			}

			// Join each contour image together to make final output
			char subCommand[1024];
			sprintf(command, "convert -size %ux%u xc:transparent", MapTiled::imageSize, MapTiled::imageSize);

			for(unsigned i=contourStep; i<100; i+=contourStep) {
				sprintf(subCommand, " %s/edge%u.png -composite", tmpDirPath, i);
				strcat(command, subCommand);
			}

			sprintf(subCommand, " %s", output);
			strcat(command, subCommand);

			system(command);

			// Remove temporary images
			for(unsigned i=contourStep; i<100; i+=contourStep) {
				sprintf(command, "rm %s/edge%u.png", tmpDirPath, i);
				system(command);
			}

			return true;
		}
	};
};
