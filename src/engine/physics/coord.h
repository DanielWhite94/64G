#ifndef ENGINE_PHYSICS_COORDS_H
#define ENGINE_PHYSICS_COORDS_H

namespace Engine {
	namespace Physics {
		typedef int CoordComponent;

		static int CoordsPerTile=8;

		enum CoordAngle {
			CoordAngle0,
			CoordAngle90,
			CoordAngle180,
			CoordAngle270,
			CoordAngleNB,
		};

		class CoordVec {
		public:
			CoordComponent x, y;

			CoordVec(): x(0), y(0) {};
			CoordVec(CoordComponent x, CoordComponent y): x(x), y(y) {};
			~CoordVec() {};

			CoordComponent getX(void) const;
			CoordComponent getY(void) const;

			void setX(CoordComponent x);
			void setY(CoordComponent y);

			friend CoordVec operator*(CoordVec lhs, const int &rhs) {
				lhs.x*=rhs;
				lhs.y*=rhs;
				return lhs;
			}

			CoordVec &operator+=(const CoordVec &rhs) {
				this->x+=rhs.x;
				this->y+=rhs.y;
				return *this;
			}

			friend CoordVec operator+(CoordVec lhs, const CoordVec &rhs) {
				lhs+=rhs;
				return lhs;
			}

			CoordVec &operator-=(const CoordVec &rhs) {
				this->x-=rhs.x;
				this->y-=rhs.y;
				return *this;
			}

			friend CoordVec operator-(CoordVec lhs, const CoordVec &rhs) {
				lhs-=rhs;
				return lhs;
			}

			friend bool operator==(CoordVec lhs, const CoordVec &rhs) {
				return (lhs.x==rhs.x && lhs.y==rhs.y);
			}
		};
	};
};

#endif
