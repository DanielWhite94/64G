#include <algorithm>
#include <cassert>

#include "renderer.h"
#include "../util.h"
#include "../map/mapgen.h"

using namespace Engine;
using namespace Engine::Graphics;

namespace Engine {
	namespace Graphics {
		Renderer::Renderer(unsigned windowWidth, unsigned windowHeight) {
			// Set parameters.
			drawTileGrid=false;
			drawCoordGrid=false;
			drawHitMasksActive=false;
			drawHitMasksInactive=false;
			drawHitMasksIntersections=false;
			drawMinimap=false;
			drawInventory=false;

			// Init SDL.
			SDL_Init(SDL_INIT_VIDEO);
			IMG_Init(IMG_INIT_PNG);
			TTF_Init();

			// Create window.
			window=SDL_CreateWindow("title", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, 0); // TODO: Check return and set a title.

			// Create renderer.
			renderer=SDL_CreateRenderer(window, -1, 0); // TODO: Check return.

			// Set textures array initially empty.
			unsigned i;
			for(i=0; i<MapTexture::IdMax; ++i)
				textures[i]=NULL;

			// Load fonts
			inventoryItemCountFont=TTF_OpenFont("FreeSans.ttf", 24); // TODO: Check return
		}

		Renderer::~Renderer() {
			TTF_CloseFont(inventoryItemCountFont);
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);

			// TODO: Call delete on textures.

			IMG_Quit();
			SDL_Quit();
		}

		void Renderer::refresh(const Camera *gCamera, class Map *gMap, const MapObject *player) {
			assert(gCamera!=NULL);
			assert(gMap!=NULL);

			int windowWidth=getWidth();
			int windowHeight=getHeight();

			// Update fields.
			camera=gCamera;
			map=gMap;

			// TODO: improve using CoordVec functions/overloads
			topLeft.x=(Util::floordiv(camera->screenXOffsetToCoordX(-windowWidth/2), CoordsPerTile)-1)*CoordsPerTile;
			topLeft.y=(Util::floordiv(camera->screenYOffsetToCoordY(-windowHeight/2), CoordsPerTile)-1)*CoordsPerTile;
			bottomRight.x=(Util::floordiv(camera->screenXOffsetToCoordX(+windowWidth/2), CoordsPerTile)+1)*CoordsPerTile;
			bottomRight.y=(Util::floordiv(camera->screenYOffsetToCoordY(+windowHeight/2), CoordsPerTile)+1)*CoordsPerTile;

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
					const MapTile *tile=map->getTileAtCoordVec(vec, Map::Map::GetTileFlag::None);
					if (tile==NULL)
						continue;

					// Loop over layers.
					SDL_Rect rect={.x=sx, .y=sy, .w=delta, .h=delta};
					for(z=0; z<MapTile::layersMax; ++z) {

						// Find layer for this (x,y,z).
						const MapTile::Layer *layer=tile->getLayer(z);
						assert(layer!=NULL);

						if (layer->textureId==0)
							continue;

						// Grab texture.
						const Texture *texture=getTexture(*map, layer->textureId);
						if (texture==NULL)
							continue;

						// Draw texture.
						SDL_RenderCopy(renderer, (SDL_Texture *)texture->getTexture(), NULL, &rect);
					}
				}

			// Draw objects.
			bool moreObjectsToRender=false;
			for(vec.y=topLeft.y,sy=syTopLeft; (vec.y<=bottomRight.y || moreObjectsToRender); vec.y+=CoordsPerTile,sy+=delta) {
				moreObjectsToRender=false;
				for(vec.x=topLeft.x,sx=sxTopLeft; vec.x<=bottomRight.x; vec.x+=CoordsPerTile,sx+=delta) {
					// Find tile at this (x,y).
					const MapTile *tile=map->getTileAtCoordVec(vec, Map::Map::GetTileFlag::None);
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

						// Check if there is even a texture set for this object.
						const MapTexture::Id objectTextureId=object->getTextureIdCurrent();
						if (objectTextureId==0)
							continue;

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

						// Grab texture.
						const Texture *texture=getTexture(*map, objectTextureId);
						if (texture==NULL)
							continue;

						// Draw texture slice.
						const int scale=texture->getScale();

						assert(texture->getWidth()/scale==coordObjectSize.x);
						assert(texture->getHeight()/scale==coordObjectSize.y);

						SDL_Rect srcRect={.x=textureCoordOffsetX*scale, .y=textureCoordOffsetY*scale, .w=textureCoordOffsetW*scale, .h=textureCoordOffsetH*scale};
						SDL_Rect destRect={.x=sliceScreenX1, .y=sliceScreenY1, .w=sliceScreenW, .h=sliceScreenH};
						SDL_RenderCopy(renderer, (SDL_Texture *)texture->getTexture(), &srcRect, &destRect);
					}
				}
			}

			// Draw object hitmasks (if needed).
			if (drawHitMasksActive || drawHitMasksInactive || drawHitMasksIntersections) {
				for(vec.y=topLeft.y,sy=syTopLeft; vec.y<=bottomRight.y; vec.y+=CoordsPerTile,sy+=delta)
					for(vec.x=topLeft.x,sx=sxTopLeft; vec.x<=bottomRight.x; vec.x+=CoordsPerTile,sx+=delta) {
						// Find tile at this (x,y).
						const MapTile *tile=map->getTileAtCoordVec(vec, Map::Map::GetTileFlag::None);
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

			// Draw minimap.
			if (drawMinimap) {
				const int minimapScreenRadius=80;
				const int minimapScreenX=10;
				const int minimapScreenY=windowHeight-2*minimapScreenRadius-10;
				const double minimapTileFactor=0.5;

				const CoordComponent minimapTileRadius=CoordsPerTile*minimapScreenRadius;
				int minimapScreenRadius2=minimapScreenRadius*minimapScreenRadius;
				CoordVec tilePos;
				SDL_Rect screenRect;
				for(screenRect.y=minimapScreenY, tilePos.y=camera->getY()-minimapTileRadius*minimapTileFactor+CoordsPerTile*(screenRect.y-minimapScreenY); screenRect.y<minimapScreenY+minimapScreenRadius*2; ++screenRect.y, tilePos.y+=CoordsPerTile*minimapTileFactor) {
					int dy=screenRect.y-(minimapScreenY+minimapScreenRadius);
					int dy2=dy*dy;
					for(screenRect.x=minimapScreenX, tilePos.x=camera->getX()-minimapTileRadius*minimapTileFactor+CoordsPerTile*(screenRect.x-minimapScreenX); screenRect.x<minimapScreenX+minimapScreenRadius*2; ++screenRect.x, tilePos.x+=CoordsPerTile*minimapTileFactor) {
						// Ensure we produce a circle.
						int dx=screenRect.x-(minimapScreenX+minimapScreenRadius);
						int dx2=dx*dx;
						int distSq=dx2+dy2;
						if (distSq>=minimapScreenRadius2)
							continue;

						// Compute tile position and attempt to grab.
						const MapTile *tile=map->getTileAtCoordVec(tilePos, Map::Map::GetTileFlag::None);

						// Choose colour (first looking at objects, then based on topmost tile layer with a texture set).
						int r=255, g=0, b=255;

						if (tile==NULL) {
							r=g=b=64;
						} else if (tile->getObjectCount()>0) {
							// TODO: Better colours.
							r=255, g=0, b=0;
						} else {
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

										height=std::min(height, map->alpineLevel);
										height=std::max(height, map->seaLevel);

										height=(height-map->seaLevel)/(map->alpineLevel-map->seaLevel);

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

										height=std::min(height, map->seaLevel);
										height=std::max(height, map->minHeight);

										height=(height-map->minHeight)/(map->seaLevel-map->minHeight);

										double min=64, max=196;

										r=g=0;
										b=min+height*(max-min);
									} break;
									case MapGen::TextureIdTree1:
									case MapGen::TextureIdTree2:
									case MapGen::TextureIdTree3:
									case MapGen::TextureIdRoseBush:
										r=0,g=100,b=0;
									break;
									case MapGen::TextureIdMan1:
									case MapGen::TextureIdOldManN:
									case MapGen::TextureIdOldManE:
									case MapGen::TextureIdOldManS:
									case MapGen::TextureIdOldManW:
									case MapGen::TextureIdSheepN:
									case MapGen::TextureIdSheepE:
									case MapGen::TextureIdSheepS:
									case MapGen::TextureIdSheepW:
									case MapGen::TextureIdDog:
									case MapGen::TextureIdCoins:
									case MapGen::TextureIdChestClosed:
									case MapGen::TextureIdChestOpen:
										// NPCs, items etc
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

										height=std::min(height, map->alpineLevel);
										height=std::max(height, map->seaLevel);

										height=(height-map->seaLevel)/(map->alpineLevel-map->seaLevel);

										double min=0, max=100;
										r=255;
										g=(layer->textureId==MapGen::TextureIdHotSand ? 102 : 204);
										b=min+height*(max-min);
									} break;
									case MapGen::TextureIdSnow: {
										double height=tile->getHeight();

										height=std::min(height, map->maxHeight);
										height=std::max(height, map->seaLevel);

										height=(height-map->seaLevel)/(map->maxHeight-map->seaLevel);

										double min=196, max=255;

										r=g=b=min+height*(max-min);
									} break;
									case MapGen::TextureIdHighAlpine:
									case MapGen::TextureIdLowAlpine: {
										double height=tile->getHeight();

										height=std::min(height, map->maxHeight);
										height=std::max(height, map->alpineLevel);

										height=(height-map->alpineLevel)/(map->maxHeight-map->alpineLevel);

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
						}

						// Draw pixel.
						screenRect.w=1;
						screenRect.h=1;
						SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
						SDL_RenderFillRect(renderer, &screenRect);
					}
				}
			}

			// Draw inventory (if needed).
			if (drawInventory && player!=NULL && player->inventoryExists()) {
				const int slotSize=32;
				const int inventorySpacing=10;
				const int slotsWide=4;
				const int slotsHigh=(player->inventoryGetNumSlots()+slotsWide-1)/slotsWide;
				const int inventoryWidth=(inventorySpacing+slotSize)*slotsWide+inventorySpacing;
				const int inventoryHeight=(inventorySpacing+slotSize)*slotsHigh+inventorySpacing;
				const int inventoryBaseX=windowWidth-inventoryWidth;
				const int inventoryBaseY=windowHeight-inventoryHeight;

				SDL_Rect bgRect;
				bgRect.x=inventoryBaseX;
				bgRect.y=inventoryBaseY;
				bgRect.w=inventoryWidth;
				bgRect.h=inventoryHeight;
				SDL_SetRenderDrawColor(renderer, 64, 64, 64, SDL_ALPHA_OPAQUE);
				SDL_RenderFillRect(renderer, &bgRect);

				for(unsigned slot=0; slot<player->inventoryGetNumSlots(); ++slot) {
					MapObjectItem item=player->inventoryGetItem(slot);

					// Determine which texture to use.
					MapTexture::Id textureId=mapObjectItemTypeData[item.type].iconTexture;
					if (textureId==0)
						continue;

					// Grab texture.
					const Texture *texture=getTexture(*map, textureId);
					if (texture==NULL)
						continue;

					// Draw texture.
					int slotX=slot%slotsWide;
					int slotY=slot/slotsWide;
					SDL_Rect destRect={.x=(int)(inventoryBaseX+inventorySpacing+(inventorySpacing+slotSize)*slotX), .y=inventoryBaseY+inventorySpacing+(inventorySpacing+slotSize)*slotY, .w=slotSize, .h=slotSize};
					SDL_RenderCopy(renderer, (SDL_Texture *)texture->getTexture(), NULL, &destRect);

					// Draw item count
					SDL_Color textColour={255, 255, 255};
					char itemCountStr[32];
					sprintf(itemCountStr, "%u", item.count);
					SDL_Surface* textSurface=TTF_RenderText_Solid(inventoryItemCountFont, itemCountStr, textColour);
					SDL_Texture* textTexture=SDL_CreateTextureFromSurface(renderer, textSurface);
					SDL_FreeSurface(textSurface);

					SDL_Rect textRect=destRect;
					textRect.w/=2;
					textRect.h/=2;
					SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

					SDL_DestroyTexture(textTexture);
				}
			}

			// Update screen.
			SDL_RenderPresent(renderer);
		}

		bool Renderer::getFullscreen(void) {
			return (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN);
		}

		void Renderer::toggleFullscreen(void) {
			bool isFs=getFullscreen();
			SDL_SetWindowFullscreen(window, (isFs ? 0 : SDL_WINDOW_FULLSCREEN));
			SDL_ShowCursor(isFs);
		}

		unsigned Renderer::getWidth(void) {
			SDL_DisplayMode dm;
			if (SDL_GetRendererOutputSize(renderer, &dm.w, &dm.h)==0)
				return dm.w;
			else
				return 1024; // HACK?
		}

		unsigned Renderer::getHeight(void) {
			SDL_DisplayMode dm;
			if (SDL_GetRendererOutputSize(renderer, &dm.w, &dm.h)==0)
				return dm.h;
			else
				return 512; // HACK?
		}

		void Renderer::renderGrid(const CoordVec &coordDelta) {
			CoordVec sDelta=CoordVec(camera->coordLengthToScreenLength(coordDelta.x),camera->coordLengthToScreenLength(coordDelta.y));

			int windowWidth=getWidth();
			int windowHeight=getHeight();

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

		const Texture *Renderer::getTexture(const class Map &map, MapTexture::Id textureId) {
			// If this texture is not loaded attempt to now.
			if (textures[textureId]==NULL) {
				const MapTexture *mapTexture=map.getTexture(textureId);
				if (mapTexture!=NULL)
					textures[textureId]=new Texture(renderer, mapTexture->getImagePath(), mapTexture->getScale());
			}

			return textures[textureId];
		}
	};
};
