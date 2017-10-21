#ifndef ENGINE_GRAPHICS_RENDERER_H
#define ENGINE_GRAPHICS_RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "camera.h"
#include "texture.h"
#include "../map/map.h"
#include "../map/maptexture.h"
#include "../physics/coord.h"

using namespace Engine::Map;

namespace Engine {
	namespace Graphics {
		class Renderer {
		public:
			bool drawTileGrid; // Draw a grid showing the tile structure.
			bool drawCoordGrid; // Draw a grid showing the coordinate structure.
			bool drawHitMasksActive;
			bool drawHitMasksInactive;
			bool drawHitMasksIntersections;

			Renderer(unsigned windowWidth, unsigned windowHeight);
			~Renderer();

			void refresh(const Engine::Graphics::Camera *camera, class Map *map);
		private:
			int windowWidth, windowHeight;

			SDL_Window *window;
			SDL_Renderer *renderer;

			CoordVec topLeft, bottomRight;

			const Camera *camera;
			class Map *map;

			Texture *textures[MapTexture::IdMax];

			void renderGrid(const CoordVec &coordDelta); // Set colour before calling.
			void renderHitMask(Engine::Physics::HitMask hitmask, int sx, int sy, int r, int g, int b);

			const Texture *getTexture(const class Map &map, MapTexture::Id textureId);
		};
	};
};

#endif
