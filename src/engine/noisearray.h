#ifndef ENGINE_NOISEARRAY_H
#define ENGINE_NOISEARRAY_H

#include "fbnnoise.h"

namespace Engine {
	void noiseArrayProgressFunctorString(unsigned noiseY, unsigned noiseHeight, void *userData);

	class NoiseArray {
	public:
		typedef void (ProgressFunctor)(unsigned noiseY, unsigned noiseHeight, void *userData);

		NoiseArray(double tileWidth, double tileHeight, unsigned noiseWidth, unsigned noiseHeight, double noiseResolution, unsigned progressDelta, ProgressFunctor *progressFunctor, void *progressUserData);
		~NoiseArray();

		double eval(double x, double y) const;
	private:
		double *array;
		unsigned noiseWidth;
		double xFactor, yFactor;
	};
};

#endif

/*


	// Create noise.
	const unsigned heightNoiseWidth=4096;
	const unsigned heightNoiseHeight=4096;
	const double heightResolution=600.0;

	double *heightArray=(double *)malloc(sizeof(double)*heightNoiseHeight*heightNoiseWidth); // TODO: never freed
	assert(heightArray!=NULL); // TODO: better
	double *heightArrayPtr;

	unsigned x, y;

	FbnNoise heightNose(8, 1.0/heightResolution, 1.0, 2.0, 0.5);
	unsigned noiseYProgressDelta=heightNoiseHeight/16;
	const float freqFactorX=(((double)width)/heightNoiseWidth)/8.0;
	const float freqFactorY=(((double)height)/heightNoiseHeight)/8.0;
	heightArrayPtr=heightArray;
	for(y=0;y<heightNoiseHeight;++y) {
		for(x=0;x<heightNoiseWidth;++x,++heightArrayPtr)
			// Calculate noise value to represent the height here.
			*heightArrayPtr=heightNose.eval(x*freqFactorX, y*freqFactorY);

		// Update progress (if needed).
		if (y%noiseYProgressDelta==noiseYProgressDelta-1) {
			Util::clearConsoleLine();
			printf("	water/land: generating height noise %.1f%%.", ((y+1)*100.0)/heightNoiseHeight); // TODO: this better
			fflush(stdout);
		}
	}
	printf("\n");

	// Create user data.
	const double heightXFactor=((double)heightNoiseWidth)/width;
	const double heightYFactor=((double)heightNoiseHeight)/height;


*/
