#ifndef COMMON_EMAP_MAPTEXTURE_H
#define COMMON_EMAP_MAPTEXTURE_H

#include <cstdint>

namespace Common {
	namespace EMap {
		class MapTexture {
		public:
			typedef uint16_t Id;
			static const unsigned IdMax=((1u)<<16)-1;

			MapTexture(unsigned id, const char *name, const char *path, unsigned scale, uint8_t mapColourR, uint8_t mapColourG, uint8_t mapColourB);
			~MapTexture();

			bool save(const char *texturesDirPath) const;

			unsigned getId(void) const;
			const char *getName(void) const;
			const char *getImagePath(void) const;
			bool getImagePathIsUpToDate(const char *texturesDirPath) const;
			unsigned getScale(void) const;

			uint8_t getMapColourR(void) const;
			uint8_t getMapColourG(void) const;
			uint8_t getMapColourB(void) const;
		private:
			unsigned id;
			char *name;
			char *path;
			unsigned scale;
			uint8_t mapColourR, mapColourG, mapColourB;

			void genImagePath(char *outPath, const char *texturesDirPath) const;
		};
	};
};

#endif
