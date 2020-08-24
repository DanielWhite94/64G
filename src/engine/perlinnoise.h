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
		PerlinNoise() {}
		~PerlinNoise() {}

		static double noise(double x);
		static double noise(double x, double y);
		static double noise(double x, double y, double z);
		static double noise(double x, double y, double z, double w);

		static double pnoise(double x, int px);
		static double pnoise(double x, double y, int px, int py);
		static double pnoise(double x, double y, double z, int px, int py, int pz);
		static double pnoise(double x, double y, double z, double w, int px, int py, int pz, int pw);

	private:
		static unsigned char perm[];
		static double grad(int hash, double x);
		static double grad(int hash, double x, double y);
		static double grad(int hash, double x, double y , double z);
		static double grad(int hash, double x, double y, double z, double t);

	};
};

#endif
