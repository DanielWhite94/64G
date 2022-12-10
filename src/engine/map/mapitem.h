#ifndef ENGINE_GRAPHICS_MAPITEM_H
#define ENGINE_GRAPHICS_MAPITEM_H

#include <cstdint>

namespace Engine {
	namespace Map {
		class MapItem {
		public:
			typedef uint16_t Id;
			static const unsigned IdMax=((1u)<<16)-1;

			MapItem(unsigned id, const char *name);
			~MapItem();

			bool save(const char *itemsDirPath) const;

			unsigned getId(void) const;
			const char *getName(void) const;

		private:
			unsigned id;
			char *name;
		};
	};
};

#endif
