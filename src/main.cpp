#include <cassert>
#include <cstdbool>
#include <cstdlib>
#include <cstdio>

#include <SDL2/SDL.h>

#include "./engine/graphics/camera.h"
#include "./engine/graphics/renderer.h"
#include "./engine/map/map.h"
#include "./engine/map/mapgen.h"
#include "./engine/map/mapobject.h"

using namespace Engine;
using namespace Engine::Graphics;
using namespace Engine::Map;

int main(int argc, char **argv) {
	// Set various constants/parameters.
	const int maxZoom=8;
	const int defaultZoom=4;
	const int TilesWide=24;
	const int TilesHigh=18;
	const double fps=120.0;

	const int WindowWidth=(TilesWide*Physics::CoordsPerTile*defaultZoom);
	const int WindowHeight=(TilesHigh*Physics::CoordsPerTile*defaultZoom);
	const int fpsDelay=1000.0/fps;

	CoordVec playerDelta(0, 0);
	bool playerRunning=false;

	class Map *map=NULL;
	if (argc<2) {
		printf("No map file given - generating one...\n");

		// Otherwise generate a map.
		MapGen::MapGen gen(1024, 1024);
		map=gen.generate();

		if (map==NULL || !map->initialized) {
			printf("Could not generate a map.\n");
			return 0;
		}
	} else {
		// Find path.
		const char *path=argv[1];

		// Log.
		printf("Loading map at '%s'.\n", path);

		// Load map.
		map=new class Map(path);

		if (map==NULL || !map->initialized) {
			printf("Could not load map.\n");
			return 0;
		}
	}

	// Add player object.
	MapObject objectPlayer(CoordAngle0, CoordVec(205*Physics::CoordsPerTile, 521*Physics::CoordsPerTile), 1, 1);
	HitMask playerHitmask;
	const unsigned playerW=4, playerH=6;
	unsigned playerX, playerY;
	for(playerY=(8-playerH)/2; playerY<(8+playerH)/2; ++playerY)
		for(playerX=(8-playerW)/2; playerX<(8+playerW)/2; ++playerX)
			playerHitmask.setXY(playerX, playerY, true);
	objectPlayer.setHitMaskByTileOffset(0, 0, playerHitmask);
	objectPlayer.tempSetTextureId(13);
	map->addObject(&objectPlayer);

	// Add other objects.
	MapObject *npc1=MapGen::addBuiltinObject(map, MapGen::BuiltinObject::OldBeardMan, CoordAngle0, CoordVec(200*Physics::CoordsPerTile, 523*Physics::CoordsPerTile));
	if (npc1!=NULL)
		npc1->setMovementModeConstantVelocity(CoordVec(2,1)); // east south east

	MapGen::addBuiltinObjectForest(map, MapGen::BuiltinObject::Bush, CoordVec(200*Physics::CoordsPerTile, 535*Physics::CoordsPerTile), CoordVec(80*Physics::CoordsPerTile, 23*Physics::CoordsPerTile), CoordVec(3*Physics::CoordsPerTile, 3*Physics::CoordsPerTile));

	MapGen::addBuiltinObjectForest(map, MapGen::BuiltinObject::Tree2, CoordVec(220*Physics::CoordsPerTile, 547*Physics::CoordsPerTile), CoordVec(40*Physics::CoordsPerTile, 12*Physics::CoordsPerTile), CoordVec(6*Physics::CoordsPerTile, 6*Physics::CoordsPerTile));

	MapGen::addBuiltinObjectForest(map, MapGen::BuiltinObject::Tree1, CoordVec(210*Physics::CoordsPerTile, 537*Physics::CoordsPerTile), CoordVec(60*Physics::CoordsPerTile, 18*Physics::CoordsPerTile), CoordVec(3*Physics::CoordsPerTile, 3*Physics::CoordsPerTile));

	// Create renderer.
	Renderer renderer(WindowWidth, WindowHeight);

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
		renderer.refresh(&camera, map);

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
					}
				break;
			}

		const int moveSpeed=(playerRunning ? Physics::CoordsPerTile/2 : 1);
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
		if (delay>0)
			printf("Main: tick (player position (%i,%i)). render took %u.%03us (delaying for %u.%03us)\n", objectPlayer.getCoordTopLeft().x, objectPlayer.getCoordTopLeft().y, deltaTime/1000, deltaTime%1000, delay/1000, delay%1000);
		else
			printf("Main: tick (player position (%i,%i)). render took %u.%03us (no delay)\n", objectPlayer.getCoordTopLeft().x, objectPlayer.getCoordTopLeft().y, deltaTime/1000, deltaTime%1000);
	}

	return EXIT_SUCCESS;
}
