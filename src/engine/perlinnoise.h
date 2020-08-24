// PerlinNoise
// Author: Stefan Gustavson (stegu@itn.liu.se)
//
// This library is public domain software, released by the author
// into the public domain in February 2011. You may do anything
// you like with it. You may even remove all attributions,
// but of course I'd appreciate it if you kept my name somewhere.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.

/** \file
		\brief Declares the PerlinNoise class for producing Perlin noise.
		\author Stefan Gustavson (stegu@itn.liu.se)
*/

#ifndef ENGINE_PERLINNOISE_H
#define ENGINE_PERLINNOISE_H

namespace Engine {
	class PerlinNoise {
	public:
		PerlinNoise(unsigned seed);
		~PerlinNoise() {}

		double noise(double x);
		double noise(double x, double y);
		double noise(double x, double y, double z);
		double noise(double x, double y, double z, double w);

		double pnoise(double x, int px);
		double pnoise(double x, double y, int px, int py);
		double pnoise(double x, double y, double z, int px, int py, int pz);
		double pnoise(double x, double y, double z, double w, int px, int py, int pz, int pw);

	private:
		unsigned char perm[512]; // Permutation table - this is just a random jumble of all numbers 0-255, repeated twice to avoid wrapping the index at 255 for each lookup

		double grad(int hash, double x);
		double grad(int hash, double x, double y);
		double grad(int hash, double x, double y , double z);
		double grad(int hash, double x, double y, double z, double t);
	};
};

#endif
