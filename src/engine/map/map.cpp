#include <cassert>
#include <dirent.h>
#include <fcntl.h>
#include <cstdio>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "map.h"
#include "../graphics/renderer.h"
#include "../util.h"

using namespace Engine::Graphics;

namespace Engine {
	namespace Map {
		Map::Map() {
			unsigned i, j;
			for(i=0; i<regionsHigh; ++i)
				for(j=0; j<regionsWide; ++j)
					regions[i][j]=NULL;

			for(i=0; i<MapTexture::IdMax; ++i)
				textures[i]=NULL;

			initialized=true;
		}

		Map::Map(const char *mapBaseDirPath) {
			assert(mapBaseDirPath!=NULL);

			size_t mapBaseDirPathLen=strlen(mapBaseDirPath);

			DIR *dirFd;
			struct dirent *dirEntry;

			// Set Map to clean state.
			initialized=false;

			unsigned i, j;
			for(i=0; i<regionsHigh; ++i)
				for(j=0; j<regionsWide; ++j)
					regions[i][j]=NULL;

			for(i=0; i<MapTexture::IdMax; ++i)
				textures[i]=NULL;

			// Load textures
			const char *texturesDirName="textures";
			size_t texturesDirPathLen=mapBaseDirPathLen+1+strlen(texturesDirName); // +1 is for '/'
			char *texturesDirPath=(char *)malloc(texturesDirPathLen+1); // TODO: check return
			sprintf(texturesDirPath, "%s/%s", mapBaseDirPath, texturesDirName);

			dirFd=opendir(texturesDirPath);
			if (dirFd==NULL) {
				fprintf(stderr, "Can't open map texture dir at '%s'\n", texturesDirPath);
				return; // TODO: Handle better.
			}

			while((dirEntry=readdir(dirFd))!=NULL) {
				char dirEntryFileName[1024]; // TODO: this better
				sprintf(dirEntryFileName , "%s/%s", texturesDirPath, dirEntry->d_name);

				struct stat stbuf;
				if (stat(dirEntryFileName,&stbuf)==-1)
					continue;

				// Skip non-regular files.
				if ((stbuf.st_mode & S_IFMT)!=S_IFREG)
					continue;

				// Attempt to decode filename as a texture.
				char *namePtr=strrchr(dirEntryFileName, '/');
				if (namePtr!=NULL)
					namePtr++;
				else
					namePtr=dirEntryFileName;

				unsigned textureId=0, textureScale=0;
				if (sscanf(namePtr, "%us%u.", &textureId, &textureScale)!=2) {
					// TODO: error msg
					continue;
				}
				if (textureId==0 || textureScale==0) {
					// TODO: error msg
					continue;
				}

				// Add texture.
				MapTexture *texture=new MapTexture(textureId, dirEntryFileName, textureScale);
				addTexture(texture); // TODO: Check return.
			}

			closedir(dirFd);

			// Load regions.
			const char *regionsDirName="regions";
			size_t regionsDirPathLen=mapBaseDirPathLen+1+strlen(regionsDirName); // +1 is for '/'
			char *regionsDirPath=(char *)malloc(regionsDirPathLen+1); // TODO: check return
			sprintf(regionsDirPath, "%s/%s", mapBaseDirPath, regionsDirName);

			dirFd=opendir(regionsDirPath);
			if (dirFd==NULL) {
				fprintf(stderr, "Can't open map regions dir at '%s'\n", regionsDirPath);
				return; // TODO: Handle better.
			}

			while((dirEntry=readdir(dirFd))!=NULL) {
				char dirEntryFileName[1024]; // TODO: this better
				sprintf(dirEntryFileName , "%s/%s", regionsDirPath, dirEntry->d_name);

				struct stat stbuf;
				if (stat(dirEntryFileName,&stbuf)==-1)
					continue;

				// Skip non-regular files.
				if ((stbuf.st_mode & S_IFMT)!=S_IFREG)
					continue;

				// Attempt to decode filename as a region.
				char *namePtr=strrchr(dirEntryFileName, '/');
				if (namePtr!=NULL)
					namePtr++;
				else
					namePtr=dirEntryFileName;

				unsigned regionX=0, regionY=0;
				if (sscanf(namePtr, "%u,%u", &regionX, &regionY)!=2)
					// TODO: error msg
					continue;
				if (regionX>=regionsWide || regionY>=regionsHigh)
					// TODO: error msg
					continue;

				// Open region file.
				FILE *regionFile=fopen(dirEntryFileName, "r");
				if (regionFile==NULL)
					// TODO: error msg
					continue;

				// Read tile data.
				unsigned tileX, tileY;
				for(tileX=0,tileY=0; tileX<256 && tileY<256; tileX=(tileX+1)%256,tileY+=(tileX==0)) {
					// Read layers in.
					unsigned textureIdArray[MapTile::layersMax];
					if (fread(&textureIdArray, sizeof(unsigned), MapTile::layersMax, regionFile)!=MapTile::layersMax) {
						printf("skipping...\n"); // TODO: better
						break;
					}

					// Create tile.
					MapTile tile;
					for(unsigned z=0; z<MapTile::layersMax; ++z) {
						MapTileLayer layer;
						layer.textureId=textureIdArray[z];
						tile.setLayer(z, layer);
					}

					// Set tile in region.
					CoordVec vec((tileX+regionX*MapRegion::tilesWide)*Physics::CoordsPerTile, (tileY+regionY*MapRegion::tilesHigh)*Physics::CoordsPerTile);
					setTileAtCoordVec(vec, tile);
				}

				// Close region file.
				fclose(regionFile);
			}

			closedir(dirFd);

			// Tidy up.
			free(regionsDirPath);
			free(texturesDirPath);

			initialized=true;
		}

		Map::~Map() {
			unsigned i, j;
			for(i=0; i<regionsHigh; ++i)
				for(j=0; j<regionsWide; ++j)
					regions[i][j]=NULL; // TODO: Free these properly.

			for(i=0; i<MapTexture::IdMax; ++i)
				removeTexture(i);

			initialized=false;
		}

		bool Map::save(const char *mapBaseDirPath) const {
			assert(mapBaseDirPath!=NULL);

			// TODO: In each case where we fail, tidy up and free anything as required.
			// TODO: Better error reporting.

			// Save 'metadata' (create directories).
			saveMetadata(mapBaseDirPath); // TODO: Check return.

			// Save all regions.
			saveRegions(mapBaseDirPath); // TODO: Check return.

			// Save all textures.
			saveTextures(mapBaseDirPath); // TODO: Check return.

			return true;
		}

		bool Map::saveMetadata(const char *mapBaseDirPath) const {
			assert(mapBaseDirPath!=NULL);

			// Create base directory for the map.
			if (mkdir(mapBaseDirPath, 0777)!=0) {
				fprintf(stderr, "error: could not create map base dir at '%s'\n", mapBaseDirPath);
				return false;
			}

			// Create 'regions' and 'textures' directories.
			int mkdirResult;

			char *regionsDirPath=saveBaseDirToRegionsDir(mapBaseDirPath); // TODO: check return
			mkdirResult=mkdir(regionsDirPath, 0777);
			free(regionsDirPath);
			if (mkdirResult!=0) {
				fprintf(stderr,"error: could not create map regions dir at '%s'\n", regionsDirPath);
				return false;
			}

			char *texturesDirPath=saveBaseDirToTexturesDir(mapBaseDirPath); // TODO: check return
			mkdirResult=mkdir(texturesDirPath, 0777);
			free(texturesDirPath);
			if (mkdirResult!=0) {
				fprintf(stderr,"error: could not create map textures dir at '%s'\n", texturesDirPath);
				return false;
			}

			return true;
		}

		bool Map::saveTextures(const char *mapBaseDirPath) const {
			assert(mapBaseDirPath!=NULL);

			// Save all textures.
			char *texturesDirPath=saveBaseDirToTexturesDir(mapBaseDirPath); // TODO: check return

			unsigned textureId;
			for(textureId=1; textureId<MapTexture::IdMax; ++textureId) {
				// Grab texture.
				const MapTexture *texture=getTexture(textureId);
				if (texture==NULL)
					continue;

				// Save texture.
				texture->save(texturesDirPath); // TODO: Check return.
			}

			free(texturesDirPath);

			return true;
		}

		bool Map::saveRegions(const char *mapBaseDirPath) const {
			assert(mapBaseDirPath!=NULL);

			// Save all regions.
			char *regionsDirPath=saveBaseDirToRegionsDir(mapBaseDirPath); // TODO: check return

			unsigned regionX, regionY;
			for(regionY=0; regionY<regionsHigh; ++regionY)
				for(regionX=0; regionX<regionsWide; ++regionX) {
					// Grab region.
					const MapRegion *region=getRegionAtOffset(regionX, regionY);
					if (region==NULL)
						continue;

					// Create file.
					char regionFilePath[1024]; // TODO: Prevent overflows.
					sprintf(regionFilePath, "%s/%u,%u", regionsDirPath, regionX, regionY);
					FILE *regionFile=fopen(regionFilePath, "w");

					// Save all tiles.
					unsigned tileX, tileY;
					for(tileY=0; tileY<MapRegion::tilesHigh; ++tileY)
						for(tileX=0; tileX<MapRegion::tilesWide; ++tileX) {
							// Grab tile.
							const MapTile *tile=region->getTileAtOffset(tileX, tileY);

							// Save all layers.
							unsigned z;
							for(z=0; z<MapTile::layersMax; ++z) {
								// Grab layer.
								const MapTileLayer *layer=tile->getLayer(z);

								// Save texture.
								fwrite(&layer->textureId, sizeof(layer->textureId), 1, regionFile); // TODO: Check return.

							}
						}

					// Close file.
					fclose(regionFile);
				}

			free(regionsDirPath);

			return true;
		}

		void Map::tick(void) {
			for(unsigned i=0; i<objects.size(); i++) {
				CoordVec delta=objects[i]->tick();
				moveObject(objects[i], objects[i]->getCoordTopLeft()+delta);
			}
		}

		MapTile *Map::getTileAtCoordVec(const CoordVec &vec) {
			MapRegion *region=getRegionAtCoordVec(vec);
			if (region==NULL)
				return NULL;

			return region->getTileAtCoordVec(vec);
		}

		const MapTile *Map::getTileAtCoordVec(const CoordVec &vec) const {
			const MapRegion *region=getRegionAtCoordVec(vec);
			if (region==NULL)
				return NULL;

			return region->getTileAtCoordVec(vec);
		}

		MapRegion *Map::getRegionAtCoordVec(const CoordVec &vec) {
			if (vec.x<0 || vec.y<0)
				return NULL;

			CoordComponent tileX=vec.x/Physics::CoordsPerTile;
			CoordComponent tileY=vec.y/Physics::CoordsPerTile;
			CoordComponent regionX=tileX/MapRegion::tilesWide;
			CoordComponent regionY=tileY/MapRegion::tilesHigh;

			return getRegionAtOffset(regionX, regionY);
		}

		const MapRegion *Map::getRegionAtCoordVec(const CoordVec &vec) const {
			if (vec.x<0 || vec.y<0)
				return NULL;

			CoordComponent tileX=vec.x/Physics::CoordsPerTile;
			CoordComponent tileY=vec.y/Physics::CoordsPerTile;
			CoordComponent regionX=tileX/MapRegion::tilesWide;
			CoordComponent regionY=tileY/MapRegion::tilesHigh;

			return getRegionAtOffset(regionX, regionY);
		}

		MapRegion *Map::getRegionAtOffset(unsigned regionX, unsigned regionY) {
			if (regionX>=regionsWide || regionY>=regionsHigh)
				return NULL;

			return regions[regionY][regionX];
		}

		const MapRegion *Map::getRegionAtOffset(unsigned regionX, unsigned regionY) const {
			if (regionX>=regionsWide || regionY>=regionsHigh)
				return NULL;

			return regions[regionY][regionX];
		}

		void Map::setTileAtCoordVec(const CoordVec &vec, const MapTile &tile) {
			MapRegion *region=getRegionAtCoordVec(vec);
			if (region==NULL) {
				region=new MapRegion();
				if (region==NULL)
					return;

				CoordComponent tileX=vec.x/Physics::CoordsPerTile;
				CoordComponent tileY=vec.y/Physics::CoordsPerTile;
				CoordComponent regionX=tileX/MapRegion::tilesWide;
				CoordComponent regionY=tileY/MapRegion::tilesHigh;

				regions[regionY][regionX]=region;
			}

			region->setTileAtCoordVec(vec, tile);
		}

		bool Map::addObject(MapObject *object) {
			assert(object!=NULL);

			// Compute dimensions.
			CoordVec vec;
			CoordVec vec1=object->getCoordTopLeft();
			CoordVec vec2=object->getCoordBottomRight();
			vec1.x=Util::floordiv(vec1.x, Physics::CoordsPerTile)*Physics::CoordsPerTile;
			vec1.y=Util::floordiv(vec1.y, Physics::CoordsPerTile)*Physics::CoordsPerTile;
			vec2.x=Util::floordiv(vec2.x, Physics::CoordsPerTile)*Physics::CoordsPerTile;
			vec2.y=Util::floordiv(vec2.y, Physics::CoordsPerTile)*Physics::CoordsPerTile;

			// Test new object is not out-of-bounds nor intersects any existing objects.
			for(vec.y=vec1.y; vec.y<=vec2.y; vec.y+=Physics::CoordsPerTile)
				for(vec.x=vec1.x; vec.x<=vec2.x; vec.x+=Physics::CoordsPerTile) {
					// Is there even a tile here?
					MapTile *tile=getTileAtCoordVec(vec);
					if (tile==NULL)
						return false;

					// Tile too full?
					if (tile->isObjectsFull())
						return false;

					// Check for intersections.
					if (tile->getHitMask(vec) & object->getHitMaskByCoord(vec))
						return false;
				}

			// Add to object list.
			objects.push_back(object);

			// Add to tiles.
			for(vec.y=vec1.y; vec.y<=vec2.y; vec.y+=Physics::CoordsPerTile)
				for(vec.x=vec1.x; vec.x<=vec2.x; vec.x+=Physics::CoordsPerTile)
					getTileAtCoordVec(vec)->addObject(object);

			return true;
		}

		bool Map::moveObject(MapObject *object, const CoordVec &newPos) {
			assert(object!=NULL);

			// No change?
			CoordVec oldPos=object->getCoordTopLeft();
			if (newPos==oldPos)
				return true;

			// TODO: Add and removing to/from tiles can be improved by considering newPos-pos.

			CoordVec vec;

			// Compute old dimensions.
			CoordVec oldVec1, oldVec2;
			oldVec1=object->getCoordTopLeft();
			oldVec2=object->getCoordBottomRight();
			oldVec1.x=Util::floordiv(oldVec1.x, Physics::CoordsPerTile)*Physics::CoordsPerTile;
			oldVec1.y=Util::floordiv(oldVec1.y, Physics::CoordsPerTile)*Physics::CoordsPerTile;
			oldVec2.x=Util::floordiv(oldVec2.x, Physics::CoordsPerTile)*Physics::CoordsPerTile;
			oldVec2.y=Util::floordiv(oldVec2.y, Physics::CoordsPerTile)*Physics::CoordsPerTile;

			// Remove from tiles.
			for(vec.y=oldVec1.y; vec.y<=oldVec2.y; vec.y+=Physics::CoordsPerTile)
				for(vec.x=oldVec1.x; vec.x<=oldVec2.x; vec.x+=Physics::CoordsPerTile)
					getTileAtCoordVec(vec)->removeObject(object);

			// Move object.
			double angle=(180.0/M_PI)*Util::angleFromXYToXY(object->getCoordTopLeft().x, object->getCoordTopLeft().y, newPos.x, newPos.y);

			object->setPos(newPos);

			assert(angle>=-180.0 && angle<=180.0);
			if (angle<-135.0 || angle>=135.0)
				object->setAngle(CoordAngle90);
			else if (angle>=-135.0 && angle<-45.0)
				object->setAngle(CoordAngle0);
			else if (angle>=-45.0 && angle<45.0)
				object->setAngle(CoordAngle270);
			else if (angle>=45.0 && angle<135.0)
				object->setAngle(CoordAngle180);
			else
				assert(false);

			// Compute new dimensions.
			CoordVec newVec1, newVec2;
			newVec1=object->getCoordTopLeft();
			newVec2=object->getCoordBottomRight();
			newVec1.x=Util::floordiv(newVec1.x, Physics::CoordsPerTile)*Physics::CoordsPerTile;
			newVec1.y=Util::floordiv(newVec1.y, Physics::CoordsPerTile)*Physics::CoordsPerTile;
			newVec2.x=Util::floordiv(newVec2.x, Physics::CoordsPerTile)*Physics::CoordsPerTile;
			newVec2.y=Util::floordiv(newVec2.y, Physics::CoordsPerTile)*Physics::CoordsPerTile;

			// Test for out-of-bounds or interesections.
			bool result=true;
			for(vec.y=newVec1.y; vec.y<=newVec2.y; vec.y+=Physics::CoordsPerTile)
				for(vec.x=newVec1.x; vec.x<=newVec2.x; vec.x+=Physics::CoordsPerTile) {
					// Is there even a tile here?
					MapTile *tile=getTileAtCoordVec(vec);
					if (tile==NULL) {
						result=false;
						break;
					}

					// Tile too full?
					if (tile->isObjectsFull()) {
						result=false;
						break;
					}

					// Check for intersections.
					if (tile->getHitMask(vec) & object->getHitMaskByCoord(vec)) {
						result=false;
						break;
					}
				}

			if (!result) {
				// Reset object position.
				object->setPos(oldPos);

				// Add to old tiles.
				for(vec.y=oldVec1.y; vec.y<=oldVec2.y; vec.y+=Physics::CoordsPerTile)
					for(vec.x=oldVec1.x; vec.x<=oldVec2.x; vec.x+=Physics::CoordsPerTile)
						getTileAtCoordVec(vec)->addObject(object);

				return false;
			}

			// Add to new tiles.
			for(vec.y=newVec1.y; vec.y<=newVec2.y; vec.y+=Physics::CoordsPerTile)
				for(vec.x=newVec1.x; vec.x<=newVec2.x; vec.x+=Physics::CoordsPerTile)
					getTileAtCoordVec(vec)->addObject(object);

			return result;
		}

		bool Map::addTexture(MapTexture *texture) {
			assert(texture!=NULL);

			// Ensure this id is free.
			if (textures[texture->getId()]!=NULL)
				return false;

			// Create texture and add to array.
			textures[texture->getId()]=texture;

			return (textures[texture->getId()]!=NULL);
		}

		void Map::removeTexture(unsigned id) {
			assert(id<MapTexture::IdMax);

			// If there is a texture here, free it and set entry to NULL.
			if (textures[id]!=NULL) {
				delete textures[id];
				textures[id]=NULL;
			}
		}

		const MapTexture *Map::getTexture(unsigned id) const {
			assert(id<MapTexture::IdMax);

			return textures[id];
		}

		char *Map::saveBaseDirToRegionsDir(const char *mapBaseDirPath) {
			assert(mapBaseDirPath!=NULL);

			const char *regionsDirName="regions";

			size_t mapBaseDirPathLen=strlen(mapBaseDirPath);
			size_t regionsDirPathLen=mapBaseDirPathLen+1+strlen(regionsDirName); // +1 is for '/'
			char *regionsDirPath=(char *)malloc(regionsDirPathLen+1); // TODO: Check return
			sprintf(regionsDirPath, "%s/%s", mapBaseDirPath, regionsDirName);

			return regionsDirPath;
		}

		char *Map::saveBaseDirToTexturesDir(const char *mapBaseDirPath) {
			assert(mapBaseDirPath!=NULL);

			const char *texturesDirName="textures";

			size_t mapBaseDirPathLen=strlen(mapBaseDirPath);
			size_t texturesDirPathLen=mapBaseDirPathLen+1+strlen(texturesDirName); // +1 is for '/'
			char *texturesDirPath=(char *)malloc(texturesDirPathLen+1); // TODO: Check return
			sprintf(texturesDirPath, "%s/%s", mapBaseDirPath, texturesDirName);

			return texturesDirPath;
		}
	};
};
