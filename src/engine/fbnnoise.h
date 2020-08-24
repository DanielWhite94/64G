#ifndef ENGINE_FBNNOISE_H
#define ENGINE_FBNNOISE_H

namespace Engine {
	class FbnNoise {
	public:
		// Octaves: Number of layers of base noise to combine.
		// Frequency: Initial frequency.
		FbnNoise(unsigned seed, unsigned octaves, double frequency);
		~FbnNoise();

		double eval(double x, double y);
	private:
		unsigned octaves;

		double preMultiplyFactor;
	};
};

#endif
