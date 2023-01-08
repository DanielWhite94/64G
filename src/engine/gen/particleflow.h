#ifndef ENGINE_GEN_PARTICLEFLOW_H
#define ENGINE_GEN_PARTICLEFLOW_H

#include "../util.h"
#include "../map/map.h"

namespace Engine {
	namespace Gen {

		class ParticleFlow {
		public:
			ParticleFlow(class Map *map, int erodeRadius, bool incMoisture): map(map), erodeRadius(erodeRadius), incMoisture(incMoisture) {
				double skew=0.8;
				seaLevelExcess=map->minHeight+(map->seaLevel-map->minHeight)*skew;
			}; // Requires the map have seaLevel set.
			~ParticleFlow() {};

			void dropParticles(unsigned x0, unsigned y0, unsigned x1, unsigned y1, double coverage, unsigned threadCount, Util::ProgressFunctor *progressFunctor, void *progressUserData); // Calls dropParticle on random coordinates within the given area, where dropParticle is ran floor(coverage*(x1-x0)*(y1-y0)) times.
			void dropParticle(double x, double y);

		private:
			class Map *map;
			int erodeRadius;
			bool incMoisture;
			double seaLevelExcess;

			double hMap(int x, int y, double unknownValue); // Returns height of tile at (x,y), returning unknownValue if tile is out of bounds or could not be loaded.
			void depositAt(int x, int y, double w, double ds); // Adjusts the height of a tile at the given (x,y). Does nothing if the tile is out of bounds or could not be loaded.
		};

	};
};

#endif
