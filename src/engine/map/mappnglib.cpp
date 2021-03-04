#include <cassert>

#include "mappnglib.h"

namespace Engine {
	bool MapPngLib::generatePng(class Map *map, const char *imagePath, int mapTileX, int mapTileY, int mapTileWidth, int mapTileHeight, int imageWidth, int imageHeight, MapTiled::ImageLayer layer, bool quiet) {
		assert(map!=NULL);
		assert(mapTileX>=0);
		assert(mapTileY>=0);

		// Create file for writing.
		if (!quiet)
			printf("Creating image at '%s'.\n", imagePath);
		FILE *file=fopen(imagePath, "wb");
		if (file==NULL) {
			if (!quiet)
				printf("Could not create image file at '%s'.\n", imagePath);
			return false;
		}

		// Setup PNG image.
		png_structp pngPtr=png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		png_init_io(pngPtr, file);

		png_infop infoPtr=png_create_info_struct(pngPtr);
		png_set_IHDR(pngPtr, infoPtr, imageWidth, imageHeight, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_write_info(pngPtr, infoPtr);

		png_set_compression_level(pngPtr, 3);

		// Create rows, looping one region at a time to avoid unnecessary unloading and reloading.
		png_bytep pngRows=(png_bytep)malloc(imageHeight*imageWidth*4*sizeof(png_byte));

		double xScale=((double)mapTileWidth)/((double)imageWidth);
		double yScale=((double)mapTileHeight)/((double)imageHeight);

		const unsigned tileX0=mapTileX;
		const unsigned tileY0=mapTileY;
		const unsigned tileX1=(imageWidth-1)*xScale+mapTileX;
		const unsigned tileY1=(imageHeight-1)*yScale+mapTileY;
		const unsigned regionX0=tileX0/MapRegion::tilesSize;
		const unsigned regionY0=tileY0/MapRegion::tilesSize;
		const unsigned regionX1=tileX1/MapRegion::tilesSize+1;
		const unsigned regionY1=tileY1/MapRegion::tilesSize+1;

		unsigned regionX, regionY;
		for(regionY=regionY0; regionY<regionY1; ++regionY) {
			const int regionTileY0=regionY*MapRegion::tilesSize;
			const int regionTileY1=regionTileY0+MapRegion::tilesSize;
			for(regionX=regionX0; regionX<regionX1; ++regionX) {
				// Loop over each pixel covered by the current region.
				const int regionTileX0=regionX*MapRegion::tilesSize;
				const int regionTileX1=regionTileX0+MapRegion::tilesSize;

				const unsigned imageX0=std::max(0, (int)floor((regionTileX0-mapTileX)/xScale));
				const unsigned imageX1=std::min(imageWidth, (int)floor((regionTileX1-mapTileX)/xScale));
				const unsigned imageY0=std::max(0, (int)floor((regionTileY0-mapTileY)/yScale));
				const unsigned imageY1=std::min(imageHeight, (int)floor((regionTileY1-mapTileY)/yScale));

				bool regionExists=true; // until shown false later
				unsigned imageX, imageY;
				for(imageY=imageY0; imageY<imageY1; ++imageY) {
					for(imageX=imageX0; imageX<imageX1; ++imageX) {
						// Find tile represented by this pixel.
						CoordComponent imageTileX=imageX*xScale+mapTileX;
						CoordComponent imageTileY=imageY*yScale+mapTileY;

						CoordVec vec={imageTileX*Physics::CoordsPerTile, imageTileY*Physics::CoordsPerTile};
						const MapTile *tile=NULL;
						if (regionExists) {
							tile=map->getTileAtCoordVec(vec, Engine::Map::Map::GetTileFlag::None);
							if (tile==NULL) {
								// If this tile does not exist, it must be because the whole region doesn't, so skip even trying to read the rest of the tiles within it.
								regionExists=false;
							}
						}

						// Choose colour (based on topmost layer with a texture set).
						uint8_t r=0, g=0, b=0, a=0;
						if (tile!=NULL)
							getColourForTile(map, imageTileX, imageTileY, tile, layer, &r, &g, &b, &a);

						// Write pixel.
						pngRows[(imageY*imageWidth+imageX)*4+0]=r;
						pngRows[(imageY*imageWidth+imageX)*4+1]=g;
						pngRows[(imageY*imageWidth+imageX)*4+2]=b;
						pngRows[(imageY*imageWidth+imageX)*4+3]=a;
					}

					// Update progress.
					if (!quiet) {
						Util::clearConsoleLine();
						double imageYDelta=imageY1-imageY0;
						double regionXDelta=regionX1-regionX0;
						double regionYDelta=regionY1-regionY0;
						double progress=100.0*((imageY-imageY0)+imageYDelta*((regionX-regionX0)+regionXDelta*(regionY-regionY0)))/(regionYDelta*regionXDelta*imageYDelta);
						printf("Creating rows... %.1f%%", progress);
						fflush(stdout);
					}
				}
			}
		}
		if (!quiet)
			printf("\n");

		// Write pixels.
		int pngY;
		for(pngY=0; pngY<imageHeight; ++pngY) {
			png_write_row(pngPtr, &pngRows[(pngY*imageWidth+0)*4*sizeof(png_byte)]);

			if (!quiet) {
				Util::clearConsoleLine();
				double progress=100.0*pngY/imageHeight;
				printf("Writing pixel data... %.1f%%", progress);
				fflush(stdout);
			}
		}
		if (!quiet)
			printf("\n");

		free(pngRows);

		// Finish writing image file.
		png_write_end(pngPtr, NULL);
		fclose(file);
		png_free_data(pngPtr, infoPtr, PNG_FREE_ALL, -1);
		png_destroy_write_struct(&pngPtr, (png_infopp)NULL);

		if (!quiet)
			printf("Done!\n");

		return true;
	}

	void MapPngLib::getColourForTile(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, MapTiled::ImageLayer layer, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) {
		assert(map!=NULL);
		assert(tile!=NULL);
		assert(r!=NULL);
		assert(g!=NULL);
		assert(b!=NULL);

		switch(layer) {
			case MapTiled::ImageLayerBase:
				return MapPngLib::getColourForTileBase(map, mapTileX, mapTileY, tile, r, g, b, a);
			break;
			case MapTiled::ImageLayerTemperature:
				return MapPngLib::getColourForTileTemperature(map, mapTileX, mapTileY, tile, r, g, b, a);
			break;
			case MapTiled::ImageLayerHeight:
				return MapPngLib::getColourForTileHeight(map, mapTileX, mapTileY, tile, r, g, b, a);
			break;
			case MapTiled::ImageLayerMoisture:
				return MapPngLib::getColourForTileMoisture(map, mapTileX, mapTileY, tile, r, g, b, a);
			break;
			case MapTiled::ImageLayerHeightContour:
				return MapPngLib::getColourForTileHeightContour(map, mapTileX, mapTileY, tile, r, g, b, a);
			break;
			case MapTiled::ImageLayerPolitical:
				return MapPngLib::getColourForTilePolitical(map, mapTileX, mapTileY, tile, r, g, b, a);
			break;
			case MapTiled::ImageLayerRegionGrid:
				return MapPngLib::getColourForTileRegionGrid(map, mapTileX, mapTileY, tile, r, g, b, a);
			break;
		}

		assert(false);
		*r=*g=*b=*a=0;
	}

	void MapPngLib::getColourForTileBase(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) {
		*a=255;

		// Loop over layers from top to bottom looking for one with a texture set
		for(int z=MapTile::layersMax-1; z>=0; --z) {
			const MapTile::Layer *layer=tile->getLayer(z);

			// Special case for heatmap textures.
			if (layer->textureId>=MapGen::TextureIdHeatMapMin && layer->textureId<MapGen::TextureIdHeatMapMax) {
				unsigned index=layer->textureId-MapGen::TextureIdHeatMapMin;
				*r=*g=*b=(index*255)/(MapGen::TextureIdHeatMapRange-1);
				break; // We have found something to draw
			}

			switch(layer->textureId) {
				case MapGen::TextureIdNone:
					continue; // Try next layer instead
				break;
				case MapGen::TextureIdGrass0:
				case MapGen::TextureIdGrass1:
				case MapGen::TextureIdGrass2:
				case MapGen::TextureIdGrass3:
				case MapGen::TextureIdGrass4:
				case MapGen::TextureIdGrass5: {
					double height=tile->getHeight();

					height=std::min(height, map->alpineLevel);
					height=std::max(height, map->seaLevel);

					height=(height-map->seaLevel)/(map->alpineLevel-map->seaLevel);

					double min=64, max=255;

					*r=*b=0;
					*g=min+height*(max-min);
				} break;
				case MapGen::TextureIdBrickPath:
				case MapGen::TextureIdDock:
					*r=102,*g=51,*b=0;
				break;
				case MapGen::TextureIdDirt:
					*r=0xCC,*g=0x66,*b=0x00;
				break;
				case MapGen::TextureIdRiver: {
					double moisture=tile->getMoisture();
					moisture=(moisture-map->minMoisture)/(map->maxMoisture-map->minMoisture);

					double min=196, max=255;
					*r=*g=0;
					*b=min+moisture*(max-min);
				} break;
				case MapGen::TextureIdWater:
				case MapGen::TextureIdDeepWater: {
					double height=tile->getHeight();

					height=std::min(height, map->seaLevel);
					height=std::max(height, map->minHeight);

					height=(height-map->minHeight)/(map->seaLevel-map->minHeight);

					double min=64, max=196;

					*r=*g=0;
					*b=min+height*(max-min);
				} break;
				case MapGen::TextureIdTree1:
				case MapGen::TextureIdTree2:
					*r=0,*g=100,*b=0;
				break;
				case MapGen::TextureIdTree3:
					*r=255;
					*g=50;
					*b=0;
				break;
				case MapGen::TextureIdRoseBush:
					*r=0,*g=100,*b=0;
				break;
				case MapGen::TextureIdMan1:
				case MapGen::TextureIdOldManN:
				case MapGen::TextureIdOldManE:
				case MapGen::TextureIdOldManS:
				case MapGen::TextureIdOldManW:
				case MapGen::TextureIdSheepN:
				case MapGen::TextureIdSheepE:
				case MapGen::TextureIdSheepS:
				case MapGen::TextureIdSheepW:
				case MapGen::TextureIdDog:
				case MapGen::TextureIdCoins:
				case MapGen::TextureIdChestClosed:
				case MapGen::TextureIdChestOpen:
					// NPCs, items etc
					*r=255,*g=0,*b=0;
				break;
				case MapGen::TextureIdHouseDoorBL:
				case MapGen::TextureIdHouseDoorBR:
				case MapGen::TextureIdHouseDoorTL:
				case MapGen::TextureIdHouseDoorTR:
				case MapGen::TextureIdHouseRoof:
				case MapGen::TextureIdHouseRoofTop:
				case MapGen::TextureIdHouseWall2:
				case MapGen::TextureIdHouseWall3:
				case MapGen::TextureIdHouseWall4:
				case MapGen::TextureIdHouseChimney:
				case MapGen::TextureIdHouseChimneyTop:
				case MapGen::TextureIdShopCobbler:
					*r=255,*g=128,*b=0;
				break;
				case MapGen::TextureIdSand:
				case MapGen::TextureIdHotSand: {
					double height=tile->getHeight();

					height=std::min(height, map->alpineLevel);
					height=std::max(height, map->seaLevel);

					height=(height-map->seaLevel)/(map->alpineLevel-map->seaLevel);

					double min=0, max=100;
					*r=255;
					*g=(layer->textureId==MapGen::TextureIdHotSand ? 102 : 204);
					*b=min+height*(max-min);
				} break;
				case MapGen::TextureIdSnow: {
					double height=tile->getHeight();

					height=std::min(height, map->maxHeight);
					height=std::max(height, map->seaLevel);

					height=(height-map->seaLevel)/(map->maxHeight-map->seaLevel);

					double min=196, max=255;

					*r=*g=*b=min+height*(max-min);
				} break;
				case MapGen::TextureIdHighAlpine:
				case MapGen::TextureIdLowAlpine: {
					double height=tile->getHeight();

					height=std::min(height, map->maxHeight);
					height=std::max(height, map->alpineLevel);

					height=(height-map->alpineLevel)/(map->maxHeight-map->alpineLevel);

					double min=127, max=255;
					if (layer->textureId==MapGen::TextureIdHighAlpine) {
						min/=2;
						max/=2;
					}

					*r=*g=*b=min+height*(max-min);
				} break;
				case MapGen::TextureIdNB:
					assert(false);
					continue; // Try next layer instead
				break;
			}

			break; // We have found something to draw
		}
	}

	void MapPngLib::getColourForTileTemperature(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) {
		*a=255;

		// Grab temperature and normalise to [0, 1]
		double temperature=tile->getTemperature();
		double temperatureNormalised=(temperature-map->minTemperature)/(map->maxTemperature-map->minTemperature);

		// Scale temperature up to [0, 1023]
		unsigned temperatureScaled=floor(temperatureNormalised*(4*256-1));
		if (temperatureScaled<256) {
			// 0x0000FF -> 0x00FFFF
			*r=0;
			*g=temperatureScaled;
			*b=255;
		} else if (temperatureScaled<512) {
			// 0x00FFFF -> 0x00FF00
			temperatureScaled-=256;
			*r=0;
			*g=255;
			*b=255-temperatureScaled;
		} else if (temperatureScaled<768) {
			// 0x00FF00 -> 0xFFFF00
			temperatureScaled-=512;
			*r=temperatureScaled;
			*g=255;
			*b=0;
		} else {
			// 0xFFFF00 -> 0xFF0000
			temperatureScaled-=768;
			*r=255;
			*g=255-temperatureScaled;
			*b=0;
		}
	}

	void MapPngLib::getColourForTileHeight(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) {
		*a=255;

		// Grab height and check for ocean tile as special case
		double height=tile->getHeight();
		if (height<=map->seaLevel) {
			*r=0;
			*g=0;
			*b=255;
			return;
		}

		// Normalise height (above sea level) to [0, 1]
		double heightNormalised=(height-map->seaLevel)/(map->maxHeight-map->seaLevel);

		// Scale height up to [0, 511]
		unsigned heightScaled=floor(heightNormalised*(2*256-1));
		if (heightScaled<128) {
			// 0x008800 -> 0x888800
			*r=heightScaled;
			*g=127;
			*b=0;
		} else if (heightScaled<256) {
			// 0x888800 -> 0x880000
			heightScaled-=128;
			*r=127;
			*g=127-heightScaled;
			*b=0;
		} else if (heightScaled<384) {
			// 0x880000 -> 0x888888
			heightScaled-=256;
			*r=127;
			*g=heightScaled;
			*b=heightScaled;
		} else {
			// 0x888888 -> 0xFFFFFF
			heightScaled-=384;
			*r=128+heightScaled;
			*g=128+heightScaled;
			*b=128+heightScaled;
		}
	}

	void MapPngLib::getColourForTileMoisture(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) {
		*a=255;

		// Special case for ocean tiles
		if (tile->getHeight()<=map->seaLevel) {
			*r=0;
			*g=0;
			*b=255;
			return;
		}

		// Grab moisture and normalise to [0, 1]
		double moisture=tile->getMoisture();
		double moistureNormalised=(moisture-map->minMoisture)/(map->maxMoisture-map->minMoisture);

		// Choose colour (low moisture as white, high moisture as dark blue)
		unsigned moistureScaled=floor(moistureNormalised*255);
		*r=255-moistureScaled;
		*g=255-moistureScaled;
		*b=255;
	}

	void MapPngLib::getColourForTileHeightContour(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) {
		// Use black pixels for the contour lines themselves, transparent for everything else
		*r=*g=*b=0;
		*a=(tile->getBitsetN(MapGen::TileBitsetIndexContour) ? 255 : 0);
	}

	void MapPngLib::getColourForTilePolitical(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) {
		static const uint8_t distinctColours[64][3]={
			{0, 0, 0},
			{1, 0, 103},
			{213, 255, 0},
			{255, 0, 86},
			{158, 0, 142},
			{14, 76, 161},
			{255, 229, 2},
			{0, 95, 57},
			{0, 255, 0},
			{149, 0, 58},
			{255, 147, 126},
			{164, 36, 0},
			{0, 21, 68},
			{145, 208, 203},
			{98, 14, 0},
			{107, 104, 130},
			{0, 0, 255},
			{0, 125, 181},
			{106, 130, 108},
			{0, 174, 126},
			{194, 140, 159},
			{190, 153, 112},
			{0, 143, 156},
			{95, 173, 78},
			{255, 0, 0},
			{255, 0, 246},
			{255, 2, 157},
			{104, 61, 59},
			{255, 116, 163},
			{150, 138, 232},
			{152, 255, 82},
			{167, 87, 64},
			{1, 255, 254},
			{255, 238, 232},
			{254, 137, 0},
			{189, 198, 255},
			{1, 208, 255},
			{187, 136, 0},
			{117, 68, 177},
			{165, 255, 210},
			{255, 166, 254},
			{119, 77, 0},
			{122, 71, 130},
			{38, 52, 0},
			{0, 71, 84},
			{67, 0, 44},
			{181, 0, 255},
			{255, 177, 103},
			{255, 219, 102},
			{144, 251, 146},
			{126, 45, 210},
			{189, 211, 147},
			{229, 111, 254},
			{222, 255, 116},
			{0, 255, 120},
			{0, 155, 255},
			{0, 100, 1},
			{0, 118, 255},
			{133, 169, 0},
			{0, 185, 23},
			{120, 130, 49},
			{0, 255, 198},
			{255, 110, 65},
			{232, 94, 190},
		};

		// Grab landmass id for this tile
		uint16_t landmassId=tile->getLandmassId();

		// Special case: id=0 implies border tile, so colour black
		if (landmassId==0) {
			*r=*g=*b=0;
			*a=255;
			return;
		}

		// Choose colour from array of distinct colours
		*r=distinctColours[landmassId%64][0];
		*g=distinctColours[landmassId%64][1];
		*b=distinctColours[landmassId%64][2];
		*a=255;
	}

	void MapPngLib::getColourForTileRegionGrid(const class Map *map, int mapTileX, int mapTileY, const MapTile *tile, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) {
		// Use black pixels for the grid lines themselves, transparent for everything else
		*r=*g=*b=0;
		*a=((mapTileX%MapRegion::tilesSize<1 || mapTileY%MapRegion::tilesSize<1) ? 255 : 0);
	}

};
