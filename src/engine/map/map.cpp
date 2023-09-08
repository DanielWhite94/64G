#include <cassert>
#include <dirent.h>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "map.h"
#include "maptiled.h"
#include "../graphics/renderer.h"
#include "../util.h"

using namespace Engine::Graphics;

namespace Engine {
	namespace Map {
		Map::Map(const char *mapBaseDirPath, unsigned mapWidth, unsigned mapHeight): mapWidth(mapWidth), mapHeight(mapHeight) {
			assert(mapBaseDirPath!=NULL);

			// Round width and height up to a a multiple of the region tile size (but do not exceed maximum size allowed)
			if (mapWidth<1) mapWidth=1;
			if (mapHeight<1) mapHeight=1;
			if (mapWidth>Map::regionsSize*MapRegion::tilesSize) mapWidth=Map::regionsSize*MapRegion::tilesSize;
			if (mapHeight>Map::regionsSize*MapRegion::tilesSize) mapHeight=Map::regionsSize*MapRegion::tilesSize;

			mapWidth=((mapWidth+(MapRegion::tilesSize-1))/MapRegion::tilesSize)*MapRegion::tilesSize;
			mapHeight=((mapHeight+(MapRegion::tilesSize-1))/MapRegion::tilesSize)*MapRegion::tilesSize;

			// Set Map to clean state.
			unsigned i, j;

			lockFd=-1;

			baseDir=NULL;
			texturesDir=NULL;
			itemsDir=NULL;
			regionsDir=NULL;
			mapTiledDir=NULL;

			regionsCount=0;
			for(i=0; i<regionsLoadedMax; ++i)
				regionsByIndex[i]=NULL;
			for(i=0; i<regionsLoadedMax; ++i)
				regionsByAge[i]=NULL;
			for(i=0; i<regionsSize; ++i)
				for(j=0; j<regionsSize; ++j)
					regionsByOffset[i][j].ptr=NULL;

			for(i=0; i<MapTexture::IdMax; ++i)
				textures[i]=NULL;

			for(i=0; i<MapItem::IdMax; ++i)
				items[i]=NULL;

			// Create directory strings.
			size_t mapBaseDirPathLen=strlen(mapBaseDirPath);
			baseDir=(char *)malloc(mapBaseDirPathLen+1); // TODO: Check return.
			strcpy(baseDir, mapBaseDirPath);

			const char *texturesDirName="textures";
			size_t texturesDirPathLen=mapBaseDirPathLen+1+strlen(texturesDirName); // +1 is for '/'
			texturesDir=(char *)malloc(texturesDirPathLen+1); // TODO: check return
			sprintf(texturesDir, "%s/%s", mapBaseDirPath, texturesDirName);

			const char *itemsDirName="items";
			size_t itemsDirPathLen=mapBaseDirPathLen+1+strlen(itemsDirName); // +1 is for '/'
			itemsDir=(char *)malloc(itemsDirPathLen+1); // TODO: check return
			sprintf(itemsDir, "%s/%s", mapBaseDirPath, itemsDirName);

			const char *regionsDirName="regions";
			size_t regionsDirPathLen=mapBaseDirPathLen+1+strlen(regionsDirName); // +1 is for '/'
			regionsDir=(char *)malloc(regionsDirPathLen+1); // TODO: check return
			sprintf(regionsDir, "%s/%s", mapBaseDirPath, regionsDirName);

			const char *mapTiledDirName="maptiled";
			size_t mapTiledDirPathLen=mapBaseDirPathLen+1+strlen(mapTiledDirName); // +1 is for '/'
			mapTiledDir=(char *)malloc(mapTiledDirPathLen+1); // TODO: check return
			sprintf(mapTiledDir, "%s/%s", mapBaseDirPath, mapTiledDirName);

			// Create map
			// Note: we do this here instead of with the other directories in createMetadata because otherwise we would not be able to create the lock file below.
			if (Util::isDir(mapBaseDirPath))
				throw std::runtime_error("map already exists");

			if (!Util::makeDir(mapBaseDirPath))
				throw std::runtime_error("could not create map base dir");

			// Attempt to obtain the lock file
			char lockPath[1024]; // TODO: better
			sprintf(lockPath, "%s/lock", mapBaseDirPath);
			lockFd=open(lockPath, O_RDWR|O_CREAT|O_EXCL, S_IWUSR);
			if (lockFd==-1)
				throw std::runtime_error("locked");

			// Create metadata and region directories etc.
			saveMetadata();
		}

		Map::Map(const char *mapBaseDirPath, bool ignoreLock) {
			// Load map
			DIR *dirFd;
			struct dirent *dirEntry;

			// Set Map to clean state.
			unsigned i, j;

			lockFd=-1;

			baseDir=NULL;
			texturesDir=NULL;
			itemsDir=NULL;
			regionsDir=NULL;
			mapTiledDir=NULL;

			regionsCount=0;
			for(i=0; i<regionsLoadedMax; ++i)
				regionsByIndex[i]=NULL;
			for(i=0; i<regionsLoadedMax; ++i)
				regionsByAge[i]=NULL;
			for(i=0; i<regionsSize; ++i)
				for(j=0; j<regionsSize; ++j)
					regionsByOffset[i][j].ptr=NULL;

			for(i=0; i<MapTexture::IdMax; ++i)
				textures[i]=NULL;

			for(i=0; i<MapItem::IdMax; ++i)
				items[i]=NULL;

			// Create directory strings.
			size_t mapBaseDirPathLen=strlen(mapBaseDirPath);
			baseDir=(char *)malloc(mapBaseDirPathLen+1); // TODO: Check return.
			strcpy(baseDir, mapBaseDirPath);

			const char *texturesDirName="textures";
			size_t texturesDirPathLen=mapBaseDirPathLen+1+strlen(texturesDirName); // +1 is for '/'
			texturesDir=(char *)malloc(texturesDirPathLen+1); // TODO: check return
			sprintf(texturesDir, "%s/%s", mapBaseDirPath, texturesDirName);

			const char *itemsDirName="items";
			size_t itemsDirPathLen=mapBaseDirPathLen+1+strlen(itemsDirName); // +1 is for '/'
			itemsDir=(char *)malloc(itemsDirPathLen+1); // TODO: check return
			sprintf(itemsDir, "%s/%s", mapBaseDirPath, itemsDirName);

			const char *regionsDirName="regions";
			size_t regionsDirPathLen=mapBaseDirPathLen+1+strlen(regionsDirName); // +1 is for '/'
			regionsDir=(char *)malloc(regionsDirPathLen+1); // TODO: check return
			sprintf(regionsDir, "%s/%s", mapBaseDirPath, regionsDirName);

			const char *mapTiledDirName="maptiled";
			size_t mapTiledDirPathLen=mapBaseDirPathLen+1+strlen(mapTiledDirName); // +1 is for '/'
			mapTiledDir=(char *)malloc(mapTiledDirPathLen+1); // TODO: check return
			sprintf(mapTiledDir, "%s/%s", mapBaseDirPath, mapTiledDirName);

			// Check map exists
			if (!Util::isDir(mapBaseDirPath))
				throw std::runtime_error("no such map");

			// Attempt to obtain the lock file
			char lockPath[1024]; // TODO: better
			sprintf(lockPath, "%s/lock", mapBaseDirPath);

			if (ignoreLock)
				unlink(lockPath);

			lockFd=open(lockPath, O_RDWR|O_CREAT|O_EXCL, S_IWUSR);
			if (lockFd==-1)
				throw std::runtime_error("locked");

			// Load metadata file
			char metadataFilePath[1024]; // TODO: Prevent overflows.
			sprintf(metadataFilePath, "%s/metadata", baseDir);
			FILE *metadataFile=fopen(metadataFilePath, "r");
			if (metadataFile!=NULL) {
				fread(&mapWidth, sizeof(double), 1, metadataFile);
				fread(&mapHeight, sizeof(double), 1, metadataFile);
				fread(&minHeight, sizeof(double), 1, metadataFile);
				fread(&maxHeight, sizeof(double), 1, metadataFile);
				fread(&minTemperature, sizeof(double), 1, metadataFile);
				fread(&maxTemperature, sizeof(double), 1, metadataFile);
				fread(&minMoisture, sizeof(double), 1, metadataFile);
				fread(&maxMoisture, sizeof(double), 1, metadataFile);
				fread(&seaLevel, sizeof(double), 1, metadataFile);
				fread(&alpineLevel, sizeof(double), 1, metadataFile);
				fread(&forestLevel, sizeof(double), 1, metadataFile);

				fclose(metadataFile);
			}

			// Ensure all directories etc exist.
			// Note: shouldn't really be needed but no harm either
			saveMetadata();

			// Load textures
			dirFd=opendir(getTexturesDir());
			if (dirFd==NULL) {
				throw std::runtime_error("could not open map texture dir");
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

					unsigned textureId=0, textureScale=0, textureMapR=0, textureMapG=0, textureMapB=0;
					if (sscanf(namePtr, "%us%ur%ug%ub%u.", &textureId, &textureScale, &textureMapR, &textureMapG, &textureMapB)!=5) {
						// TODO: error msg
						continue;
					}
					if (textureScale==0) {
						// TODO: error msg
						continue;
					}

					// Add texture.
					MapTexture *texture=new MapTexture(textureId, dirEntryFileName, textureScale, textureMapR, textureMapG, textureMapB);
					addTexture(texture); // TODO: Check return.
				}

				closedir(dirFd);
			}

			// Load items.
			dirFd=opendir(getItemsDir());
			if (dirFd==NULL) {
				throw std::runtime_error("could not open map item dir");
			} else {
				while((dirEntry=readdir(dirFd))!=NULL) {
					char dirEntryFileName[1024]; // TODO: this better
					sprintf(dirEntryFileName , "%s/%s", getItemsDir(), dirEntry->d_name);

					struct stat stbuf;
					if (stat(dirEntryFileName,&stbuf)==-1)
						continue;

					// Skip non-regular files.
					if ((stbuf.st_mode & S_IFMT)!=S_IFREG)
						continue;

					// Attempt to decode filename as an item id.
					char *idPtr=strrchr(dirEntryFileName, '/');
					if (idPtr!=NULL)
						idPtr++;
					else
						idPtr=dirEntryFileName;

					MapItem::Id itemId=atoi(idPtr);

					FILE *itemFile=fopen(dirEntryFileName, "r");
					if (itemFile==NULL)
						continue;

					char itemName[4096]; // TODO: this better
					if (fgets(itemName, 4096, itemFile)==NULL) {
						fclose(itemFile);
						continue;
					}
					if (itemName[strlen(itemName)-1]=='\n')
						itemName[strlen(itemName)-1]='\0'; // trim newline

					fclose(itemFile);

					// Add item.
					MapItem *item=new MapItem(itemId, itemName);
					addItem(item); // TODO: Check return.
				}

				closedir(dirFd);
			}

			// Note: Regions are loaded on demand.
		}

		Map::~Map() {
			unsigned i;

			// Ensure any changes are saved (including stuff like metadata and regions)
			save();

			// Remove regions.
			while(regionsCount>0)
				regionUnload(regionsCount-1);

			// Remove textures.
			for(i=0; i<MapTexture::IdMax; ++i)
				removeTexture(i);

			// Remove items.
			for(i=0; i<MapItem::IdMax; ++i)
				removeItem(i);

			// Close and remove lock file
			if (lockFd!=-1)
				close(lockFd);
			if (baseDir!=NULL) {
				char lockPath[1024]; // TODO: better
				sprintf(lockPath, "%s/lock", baseDir);
				unlink(lockPath);
			}

			// Free paths.
			free(baseDir);
			baseDir=NULL;
			free(texturesDir);
			texturesDir=NULL;
			free(itemsDir);
			itemsDir=NULL;
			free(regionsDir);
			regionsDir=NULL;
			free(mapTiledDir);
			mapTiledDir=NULL;
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

			// Save all items.
			if (!saveItems())
				return false;

			return true;
		}

		bool Map::saveMetadata(void) const {
			// Do we need to create regions directory?
			const char *regionsDirPath=getRegionsDir();
			if (!Util::isDir(regionsDirPath)) {
				if (!Util::makeDir(regionsDirPath)) {
					fprintf(stderr,"error: could not create map regions dir at '%s'\n", regionsDirPath);
					return false;
				}
			}

			// Do we need to create textures directory?
			const char *texturesDirPath=getTexturesDir();
			if (!Util::isDir(texturesDirPath)) {
				if (!Util::makeDir(texturesDirPath)) {
					fprintf(stderr,"error: could not create map textures dir at '%s'\n", texturesDirPath);
					return false;
				}
			}

			// Do we need to create items directory?
			const char *itemsDirPath=getItemsDir();
			if (!Util::isDir(itemsDirPath)) {
				if (!Util::makeDir(itemsDirPath)) {
					fprintf(stderr,"error: could not create map items dir at '%s'\n", itemsDirPath);
					return false;
				}
			}

			// Do we need to create map tiled directory?
			const char *mapTiledDirPath=getMapTiledDir();
			if (!Util::isDir(mapTiledDirPath)) {
				if (!Util::makeDir(mapTiledDirPath)) {
					fprintf(stderr,"error: could not create map mapTiled dir at '%s'\n", mapTiledDirPath);
					return false;
				}
				if (!MapTiled::createMetadata(this)) {
					fprintf(stderr,"error: could not create map mapTiled metadata (within '%s')\n", mapTiledDirPath);
					return false;
				}
			}

			// Copy slippymap files if needed
			char slippymapPath[1024]; // TODO: better

			sprintf(slippymapPath, "%s/leaflet.css", baseDir);
			if (!Util::isFile(slippymapPath) && !std::filesystem::copy_file("../src/slippymap/leaflet.css", slippymapPath)) {
				fprintf(stderr,"error: could not copy slippymap leaflet.css file to '%s'\n", slippymapPath);
				return false;
			}

			sprintf(slippymapPath, "%s/leaflet.js", baseDir);
			if (!Util::isFile(slippymapPath) && !std::filesystem::copy_file("../src/slippymap/leaflet.js", slippymapPath)) {
				fprintf(stderr,"error: could not copy slippymap leaflet.js file to '%s'\n", slippymapPath);
				return false;
			}

			sprintf(slippymapPath, "%s/slippymap.html", baseDir);
			if (!Util::isFile(slippymapPath) && !std::filesystem::copy_file("../src/slippymap/slippymap.html", slippymapPath)) {
				fprintf(stderr,"error: could not copy slippymap slippymap.html file to '%s'\n", slippymapPath);
				return false;
			}

			sprintf(slippymapPath, "%s/images", baseDir);
			if (!Util::isDir(slippymapPath) && !Util::makeDir(slippymapPath)) {
				fprintf(stderr,"error: could not create slippymap images directory to '%s'\n", slippymapPath);
				return false;
			}

			sprintf(slippymapPath, "%s/images/layers.png", baseDir);
			if (!Util::isFile(slippymapPath) && !std::filesystem::copy_file("../src/slippymap/images/layers.png", slippymapPath)) {
				fprintf(stderr,"error: could not copy slippymap layers.png image file to '%s'\n", slippymapPath);
				return false;
			}

			sprintf(slippymapPath, "%s/images/layers-2x.png", baseDir);
			if (!Util::isFile(slippymapPath) && !std::filesystem::copy_file("../src/slippymap/images/layers-2x.png", slippymapPath)) {
				fprintf(stderr,"error: could not copy slippymap layers-2x.png image file to '%s'\n", slippymapPath);
				return false;
			}

			sprintf(slippymapPath, "%s/images/marker-icon.png", baseDir);
			if (!Util::isFile(slippymapPath) && !std::filesystem::copy_file("../src/slippymap/images/marker-icon.png", slippymapPath)) {
				fprintf(stderr,"error: could not copy slippymap marker-icon.png image file to '%s'\n", slippymapPath);
				return false;
			}

			sprintf(slippymapPath, "%s/images/marker-icon-2x.png", baseDir);
			if (!Util::isFile(slippymapPath) && !std::filesystem::copy_file("../src/slippymap/images/marker-icon-2x.png", slippymapPath)) {
				fprintf(stderr,"error: could not copy slippymap marker-icon-2x.png image file to '%s'\n", slippymapPath);
				return false;
			}

			sprintf(slippymapPath, "%s/images/marker-shadow.png", baseDir);
			if (!Util::isFile(slippymapPath) && !std::filesystem::copy_file("../src/slippymap/images/marker-shadow.png", slippymapPath)) {
				fprintf(stderr,"error: could not copy slippymap marker-shadow.png image file to '%s'\n", slippymapPath);
				return false;
			}

			// Write metadata file.
			char metadataFilePath[1024]; // TODO: Prevent overflows.
			sprintf(metadataFilePath, "%s/metadata", baseDir);
			FILE *metadataFile=fopen(metadataFilePath, "w");
			if (metadataFile==NULL)
				return false;

			bool result=true;
			result&=(fwrite(&mapWidth, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&mapHeight, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&minHeight, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&maxHeight, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&minTemperature, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&maxTemperature, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&minMoisture, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&maxMoisture, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&seaLevel, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&alpineLevel, sizeof(double), 1, metadataFile)==1);
			result&=(fwrite(&forestLevel, sizeof(double), 1, metadataFile)==1);

			fclose(metadataFile);

			return result;
		}

		bool Map::saveTextures(void) const {
			bool success=true;

			// Save all textures.
			const char *texturesDirPath=getTexturesDir();

			unsigned textureId;
			for(textureId=0; textureId<MapTexture::IdMax; ++textureId) {
				// Grab texture.
				const MapTexture *texture=getTexture(textureId);
				if (texture==NULL)
					continue;

				// Save texture.
				success&=texture->save(texturesDirPath);
			}

			return success;
		}

		bool Map::saveItems(void) const {
			bool success=true;

			// Save all items.
			const char *itemsDirPath=getItemsDir();

			unsigned itemId;
			for(itemId=0; itemId<MapItem::IdMax; ++itemId) {
				// Grab item.
				const MapItem *item=getItem(itemId);
				if (item==NULL)
					continue;

				// Save item.
				success&=item->save(itemsDirPath);
			}

			return success;
		}

		bool Map::saveRegions(void) {
			bool success=true;

			// Save all regions.
			const char *regionsDirPath=getRegionsDir();

			regionsLock.lock();

			for(unsigned i=0; i<regionsCount; ++i) {
				// Grab region.
				MapRegion *region=getRegionAtIndex(i);

				// Is the region even dirty?
				if (!region->getIsDirty())
					continue;

				// Save region.
				success&=region->save(regionsDirPath, regionsByIndex[i]->offsetX, regionsByIndex[i]->offsetY);
			}

			regionsLock.unlock();

			return success;
		}

		bool Map::loadRegion(unsigned regionX, unsigned regionY, const char *regionPath) {
			assert(regionX<regionsSize && regionY<regionsSize);
			assert(regionX*MapRegion::tilesSize<mapWidth && regionY*MapRegion::tilesSize<mapHeight);
			assert(regionPath!=NULL);

			// Grab lock
			regionsLock.lock();

			// Do we need to evict a region to make space for the new one?
			assert(regionsCount<=regionsLoadedMax);
			if (regionsCount==regionsLoadedMax) {
				// Find the last-recently used region.
				RegionData *regionData=regionsByAge[regionsCount-1];
				assert(regionData!=NULL);
				MapRegion *region=regionData->ptr;

				// If this region is dirty, save it back to disk.
				if (region->getIsDirty() && !region->save(getRegionsDir(), regionData->offsetX, regionData->offsetY)) {
					// Unable to save modified region - abort to avoid losing data
					regionsLock.unlock();
					return false;
				}

				// Unload the region.
				regionUnload(regionData->index);
			}

			// Create new blank region.
			MapRegion *region=new MapRegion(regionX, regionY);
			if (region==NULL) {
				regionsLock.unlock();
				return false;
			}

			// Add region to map.
			assert(regionsCount<regionsLoadedMax);
			assert(regionsByOffset[regionY][regionX].ptr==NULL);

			regionsByOffset[regionY][regionX].ptr=region;
			regionsByOffset[regionY][regionX].index=regionsCount;
			regionsByOffset[regionY][regionX].offsetX=regionX;
			regionsByOffset[regionY][regionX].offsetY=regionY;
			regionsByIndex[regionsCount]=&(regionsByOffset[regionY][regionX]);

			memmove(regionsByAge+1, regionsByAge, (regionsCount)*sizeof(RegionData *));
			regionsByAge[0]=regionsByIndex[regionsCount];

			++regionsCount;

			// Release lock
			regionsLock.unlock();

			// Attempt to load region data from file.
			return region->load(regionPath);
		}

		bool Map::markRegionDirtyAtTileOffset(unsigned offsetX, unsigned offsetY, bool create) {
			assert(offsetX>=0 && offsetX<regionsSize*MapRegion::tilesSize);
			assert(offsetY>=0 && offsetY<regionsSize*MapRegion::tilesSize);

			unsigned regionX=offsetX/MapRegion::tilesSize;
			unsigned regionY=offsetY/MapRegion::tilesSize;
			MapRegion *region=getRegionAtOffset(regionX, regionY, create);
			if (region==NULL)
				return false;

			region->setDirty();

			return true;
		}

		void Map::tick(void) {
			// Call region tick on each loaded region.
			for(unsigned i=0; i<regionsCount; ++i) {
				MapRegion *region=regionsByIndex[i]->ptr;

				// TODO: Move this logic into MapRegion itself so that objects list can be made private.
				// TODO: be careful as objects could be removed from our object list if we move them into a different region
				for(unsigned i=0; i<region->objects.size(); i++) {
					CoordVec delta=region->objects[i]->tick();
					moveObject(region->objects[i], region->objects[i]->getCoordTopLeft()+delta);
				}
			}
		}

		unsigned Map::getWidth(void) const {
			return mapWidth;
		}

		unsigned Map::getHeight(void) const {
			return mapHeight;
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
			assert(offsetX>=0 && offsetX<regionsSize*MapRegion::tilesSize);
			assert(offsetY>=0 && offsetY<regionsSize*MapRegion::tilesSize);

			unsigned regionX=offsetX/MapRegion::tilesSize;
			unsigned regionY=offsetY/MapRegion::tilesSize;
			MapRegion *region=getRegionAtOffset(regionX, regionY, (flags & GetTileFlag::Create)!=0);
			if (region==NULL)
				return NULL;

			if (flags & GetTileFlag::Dirty)
				region->setDirty();

			unsigned regionTileOffsetX=offsetX%MapRegion::tilesSize;
			unsigned regionTileOffsetY=offsetY%MapRegion::tilesSize;
			return region->getTileAtOffset(regionTileOffsetX, regionTileOffsetY);
		}

		MapRegion *Map::getRegionAtCoordVec(const CoordVec &vec, bool create) {
			if (vec.x<0 || vec.y<0)
				return NULL;

			CoordComponent tileX=vec.x/Physics::CoordsPerTile;
			CoordComponent tileY=vec.y/Physics::CoordsPerTile;
			CoordComponent regionX=tileX/MapRegion::tilesSize;
			CoordComponent regionY=tileY/MapRegion::tilesSize;

			return getRegionAtOffset(regionX, regionY, create);
		}

		MapRegion *Map::getRegionAtOffset(unsigned regionX, unsigned regionY, bool create) {
			// Out of bounds?
			if (regionX*MapRegion::tilesSize>=mapWidth || regionY*MapRegion::tilesSize>=mapHeight)
				return NULL;

			// Region not loaded?
			if (regionsByOffset[regionY][regionX].ptr==NULL) {
				// Create path.
				const char *regionsDirPath=getRegionsDir();
				char regionPath[4096]; // TODO: this better
				sprintf(regionPath, "%s/%u,%u", regionsDirPath, regionX, regionY); // TODO: Check return.

				// Attempt to load region.
				if (!loadRegion(regionX, regionY, regionPath)) {
					if (!create) {
						// Failed - ensure this region is unloaded as it is not correct.
						for(unsigned r=0; r<regionsCount; ++r)
							if (regionsByIndex[r]->offsetX==regionX && regionsByIndex[r]->offsetY==regionY) {
								regionUnload(r);
								break;
							}
					}
				}
			}

			// Return region (or NULL if we could not load/create).
			return regionsByOffset[regionY][regionX].ptr;
		}

		bool Map::addObject(MapObject *object) {
			assert(object!=NULL);

			CoordVec vec=object->getCoordTopLeft();
			MapRegion *region=getRegionAtCoordVec(vec, false);
			if (region==NULL)
				return false;

			return region->addObject(object);
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

			// Remove from old region.
			MapRegion *oldRegion=getRegionAtCoordVec(oldVec1, false);
			oldRegion->disownObject(object);

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

				// Add to old region.
				oldRegion->ownObject(object);

				return false;
			}

			// Add to new tiles.
			for(vec.y=newVec1.y; vec.y<=newVec2.y; vec.y+=Physics::CoordsPerTile)
				for(vec.x=newVec1.x; vec.x<=newVec2.x; vec.x+=Physics::CoordsPerTile)
					getTileAtCoordVec(vec, GetTileFlag::Dirty)->addObject(object);

			// Add to new region.
			MapRegion *newRegion=getRegionAtCoordVec(newVec1, false);
			newRegion->ownObject(object);

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

		bool Map::addItem(MapItem *item) {
			assert(item!=NULL);

			// Ensure this id is free.
			if (items[item->getId()]!=NULL)
				return false;

			// Create item and add to array.
			items[item->getId()]=item;

			return (items[item->getId()]!=NULL);
		}

		void Map::removeItem(unsigned id) {
			assert(id<MapItem::IdMax);

			// If there is a item here, free it and set entry to NULL.
			if (items[id]!=NULL) {
				delete items[id];
				items[id]=NULL;
			}
		}

		const MapItem *Map::getItem(unsigned id) const {
			assert(id<MapItem::IdMax);

			return items[id];
		}

		const char *Map::getBaseDir(void) const {
			return baseDir;
		}

		const char *Map::getMapTiledDir(void) const {
			return mapTiledDir;
		}

		unsigned Map::wrappingDistX(unsigned x1, unsigned x2) {
			return Util::wrappingDistX(x1, x2, getWidth());
		}

		unsigned Map::wrappingDistY(unsigned y1, unsigned y2) {
			return Util::wrappingDistY(y1, y2, getHeight());
		}

		unsigned Map::wrappingDist(unsigned x1, unsigned y1, unsigned x2, unsigned y2) {
			return Util::wrappingDist(x1, y1, x2, y2, getWidth(), getHeight());
		}

		unsigned Map::addTileOffsetX(unsigned offsetX, int dx) {
			return Util::addTileOffsetX(offsetX, dx, getWidth());
		}

		unsigned Map::addTileOffsetY(unsigned offsetY, int dy) {
			return Util::addTileOffsetY(offsetY, dy, getHeight());
		}

		const char *Map::getRegionsDir(void) const {
			return regionsDir;
		}

		const char *Map::getTexturesDir(void) const {
			return texturesDir;
		}

		const char *Map::getItemsDir(void) const {
			return itemsDir;
		}

		MapRegion *Map::getRegionAtIndex(unsigned index) {
			assert(index<regionsCount);

			return regionsByIndex[index]->ptr;
		}

		const MapRegion *Map::getRegionAtIndex(unsigned index) const {
			assert(index<regionsCount);

			return regionsByIndex[index]->ptr;
		}

		void Map::updateRegionAge(const MapRegion *region) {
			assert(region!=NULL);

			// See if this region is already the newest (as is common when performing many operations within a single region)
			if (regionsByAge[0]->ptr==region)
				return;

			// Update age array
			regionsLock.lock();

			for(int i=0; i<regionsCount; i++) {
				if (regionsByAge[i]->ptr==region) {
					// Found - move to front.
					RegionData *regionDataPtr=regionsByAge[i];
					memmove(regionsByAge+1, regionsByAge, i*sizeof(RegionData *));
					regionsByAge[0]=regionDataPtr;
					regionsLock.unlock();
					return;
				}
			}
			assert(false);

			regionsLock.unlock();
		}

		void Map::regionUnload(unsigned index) {
			assert(index<regionsCount);

			// Remove from regionsByAge array.
			for(unsigned i=0; i<regionsCount; ++i) {
				if (regionsByAge[i]==regionsByIndex[index]) {
					memmove(regionsByAge+i, regionsByAge+i+1, (regionsCount-1-i)*sizeof(RegionData *));
					regionsByAge[regionsCount-1]=NULL;
					break;
				}
			}

			// Free the region and clear the RegionData pointer.
			MapRegion *region=getRegionAtIndex(index);
			delete region;

			regionsByIndex[index]->ptr=NULL;

			// Copy last array element into this gap and update its index.
			regionsByIndex[index]=regionsByIndex[regionsCount-1];
			regionsByIndex[index]->index=index;

			// Decrement index next.
			--regionsCount;
		}
	};
};
