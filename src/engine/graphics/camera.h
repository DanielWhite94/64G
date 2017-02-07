#ifndef ENGINE_GRAPHICS_CAMERA_H
#define ENGINE_GRAPHICS_CAMERA_H

#include "../physics/coord.h"

using namespace Engine::Physics;

namespace Engine {
	namespace Graphics {
		class Camera {
		public:
			Camera(const CoordVec &pos, int zoom); // Create a fixed camera.
			~Camera();

			CoordComponent getX(void) const;
			CoordComponent getY(void) const;
			const CoordVec &getVec(void) const;
			int getZoom(void) const;

			void setVec(const CoordVec &gPos);
			void setZoom(int zoom);

			int coordXToScreenXOffset(CoordComponent coordX) const;
			int coordYToScreenYOffset(CoordComponent coordY) const;
			int coordLengthToScreenLength(CoordComponent coordLen) const;

			CoordComponent screenXOffsetToCoordX(int screenXOffset) const;
			CoordComponent screenYOffsetToCoordY(int screenYOffset) const;
			CoordComponent screenLengthToCoordLength(int screenLen) const;
		private:
			CoordVec pos;
			int zoom;
		};
	};
};

#endif
