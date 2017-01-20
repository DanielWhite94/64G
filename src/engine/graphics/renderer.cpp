#include <cassert>

#include "renderer.h"
#include "../util.h"

using namespace Engine;
using namespace Engine::Graphics;

namespace Engine {
	namespace Graphics {
		Renderer::Renderer(unsigned gWindowWidth, unsigned gWindowHeight) {
			// Init SDL.
			SDL_Init(SDL_INIT_VIDEO);
			IMG_Init(IMG_INIT_PNG);

			// Copy width and height.
			windowWidth=gWindowWidth;
			windowHeight=gWindowHeight;

			// Create window.
			window=SDL_CreateWindow("title", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, 0); // TODO: Check return and set a title.

			// Create renderer.
			renderer=SDL_CreateRenderer(window, -1, 0); // TODO: Check return.
		}

		Renderer::~Renderer() {
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);

			IMG_Quit();
			SDL_Quit();
		}

		void Renderer::refresh(const Camera *gCamera, const Map *gMap) {
			assert(gCamera!=NULL);
			assert(gMap!=NULL);

			// Update fields.
			camera=gCamera;
			map=gMap;

			// TODO: improve using CoordVec functions/overloads
			topLeft.x=Util::floordiv(camera->screenXOffsetToCoordX(-windowWidth/2), CoordsPerTile)*CoordsPerTile;
			topLeft.y=Util::floordiv(camera->screenYOffsetToCoordY(-windowHeight/2), CoordsPerTile)*CoordsPerTile;
			bottomRight.x=topLeft.x+TilesWide*CoordsPerTile;
			bottomRight.y=topLeft.y+TilesHigh*CoordsPerTile;

			// Clear screen (to pink to highlight any areas not being drawn).
			SDL_SetRenderDrawColor(renderer, 255, 0, 255, SDL_ALPHA_OPAQUE);
			SDL_RenderClear(renderer);

			// Draw tiles.
			CoordVec vec;
			int z;
			int delta=camera->coordLengthToScreenLength(CoordsPerTile);
			int sy=camera->coordYToScreenYOffset(topLeft.y)+windowHeight/2;
			for(vec.y=topLeft.y; vec.y<=bottomRight.y; vec.y+=CoordsPerTile,sy+=delta) {
				int sx=camera->coordXToScreenXOffset(topLeft.x)+windowWidth/2;
				for(vec.x=topLeft.x; vec.x<=bottomRight.x; vec.x+=CoordsPerTile,sx+=delta) {
					for(z=0; z<MapTile::layersMax; ++z) {
						// Find layer for this (x,y,z).
						const MapTile *tile=map->getTileAtCoordVec(vec);
						if (tile==NULL)
							continue;
						const MapTileLayer *layer=tile->getLayer(z);
						assert(layer!=NULL);

						// Draw layer.
						SDL_Rect rect={.x=sx, .y=sy, .w=delta, .h=delta};
						switch(layer->textureId) {
							case 0:
								// Empty layer.
							break;
							case 1:
								// Wall - black.
								SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
								SDL_RenderFillRect(renderer, &rect);
							break;
							case 2:
								// Floor - green.
								SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
								SDL_RenderFillRect(renderer, &rect);
							break;
						}
					}
				}
			}

			// Draw grid (if needed).
			if (drawGrid)
				renderGrid();

			// Update screen.
			SDL_RenderPresent(renderer);
		}

		void Renderer::renderGrid(void) {
			SDL_SetRenderDrawColor(renderer, 128, 128, 128, SDL_ALPHA_OPAQUE);

			int delta=camera->coordLengthToScreenLength(CoordsPerTile);

			// Draw vertical lines.
			int x;
			int sx=camera->coordXToScreenXOffset(topLeft.x)+windowWidth/2;
			for(x=topLeft.x; x<=bottomRight.x; x+=CoordsPerTile,sx+=delta)
				SDL_RenderDrawLine(renderer, sx, 0, sx, windowHeight);

			// Draw horizontal lines.
			int y;
			int sy=camera->coordYToScreenYOffset(topLeft.y)+windowHeight/2;
			for(y=topLeft.y; y<=bottomRight.y; y+=CoordsPerTile,sy+=delta)
				SDL_RenderDrawLine(renderer, 0, sy, windowWidth, sy);
		}
	};
};
