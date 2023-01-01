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
		void mapTiledGenerateImageProgressString(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData) {
			assert(map!=NULL);
			assert(progress>=0.0 && progress<=1.0);
			assert(userData!=NULL);

			const char *string=(const char *)userData;

			// Clear old line.
			Util::clearConsoleLine();

			// Print start of new line, including users message and the percentage complete.
			printf("%s%.3f%% ", string, progress*100.0);

			// Append time elapsed so far.
			Util::printTime(elapsedTimeMs);

			// Attempt to compute estimated total time.
			if (progress>=0.0001 && progress<=0.9999) {
				Util::TimeMs estRemainingTimeMs=elapsedTimeMs*(1.0/progress-1.0);
				if (estRemainingTimeMs>=1000 && estRemainingTimeMs<365ll*24ll*60ll*60ll*1000ll) {
					printf(" (~");
					Util::printTime(estRemainingTimeMs);
					printf(" remaining)");
				}
			}

			// Flush output manually (as we are not printing a newline).
			fflush(stdout);
		}

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

			// Create transparent image (used by MapTiled to speed up generating out of map bounds images)
			getTransparentImagePath(map, imagePath);
			imageFile=fopen(imagePath, "wb");
			pngPtr=png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
			png_init_io(pngPtr, imageFile);
			infoPtr=png_create_info_struct(pngPtr);
			png_set_IHDR(pngPtr, infoPtr, imageSize, imageSize, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
			png_write_info(pngPtr, infoPtr);
			png_set_compression_level(pngPtr, 3);

			pngRows=(png_bytep)malloc(imageSize*imageSize*4*sizeof(png_byte));
			r=0;
			g=0;
			b=0;
			uint8_t a=0;
			for(imageY=0; imageY<imageSize; ++imageY) {
				unsigned imageX;
				for(imageX=0; imageX<imageSize; ++imageX) {
					pngRows[(imageY*imageSize+imageX)*4+0]=r;
					pngRows[(imageY*imageSize+imageX)*4+1]=g;
					pngRows[(imageY*imageSize+imageX)*4+2]=b;
					pngRows[(imageY*imageSize+imageX)*4+3]=a;
				}
				png_write_row(pngPtr, &pngRows[(imageY*imageSize+0)*4*sizeof(png_byte)]);
			}
			free(pngRows);

			png_write_end(pngPtr, NULL);
			fclose(imageFile);
			png_free_data(pngPtr, infoPtr, PNG_FREE_ALL, -1);
			png_destroy_write_struct(&pngPtr, (png_infopp)NULL);

			return true;
		}

		bool MapTiled::generateImage(class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayerSet imageLayerSet, Util::TimeMs timeoutMs, GenerateImageProgress *progressFunctor, void *progressUserData) {
			// Compute total number of images we need to generate worst case
			unsigned layerCount=0;
			for(ImageLayer layer=0; layer<ImageLayerNB; ++layer)
				layerCount+=(imageLayerSet & (1u<<layer))!=0;

			int zoomDelta=maxZoom-zoom;
			unsigned long long int imagesTotal=0;
			for(unsigned long long int i=0; i<zoomDelta; ++i)
				imagesTotal+=std::pow(4llu, i);
			imagesTotal*=layerCount;

			// Use recursive helper function
			Util::TimeMs endTimeMs=(timeoutMs>0 ? Util::getTimeMs()+timeoutMs : 0);
			unsigned long long int imagesDone=0;
			return MapTiled::generateImageHelper(map, zoom, x, y, imageLayerSet, endTimeMs, &imagesDone, imagesTotal, progressFunctor, progressUserData, Util::getTimeMs());
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

		void MapTiled::getTransparentImagePath(const class Map *map, char path[1024]) {
			sprintf(path, "%s/transparent.png", map->getMapTiledDir());
		}

		bool MapTiled::generateImageHelper(class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayerSet imageLayerSet, Util::TimeMs endTimeMs, unsigned long long int *imagesDone, unsigned long long int imagesTotal, GenerateImageProgress *progressFunctor, void *progressUserData, Util::TimeMs startTimeMs) {
			// Invoke progress update if needed
			assert(*imagesDone<=imagesTotal);
			if (progressFunctor!=NULL)
				progressFunctor(map, ((double)*imagesDone)/imagesTotal, Util::getTimeMs()-startTimeMs, progressUserData);

			// Bad zoom value?
			if (zoom>=maxZoom)
				return false;

			// Check if any needed images at this zoom level already exist
			for(ImageLayer layer=0; layer<ImageLayerNB; ++layer) {
				// Check if we even need to generate this layer
				if (!(imageLayerSet & (1u<<layer)))
					continue;

				// Get path for this image
				char path[1024];
				getZoomXYPath(map, zoom, x, y, layer, path);

				// Does this image already exist?
				if (Util::isFile(path)) {
					// Remove it from the set of layers to do
					imageLayerSet&=~(1u<<layer);

					// Update imagesDone counter to account for any children we may have skipped
					for(unsigned long long int i=0; i<maxZoom-zoom; ++i)
						*imagesDone+=std::pow(4llu, i);

					continue;
				}
			}

			if (imageLayerSet==0) {
				// Invoke progress update if needed
				assert(*imagesDone<=imagesTotal);
				if (progressFunctor!=NULL)
					progressFunctor(map, ((double)*imagesDone)/imagesTotal, Util::getTimeMs()-startTimeMs, progressUserData);

				return true;
			}

			// If we are at the maximum zoom level then this is a 'leaf node' that needs rendering from scratch.
			if (zoom==maxZoom-1) {
				// Loop over image layer set
				for(ImageLayer layer=0; layer<ImageLayerNB; ++layer) {
					// Check if we even need to generate this layer
					if (!(imageLayerSet & (1u<<layer)))
						continue;

					// Get path for this image
					char path[1024];
					getZoomXYPath(map, zoom, x, y, layer, path);

					// Compute mappng arguments.
					unsigned mapSize=imageSize;
					unsigned mapX=x*mapSize;
					unsigned mapY=y*mapSize;

					// Generate image
					bool res=MapPngLib::generatePng(map, path, mapX, mapY, mapSize, mapSize, imageSize, imageSize, layer, true);
					if (!res)
						return false;

					++*imagesDone;

					// Time limit reached?
					if (endTimeMs>0 && Util::getTimeMs()>=endTimeMs)
						return false;

					continue;
				}
			} else {
				// Recurse to generate any needed children so we can then stitch them together
				const unsigned childZoom=zoom+1;
				const unsigned childBaseX=x*2;
				const unsigned childBaseY=y*2;
				for(unsigned tx=0; tx<2; ++tx)
					for(unsigned ty=0; ty<2; ++ty) {
						unsigned childX=childBaseX+tx;
						unsigned childY=childBaseY+ty;
						if (!generateImageHelper(map, childZoom, childX, childY, imageLayerSet, endTimeMs, imagesDone, imagesTotal, progressFunctor, progressUserData, startTimeMs))
							return false;
					}

				// Loop over image layer set
				for(ImageLayer layer=0; layer<ImageLayerNB; ++layer) {
					// Check if we even need to generate this layer
					if (!(imageLayerSet & (1u<<layer)))
						continue;

					// Get path for this image
					char path[1024];
					getZoomXYPath(map, zoom, x, y, layer, path);

					// If not a leaf node then need to compose from 4 children so generate their paths.
					char childPaths[2][2][1024]; // TODO: Improve this.
					for(unsigned tx=0; tx<2; ++tx)
						for(unsigned ty=0; ty<2; ++ty) {
							unsigned childX=childBaseX+tx;
							unsigned childY=childBaseY+ty;
							getZoomXYPath(map, childZoom, childX, childY, layer, childPaths[tx][ty]);
						}

					// Stitch together the four child images
					char stitchCommand[4096]; // TODO: better
					sprintf(stitchCommand, "montage %s %s %s %s -background none -geometry 50%%x50%% -mode concatenate -tile 2x2 PNG32:%s", childPaths[0][0], childPaths[1][0], childPaths[0][1], childPaths[1][1], path);
					system(stitchCommand);

					++*imagesDone;

					// Time limit reached?
					if (endTimeMs>0 && Util::getTimeMs()>=endTimeMs)
						return false;
				}
			}

			// Invoke progress update if needed
			assert(*imagesDone<=imagesTotal);
			if (progressFunctor!=NULL)
				progressFunctor(map, ((double)*imagesDone)/imagesTotal, Util::getTimeMs()-startTimeMs, progressUserData);

			return true;
		}

	};
};
