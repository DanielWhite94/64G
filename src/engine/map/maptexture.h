#ifndef ENGINE_GRAPHICS_MAPTEXTURE_H
#define ENGINE_GRAPHICS_MAPTEXTURE_H

namespace Engine {
	namespace Map {
		class MapTexture {
		public:
			static const unsigned IdMax=((1u)<<16);

			MapTexture(unsigned id, const char *path, unsigned scale);
			~MapTexture();
		private:
		};
	};
};

#endif
