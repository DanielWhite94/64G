#include <cassert>
#include <cmath>

#include "fbnnoise.h"

namespace Engine {
	FbnNoise::FbnNoise(unsigned seed, unsigned gOctaves, double gFrequency, double gAmplitude, double gLacunarity, double gGain) {
		baseNoise=new OpenSimplexNoise(seed);
		octaves=gOctaves;
		frequency=gFrequency;
		amplitude=gAmplitude;
		lacunarity=gLacunarity;
		gain=gGain;
	}

	FbnNoise::~FbnNoise() {
		delete baseNoise;
	}

	double FbnNoise::eval(double x, double y) {
		double result=0.0;
		double dividend=0.0;

		// Loop in reverse so that when gain<1.0, and thus amp is very small, we do not lose as much precision when summing result.
		for(int i=octaves-1;i>=0;--i) {
			// TODO: Is calculating these each time needed? (or can we just do e.g. amp*=gain each time)
			double amp=amplitude*pow(gain, i);
			double freq=frequency*pow(lacunarity, i);
			result+=amp*baseNoise->eval(x*freq, y*freq);
			dividend+=amp;
		}

		result/=dividend;

		assert(result>=-1.0 && result<=1.0);

		return result;
	}
};
