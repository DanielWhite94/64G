#ifndef ENGINE_MAP_COMMON_H
#define ENGINE_MAP_COMMON_H

#include "../physics/coord.h"
#include "../util.h"

using namespace std;

using namespace Engine;
using namespace Engine::Physics;

namespace Engine {
	namespace Map {

		// Defined here to avoid circular references between MapKingdom and MapLandmass
		typedef uint16_t MapKingdomId;
		static const MapKingdomId MapKingdomIdNone=0;
		static const MapKingdomId MapKingdomIdMax=((1u)<<16)-1;

	};
};

#endif
