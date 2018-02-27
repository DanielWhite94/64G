#ifndef ENGINE_NOISEARRAY_H
#define ENGINE_NOISEARRAY_H

#include "fbnnoise.h"

namespace Engine {
	void noiseArrayProgressFunctorString(unsigned noiseY, unsigned noiseHeight, void *userData);

	class NoiseArray {
	public:
		typedef void (ProgressFunctor)(unsigned noiseY, unsigned noiseHeight, void *userData);

		NoiseArray(unsigned seed, double tileWidth, double tileHeight, unsigned noiseWidth, unsigned noiseHeight, double noiseResolution, unsigned noiseOctaves, unsigned progressDelta, ProgressFunctor *progressFunctor, void *progressUserData);
		~NoiseArray();

		double eval(double x, double y) const;
	private:
		double *array;
		unsigned noiseWidth;
		double xFactor, yFactor;
	};
};

#endif
