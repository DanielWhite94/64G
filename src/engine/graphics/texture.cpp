#include "texture.h"

namespace Engine {
	namespace Graphics {
		Texture::Texture(SDL_Renderer *renderer, const char *path) {
			SDL_Surface *image=IMG_Load(path);
			texture=SDL_CreateTextureFromSurface(renderer, image);
			SDL_FreeSurface(image);
		}

		Texture::~Texture() {
			SDL_DestroyTexture(texture);
		}

		const SDL_Texture *Texture::getTexture(void) const {
			return texture;
		}
	};
};
