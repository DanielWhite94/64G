#ifndef ENGINE_MAP_MAPTILED_H
#define ENGINE_MAP_MAPTILED_H

#include "map.h"
#include "maptexture.h"
#include "maptile.h"
#include "../util.h"

namespace Engine {
	namespace Map {
		class MapTiled {
		public:
			static const unsigned imageSize=256;
			static const unsigned maxZoom=9; // zoom in range [0,maxZoom-1], equal to 1+log2((Map::regionsSize*MapRegion::tilesSize)/imageSize)

			typedef unsigned ImageLayer;
			static const ImageLayer	ImageLayerHeight=0;
			static const ImageLayer	ImageLayerTemperature=1;
			static const ImageLayer	ImageLayerMoisture=2;
			static const ImageLayer	ImageLayerTexture=3;
			static const ImageLayer	ImageLayerHeightContour=4;
			static const ImageLayer	ImageLayerPath=5;
			static const ImageLayer	ImageLayerPolitical=6;
			static const ImageLayer	ImageLayerRegionGrid=7;
			static const ImageLayer	ImageLayerNB=8;

			typedef unsigned ImageLayerSet;
			static const ImageLayerSet ImageLayerSetNone=0;
			static const ImageLayerSet ImageLayerSetHeight=(1u<<ImageLayerHeight);
			static const ImageLayerSet ImageLayerSetTemperature=(1u<<ImageLayerTemperature);
			static const ImageLayerSet ImageLayerSetMoisture=(1u<<ImageLayerMoisture);
			static const ImageLayerSet ImageLayerSetTexture=(1u<<ImageLayerTexture);
			static const ImageLayerSet ImageLayerSetHeightContour=(1u<<ImageLayerHeightContour);
			static const ImageLayerSet ImageLayerSetPath=(1u<<ImageLayerPath);
			static const ImageLayerSet ImageLayerSetPolitical=(1u<<ImageLayerPolitical);
			static const ImageLayerSet ImageLayerSetRegionGrid=(1u<<ImageLayerRegionGrid);
			static const ImageLayerSet ImageLayerSetAll=ImageLayerSetHeight|ImageLayerSetTemperature|ImageLayerSetMoisture|ImageLayerSetTexture|ImageLayerSetHeightContour|ImageLayerSetPath|ImageLayerSetPolitical|ImageLayerSetRegionGrid;

			MapTiled();
			~MapTiled();

			static bool createMetadata(const class Map *map);

			// The conditions following are considered in the order listed.
			// If the image already exists, nothing is done.
			// If the zoom level is the max then we generate the image directly with MapPngLib.
			// In any of the four child images are missing, we recurse to generate them.
			// If/once all four children exist, we scale and stitch them to create the desired image.
			// If timeoutMs is not 0 and has been exceed, once at least one new image has been generated the function will return early, potentially unfinished.
			static bool generateImage(class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayerSet imageLayerSet, Util::TimeMs timeoutMs, Util::ProgressFunctor *progressFunctor, void *progressUserData);

			// The following functions delete pre-generated/cached MapTiled images and should be called if changes have been made to the Map.
			static bool clearImage(class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayerSet imageLayerSet); // clear only the images (one per layer) for this exact x/y/zoom combination
			static bool clearImagesAll(class Map *map, ImageLayerSet imageLayerSet, Util::ProgressFunctor *progressFunctor, void *progressUserData); // clear all images
			static bool clearImagesRegion(class Map *map, unsigned regionX, unsigned regionY, ImageLayerSet imageLayerSet); // clear all images which could be affected by changing the given region

			static void getZoomPath(const class Map *map, unsigned zoom, char path[1024]); // TODO: improve hardcoded size
			static void getZoomXPath(const class Map *map, unsigned zoom, unsigned x, char path[1024]); // TODO: improve hardcoded size
			static void getZoomXYPath(const class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayer layer, char path[1024]); // TODO: improve hardcoded size
			static void getBlankImagePath(const class Map *map, char path[1024]); // TODO: improve hardcoded size
			static void getTransparentImagePath(const class Map *map, char path[1024]); // TODO: improve hardcoded size

		private:
			static bool generateImageHelper(class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayerSet imageLayerSet, Util::TimeMs endTimeMs, unsigned long long int *imagesDone, unsigned long long int imagesTotal, Util::ProgressFunctor *progressFunctor, void *progressUserData, Util::TimeMs startTimeMs);

			static bool clearImageHelper(class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayerSet imageLayerSet, bool recurseChild, bool recurseParent);
		};
	};
};

#endif
