#ifndef ENGINE_GRAPHICS_MAPTEXTURE_H
#define ENGINE_GRAPHICS_MAPTEXTURE_H

namespace Engine {
	namespace Map {
		class MapTexture {
		public:
			static const unsigned IdMax=((1u)<<16);

			MapTexture(unsigned id, const char *path, unsigned scale);
			~MapTexture();

			bool save(const char *texturesDirPath) const;

			unsigned getId(void) const;
			const char *getImagePath(void) const;
			unsigned getScale(void) const;
		private:
			unsigned id;
			char *path;
			unsigned scale;
		};
	};
};

#endif
