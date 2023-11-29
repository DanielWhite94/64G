#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "../../engine/client.h"

#include "../common.h"

using namespace Engine;

int main(int argc, char **argv) {
	// Parse arguments.
	if (argc!=4 && argc!=5) {
		printf("Usage: %s mapfile starttilex starttiley [--debug]\n", argv[0]);
		return EXIT_FAILURE;
	}

	const char *path=argv[1];
	int startTileX=atoi(argv[2]);
	int startTileY=atoi(argv[3]);
	bool debug=(argc==5 && strcmp(argv[4], "--debug")==0);

	// Set various constants/parameters.
	const int defaultZoom=4;
	const int maxZoom=8;
	const double fps=30.0;
	const int TilesWide=24;
	const int TilesHigh=18;

	const int windowWidth=(TilesWide*Physics::CoordsPerTile*defaultZoom);
	const int windowHeight=(TilesHigh*Physics::CoordsPerTile*defaultZoom);

	// Initialise engine
	class Client *client;
	try {
		client=new class Client(path, windowWidth, windowHeight, defaultZoom, maxZoom, fps, debug);
	} catch (std::exception& e) {
		std::cout << "Could not initialise client: " << e.what() << '\n';
		return EXIT_FAILURE;
	}

	// Create, add and set player object.
	class Map *map=client->getMap();

	MapObject player(CoordAngle0, CoordVec(startTileX*Physics::CoordsPerTile, startTileY*Physics::CoordsPerTile), 1, 1);
	HitMask playerHitmask;
	const unsigned playerW=4, playerH=6;
	unsigned playerX, playerY;
	for(playerY=(8-playerH)/2; playerY<(8+playerH)/2; ++playerY)
		for(playerX=(8-playerW)/2; playerX<(8+playerW)/2; ++playerX)
			playerHitmask.setXY(playerX, playerY, true);
	player.setHitMaskByTileOffset(0, 0, playerHitmask);
	player.setTextureIdForAngle(CoordAngle0, TextureIdOldManS);
	player.setTextureIdForAngle(CoordAngle90, TextureIdOldManW);
	player.setTextureIdForAngle(CoordAngle180, TextureIdOldManN);
	player.setTextureIdForAngle(CoordAngle270, TextureIdOldManE);

	player.inventoryClear();
	MapObjectItem item;
	item.id=ItemIdCoins;
	player.inventoryAddItem(item);

	if (!map->addObject(&player)) {
		printf("Could not add player object.\n");
		delete map;
		return EXIT_FAILURE;
	}

	client->setPlayerObject(&player);

	// Enter main loop to begin the game
	client->start();

	// Tidy up
	delete client;

	return EXIT_SUCCESS;
}
