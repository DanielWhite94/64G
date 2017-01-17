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
			switch(type) {
				case Camera::Type::Fixed:
					return (coordX-getX())*d.fixed.zoom;
				break;
			}

			assert(false);
			return 0;
		}

		int Camera::coordYToScreenYOffset(int coordY) const {
			switch(type) {
				case Camera::Type::Fixed:
					return (coordY-getY())*d.fixed.zoom;
				break;
			}

			assert(false);
			return 0;
		}

		int Camera::screenXOffsetToCoordX(int screenXOffset) const {
			switch(type) {
				case Camera::Type::Fixed:
					return screenXOffset/d.fixed.zoom+getX();
				break;
			}

			assert(false);
			return 0;
		}

		int Camera::screenYOffsetToCoordY(int screenYOffset) const {
			switch(type) {
				case Camera::Type::Fixed:
					return screenYOffset/d.fixed.zoom+getY();
				break;
			}

			assert(false);
			return 0;
		}
	};
};
