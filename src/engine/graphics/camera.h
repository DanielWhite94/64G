#ifndef ENGINE_GRAPHICS_CAMERA_H
#define ENGINE_GRAPHICS_CAMERA_H

namespace Engine {
	namespace Graphics {
		class Camera {
			enum class Type {
				Fixed
			};

		public:
			Camera::Type type;
			Camera(int x, int y, int zoom); // Create a fixed camera.
			~Camera();

			int getX(void) const;
			int getY(void) const;
			int getZoom(void) const;

			int coordXToScreenXOffset(int coordX) const;
			int coordYToScreenYOffset(int coordY) const;
			int coordLengthToScreenLength(int coordLen) const;

			int screenXOffsetToCoordX(int screenXOffset) const;
			int screenYOffsetToCoordY(int screenYOffset) const;
			int screenLengthToCoordLength(int screenLen) const;
		private:
			union {
				struct { int x, y, zoom; } fixed;
			} d;
		};
	};
};

#endif
