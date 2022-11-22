#include <SDL2/SDL.h>

#include "engine.h"

using namespace Engine;

namespace Engine {

Engine::Engine(const char *mapPath, int windowWidth, int windowHeight, int defaultZoom, int maxZoom, int fps, bool debug): maxZoom(maxZoom), fps(fps), debug(debug), stopFlag(false), renderer(windowWidth, windowHeight), camera(CoordVec(0,0), defaultZoom) {
	// Load map.
	printf("Loading map at '%s'...\n", mapPath);

	map=new class Map(mapPath, false);

	// Set renderer parameters.
	renderer.drawMinimap=true;
	renderer.drawInventory=true;
}

Engine::~Engine() {
	if (map!=NULL)
		delete map;
}

void Engine::start(void) {
	// Init
	CoordVec playerDelta(0, 0);
	bool playerRunning=false;

	// Main loop.
	const unsigned mapTickRate=8;

	unsigned tick=0;
	for(tick=0; !stopFlag; ++tick) {
		// Note tick start time to maintain constant FPS later.
		unsigned startTime=SDL_GetTicks();

		// Redraw screen.
		if (playerObject!=NULL) {
			camera.setVec(playerObject->getCoordTopLeft());
			renderer.refresh(&camera, map, playerObject);
		}

		// Check keyboard and events and move camera.
		SDL_Event event;
		while(SDL_PollEvent(&event))
			switch (event.type) {
				case SDL_QUIT:
					stop();
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
							stop();
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

		if (playerObject!=NULL) {
			const int moveSpeed=(playerRunning ? 2*Physics::CoordsPerTile : 1);
			map->moveObject(playerObject, playerObject->getCoordTopLeft()+playerDelta*moveSpeed);
		}

		// Tick map every so often.
		if (tick%mapTickRate==0)
			map->tick();

		// Delay to maintain a constant FPS.
		unsigned endTime=SDL_GetTicks();
		unsigned deltaTime=endTime-startTime;
		int delay=1000/fps-deltaTime;
		if (delay>0)
			SDL_Delay(delay);

		// Debugging.
		if (debug) {
			if (playerObject!=NULL) {
				if (delay>0)
					printf("Main: tick (player position (%i,%i)). render took %u.%03us (delaying for %u.%03us)\n", playerObject->getCoordTopLeft().x, playerObject->getCoordTopLeft().y, deltaTime/1000, deltaTime%1000, delay/1000, delay%1000);
				else
					printf("Main: tick (player position (%i,%i)). render took %u.%03us (no delay)\n", playerObject->getCoordTopLeft().x, playerObject->getCoordTopLeft().y, deltaTime/1000, deltaTime%1000);
			} else {
				if (delay>0)
					printf("Main: tick (player position (NULL). render took %u.%03us (delaying for %u.%03us)\n", deltaTime/1000, deltaTime%1000, delay/1000, delay%1000);
				else
					printf("Main: tick (player position (NULL). render took %u.%03us (no delay)\n", deltaTime/1000, deltaTime%1000);
			}
		}
	}
}

void Engine::stop(void) {
	stopFlag=true;
}

class Map *Engine::getMap(void) {
	return map;
}

MapObject *Engine::getPlayerObject(void) {
	return playerObject;
}

void Engine::setPlayerObject(MapObject *object) {
	playerObject=object;
}

};
