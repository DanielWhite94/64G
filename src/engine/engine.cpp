#include <cassert>
#include <cstdbool>
#include <cstdlib>
#include <cstdio>

#include <SDL2/SDL.h>

#include "./graphics/camera.h"
#include "./graphics/renderer.h"
#include "./map/map.h"
#include "./map/mapgen.h"
#include "./map/mapobject.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Map;

int main(int argc, char **argv) {
	// Parse arguments.
	if (argc!=4 && argc!=5) {
		printf("Usage: %s mapfile starttilex starttiley [--debug]\n", argv[0]);
		return EXIT_FAILURE;
	}

	const char *path=argv[1];
	int startTileX=atoi(argv[2]);
	int startTileY=atoi(argv[3]);
	bool debug=(argc==5 && strcmp(argv[4], "--debug")==0);

	// Set various constants/parameters.
	const int maxZoom=8;
	const int defaultZoom=4;
	const int TilesWide=24;
	const int TilesHigh=18;
	const double fps=30.0;

	const int windowWidth=(TilesWide*Physics::CoordsPerTile*defaultZoom);
	const int windowHeight=(TilesHigh*Physics::CoordsPerTile*defaultZoom);
	const int fpsDelay=1000.0/fps;

	CoordVec playerDelta(0, 0);
	bool playerRunning=false;

	// Load map.
	printf("Loading map at '%s'.\n", path);
	class Map *map=new class Map(path);
	if (map==NULL || !map->initialized) {
		printf("Could not load map.\n");
		return EXIT_FAILURE;
	}

	// Add player object.
	MapObject objectPlayer(CoordAngle0, CoordVec(startTileX*Physics::CoordsPerTile, startTileY*Physics::CoordsPerTile), 1, 1);
	HitMask playerHitmask;
	const unsigned playerW=4, playerH=6;
	unsigned playerX, playerY;
	for(playerY=(8-playerH)/2; playerY<(8+playerH)/2; ++playerY)
		for(playerX=(8-playerW)/2; playerX<(8+playerW)/2; ++playerX)
			playerHitmask.setXY(playerX, playerY, true);
	objectPlayer.setHitMaskByTileOffset(0, 0, playerHitmask);
	objectPlayer.setTextureIdForAngle(CoordAngle0, MapGen::TextureIdOldManS);
	objectPlayer.setTextureIdForAngle(CoordAngle90, MapGen::TextureIdOldManW);
	objectPlayer.setTextureIdForAngle(CoordAngle180, MapGen::TextureIdOldManN);
	objectPlayer.setTextureIdForAngle(CoordAngle270, MapGen::TextureIdOldManE);

	objectPlayer.inventoryEmpty(24);
	MapObjectItem coins={.type=mapObjectItemTypeCoins, .count=1035};
	objectPlayer.inventoryAddItem(coins);
	MapObjectItem chest={.type=mapObjectItemTypeChest, .count=19};
	objectPlayer.inventoryAddItem(chest);

	if (!map->addObject(&objectPlayer)) {
		printf("Could not add player object.\n");
		delete map;
		return EXIT_FAILURE;
	}

	// Create renderer.
	Renderer renderer(windowWidth, windowHeight);
	renderer.drawMinimap=true;
	renderer.drawInventory=true;

	// Create camera variables.
	Camera camera(CoordVec(0,0), defaultZoom);

	// Main loop.
	const unsigned mapTickRate=8;

	bool quit=false;
	unsigned tick=0;
	for(tick=0; !quit; ++tick) {
		// Note tick start time to maintain constant FPS later.
		unsigned startTime=SDL_GetTicks();

		// Redraw screen.
		camera.setVec(objectPlayer.getCoordTopLeft());
		renderer.refresh(&camera, map, &objectPlayer);

		// Check keyboard and events and move camera.
		SDL_Event event;
		while(SDL_PollEvent(&event))
			switch (event.type) {
				case SDL_QUIT:
					quit=true;
				break;
				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
						case SDLK_LEFT:
							playerDelta.x=-1;
						break;
						case SDLK_RIGHT:
							playerDelta.x=1;
						break;
						case SDLK_UP:
							playerDelta.y=-1;
						break;
						case SDLK_DOWN:
							playerDelta.y=1;
						break;
						case SDLK_SPACE:
							playerRunning=true;
						break;
						case SDLK_TAB: {
							int oldZoom=camera.getZoom();
							int newZoom=(oldZoom<=1 ? maxZoom : oldZoom-1);
							camera.setZoom(newZoom);
						} break;
						case SDLK_g:
							if (renderer.drawCoordGrid) {
								renderer.drawTileGrid=false;
								renderer.drawCoordGrid=false;
							} else if (renderer.drawTileGrid)
								renderer.drawCoordGrid=true;
							else
								renderer.drawTileGrid=true;
						break;
						case SDLK_q:
							quit=true;
						break;
					}
				break;
				case SDL_KEYUP:
					switch(event.key.keysym.sym) {
						case SDLK_LEFT:
							if (playerDelta.x<0)
								playerDelta.x=0;
						break;
						case SDLK_RIGHT:
							if (playerDelta.x>0)
								playerDelta.x=0;
						break;
						case SDLK_UP:
							if (playerDelta.y<0)
								playerDelta.y=0;
						break;
						case SDLK_DOWN:
							if (playerDelta.y>0)
								playerDelta.y=0;
						break;
						case SDLK_SPACE:
							playerRunning=false;
						break;
						case SDLK_F11:
							renderer.toggleFullscreen();
						break;
					}
				break;
			}

		const int moveSpeed=(playerRunning ? 2*Physics::CoordsPerTile : 1);
		map->moveObject(&objectPlayer, objectPlayer.getCoordTopLeft()+playerDelta*moveSpeed);

		// Tick map every so often.
		if (tick%mapTickRate==0)
			map->tick();

		// Delay to maintain a constant FPS.
		unsigned endTime=SDL_GetTicks();
		unsigned deltaTime=endTime-startTime;
		int delay=fpsDelay-deltaTime;
		if (delay>0)
			SDL_Delay(delay);

		// Debugging.
		if (debug) {
			if (delay>0)
				printf("Main: tick (player position (%i,%i)). render took %u.%03us (delaying for %u.%03us)\n", objectPlayer.getCoordTopLeft().x, objectPlayer.getCoordTopLeft().y, deltaTime/1000, deltaTime%1000, delay/1000, delay%1000);
			else
				printf("Main: tick (player position (%i,%i)). render took %u.%03us (no delay)\n", objectPlayer.getCoordTopLeft().x, objectPlayer.getCoordTopLeft().y, deltaTime/1000, deltaTime%1000);
		}
	}

	// Tidy up
	delete map;

	return EXIT_SUCCESS;
}
