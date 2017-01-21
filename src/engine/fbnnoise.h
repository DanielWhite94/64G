#ifndef ENGINE_FBNNOISE_H
#define ENGINE_FBNNOISE_H

#include "opensimplexnoise.h"

namespace Engine {
	class FbnNoise {
	public:
		// Octaves: Number of layers of base noise to combine.
		// Frequency: Initial frequency.
		// Amplitude: Initial amplitude.
		// Lacunarity: Multiplied by the frequency every octave, makes the frequency grow (or shrink).
		// Gain: Multiplied by the amplitude every octave. Often 1/lacunarity.
		FbnNoise(unsigned octaves, double frequency, double amplitude, double lacunarity, double gain);
		~FbnNoise();

		double eval(double x, double y);
	private:
		OpenSimplexNoise *baseNoise;
		unsigned octaves;
		double frequency, amplitude, lacunarity, gain;
	};
};

#endif
