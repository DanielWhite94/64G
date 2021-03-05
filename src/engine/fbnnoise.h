#ifndef ENGINE_FBNNOISE_H
#define ENGINE_FBNNOISE_H

#include "perlinnoise.h"

namespace Engine {
	class FbnNoise {
	public:
		// Octaves: Number of layers of base noise to combine.
		// Frequency: Initial frequency.
		FbnNoise(unsigned seed, unsigned octaves, double frequency);
		~FbnNoise();

		double eval(double x, double y) const;
	private:
		unsigned octaves;

		double preMultiplyFactor;

		PerlinNoise baseNoise;
	};
};

#endif
