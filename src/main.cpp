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

#define Zoom 4
#define TilesWide 24
#define TilesHigh 18
#define WindowWidth (TilesWide*Physics::CoordsPerTile*Zoom)
#define WindowHeight (TilesHigh*Physics::CoordsPerTile*Zoom)

int main(int argc, char **argv) {
	// Create map.
	MapGen::MapGen gen(1024, 1024);
	class Map *map=gen.generate();
	assert(map!=NULL);

	// Add objects.
	MapObject objectPlayer(CoordAngle0, CoordVec(205*Physics::CoordsPerTile, 521*Physics::CoordsPerTile), 1, 1);
	HitMask playerHitmask;
	const unsigned playerW=4, playerH=6;
	unsigned playerX, playerY;
	for(playerY=(8-playerH)/2; playerY<(8+playerH)/2; ++playerY)
		for(playerX=(8-playerW)/2; playerX<(8+playerW)/2; ++playerX)
			playerHitmask.setXY(playerX, playerY, true);
	objectPlayer.setHitMaskByTileOffset(0, 0, playerHitmask);
	map->addObject(&objectPlayer);
	CoordVec playerDelta(0, 0);

	MapObject objectNpc(CoordAngle0, CoordVec(200*Physics::CoordsPerTile, 523*Physics::CoordsPerTile), 1, 1);
	HitMask npcHitmask;
	const unsigned npcW=4, npcH=5;
	unsigned npcX, npcY;
	for(npcY=(8-npcH)/2; npcY<(8+npcH)/2; ++npcY)
		for(npcX=(8-npcW)/2; npcX<(8+npcW)/2; ++npcX)
			npcHitmask.setXY(npcX, npcY, true);
	objectNpc.setHitMaskByTileOffset(0, 0, npcHitmask);
	map->addObject(&objectNpc);

	// Create renderer.
	Renderer renderer(WindowWidth, WindowHeight);
	renderer.drawTileGrid=true;
	//renderer.drawCoordGrid=true;
	renderer.drawHitMasks=true;

	// Create camera variables.
	Camera camera(CoordVec(0,0), Zoom);

	// Main loop.
	bool quit=false;
	unsigned tick=0;
	for(tick=0; !quit; ++tick) {
		// Debugging.
		printf("Main: tick (player position (%i,%i))\n", objectPlayer.getCoordTopLeft().x, objectPlayer.getCoordTopLeft().y);

		// Redraw screen.
		camera.setVec(objectPlayer.getCoordTopLeft());
		renderer.refresh(&camera, map);

		// Check keyboard and events and move camera.
		const int moveSpeed=Physics::CoordsPerTile/2;
		SDL_Event event;
		while(SDL_PollEvent(&event))
			switch (event.type) {
				case SDL_QUIT:
					quit=true;
				break;
				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
						case SDLK_LEFT:
							playerDelta.x=-moveSpeed;
						break;
						case SDLK_RIGHT:
							playerDelta.x=moveSpeed;
						break;
						case SDLK_UP:
							playerDelta.y=-moveSpeed;
						break;
						case SDLK_DOWN:
							playerDelta.y=moveSpeed;
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
					}
				break;
			}

		map->moveObject(&objectPlayer, objectPlayer.getCoordTopLeft()+playerDelta);

		// Delay
		// TODO: Constant FPS to avoid character speed changes.
		SDL_Delay(1000/32);
	}

	return EXIT_SUCCESS;
}
