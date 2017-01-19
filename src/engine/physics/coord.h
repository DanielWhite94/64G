#ifndef ENGINE_PHYSICS_COORDS_H
#define ENGINE_PHYSICS_COORDS_H

namespace Engine {
	namespace Physics {
		typedef int CoordComponent;

		class CoordVec {
		public:
			CoordComponent x, y;

			CoordVec();
			CoordVec(CoordComponent x, CoordComponent y);
			~CoordVec();

			CoordComponent getX(void) const;
			CoordComponent getY(void) const;

			void setX(CoordComponent x);
			void setY(CoordComponent y);

			CoordVec &operator+=(const CoordVec &rhs) {
				this->x+=rhs.x;
				this->y+=rhs.y;
				return *this;
			}

			friend CoordVec operator+(CoordVec lhs, const CoordVec &rhs) {
				lhs+=rhs;
				return lhs;
			}
		};
	};
};

#endif
