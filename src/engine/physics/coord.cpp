#include "coord.h"

namespace Engine {
	namespace Physics {
		CoordVec::CoordVec() {
			setX(0);
			setY(0);
		}

		CoordVec::CoordVec(CoordComponent x, CoordComponent y) {
			setX(x);
			setY(y);
		}

		CoordVec::~CoordVec() {
		}

		CoordComponent CoordVec::getX(void) const {
			return c[0];
		}

		CoordComponent CoordVec::getY(void) const {
			return c[1];
		}

		void CoordVec::setX(CoordComponent x) {
			c[0]=x;
		}

		void CoordVec::setY(CoordComponent y) {
			c[1]=y;
		}

	};
};
