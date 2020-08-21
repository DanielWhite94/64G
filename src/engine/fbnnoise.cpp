#include <cassert>
#include <cmath>

#include "fbnnoise.h"
#include "perlinnoise.h"

namespace Engine {
	FbnNoise::FbnNoise(unsigned seed, unsigned octaves, double frequency): octaves(octaves) {
		assert(octaves>0);

		preMultiplyFactor=frequency*exp2(octaves-1);
	}

	FbnNoise::~FbnNoise() {
	}

	double FbnNoise::eval(double x, double y) {
		// Adjust x/y based on frequency and octaves to ensure tilable (and so that we can loop in 'reverse' below)
		x*=preMultiplyFactor;
		y*=preMultiplyFactor;

		// Loop in reverse so that we start with small (implicit) amplitude, so that we do not lose as much precision when summing the result.
		double scale=preMultiplyFactor;
		double result=0;
		for(unsigned i=0; i<octaves; ++i) {
			result=result/2+PerlinNoise::pnoise(x, y, scale, scale);
			x/=2;
			y/=2;
			scale/=2;
		}
		result/=2;

		assert(result>=-1.0 && result<=1.0);
		return result;
	}
};
