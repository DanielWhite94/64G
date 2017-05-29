#include <cassert>
#include <cstdlib>
#include <cstring>

#include "maptexture.h"

namespace Engine {
	namespace Map {
		MapTexture::MapTexture(unsigned gId, const char *gPath, unsigned gScale) {
			assert(gId<IdMax);
			assert(gPath!=NULL);
			assert(gScale>=1);

			id=gId;
			path=(char *)malloc(strlen(gPath)+1); // TODO: Check return.
			strcpy(path, gPath);
			scale=gScale;
		}

		MapTexture::~MapTexture() {
			free(path);
			path=NULL;
		}

		unsigned MapTexture::getId(void) const {
			return id;
		}

		const char *MapTexture::getImagePath(void) const {
			return path;
		}

		unsigned MapTexture::getScale(void) const {
			return scale;
		}
	};
};
