#include <assert.h>

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
		double freq=frequency;
		double amp=amplitude;
		double dividend=0.0;

		unsigned i;
		for(i=0;i<octaves;++i) {
			result+=amp*baseNoise->eval(x*freq, y*freq);
			dividend+=amp;
			freq*=lacunarity;
			amp*=gain;
		}

		result/=dividend;

		assert(result>=-1.0 && result<=1.0);

		return result;
	}
};
