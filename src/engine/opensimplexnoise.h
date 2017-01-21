#ifndef ENGINE_OPENSIMPLEXNOISE_H
#define ENGINE_OPENSIMPLEXNOISE_H

namespace Engine {
	class OpenSimplexNoise {
	public:
		OpenSimplexNoise(unsigned seed=17);
		~OpenSimplexNoise();

		double eval(double x, double y);
	private:
		const double stretchConstant=-0.211324865405187; // (1/sqrt(2+1)-1)/2
		const double squishConstant=0.366025403784439; // (sqrt(2+1)-1)/2
		const double normConstant=47;

		const short gradients[16]={
		  5, 2, 2, 5,
		  -5, 2, -2, 5,
		  5, -2, 2, -5,
		  -5, -2, -2, -5};

		short perm[256];

		double eval(int xsb, int ysb, double dx, double dy);
		double extrapolate(int xsb, int ysb, double dx, double dy);
	};
};

#endif
