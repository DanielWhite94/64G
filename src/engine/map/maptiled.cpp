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

		bool MapTiled::generateImage(class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayerSet imageLayerSet, Util::TimeMs timeoutMs, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			assert(zoom<=maxZoom);

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

		bool MapTiled::clearImage(class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayerSet imageLayerSet) {
			assert(zoom<=maxZoom);

			return clearImageHelper(map, zoom, x, y, imageLayerSet, false, false);
		}

		bool MapTiled::clearImagesAll(class Map *map, ImageLayerSet imageLayerSet, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			bool result=true;
			Util::TimeMs startTimeMs=Util::getTimeMs();

			// Call progress functor
			if (progressFunctor!=NULL && !progressFunctor(0.0, Util::getTimeMs()-startTimeMs, progressUserData))
				return false;

			// Loop over zoom levels
			// TODO: find closed form for this
			unsigned totalImages=0;
			for(unsigned zoom=0; zoom<maxZoom; ++zoom) {
				unsigned perSide=(1u<<zoom);
				totalImages+=perSide*perSide;
			}
			unsigned imagesHandled=0;

			for(unsigned zoom=0; zoom<maxZoom; ++zoom) {
				// Loop over in X direction
				unsigned maxXY=(1u<<zoom);
				for(unsigned x=0; x<maxXY; ++x) {
					// Loop over in Y direction
					for(unsigned y=0; y<maxXY; ++y) {
						result&=clearImage(map, zoom, x, y, imageLayerSet);
						++imagesHandled;
					}

					// Call progress functor
					if (progressFunctor!=NULL) {
						assert(imagesHandled<=totalImages);
						double progress=((double)imagesHandled)/totalImages;
						if (!progressFunctor(progress, Util::getTimeMs()-startTimeMs, progressUserData))
							return false;
					}
				}
			}

			// Call progress functor
			if (progressFunctor!=NULL && !progressFunctor(1.0, Util::getTimeMs()-startTimeMs, progressUserData))
				return false;

			return result;
		}

		bool MapTiled::clearImagesRegion(class Map *map, unsigned regionX, unsigned regionY, ImageLayerSet imageLayerSet) {
			// Clear MapTiled image which matches this region 1:1 and all those affected by it
			return clearImageHelper(map, maxZoom, regionX, regionY, imageLayerSet, true, true);
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

		bool MapTiled::generateImageHelper(class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayerSet imageLayerSet, Util::TimeMs endTimeMs, unsigned long long int *imagesDone, unsigned long long int imagesTotal, Util::ProgressFunctor *progressFunctor, void *progressUserData, Util::TimeMs startTimeMs) {
			// Invoke progress update if needed
			assert(*imagesDone<=imagesTotal);
			if (progressFunctor!=NULL && !progressFunctor(((double)*imagesDone)/imagesTotal, Util::getTimeMs()-startTimeMs, progressUserData))
				return false;

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
				if (progressFunctor!=NULL && !progressFunctor(((double)*imagesDone)/imagesTotal, Util::getTimeMs()-startTimeMs, progressUserData))
					return false;

				return true;
			}

			// If we are at the maximum zoom level then this is a 'leaf node' that needs rendering from scratch.
			// Also choose this path if the area the image covers is beyond the map boundaries as an optimisation taking advantage of a similar special case in MapPngLib::generatePng.
			unsigned mapSize=imageSize;
			unsigned mapX=x*mapSize;
			unsigned mapY=y*mapSize;
			if (zoom==maxZoom-1 || (mapX>=map->getWidth() || mapY>=map->getHeight())) {
				// Loop over image layer set
				for(ImageLayer layer=0; layer<ImageLayerNB; ++layer) {
					// Check if we even need to generate this layer
					if (!(imageLayerSet & (1u<<layer)))
						continue;

					// Get path for this image
					char path[1024];
					getZoomXYPath(map, zoom, x, y, layer, path);

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
			if (progressFunctor!=NULL && !progressFunctor(((double)*imagesDone)/imagesTotal, Util::getTimeMs()-startTimeMs, progressUserData))
				return false;

			return true;
		}

		bool MapTiled::clearImageHelper(class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayerSet imageLayerSet, bool recurseChild, bool recurseParent) {
			bool result=true;

			// First handle the given image itself
			for(ImageLayer layer=0; layer<ImageLayerNB; ++layer) {
				// Check if we even need to clear this layer
				if (!(imageLayerSet & (1u<<layer)))
					continue;

				// Get path for this image
				char path[1024];
				getZoomXYPath(map, zoom, x, y, layer, path);

				// Delete image (if it even exists)
				result&=Util::unlinkFile(path);
			}

			// Handle 'child' images if needed
			if (recurseChild && zoom+1<maxZoom) {
				unsigned cx=x*2;
				unsigned cy=y*2;
				unsigned pz=zoom+1;
				result&=clearImageHelper(map, pz, cx+0, cy+0, imageLayerSet, true, false);
				result&=clearImageHelper(map, pz, cx+0, cy+1, imageLayerSet, true, false);
				result&=clearImageHelper(map, pz, cx+1, cy+0, imageLayerSet, true, false);
				result&=clearImageHelper(map, pz, cx+1, cy+1, imageLayerSet, true, false);

			}

			// Handle 'parent' images if needed
			if (recurseParent && zoom>0) {
				unsigned px=x/2;
				unsigned py=y/2;
				unsigned pz=zoom-1;
				result&=clearImageHelper(map, pz, px, py, imageLayerSet, false, true);
			}

			return result;

			return result;
		}

	};
};
