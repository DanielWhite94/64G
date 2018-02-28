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
		double result=0.0;

		// Loop in reverse so that we start with amp small, so that we do not lose as much precision when summing result.
		for(int i=octaves-1;i>=0;--i) {
			// TODO: Is calculating these each time needed? (or can we just do e.g. amp*=0.5 each time)
			double exp2I=exp2(i);
			double freq=frequency*exp2I;
			result+=baseNoise->eval(x*freq, y*freq)/exp2I;
		}

		double dividend=2.0-pow(0.5, octaves-1);
		result/=dividend;

		assert(result>=-1.0 && result<=1.0);

		return result;
	}
};
