#ifndef ENGINE_GRAPHICS_MAPTILE_H
#define ENGINE_GRAPHICS_MAPTILE_H

namespace Engine {
	struct MapTileLayer {
		int texture;
	};

	class MapTile {
	public:
		static const unsigned layersMax=16;

		MapTile(int temp);
		~MapTile();

		const MapTileLayer *getLayer(unsigned z) const;
	private:
		MapTileLayer layers[layersMax];
	};
};

#endif
