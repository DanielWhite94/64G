#ifndef ENGINE_GEN_TOWN_H
#define ENGINE_GEN_TOWN_H

#include "house.h"
#include "../map/map.h"

namespace Engine {
	namespace Gen {
		enum AddTownsShopType {
			Shoemaker,
			Furrier,
			Tailor,
			Barber,
			Jeweler,
			Tavern,
			Carpenters,
			Bakers,
			NB,
		};

		struct AddTownParameters {
			unsigned roadTileLayer;
			unsigned houseDecorationLayer;

			TileTestFunctor *testFunctor;
			void *testFunctorUserData;

			MapTexture::Id textureIdMajorPath;
			MapTexture::Id textureIdMinorPath;
			MapTexture::Id textureIdShopSignNone;
			MapTexture::Id textureIdShopSignCobbler;
		};

		// Note: in houseParams the following fields will be set internally by addTown: roofHeight, doorOffset, chimneyOffset and the ShowDoor flag will be set as needed
		bool addTown(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, const AddTownParameters *params, const AddHouseParameters *houseParams, int townPop);
		bool addTowns(class Map *map, unsigned x0, unsigned y0, unsigned x1, unsigned y1, const AddTownParameters *params, const AddHouseParameters *houseParams, double totalPopulation);
	};
};

#endif
