#ifndef ENGINE_GRAPHICS_RENDERER_H
#define ENGINE_GRAPHICS_RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "camera.h"
#include "texture.h"
#include "../map/map.h"
#include "../physics/coord.h"

// TODO: this better
const unsigned TextureIdNone=0;
const unsigned TextureIdGrass0=1;
const unsigned TextureIdGrass1=2;
const unsigned TextureIdGrass2=3;
const unsigned TextureIdGrass3=4;
const unsigned TextureIdGrass4=5;
const unsigned TextureIdGrass5=6;
const unsigned TextureIdBrickPath=7;
const unsigned TextureIdDirt=8;
const unsigned TextureIdDock=9;
const unsigned TextureIdWater=10;
const unsigned TextureIdTree1=11;
const unsigned TextureIdTree2=12;
const unsigned TextureIdMan1=13;
const unsigned TextureIdNB=14;

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

			void refresh(const Engine::Graphics::Camera *camera, const class Map *map);
		private:
			int windowWidth, windowHeight;

			SDL_Window *window;
			SDL_Renderer *renderer;

			CoordVec topLeft, bottomRight;

			const Camera *camera;
			const class Map *map;

			Texture *textures[TextureIdNB];

			void renderGrid(const CoordVec &coordDelta); // Set colour before calling.
			void renderHitMask(Engine::Physics::HitMask hitmask, int sx, int sy, int r, int g, int b);
		};
	};
};

#endif
