#include <cassert>
#include <cmath>

#include "opensimplexnoise.h"

namespace Engine {
	OpenSimplexNoise::OpenSimplexNoise(unsigned seed) {
		short source[256];

		int i;
		for(i=0;i<256;++i)
			source[i]=i;

			seed=seed*6364136223846793005l+1442695040888963407l;
			seed=seed*6364136223846793005l+1442695040888963407l;
			seed=seed*6364136223846793005l+1442695040888963407l;

			for(i=255;i>=0;--i) {
				seed=seed*6364136223846793005l+1442695040888963407l;

			int r=(seed+31)%(i+1);
			if (r<0)
				r+=(i+1);

			perm[i]=source[r];
			source[r]=source[i];
		}
	}

	OpenSimplexNoise::~OpenSimplexNoise() {
	}

	double OpenSimplexNoise::eval(double x, double y) {
		// Place input coordinates onto grid.
		double stretchOffset=(x+y)*stretchConstant;
		double xs=x+stretchOffset;
		double ys=y+stretchOffset;

		// Floor to get grid coordinates of rhombus (stretched square) super-cell origin.
		int xsb=floor(xs);
		int ysb=floor(ys);

		// Skew out to get actual coordinates of rhombus origin. We'll need these later.
		double squishOffset=(xsb+ysb)*squishConstant;
		double xb=xsb+squishOffset;
		double yb=ysb+squishOffset;

		// Compute grid coordinates relative to rhombus origin.
		double xins=xs-xsb;
		double yins=ys-ysb;

		// Sum those together to get a value that determines which region we're in.
		double inSum=xins+yins;

		// Positions relative to origin point.
		double dx0=x-xb;
		double dy0=y-yb;

		// We'll be defining these inside the next block and using them afterwards.
		double dx_ext, dy_ext;
		int xsv_ext, ysv_ext;

		double value=0;

		// Contribution (1,0).
		double dx1=dx0-1-squishConstant;
		double dy1=dy0-0-squishConstant;
		double attn1=2-dx1*dx1-dy1*dy1;
		if (attn1>0) {
			attn1*=attn1;
			value+=attn1*attn1*extrapolate(xsb+1, ysb+0, dx1, dy1);
		}

		// Contribution (0,1).
		double dx2=dx0-0-squishConstant;
		double dy2=dy0-1-squishConstant;
		double attn2=2-dx2*dx2-dy2*dy2;
		if (attn2>0) {
			attn2*=attn2;
			value+=attn2*attn2*extrapolate(xsb+0, ysb+1, dx2, dy2);
		}

		if (inSum<=1) {
			// We're inside the triangle (2-Simplex) at (0,0).
			double zins=1-inSum;
			if (zins>xins || zins>yins) {
				// (0,0) is one of the closest two triangular vertices.
				if (xins>yins) {
					xsv_ext=xsb+1;
					ysv_ext=ysb-1;
					dx_ext=dx0-1;
					dy_ext=dy0+1;
				} else {
					xsv_ext=xsb-1;
					ysv_ext=ysb+1;
					dx_ext=dx0+1;
					dy_ext=dy0-1;
				}
			} else {
				// (1,0) and (0,1) are the closest two vertices.
				xsv_ext=xsb+1;
				ysv_ext=ysb+1;
				dx_ext=dx0-1-2*squishConstant;
				dy_ext=dy0-1-2*squishConstant;
			}
		} else {
			// We're inside the triangle (2-Simplex) at (1,1).
			double zins=2-inSum;
			if (zins<xins || zins<yins) {
				// (0,0) is one of the closest two triangular vertices.
				if (xins>yins) {
					xsv_ext=xsb+2;
					ysv_ext=ysb+0;
					dx_ext=dx0-2-2*squishConstant;
					dy_ext=dy0+0-2*squishConstant;
				} else {
					xsv_ext=xsb+0;
					ysv_ext=ysb+2;
					dx_ext=dx0+0-2*squishConstant;
					dy_ext=dy0-2-2*squishConstant;
				}
			} else {
				// (1,0) and (0,1) are the closest two vertices.
				dx_ext=dx0;
				dy_ext=dy0;
				xsv_ext=xsb;
				ysv_ext=ysb;
			}
			xsb+=1;
			ysb+=1;
			dx0=dx0-1-2*squishConstant;
			dy0=dy0-1-2*squishConstant;
		}

		// Contribution (0,0) or (1,1).
		double attn0=2-dx0*dx0-dy0*dy0;
		if (attn0>0) {
			attn0*=attn0;
			value+=attn0*attn0*extrapolate(xsb, ysb, dx0, dy0);
		}

		// Extra Vertex.
		double attn_ext=2-dx_ext*dx_ext-dy_ext*dy_ext;
		if (attn_ext>0) {
			attn_ext*=attn_ext;
			value+=attn_ext*attn_ext*extrapolate(xsv_ext, ysv_ext, dx_ext, dy_ext);
		}

		value/=normConstant;

		assert(value>=-1.0 && value<=1.0);

		return value;
	}

	double OpenSimplexNoise::extrapolate(int xsb, int ysb, double dx, double dy) {
		int index=perm[(perm[xsb&0xFF]+ysb)&0xFF]&0x0E;
		return gradients[index]*dx+gradients[index+1]*dy;
	}
};
