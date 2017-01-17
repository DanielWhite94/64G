#ifndef ENGINE_PHYSICS_COORDS_H
#define ENGINE_PHYSICS_COORDS_H

namespace Engine {
	namespace Physics {
		typedef unsigned CoordComponent;

		class CoordVec {
		public:
			union {
				struct { CoordComponent c[2]; };
				struct { CoordComponent x, y; };
			};

			CoordVec();
			CoordVec(CoordComponent x, CoordComponent y);
			~CoordVec();

			CoordComponent getX(void) const;
			CoordComponent getY(void) const;

			void setX(CoordComponent x);
			void setY(CoordComponent y);
		};
	};
};

#endif
