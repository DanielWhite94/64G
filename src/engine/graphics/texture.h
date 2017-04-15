#ifndef ENGINE_GRAPHICS_TEXTURE_H
#define ENGINE_GRAPHICS_TEXTURE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace Engine {
	namespace Graphics {
		class Texture {
		public:
			Texture(SDL_Renderer *renderer, const char *path, int scale); // scale>=1
			~Texture();

			const SDL_Texture *getTexture(void) const;
			const int getWidth(void) const;
			const int getHeight(void) const;
			const int getScale(void) const;
		private:
			SDL_Texture *texture;

			int width, height;
			int scale;
		};
	};
};

#endif
