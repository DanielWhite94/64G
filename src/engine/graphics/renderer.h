#ifndef ENGINE_GRAPHICS_RENDERER_H
#define ENGINE_GRAPHICS_RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "camera.h"
#include "../map/map.h"
#include "../physics/coord.h"

namespace Engine {
	namespace Graphics {
		class Renderer {
		public:
			static const int TilesWide=24;
			static const int TilesHigh=18;

			bool drawGrid;

			Renderer(unsigned windowWidth, unsigned windowHeight);
			~Renderer();

			void refresh(const Engine::Graphics::Camera *camera, const Map *map);
		private:
			int windowWidth, windowHeight;

			SDL_Window *window;
			SDL_Renderer *renderer;

			CoordVec topLeft, bottomRight;

			const Camera *camera;
			const Map *map;

			void renderGrid(void);
		};
	};
};

#endif
