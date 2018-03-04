#include "coord.h"

namespace Engine {
	namespace Physics {
		CoordComponent CoordVec::getX(void) const {
			return x;
		}

		CoordComponent CoordVec::getY(void) const {
			return y;
		}

		void CoordVec::setX(CoordComponent gX) {
			x=gX;
		}

		void CoordVec::setY(CoordComponent gY) {
			y=gY;
		}

	};
};
