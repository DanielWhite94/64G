#ifndef ENGINE_FBNNOISE_H
#define ENGINE_FBNNOISE_H

#include "opensimplexnoise.h"

namespace Engine {
	class FbnNoise {
	public:
		// Octaves: Number of layers of base noise to combine.
		// Frequency: Initial frequency.
		FbnNoise(unsigned seed, unsigned octaves, double frequency);
		~FbnNoise();

		double eval(double x, double y);
	private:
		OpenSimplexNoise *baseNoise;
		unsigned octaves;
		double frequency;
	};
};

#endif
