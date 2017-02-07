#include <cassert>

#include "camera.h"

namespace Engine {
	namespace Graphics {
		Camera::Camera(const CoordVec &gPos, int gZoom) {
			pos=gPos;
			zoom=gZoom;
		}

		Camera::~Camera() {
		}

		CoordComponent Camera::getX(void) const {
			return pos.x;
		}

		CoordComponent Camera::getY(void) const {
			return pos.y;
		}

		const CoordVec &Camera::getVec(void) const {
			return pos;
		}

		int Camera::getZoom(void) const {
			return zoom;
		}

		void Camera::setVec(const CoordVec &gPos) {
			pos=gPos;
		}

		void Camera::setZoom(int gZoom) {
			zoom=gZoom;
		}

		int Camera::coordXToScreenXOffset(CoordComponent coordX) const {
			return (coordX-getX())*getZoom();
		}

		int Camera::coordYToScreenYOffset(CoordComponent coordY) const {
			return (coordY-getY())*getZoom();
		}

		int Camera::coordLengthToScreenLength(CoordComponent coordLen) const {
			return coordLen*getZoom();
		}

		CoordComponent Camera::screenXOffsetToCoordX(int screenXOffset) const {
			return screenXOffset/getZoom()+getX();
		}

		CoordComponent Camera::screenYOffsetToCoordY(int screenYOffset) const {
			return screenYOffset/getZoom()+getY();
		}

		CoordComponent Camera::screenLengthToCoordLength(int screenLen) const {
			return screenLen/getZoom();
		}
	};
};
