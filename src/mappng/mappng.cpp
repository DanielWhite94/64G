#include <cassert>
#include <cmath>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <png.h>

#include "../engine/map/map.h"
#include "../engine/map/mapgen.h"
#include "../engine/map/mapobject.h"
#include "../engine/util.h"

using namespace Engine;
using namespace Engine::Map;

int main(int argc, char **argv) {
	// Grab arguments.
	if (argc!=9 && argc!=10) {
		printf("Usage: %s [--quiet] mappath mapx mapy mapw maph imagewidth imageheight imagepath\n", argv[0]);
		return EXIT_FAILURE;
	}

	int arg=1;

	bool quiet=false;
	if (strcmp(argv[arg], "--quiet")==0) {
		quiet=true;
		++arg;
	}

	const char *mapPath=argv[arg++];
	int mapTileX=atoi(argv[arg++]);
	int mapTileY=atoi(argv[arg++]);
	int mapTileWidth=atoi(argv[arg++]);
	int mapTileHeight=atoi(argv[arg++]);
	int imageWidth=atoi(argv[arg++]);
	int imageHeight=atoi(argv[arg++]);
	const char *imagePath=argv[arg++];

	if (mapTileX<0 || mapTileX>=Engine::Map::Map::regionsWide*MapRegion::tilesWide) {
		if (!quiet)
			printf("Bad mapX (%i)", mapTileX);
		return EXIT_FAILURE;
	}

	if (mapTileY<0 || mapTileY>=Engine::Map::Map::regionsHigh*MapRegion::tilesHigh) {
		if (!quiet)
			printf("Bad mapY (%i)", mapTileY);
		return EXIT_FAILURE;
	}

	if (mapTileWidth<=0 || mapTileHeight<=0) {
		if (!quiet)
			printf("Bad map width or height (%i and %i)", mapTileWidth, mapTileHeight);
		return EXIT_FAILURE;
	}

	if (imageWidth<=0 || imageHeight<=0) {
		if (!quiet)
			printf("Bad image width or height (%i and %i)", imageWidth, imageHeight);
		return EXIT_FAILURE;
	}

	// Load map.
	if (!quiet)
		printf("Loading map at '%s'.\n", mapPath);
	class Map *map=new class Map(mapPath);
	if (map==NULL || !map->initialized) {
		if (!quiet)
			printf("Could not load map.\n");
		return EXIT_FAILURE;
	}

	// Create file for writing.
	if (!quiet)
		printf("Creating image at '%s'.\n", imagePath);
	FILE *file=fopen(imagePath, "wb"); // TODO: Check return.

	// Setup PNG image.
	png_structp pngPtr=png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_init_io(pngPtr, file);

	png_infop infoPtr=png_create_info_struct(pngPtr);
	png_set_IHDR(pngPtr, infoPtr, imageWidth, imageHeight, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(pngPtr, infoPtr);

	png_set_compression_level(pngPtr, Z_NO_COMPRESSION);

	// Create rows.
	png_bytep pngRows=(png_bytep)malloc(imageHeight*imageWidth*3*sizeof(png_byte));

	double xScale=((double)mapTileWidth)/((double)imageWidth);
	double yScale=((double)mapTileHeight)/((double)imageHeight);

	const unsigned tileX0=mapTileX;
	const unsigned tileY0=mapTileY;
	const unsigned tileX1=(imageWidth-1)*xScale+mapTileX;
	const unsigned tileY1=(imageHeight-1)*yScale+mapTileY;
	const unsigned regionX0=tileX0/MapRegion::tilesWide;
	const unsigned regionY0=tileY0/MapRegion::tilesHigh;
	const unsigned regionX1=tileX1/MapRegion::tilesWide+1;
	const unsigned regionY1=tileY1/MapRegion::tilesHigh+1;

	unsigned regionX, regionY;
	for(regionY=regionY0; regionY<regionY1; ++regionY) {
		const unsigned regionTileY0=regionY*MapRegion::tilesHigh;
		const unsigned regionTileY1=regionTileY0+MapRegion::tilesHigh;
		for(regionX=regionX0; regionX<regionX1; ++regionX) {
			const unsigned regionTileX0=regionX*MapRegion::tilesWide;
			const unsigned regionTileX1=regionTileX0+MapRegion::tilesWide;

			const unsigned imageX0=std::max((unsigned)0, (unsigned)floor((regionTileX0-mapTileX)/xScale));
			const unsigned imageX1=std::min((unsigned)imageWidth, (unsigned)floor((regionTileX1-mapTileX)/xScale));
			const unsigned imageY0=std::max((unsigned)0, (unsigned)floor((regionTileY0-mapTileY)/yScale));
			const unsigned imageY1=std::min((unsigned)imageHeight, (unsigned)floor((regionTileY1-mapTileY)/yScale));

			unsigned imageX, imageY;
			for(imageY=imageY0; imageY<imageY1; ++imageY) {
				for(imageX=imageX0; imageX<imageX1; ++imageX) {
					// Find tile represented by this pixel.
					// TODO: It would be better to scale properly (i.e consider tiles in this neighbourhood).
					CoordComponent imageTileX=imageX*xScale+mapTileX;
					CoordComponent imageTileY=imageY*yScale+mapTileY;

					CoordVec vec={imageTileX*Physics::CoordsPerTile, imageTileY*Physics::CoordsPerTile};
					const MapTile *tile=map->getTileAtCoordVec(vec, Engine::Map::Map::GetTileFlag::None);
					if (tile==NULL)
						continue; // TODO: Handle better

					// Choose colour (based on topmost layer with a texture set).
					uint8_t r=0, g=0, b=0;

					const double seaLevelFactor=0.05;
					const double alpineLevelFactor=0.7;

					// Standard case.
					for(int z=MapTile::layersMax-1; z>=0; --z) {
						const MapTile::Layer *layer=tile->getLayer(z);

						// Special case for heatmap textures.
						if (layer->textureId>=MapGen::TextureIdHeatMapMin && layer->textureId<MapGen::TextureIdHeatMapMax) {
							unsigned index=layer->textureId-MapGen::TextureIdHeatMapMin;
							r=g=b=(index*255)/(MapGen::TextureIdHeatMapRange-1);
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

								height=std::min(height, map->maxHeight*alpineLevelFactor);
								height=std::max(height, seaLevelFactor*map->maxHeight*alpineLevelFactor);

								height=(height-seaLevelFactor*map->maxHeight)/(map->maxHeight*alpineLevelFactor-seaLevelFactor*map->maxHeight);

								double min=64, max=255;

								r=b=0;
								g=min+height*(max-min);
							} break;
							case MapGen::TextureIdBrickPath:
							case MapGen::TextureIdDock:
								r=102,g=51,b=0;
							break;
							case MapGen::TextureIdDirt:
								r=0xCC,g=0x66,b=0x00;
							break;
							case MapGen::TextureIdRiver: {
								double moisture=tile->getMoisture();
								moisture=(moisture-map->minMoisture)/(map->maxMoisture-map->minMoisture);

								double min=196, max=255;
								r=g=0;
								b=min+moisture*(max-min);
							} break;
							case MapGen::TextureIdWater:
							case MapGen::TextureIdDeepWater: {
								double height=tile->getHeight();

								height=std::min(height, seaLevelFactor*map->maxHeight);
								height=std::max(height, map->minHeight);

								height=(height-map->minHeight)/(seaLevelFactor*map->maxHeight-map->minHeight);

								double min=64, max=196;

								r=g=0;
								b=min+height*(max-min);
							} break;
							case MapGen::TextureIdTree1:
							case MapGen::TextureIdTree2:
							case MapGen::TextureIdTree3:
								r=0,g=100,b=0;
							break;
							case MapGen::TextureIdMan1:
							case MapGen::TextureIdOldManN:
							case MapGen::TextureIdOldManE:
							case MapGen::TextureIdOldManS:
							case MapGen::TextureIdOldManW:
							case MapGen::TextureIdSheep:
								// NPCs etc
								r=255,g=0,b=0;
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
								r=255,g=128,b=0;
							break;
							case MapGen::TextureIdSand:
							case MapGen::TextureIdHotSand: {
								double height=tile->getHeight();

								height=std::min(height, map->maxHeight*alpineLevelFactor);
								height=std::max(height, seaLevelFactor*map->maxHeight);

								height=(height-seaLevelFactor*map->maxHeight)/(map->maxHeight*alpineLevelFactor-seaLevelFactor*map->maxHeight);

								double min=0, max=100;
								r=255;
								g=(layer->textureId==MapGen::TextureIdHotSand ? 102 : 204);
								b=min+height*(max-min);
							} break;
							case MapGen::TextureIdSnow: {
								double height=tile->getHeight();

								height=std::min(height, map->maxHeight);
								height=std::max(height, seaLevelFactor*map->maxHeight);

								height=(height-seaLevelFactor*map->maxHeight)/(map->maxHeight-seaLevelFactor*map->maxHeight);

								double min=196, max=255;

								r=g=b=min+height*(max-min);
							} break;
							case MapGen::TextureIdHighAlpine:
							case MapGen::TextureIdLowAlpine: {
								double height=tile->getHeight();

								height=std::min(height, map->maxHeight);
								height=std::max(height, alpineLevelFactor*map->maxHeight);

								height=(height-alpineLevelFactor*map->maxHeight)/(map->maxHeight-alpineLevelFactor*map->maxHeight);

								double min=127, max=255;
								if (layer->textureId==MapGen::TextureIdHighAlpine) {
									min/=2;
									max/=2;
								}

								r=g=b=min+height*(max-min);
							} break;
							case MapGen::TextureIdNB:
								assert(false);
								continue; // Try next layer instead
							break;
						}

						break; // We have found something to draw
					}

					// Write pixel.
					pngRows[(imageY*imageWidth+imageX)*3+0]=r;
					pngRows[(imageY*imageWidth+imageX)*3+1]=g;
					pngRows[(imageY*imageWidth+imageX)*3+2]=b;
				}
			}

			// Update progress.
			if (!quiet)
				Util::clearConsoleLine();
			double progress=100.0*((regionX-regionX0)+(regionY-regionY0)*(regionX1-regionX0))/((regionY1-regionY0)*(regionX1-regionX0));
			if (!quiet)
				printf("Creating rows... %.1f%%", progress);
			fflush(stdout);
		}
	}
	if (!quiet)
		printf("\n");

	// Write pixels.
	int pngY;
	for(pngY=0; pngY<imageHeight; ++pngY) {
		png_write_row(pngPtr, &pngRows[(pngY*imageWidth+0)*3*sizeof(png_byte)]);

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

	// Tidy up.
	delete map;

	return EXIT_SUCCESS;
}
