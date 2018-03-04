#include <cassert>
#include <dirent.h>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "map.h"
#include "../graphics/renderer.h"
#include "../util.h"

using namespace Engine::Graphics;

namespace Engine {
	namespace Map {
		Map::Map(const char *mapBaseDirPath) {
			assert(mapBaseDirPath!=NULL);

			DIR *dirFd;
			struct dirent *dirEntry;

			// Set Map to clean state.
			unsigned i, j;

			baseDir=NULL;
			texturesDir=NULL;
			regionsDir=NULL;

			regionsByIndexNext=0;
			for(i=0; i<regionsLoadedMax; ++i)
				regionsByIndex[i]=NULL;
			for(i=0; i<regionsLoadedMax; ++i)
				regionsByAge[i]=NULL;
			for(i=0; i<regionsHigh; ++i)
				for(j=0; j<regionsWide; ++j)
					regionsByOffset[i][j].ptr=NULL;

			for(i=0; i<MapTexture::IdMax; ++i)
				textures[i]=NULL;

			initialized=false; // Needed as initclean sets this to true.

			// Create directory strings.
			size_t mapBaseDirPathLen=strlen(mapBaseDirPath);
			baseDir=(char *)malloc(mapBaseDirPathLen+1); // TODO: Check return.
			strcpy(baseDir, mapBaseDirPath);

			const char *texturesDirName="textures";
			size_t texturesDirPathLen=mapBaseDirPathLen+1+strlen(texturesDirName); // +1 is for '/'
			texturesDir=(char *)malloc(texturesDirPathLen+1); // TODO: check return
			sprintf(texturesDir, "%s/%s", mapBaseDirPath, texturesDirName);

			const char *regionsDirName="regions";
			size_t regionsDirPathLen=mapBaseDirPathLen+1+strlen(regionsDirName); // +1 is for '/'
			regionsDir=(char *)malloc(regionsDirPathLen+1); // TODO: check return
			sprintf(regionsDir, "%s/%s", mapBaseDirPath, regionsDirName);

			// Load metadata file if exists.
			char metadataFilePath[1024]; // TODO: Prevent overflows.
			sprintf(metadataFilePath, "%s/metadata", baseDir);
			FILE *metadataFile=fopen(metadataFilePath, "r");
			if (metadataFile!=NULL) {
				fread(&minHeight, sizeof(double), 1, metadataFile);
				fread(&maxHeight, sizeof(double), 1, metadataFile);
				fread(&minTemperature, sizeof(double), 1, metadataFile);
				fread(&maxTemperature, sizeof(double), 1, metadataFile);
				fread(&minMoisture, sizeof(double), 1, metadataFile);
				fread(&maxMoisture, sizeof(double), 1, metadataFile);

				fclose(metadataFile);
			}

			// Ensure directories etc exist.
			saveMetadata();

			// Load textures
			dirFd=opendir(getTexturesDir());
			if (dirFd==NULL) {
				fprintf(stderr, "Can't open map texture dir at '%s'.\n", getTexturesDir());
			} else {
				while((dirEntry=readdir(dirFd))!=NULL) {
					char dirEntryFileName[1024]; // TODO: this better
					sprintf(dirEntryFileName , "%s/%s", getTexturesDir(), dirEntry->d_name);

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
			}

			// Note: Regions are loaded on demand.

			initialized=true;
		}

		Map::~Map() {
			unsigned i;

			// Remove regions.
			while(regionsByIndexNext>0)
				regionUnload(regionsByIndexNext-1);

			// Remove textures.
			for(i=0; i<MapTexture::IdMax; ++i)
				removeTexture(i);

			// Free paths.
			free(baseDir);
			free(texturesDir);
			free(regionsDir);

			// Clear initialized flag to be safe.
			initialized=false;
		}

		bool Map::save(void) {
			// TODO: In each case where we fail, tidy up and free anything as required.
			// TODO: Better error reporting.

			// Save 'metadata' (create directories).
			if (!saveMetadata())
				return false;

			// Save all regions.
			if (!saveRegions())
				return false;

			// Save all textures.
			if (!saveTextures())
				return false;

			return true;
		}

		bool Map::saveMetadata(void) const {
			// Do we need to create the base directory?
			const char *mapBaseDirPath=getBaseDir();
			if (!Util::isDir(mapBaseDirPath)) {
				if (mkdir(mapBaseDirPath, 0777)!=0) {
					perror(NULL);
					fprintf(stderr, "error: could not create map base dir at '%s'\n", mapBaseDirPath);
					return false;
				}
			}

			// Do we need to create regions directory?
			const char *regionsDirPath=getRegionsDir();
			if (!Util::isDir(regionsDirPath)) {
				if (mkdir(regionsDirPath, 0777)!=0) {
					fprintf(stderr,"error: could not create map regions dir at '%s'\n", regionsDirPath);
					return false;
				}
			}

			// Do we need to create textures directory?
			const char *texturesDirPath=getTexturesDir();
			if (!Util::isDir(texturesDirPath)) {
				if (mkdir(texturesDirPath, 0777)!=0) {
					fprintf(stderr,"error: could not create map textures dir at '%s'\n", texturesDirPath);
					return false;
				}
			}

			// Write metadata file.
			char metadataFilePath[1024]; // TODO: Prevent overflows.
			sprintf(metadataFilePath, "%s/metadata", baseDir);
			FILE *metadataFile=fopen(metadataFilePath, "w");
			if (metadataFile==NULL)
				return false;

			bool result=true;
			result&=(fwrite(&minHeight, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&maxHeight, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&minTemperature, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&maxTemperature, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&minMoisture, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&maxMoisture, sizeof(double), 1, metadataFile)==1);

			fclose(metadataFile);

			return result;
		}

		bool Map::saveTextures(void) const {
			bool success=true;

			// Save all textures.
			const char *texturesDirPath=getTexturesDir();

			unsigned textureId;
			for(textureId=1; textureId<MapTexture::IdMax; ++textureId) {
				// Grab texture.
				const MapTexture *texture=getTexture(textureId);
				if (texture==NULL)
					continue;

				// Save texture.
				success&=texture->save(texturesDirPath);
			}

			return success;
		}

		bool Map::saveRegions(void) {
			bool success=true;

			// Save all regions.
			const char *regionsDirPath=getRegionsDir();

			for(unsigned i=0; i<regionsByIndexNext; ++i) {
				// Grab region.
				MapRegion *region=getRegionAtIndex(i);

				// Is the region even dirty?
				if (!region->getIsDirty())
					continue;

				// Save region.
				success&=region->save(regionsDirPath, regionsByIndex[i]->offsetX, regionsByIndex[i]->offsetY);
			}

			return success;
		}

		bool Map::loadRegion(unsigned regionX, unsigned regionY, const char *regionPath) {
			assert(regionX<regionsWide && regionY<regionsHigh);
			assert(regionPath!=NULL);
			assert(regionsByOffset[regionY][regionX].ptr==NULL);

			// Create empty region to add to.
			if (!createBlankRegion(regionX, regionY))
				return false;

			// Attempt to load.
			MapRegion *region=getRegionAtOffset(regionX, regionY, false);
			return region->load(regionPath);
		}

		bool Map::markRegionDirtyAtTileOffset(unsigned offsetX, unsigned offsetY, bool create) {
			assert(offsetX>=0 && offsetX<regionsWide*MapRegion::tilesWide);
			assert(offsetY>=0 && offsetY<regionsHigh*MapRegion::tilesHigh);

			unsigned regionX=offsetX/MapRegion::tilesWide;
			unsigned regionY=offsetY/MapRegion::tilesHigh;
			MapRegion *region=getRegionAtOffset(regionX, regionY, create);
			if (region==NULL)
				return false;

			region->setDirty();

			return true;
		}

		void Map::tick(void) {
			for(unsigned i=0; i<objects.size(); i++) {
				CoordVec delta=objects[i]->tick();
				moveObject(objects[i], objects[i]->getCoordTopLeft()+delta);
			}
		}

		MapTile *Map::getTileAtCoordVec(const CoordVec &vec, GetTileFlag flags) {
			MapRegion *region=getRegionAtCoordVec(vec, (flags & GetTileFlag::Create)!=0);
			if (region==NULL)
				return NULL;

			if (flags & GetTileFlag::Dirty)
				region->setDirty();

			return region->getTileAtCoordVec(vec);
		}

		MapTile *Map::getTileAtOffset(unsigned offsetX, unsigned offsetY, GetTileFlag flags) {
			assert(offsetX>=0 && offsetX<regionsWide*MapRegion::tilesWide);
			assert(offsetY>=0 && offsetY<regionsHigh*MapRegion::tilesHigh);

			unsigned regionX=offsetX/MapRegion::tilesWide;
			unsigned regionY=offsetY/MapRegion::tilesHigh;
			MapRegion *region=getRegionAtOffset(regionX, regionY, (flags & GetTileFlag::Create)!=0);
			if (region==NULL)
				return NULL;

			if (flags & GetTileFlag::Dirty)
				region->setDirty();

			unsigned regionTileOffsetX=offsetX%MapRegion::tilesWide;
			unsigned regionTileOffsetY=offsetY%MapRegion::tilesHigh;
			return region->getTileAtOffset(regionTileOffsetX, regionTileOffsetY);
		}

		MapRegion *Map::getRegionAtCoordVec(const CoordVec &vec, bool create) {
			if (vec.x<0 || vec.y<0)
				return NULL;

			CoordComponent tileX=vec.x/Physics::CoordsPerTile;
			CoordComponent tileY=vec.y/Physics::CoordsPerTile;
			CoordComponent regionX=tileX/MapRegion::tilesWide;
			CoordComponent regionY=tileY/MapRegion::tilesHigh;

			return getRegionAtOffset(regionX, regionY, create);
		}

		MapRegion *Map::getRegionAtOffset(unsigned regionX, unsigned regionY, bool create) {
			// Out of bounds?
			if (regionX>=regionsWide || regionY>=regionsHigh)
				return NULL;

			// Region not loaded?
			if (regionsByOffset[regionY][regionX].ptr==NULL) {
				// Free a region if we need to.
				if (ensureSpaceForRegion()) {
					// Create path.
					const char *regionsDirPath=getRegionsDir();
					char regionPath[4096]; // TODO: this better
					sprintf(regionPath, "%s/%u,%u", regionsDirPath, regionX, regionY); // TODO: Check return.

					// Attempt to load region.
					if (!loadRegion(regionX, regionY, regionPath)) {
						if (!create) {
							// Failed - ensure this region is unloaded as it is not correct.
							for(unsigned r=0; r<regionsByIndexNext; ++r)
								if (regionsByIndex[r]->offsetX==regionX && regionsByIndex[r]->offsetY==regionY) {
									regionUnload(r);
									break;
								}
						}
					}
				}
			} else {
				// Exists already - simply update age.
				updateRegionAge(regionsByOffset[regionY][regionX].ptr);
			}

			// Return region (or NULL if we could not load/create).
			return regionsByOffset[regionY][regionX].ptr;
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
					MapTile *tile=getTileAtCoordVec(vec, GetTileFlag::None);
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
					getTileAtCoordVec(vec, GetTileFlag::Dirty)->addObject(object);

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
					getTileAtCoordVec(vec, GetTileFlag::Dirty)->removeObject(object);

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
					MapTile *tile=getTileAtCoordVec(vec, GetTileFlag::None);
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
						getTileAtCoordVec(vec, GetTileFlag::Dirty)->addObject(object);

				return false;
			}

			// Add to new tiles.
			for(vec.y=newVec1.y; vec.y<=newVec2.y; vec.y+=Physics::CoordsPerTile)
				for(vec.x=newVec1.x; vec.x<=newVec2.x; vec.x+=Physics::CoordsPerTile)
					getTileAtCoordVec(vec, GetTileFlag::Dirty)->addObject(object);

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

		const char *Map::getBaseDir(void) const {
			return baseDir;
		}

		const char *Map::getRegionsDir(void) const {
			return regionsDir;
		}

		const char *Map::getTexturesDir(void) const {
			return texturesDir;
		}

		MapRegion *Map::getRegionAtIndex(unsigned index) {
			assert(index<regionsByIndexNext);

			return regionsByIndex[index]->ptr;
		}

		const MapRegion *Map::getRegionAtIndex(unsigned index) const {
			assert(index<regionsByIndexNext);

			return regionsByIndex[index]->ptr;
		}

		bool Map::createBlankRegion(unsigned regionX, unsigned regionY) {
			assert(regionX<regionsWide && regionY<regionsHigh);
			assert(regionsByOffset[regionY][regionX].ptr==NULL);

			// Do we need to free a region to allocate this one?
			if (!ensureSpaceForRegion())
				return false;

			// Create new blank region.
			MapRegion *region=new MapRegion(regionX, regionY);
			if (region==NULL)
				return false;

			// Add region.
			assert(regionsByIndexNext<regionsLoadedMax);
			regionsByOffset[regionY][regionX].ptr=region;
			regionsByOffset[regionY][regionX].index=regionsByIndexNext;
			regionsByOffset[regionY][regionX].offsetX=regionX;
			regionsByOffset[regionY][regionX].offsetY=regionY;
			regionsByIndex[regionsByIndexNext]=&(regionsByOffset[regionY][regionX]);
			assert(regionsByAge[regionsByIndexNext]==NULL);
			regionsByAge[regionsByIndexNext]=regionsByIndex[regionsByIndexNext];
			regionsByIndexNext++;

			updateRegionAge(region);

			return true;
		}

		bool Map::ensureSpaceForRegion(void) {
			// Do we need to evict something?
			assert(regionsByIndexNext<=regionsLoadedMax);
			if (regionsByIndexNext==regionsLoadedMax) {
				// Find the last-recently used region.
				RegionData *regionData=regionsByAge[regionsByIndexNext-1];
				assert(regionData!=NULL);
				MapRegion *region=regionData->ptr;

				// If this region is dirty, save it back to disk.
				if (region->getIsDirty())
					region->save(getRegionsDir(), regionsByIndex[regionData->index]->offsetX, regionsByIndex[regionData->index]->offsetY); // TODO: Check return - don't want to lose data.

				// Unload the region.
				regionUnload(regionData->index);
			}

			return (regionsByIndexNext<regionsLoadedMax);
		}

		void Map::updateRegionAge(const MapRegion *region) {
			assert(region!=NULL);

			for(int i=0; i<regionsByIndexNext; i++) {
				if (regionsByAge[i]->ptr==region) {
					// Found - move to front.
					RegionData *regionDataPtr=regionsByAge[i];
					memmove(regionsByAge+1, regionsByAge, i*sizeof(RegionData *));
					regionsByAge[0]=regionDataPtr;
					return;
				}
			}
			assert(false);
		}

		void Map::regionUnload(unsigned index) {
			assert(index<regionsByIndexNext);

			// Remove from regionsByAge array.
			for(unsigned i=0; i<regionsByIndexNext; ++i) {
				if (regionsByAge[i]==regionsByIndex[index]) {
					memmove(regionsByAge+i, regionsByAge+i+1, (regionsByIndexNext-1-i)*sizeof(RegionData *));
					regionsByAge[regionsByIndexNext-1]=NULL;
					break;
				}
			}

			// Free the region and clear the RegionData pointer.
			MapRegion *region=getRegionAtIndex(index);
			delete region;

			regionsByIndex[index]->ptr=NULL;

			// Copy last array element into this gap and update its index.
			regionsByIndex[index]=regionsByIndex[regionsByIndexNext-1];
			regionsByIndex[index]->index=index;

			// Decrement index next.
			--regionsByIndexNext;
		}
	};
};
