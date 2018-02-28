#include <cassert>
#include <cmath>

#include "fbnnoise.h"

namespace Engine {
	FbnNoise::FbnNoise(unsigned seed, unsigned octaves, double frequency): octaves(octaves) {
		assert(octaves>0);

		baseNoise=new OpenSimplexNoise(seed);

		double exp2Octaves=exp2(octaves-1);
		inputFactor=frequency*exp2Octaves;
		outputDivisor=2.0-1.0/exp2Octaves;
	}

	FbnNoise::~FbnNoise() {
		delete baseNoise;
	}

	double FbnNoise::eval(double x, double y) {
		// Premultiply x and y for speed.
		x*=inputFactor;
		y*=inputFactor;

		// Loop in reverse so that we start with amp small, so that we do not lose as much precision when summing result.
		double result=0.0;
		for(unsigned i=0;i<octaves;++i) {
			result=result/2+baseNoise->eval(x, y);
			x/=2.0;
			y/=2.0;
		}
		result/=outputDivisor;

		assert(result>=-1.0 && result<=1.0);
		return result;
	}
};
