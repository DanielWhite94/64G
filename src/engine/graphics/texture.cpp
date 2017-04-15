#include <cassert>

#include "texture.h"

namespace Engine {
	namespace Graphics {
		Texture::Texture(SDL_Renderer *renderer, const char *path, int gScale) {
			assert(gScale>=1);

			SDL_Surface *image=IMG_Load(path);
			texture=SDL_CreateTextureFromSurface(renderer, image);
			SDL_FreeSurface(image);

			Uint32 format;
			int access;
			width=height=0;
			SDL_QueryTexture(texture, &format, &access, &width, &height);

			scale=gScale;
		}

		Texture::~Texture() {
			SDL_DestroyTexture(texture);
		}

		const SDL_Texture *Texture::getTexture(void) const {
			return texture;
		}

		const int Texture::getWidth(void) const {
			return width;
		}

		const int Texture::getHeight(void) const {
			return height;
		}

		const int Texture::getScale(void) const {
			return scale;
		}
	};
};
