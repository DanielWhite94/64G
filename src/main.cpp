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
	map->addObject(&objectPlayer);

	MapObject objectNpc(CoordAngle0, CoordVec(200*Physics::CoordsPerTile, 523*Physics::CoordsPerTile), 2, 2);
	map->addObject(&objectNpc);

	// Create renderer.
	Renderer renderer(WindowWidth, WindowHeight);
	renderer.drawTileGrid=true;
	//renderer.drawCoordGrid=true;
	renderer.drawHitMasks=true;

	// Create camera variables.
	CoordVec cameraPos(205*Physics::CoordsPerTile, 523*Physics::CoordsPerTile);
	CoordVec cameraDelta(0, 0);

	// Main loop.
	bool quit=false;
	while(!quit) {
		// Debugging.
		printf("Main: tick (cameraPos (%i,%i))\n", cameraPos.x, cameraPos.y);

		// Redraw screen.
		Camera camera(cameraPos, Zoom); // TODO: update rather than create/destroy each time
		renderer.refresh(&camera, map);

		// Check keyboard and events and move camera.
		const int moveSpeed=3*Physics::CoordsPerTile+3; // More realistic character running speed is Physics::CoordsPerTile/2.
		SDL_Event event;
		while(SDL_PollEvent(&event))
			switch (event.type) {
				case SDL_QUIT:
					quit=true;
				break;
				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
						case SDLK_LEFT:
							cameraDelta.x=-moveSpeed;
						break;
						case SDLK_RIGHT:
							cameraDelta.x=moveSpeed;
						break;
						case SDLK_UP:
							cameraDelta.y=-moveSpeed;
						break;
						case SDLK_DOWN:
							cameraDelta.y=moveSpeed;
						break;
					}
				break;
				case SDL_KEYUP:
					switch(event.key.keysym.sym) {
						case SDLK_LEFT:
							if (cameraDelta.x<0)
								cameraDelta.x=0;
						break;
						case SDLK_RIGHT:
							if (cameraDelta.x>0)
								cameraDelta.x=0;
						break;
						case SDLK_UP:
							if (cameraDelta.y<0)
								cameraDelta.y=0;
						break;
						case SDLK_DOWN:
							if (cameraDelta.y>0)
								cameraDelta.y=0;
						break;
					}
				break;
			}
		cameraPos+=cameraDelta;

		// Delay
		// TODO: Constant FPS to avoid character speed changes.
		SDL_Delay(1000/16);
	}

	return EXIT_SUCCESS;
}
