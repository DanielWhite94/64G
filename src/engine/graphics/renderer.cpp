#include <cassert>

#include "renderer.h"
#include "../util.h"

using namespace Engine;
using namespace Engine::Graphics;

namespace Engine {
	namespace Graphics {
		Renderer::Renderer(unsigned gWindowWidth, unsigned gWindowHeight) {
			// Set parameters.
			drawTileGrid=false;
			drawCoordGrid=false;
			drawHitMasksActive=false;
			drawHitMasksInactive=false;
			drawHitMasksIntersections=false;

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

			// Load textures.
			const char *paths[TextureIdNB]={
				[TextureIdGrass0]="./images/tiles/grass0.png",
				[TextureIdGrass1]="./images/tiles/grass1.png",
				[TextureIdGrass2]="./images/tiles/grass2.png",
				[TextureIdGrass3]="./images/tiles/grass3.png",
				[TextureIdGrass4]="./images/tiles/grass4.png",
				[TextureIdGrass5]="./images/tiles/grass5.png",
				[TextureIdGrass6]="./images/tiles/grass6.png",
				[TextureIdBrickPath]="./images/tiles/tile.png",
				[TextureIdDirt]="./images/tiles/dirt.png",
				[TextureIdDock]="./images/tiles/dock.png",
				[TextureIdWater]="./images/tiles/water.png",
			};
			unsigned i;
			for(i=1; i<TextureIdNB; ++i)
				textures[i]=new Texture(renderer, paths[i]); // TODO: call delete
		}

		Renderer::~Renderer() {
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);

			IMG_Quit();
			SDL_Quit();
		}

		void Renderer::refresh(const Camera *gCamera, const class Map *gMap) {
			assert(gCamera!=NULL);
			assert(gMap!=NULL);

			// Update fields.
			camera=gCamera;
			map=gMap;

			// TODO: improve using CoordVec functions/overloads
			topLeft.x=Util::floordiv(camera->screenXOffsetToCoordX(-windowWidth/2), CoordsPerTile)*CoordsPerTile;
			topLeft.y=Util::floordiv(camera->screenYOffsetToCoordY(-windowHeight/2), CoordsPerTile)*CoordsPerTile;
			bottomRight.x=Util::floordiv(camera->screenXOffsetToCoordX(+windowWidth/2), CoordsPerTile)*CoordsPerTile;
			bottomRight.y=Util::floordiv(camera->screenYOffsetToCoordY(+windowHeight/2), CoordsPerTile)*CoordsPerTile;

			// Clear screen (to pink to highlight any areas not being drawn).
			SDL_SetRenderDrawColor(renderer, 255, 0, 255, SDL_ALPHA_OPAQUE);
			SDL_RenderClear(renderer);

			// Prepare to draw tiles and objects.
			CoordVec vec;
			int z;
			int delta=camera->coordLengthToScreenLength(CoordsPerTile);
			int sx, sy;
			int sxTopLeft=camera->coordXToScreenXOffset(topLeft.x)+windowWidth/2;
			int syTopLeft=camera->coordYToScreenYOffset(topLeft.y)+windowHeight/2;

			// Draw tiles.
			for(vec.y=topLeft.y,sy=syTopLeft; vec.y<=bottomRight.y; vec.y+=CoordsPerTile,sy+=delta)
				for(vec.x=topLeft.x,sx=sxTopLeft; vec.x<=bottomRight.x; vec.x+=CoordsPerTile,sx+=delta) {
					// Find tile for this (x,y).
					const MapTile *tile=map->getTileAtCoordVec(vec);
					if (tile==NULL)
						continue;

					for(z=0; z<MapTile::layersMax; ++z) {
						// Find layer for this (x,y,z).
						const MapTileLayer *layer=tile->getLayer(z);
						assert(layer!=NULL);

						if (layer->textureId==TextureIdNone)
							continue;

						// Draw layer.
						SDL_Rect rect={.x=sx, .y=sy, .w=delta, .h=delta};
						SDL_RenderCopy(renderer, (SDL_Texture *)textures[layer->textureId]->getTexture(), NULL, &rect);
					}
				}

			// Draw objects.
			for(vec.y=topLeft.y,sy=syTopLeft; vec.y<=bottomRight.y; vec.y+=CoordsPerTile,sy+=delta)
				for(vec.x=topLeft.x,sx=sxTopLeft; vec.x<=bottomRight.x; vec.x+=CoordsPerTile,sx+=delta) {
					// Find tile at this (x,y).
					const MapTile *tile=map->getTileAtCoordVec(vec);
					if (tile==NULL)
						continue;

					// Loop over all objects on this tile.
					HitMask intersectionHitMask, totalHitMask;
					unsigned i, max=tile->getObjectCount();
					for(i=0; i<max; ++i) {
						// Grab object.
						const MapObject *object=tile->getObject(i);
						assert(object!=NULL);

						// Draw textures.
						// TODO: this

						// Draw hitmasks if needed.
						if (drawHitMasksActive || drawHitMasksInactive || drawHitMasksIntersections) {
							HitMask activeHitmask=object->getHitMaskByCoord(vec);

							if (drawHitMasksActive)
								renderHitMask(activeHitmask, sx, sy, 0, 0, 0);
							if (drawHitMasksInactive)
								renderHitMask(~activeHitmask, sx, sy, 255, 255, 255);
							if (drawHitMasksIntersections) {
								intersectionHitMask|=(totalHitMask & activeHitmask);
								totalHitMask|=activeHitmask;
							}
						}
					}

					// Draw hitmask intersections if needed.
					if (drawHitMasksIntersections)
						renderHitMask(intersectionHitMask, sx, sy, 255, 0, 0);
				}

			// Draw grids (if needed).
			if (drawCoordGrid) {
				SDL_SetRenderDrawColor(renderer, 160, 160, 160, SDL_ALPHA_OPAQUE);
				renderGrid(CoordVec(1,1));
			}
			if (drawTileGrid) {
				SDL_SetRenderDrawColor(renderer, 128, 128, 128, SDL_ALPHA_OPAQUE);
				renderGrid(CoordVec(CoordsPerTile,CoordsPerTile));
			}

			// Update screen.
			SDL_RenderPresent(renderer);
		}

		void Renderer::renderGrid(const CoordVec &coordDelta) {
			CoordVec sDelta=CoordVec(camera->coordLengthToScreenLength(coordDelta.x),camera->coordLengthToScreenLength(coordDelta.y));

			// Draw vertical lines.
			int x;
			int sx=camera->coordXToScreenXOffset(topLeft.x)+windowWidth/2;
			for(x=topLeft.x; x<=bottomRight.x; x+=coordDelta.x,sx+=sDelta.x)
				SDL_RenderDrawLine(renderer, sx, 0, sx, windowHeight);

			// Draw horizontal lines.
			int y;
			int sy=camera->coordYToScreenYOffset(topLeft.y)+windowHeight/2;
			for(y=topLeft.y; y<=bottomRight.y; y+=coordDelta.y,sy+=sDelta.y)
				SDL_RenderDrawLine(renderer, 0, sy, windowWidth, sy);
		}

		void Renderer::renderHitMask(HitMask hitmask, int sx, int sy, int r, int g, int b) {
			int delta=camera->getZoom();
			int tx, ty;
			SDL_Rect rect;
			rect.w=rect.h=delta;
			for(ty=0,rect.y=sy; ty<8; ++ty,rect.y+=delta)
				for(tx=0,rect.x=sx; tx<8; ++tx,rect.x+=delta) {
					if (!hitmask.getXY(tx, ty))
						continue;
					SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
					SDL_RenderFillRect(renderer, &rect);
				}
		}
	};
};
