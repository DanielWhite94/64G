#include <cassert>
#include <cmath>

#include "fbnnoise.h"

namespace Engine {
	FbnNoise::FbnNoise(unsigned seed, unsigned gOctaves, double gFrequency) {
		baseNoise=new OpenSimplexNoise(seed);
		octaves=gOctaves;
		frequency=gFrequency;
	}

	FbnNoise::~FbnNoise() {
		delete baseNoise;
	}

	double FbnNoise::eval(double x, double y) {
		// Premultiply x and y.
		x*=frequency;
		y*=frequency;

		// Loop in reverse so that we start with amp small, so that we do not lose as much precision when summing result.
		double result=0.0;
		double exp2I=exp2(octaves-1);
		for(int i=octaves-1;i>=0;--i) {
			result+=baseNoise->eval(x*exp2I, y*exp2I)/exp2I;
			exp2I/=2.0;
		}

		double dividend=2.0-pow(0.5, octaves-1);
		result/=dividend;

		assert(result>=-1.0 && result<=1.0);

		return result;
	}
};
