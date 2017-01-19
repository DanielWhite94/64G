#include "coord.h"

namespace Engine {
	namespace Physics {
		CoordVec::CoordVec() {
			setX(0);
			setY(0);
		}

		CoordVec::CoordVec(CoordComponent gX, CoordComponent gY) {
			setX(gX);
			setY(gY);
		}

		CoordVec::~CoordVec() {
		}

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
