#include <cassert>

#include "camera.h"

namespace Engine {
	namespace Graphics {
		Camera::Camera(int x, int y, int zoom) {
			type=Camera::Type::Fixed;
			d.fixed.x=x;
			d.fixed.y=y;
			d.fixed.zoom=zoom;
		}

		Camera::~Camera() {
		}

		int Camera::getX(void) const {
			switch(type) {
				case Camera::Type::Fixed:
					return d.fixed.x;
				break;
			}

			assert(false);
			return 0;
		}

		int Camera::getY(void) const {
			switch(type) {
				case Camera::Type::Fixed:
					return d.fixed.y;
				break;
			}

			assert(false);
			return 0;
		}

		int Camera::getZoom(void) const {
			switch(type) {
				case Camera::Type::Fixed:
					return d.fixed.zoom;
				break;
			}

			assert(false);
			return 1;
		}

		int Camera::coordXToScreenXOffset(int coordX) const {
			return (coordX-getX())*getZoom();
		}

		int Camera::coordYToScreenYOffset(int coordY) const {
			return (coordY-getY())*getZoom();
		}

		int Camera::coordLengthToScreenLength(int coordLen) const {
			return coordLen*getZoom();
		}

		int Camera::screenXOffsetToCoordX(int screenXOffset) const {
			return screenXOffset/getZoom()+getX();
		}

		int Camera::screenYOffsetToCoordY(int screenYOffset) const {
			return screenYOffset/getZoom()+getY();
		}

		int Camera::screenLengthToCoordLength(int screenLen) const {
			return screenLen/getZoom();
		}
	};
};
