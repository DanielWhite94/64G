#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "noisearray.h"
#include "util.h"

namespace Engine {
	void noiseArrayProgressFunctorString(unsigned noiseY, unsigned noiseHeight, void *userData) {
		assert(userData!=NULL);

		const char *string=(const char *)userData;

		Util::clearConsoleLine();
		printf("%s%.1f%%.", string, ((noiseY+1)*100.0)/noiseHeight);
		fflush(stdout);
	}

	NoiseArray::NoiseArray(double tileWidth, double tileHeight, unsigned gNoiseWidth, unsigned noiseHeight, double noiseResolution, unsigned progressDelta, ProgressFunctor *progressFunctor, void *progressUserData) {
		assert(tileWidth>0.0);
		assert(tileHeight>0.0);
		assert(gNoiseWidth>0);
		assert(noiseHeight>0);
		assert(noiseResolution>0.0);
		assert(progressFunctor==NULL || progressDelta>0);

		// Init
		array=NULL;
		noiseWidth=gNoiseWidth;
		xFactor=((double)noiseWidth)/tileWidth;
		yFactor=((double)noiseHeight)/tileHeight;

		// Allocate and fill array.
		array=(double *)malloc(sizeof(double)*noiseHeight*noiseWidth);
		assert(array!=NULL); // TODO: better
		double *arrayPtr;

		unsigned x, y;

		FbnNoise heightNose(8, 1.0/noiseResolution, 1.0, 2.0, 0.5);
		const float freqFactorX=(tileWidth/noiseWidth)/8.0;
		const float freqFactorY=(tileHeight/noiseHeight)/8.0;
		arrayPtr=array;
		for(y=0;y<noiseHeight;++y) {
			for(x=0;x<noiseWidth;++x,++arrayPtr)
				// Calculate noise value to represent the height here.
				*arrayPtr=heightNose.eval(x*freqFactorX, y*freqFactorY);

			// Update progress (if needed).
			if (progressFunctor!=NULL && y%progressDelta==progressDelta-1)
				progressFunctor(y, noiseHeight, progressUserData);
		}
	}

	NoiseArray::~NoiseArray() {
		free(array);
	}

	double NoiseArray::eval(double x, double y) const {
		unsigned noiseX=floor(x*xFactor);
		unsigned noiseY=floor(y*yFactor);
		assert(noiseX<noiseWidth);
		return array[noiseX+noiseY*noiseWidth];
	}

};
