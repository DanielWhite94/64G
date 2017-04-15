#include <algorithm>
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
			const char *texturePaths[TextureIdNB]={
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
				[TextureIdTree1]="./images/objects/tree1.png",
				[TextureIdTree2]="./images/objects/tree2.png",
				[TextureIdMan1]="./images/objects/man1.png",
			};
			const int textureScales[TextureIdNB]={
				[TextureIdGrass0]=1,
				[TextureIdGrass1]=1,
				[TextureIdGrass2]=1,
				[TextureIdGrass3]=1,
				[TextureIdGrass4]=1,
				[TextureIdGrass5]=1,
				[TextureIdGrass6]=1,
				[TextureIdBrickPath]=1,
				[TextureIdDirt]=1,
				[TextureIdDock]=1,
				[TextureIdWater]=1,
				[TextureIdTree1]=4,
				[TextureIdTree2]=4,
				[TextureIdMan1]=4,
			};
			unsigned i;
			for(i=1; i<TextureIdNB; ++i)
				textures[i]=new Texture(renderer, texturePaths[i], textureScales[i]); // TODO: call delete
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
			bool moreObjectsToRender=false;
			for(vec.y=topLeft.y,sy=syTopLeft; (vec.y<=bottomRight.y || moreObjectsToRender); vec.y+=CoordsPerTile,sy+=delta) {
				moreObjectsToRender=false;
				for(vec.x=topLeft.x,sx=sxTopLeft; vec.x<=bottomRight.x; vec.x+=CoordsPerTile,sx+=delta) {
					// Find tile at this (x,y).
					const MapTile *tile=map->getTileAtCoordVec(vec);
					if (tile==NULL)
						continue;

					// Loop over all objects on this tile.
					unsigned i, max=tile->getObjectCount();
					for(i=0; i<max; ++i) {
						// Grab object.
						const MapObject *object=tile->getObject(i);
						assert(object!=NULL);

						// We wait to draw a whole vertical slice once we reach the lowest tile in a column - is this such a tile?
						CoordVec objectCoordBottomRight=object->getCoordBottomRight();
						if (objectCoordBottomRight.y>=vec.y+CoordsPerTile) {
							moreObjectsToRender=true; // FIXME: Technically this could cause us to loop over MANY more rows than is necesarry.

							continue;
						}

						// Compute which part of the object we are drawing for this vertical slice.
						CoordVec objectCoordTopLeft=object->getCoordTopLeft();

						const int sliceCoordX1=std::max(objectCoordTopLeft.x, vec.x);
						const int sliceCoordX2=std::min(objectCoordBottomRight.x+1, vec.x+CoordsPerTile);

						const int sliceCoordY1=objectCoordTopLeft.y;
						const int sliceCoordY2=objectCoordBottomRight.y+1;

						// Covert grid coordinates into screen coordinates.
						const int sliceScreenX1=windowWidth/2+camera->coordXToScreenXOffset(sliceCoordX1);
						const int sliceScreenY1=windowHeight/2+camera->coordYToScreenYOffset(sliceCoordY1);

						const int sliceScreenW=camera->coordLengthToScreenLength(sliceCoordX2-sliceCoordX1);
						const int sliceScreenH=camera->coordLengthToScreenLength(sliceCoordY2-sliceCoordY1);

						// Compute which part of the object's texture we require.
						const CoordVec coordObjectSize=object->getCoordSize();

						const int textureCoordOffsetX=sliceCoordX1-objectCoordTopLeft.x;
						const int textureCoordOffsetW=sliceCoordX2-sliceCoordX1;

						const int textureCoordOffsetY=0;
						const int textureCoordOffsetH=coordObjectSize.y;

						// Draw slice of texture for this x-offset.
						const unsigned objectTextureId=object->tempGetTextureId();
						if (objectTextureId>0) {
							const Texture *texture=textures[objectTextureId];
							const int scale=texture->getScale();

							assert(texture->getWidth()/scale==coordObjectSize.x);
							assert(texture->getHeight()/scale==coordObjectSize.y);

							SDL_Rect srcRect={.x=textureCoordOffsetX*scale, .y=textureCoordOffsetY*scale, .w=textureCoordOffsetW*scale, .h=textureCoordOffsetH*scale};
							SDL_Rect destRect={.x=sliceScreenX1, .y=sliceScreenY1, .w=sliceScreenW, .h=sliceScreenH};
							SDL_RenderCopy(renderer, (SDL_Texture *)texture->getTexture(), &srcRect, &destRect);
						} else {
							// TEMP

							SDL_Rect rect;
							rect.x=sliceScreenX1;
							rect.y=sliceScreenY1;
							rect.w=sliceScreenW;
							rect.h=sliceScreenH;
							SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);
							SDL_RenderFillRect(renderer, &rect);
						}
					}
				}
			}

			// Draw object hitmasks (if needed).
			if (drawHitMasksActive || drawHitMasksInactive || drawHitMasksIntersections) {
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

							// Draw hitmasks.
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

						// Draw hitmask intersections if needed.
						if (drawHitMasksIntersections)
							renderHitMask(intersectionHitMask, sx, sy, 255, 0, 0);
					}
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
			for(x=topLeft.x; x<=bottomRight.x+CoordsPerTile; x+=coordDelta.x,sx+=sDelta.x)
				SDL_RenderDrawLine(renderer, sx, 0, sx, windowHeight);

			// Draw horizontal lines.
			int y;
			int sy=camera->coordYToScreenYOffset(topLeft.y)+windowHeight/2;
			for(y=topLeft.y; y<=bottomRight.y+CoordsPerTile; y+=coordDelta.y,sy+=sDelta.y)
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
