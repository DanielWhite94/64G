#include <cassert>
#include <cstdbool>
#include <cstdlib>
#include <cstdio>
#include <png.h>

#include "../engine/map/map.h"
#include "../engine/map/mapgen.h"
#include "../engine/map/mapobject.h"

using namespace Engine;
using namespace Engine::Map;

int main(int argc, char **argv) {
	// Grab arguments.
	if (argc!=9) {
		printf("Usage: %s mappath mapx mapy mapw maph imagewidth imageheight imagepath", argv[0]);
		return EXIT_FAILURE;
	}

	int arg=1;
	const char *mapPath=argv[arg++];
	int mapTileX=atoi(argv[arg++]);
	int mapTileY=atoi(argv[arg++]);
	int mapTileWidth=atoi(argv[arg++]);
	int mapTileHeight=atoi(argv[arg++]);
	int imageWidth=atoi(argv[arg++]);
	int imageHeight=atoi(argv[arg++]);
	const char *imagePath=argv[arg++];

	if (mapTileX<0 || mapTileX>=Engine::Map::Map::regionsWide*MapRegion::tilesWide) {
		printf("Bad mapX (%i)", mapTileX);
		return EXIT_FAILURE;
	}

	if (mapTileY<0 || mapTileY>=Engine::Map::Map::regionsHigh*MapRegion::tilesHigh) {
		printf("Bad mapY (%i)", mapTileY);
		return EXIT_FAILURE;
	}

	if (mapTileWidth<=0 || mapTileHeight<=0) {
		printf("Bad map width or height (%i and %i)", mapTileWidth, mapTileHeight);
		return EXIT_FAILURE;
	}

	if (imageWidth<=0 || imageHeight<=0) {
		printf("Bad image width or height (%i and %i)", imageWidth, imageHeight);
		return EXIT_FAILURE;
	}

	// Load map.
	printf("Loading map at '%s'.\n", mapPath);
	class Map *map=new class Map(mapPath);
	if (map==NULL || !map->initialized) {
		printf("Could not load map.\n");
		return EXIT_FAILURE;
	}

	// Create file for writing.
	printf("Creating image at '%s'.\n", imagePath);
	FILE *file=fopen(imagePath, "wb"); // TODO: Check return.

	// Setup PNG image.
	png_structp pngPtr=png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop infoPtr=png_create_info_struct(pngPtr);
	png_init_io(pngPtr, file);
	png_set_IHDR(pngPtr, infoPtr, imageWidth, imageHeight, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(pngPtr, infoPtr);

	// Write pixels.
	printf("Writing pixel data...\n");

	double xScale=((double)mapTileWidth)/((double)imageWidth);
	double yScale=((double)mapTileHeight)/((double)imageHeight);

	unsigned x, y;
	png_bytep pngRow=(png_bytep)malloc(3*imageWidth*sizeof(png_byte));
	for(y=0;y<imageHeight;++y) {
		for(x=0;x<imageWidth;++x) {
			// Find tile represented by this pixel.
			// TODO: It would be better to scale properly (i.e consider tiles in this neighbourhood).
			CoordComponent tileX=x*xScale+mapTileX;
			CoordComponent tileY=y*yScale+mapTileY;

			CoordVec vec={tileX*Physics::CoordsPerTile, tileY*Physics::CoordsPerTile};
			const MapTile *tile=map->getTileAtCoordVec(vec);
			assert(tile!=NULL); // TODO: Handle better

			// Choose colour (based on topmost layer with a texture set).
			uint8_t r=0, g=0, b=0;

			for(int z=MapTile::layersMax-1; z>=0; --z) {
				const MapTileLayer *layer=tile->getLayer(0); //
				switch(layer->textureId) {
					case MapGen::TextureIdNone:
						continue; // Try next layer instead
					break;
					case MapGen::TextureIdGrass0:
						r=0,g=255,b=0;
					break;
					case MapGen::TextureIdGrass1:
					case MapGen::TextureIdGrass2:
					case MapGen::TextureIdGrass3:
					case MapGen::TextureIdGrass4:
					case MapGen::TextureIdGrass5:
						r=0,g=200,b=0;
					break;
					case MapGen::TextureIdBrickPath:
					case MapGen::TextureIdDirt:
					case MapGen::TextureIdDock:
						r=0xCC,g=0x66,b=0x00;
					break;
					case MapGen::TextureIdWater:
						r=0,g=0,b=255;
					break;
					case MapGen::TextureIdTree1:
					case MapGen::TextureIdTree2:
						r=0,g=100,b=0;
					break;
					case MapGen::TextureIdMan1:
					case MapGen::TextureIdOldManN:
					case MapGen::TextureIdOldManE:
					case MapGen::TextureIdOldManS:
					case MapGen::TextureIdOldManW:
						r=255,g=0,b=0;
					break;
					case MapGen::TextureIdNB:
						assert(false);
						continue; // Try next layer instead
					break;
				}

				break; // We have found something to draw
			}

			// Write pixel.
			pngRow[x*3+0]=r;
			pngRow[x*3+1]=g;
			pngRow[x*3+2]=b;
		}
		png_write_row(pngPtr, pngRow);
	}

	free(pngRow);

	// Finish writing image file.
	png_write_end(pngPtr, NULL);
	fclose(file);
	png_free_data(pngPtr, infoPtr, PNG_FREE_ALL, -1);
	png_destroy_write_struct(&pngPtr, (png_infopp)NULL);

	printf("Done!\n");

	// Tidy up.
	delete map;

	return EXIT_SUCCESS;
}
