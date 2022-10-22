#ifndef ENGINE_MAP_MAPTILED_H
#define ENGINE_MAP_MAPTILED_H

#include "map.h"
#include "maptexture.h"
#include "maptile.h"
#include "../util.h"

namespace Engine {
	namespace Map {
		void mapTiledGenerateImageProgressString(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData);

		class MapTiled {
		public:
			static const unsigned imageSize=256;
			static const unsigned maxZoom=9; // zoom in range [0,maxZoom-1], equal to 1+log2((Map::regionsSize*MapRegion::tilesSize)/imageSize)

			typedef unsigned ImageLayer;
			static const ImageLayer	ImageLayerBase=0;
			static const ImageLayer	ImageLayerTemperature=1;
			static const ImageLayer	ImageLayerHeight=2;
			static const ImageLayer	ImageLayerMoisture=3;
			static const ImageLayer	ImageLayerHeightContour=4;
			static const ImageLayer	ImageLayerPolitical=5;
			static const ImageLayer	ImageLayerRegionGrid=6;
			static const ImageLayer	ImageLayerNB=7;

			typedef unsigned ImageLayerSet;
			static const ImageLayerSet ImageLayerSetNone=0;
			static const ImageLayerSet ImageLayerSetBase=(1u<<ImageLayerBase);
			static const ImageLayerSet ImageLayerSetTemperature=(1u<<ImageLayerTemperature);
			static const ImageLayerSet ImageLayerSetHeight=(1u<<ImageLayerHeight);
			static const ImageLayerSet ImageLayerSetMoisture=(1u<<ImageLayerMoisture);
			static const ImageLayerSet ImageLayerSetHeightContour=(1u<<ImageLayerHeightContour);
			static const ImageLayerSet ImageLayerSetPolitical=(1u<<ImageLayerPolitical);
			static const ImageLayerSet ImageLayerSetRegionGrid=(1u<<ImageLayerRegionGrid);
			static const ImageLayerSet ImageLayerSetAll=ImageLayerSetBase|ImageLayerSetTemperature|ImageLayerSetHeight|ImageLayerSetMoisture|ImageLayerSetHeightContour|ImageLayerSetPolitical|ImageLayerSetRegionGrid;

			typedef void (GenerateImageProgress)(class Map *map, double progress, Util::TimeMs elapsedTimeMs, void *userData);

			MapTiled();
			~MapTiled();

			static bool createMetadata(const class Map *map);

			// The conditions following are considered in the order listed.
			// If the image already exists, nothing is done.
			// If the zoom level is the max then we generate the image directly with MapPngLib.
			// In any of the four child images are missing, we recurse to generate them.
			// If/once all four children exist, we scale and stitch them to create the desired image.
			// If timeoutMs is not 0 and has been exceed, once at least one new image has been generated the function will return early, potentially unfinished.
			static bool generateImage(class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayerSet imageLayerSet, Util::TimeMs timeoutMs, GenerateImageProgress *progressFunctor, void *progressUserData);

			static void getZoomPath(const class Map *map, unsigned zoom, char path[1024]); // TODO: improve hardcoded size
			static void getZoomXPath(const class Map *map, unsigned zoom, unsigned x, char path[1024]); // TODO: improve hardcoded size
			static void getZoomXYPath(const class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayer layer, char path[1024]); // TODO: improve hardcoded size
			static void getBlankImagePath(const class Map *map, char path[1024]); // TODO: improve hardcoded size

		private:
			static bool generateImageHelper(class Map *map, unsigned zoom, unsigned x, unsigned y, ImageLayerSet imageLayerSet, Util::TimeMs endTimeMs, unsigned long long int *imagesDone, unsigned long long int imagesTotal, GenerateImageProgress *progressFunctor, void *progressUserData, Util::TimeMs startTimeMs);
		};
	};
};

#endif
