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

	NoiseArray::NoiseArray(unsigned seed, double tileWidth, double tileHeight, unsigned noiseWidth, unsigned noiseHeight, double noiseResolution, unsigned noiseOctaves, unsigned progressDelta, ProgressFunctor *progressFunctor, void *progressUserData): noiseWidth(noiseWidth), noiseHeight(noiseHeight) {
		assert(tileWidth>0.0);
		assert(tileHeight>0.0);
		assert(noiseWidth>0);
		assert(noiseHeight>0);
		assert(noiseResolution>0.0);
		assert(noiseOctaves>0);
		assert(progressFunctor==NULL || progressDelta>0);

		// Init
		array=NULL;
		xFactor=((double)noiseWidth)/tileWidth;
		yFactor=((double)noiseHeight)/tileHeight;

		// Allocate and fill array.
		array=(double *)malloc(sizeof(double)*noiseHeight*noiseWidth);
		assert(array!=NULL); // TODO: better
		double *arrayPtr;

		FbnNoise heightNose(seed, noiseOctaves, 1.0/noiseResolution);
		const float freqFactorX=tileWidth/noiseWidth;
		const float freqFactorY=tileHeight/noiseHeight;
		unsigned x, y;
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
		assert(noiseY<noiseHeight);
		return array[noiseX+noiseY*noiseWidth];
	}

};
