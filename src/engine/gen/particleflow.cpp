#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <random>

#include "particleflow.h"

using namespace Engine;

namespace Engine {
	namespace Gen {

		void ParticleFlow::dropParticles(unsigned x0, unsigned y0, unsigned x1, unsigned y1, double coverage, unsigned threadCount, Util::ProgressFunctor *progressFunctor, void *progressUserData) {
			assert(map!=NULL);
			assert(x0<=x1);
			assert(y0<=y1);
			assert(coverage>=0.0);

			// TODO: Fix the distribution and calculations if the given area is not a multiple of the region size (in either direction).

			// Record start time.
			Util::TimeMs startTimeMs=Util::getTimeMs();

			// Run progress functor initially (if needed).
			Util::TimeMs lastProgressTimeMs=0;
			if (progressFunctor!=NULL) {
				Util::TimeMs currTimeMs=Util::getTimeMs();
				progressFunctor(0.0, currTimeMs-startTimeMs, progressUserData);
				lastProgressTimeMs=Util::getTimeMs();
			}

			// Compute region loop bounds.
			unsigned rX0=x0/MapRegion::tilesSize;
			unsigned rY0=y0/MapRegion::tilesSize;
			unsigned rX1=x1/MapRegion::tilesSize;
			unsigned rY1=y1/MapRegion::tilesSize;

			// Generate list of regions.
			struct RegionPos { unsigned x, y; } regionPos;
			std::vector<RegionPos> regionList;
			for(regionPos.y=rY0; regionPos.y<rY1; ++regionPos.y)
				for(regionPos.x=rX0; regionPos.x<rX1; ++regionPos.x)
					regionList.push_back(regionPos);

			// Shuffle list.
			auto rng=std::default_random_engine{};
			std::shuffle(std::begin(regionList), std::end(regionList), rng);

			// Calculate constants
			double trialsPerRegion=coverage*MapRegion::tilesSize*MapRegion::tilesSize;

			// Loop over regions (this saves unnecessary loading and saving of regions compared to picking random locations across the whole area given).
			unsigned rI=0;
			for(auto const& regionPos: regionList) {
				// Compute number of trials to perform for this region (may be 0).
				unsigned trials=floor(trialsPerRegion)+(trialsPerRegion>Util::randFloatInInterval(0.0, 1.0) ? 1 : 0);

				// Run said number of trials.
				unsigned tileOffsetBaseX=regionPos.x*MapRegion::tilesSize;
				unsigned tileOffsetBaseY=regionPos.y*MapRegion::tilesSize;
				for(unsigned i=0; i<trials; ++i) {
					// Compute random tile position within this region.
					double randX=Util::randFloatInInterval(0.0, MapRegion::tilesSize);
					double randY=Util::randFloatInInterval(0.0, MapRegion::tilesSize);

					double tileX=tileOffsetBaseX+randX;
					double tileY=tileOffsetBaseY+randY;

					// Grab tile.
					const MapTile *tile=map->getTileAtOffset((int)floor(tileX), (int)floor(tileY), Map::Map::GetTileFlag::None);
					if (tile==NULL)
						continue;

					// Drop particle.
					dropParticle(tileX, tileY);

					// Call progress functor (if needed).
					if (progressFunctor!=NULL) {
						Util::TimeMs currTimeMs=Util::getTimeMs();
						if (currTimeMs-lastProgressTimeMs>500) {
							double progress=(rI+((double)i)/trials)/regionList.size();
							progressFunctor(progress, currTimeMs-startTimeMs, progressUserData);
							lastProgressTimeMs=currTimeMs;
						}
					}
				}

				// Call progress functor (if needed).
				if (progressFunctor!=NULL) {
					double progress=(rI+1.0)/regionList.size();
					Util::TimeMs elapsedTimeMs=Util::getTimeMs()-startTimeMs;
					progressFunctor(progress, elapsedTimeMs, progressUserData);
				}

				++rI;
			}
		}

		void ParticleFlow::dropParticle(double xp, double yp) {
			const double Kq=20, Kw=0.0005, Kr=0.95, Kd=0.02, Ki=0.1/4.0, minSlope=0.03, Kg=20*2;

			#define DEPOSIT(H) \
				depositAt(xi  , yi  , (1-xf)*(1-yf), ds); \
				depositAt(xi+1, yi  ,    xf *(1-yf), ds); \
				depositAt(xi  , yi+1, (1-xf)*   yf , ds); \
				depositAt(xi+1, yi+1,    xf *   yf , ds); \
				(H)+=ds;

			int xi=floor(xp);
			int yi=floor(yp);
			double xf=xp-xi;
			double yf=yp-yi;

			double dx=0.0, dy=0.0;
			double s=0.0, v=0.0, w=1.0;

			double h=hMap(xi, yi, DBL_MIN);

			double h00=h;
			double h01=hMap(xi, yi+1, DBL_MIN);
			double h10=hMap(xi+1, yi, DBL_MIN);
			double h11=hMap(xi+1, yi+1, DBL_MIN);

			if (h<seaLevelExcess)
				return;

			if (h00==DBL_MIN || h01==DBL_MIN || h10==DBL_MIN || h11==DBL_MIN)
				return; // TODO: think about this

			int maxPathLen=4.0*(MapRegion::tilesSize+MapRegion::tilesSize);
			for(int pathLen=0; pathLen<maxPathLen; ++pathLen) {
				// Increment moisture counter for the current tile.
				if (incMoisture) {
					// Calculate normalised x and y values for the tile
					int tempX=xi, tempY=yi;
					tempX=(tempX+map->getWidth())%map->getWidth();
					tempY=(tempY+map->getHeight())%map->getHeight();

					assert(tempX>=0 && tempX<map->getWidth());
					assert(tempY>=0 && tempY<map->getHeight());

					// Lookup tile
					MapTile *tempTile=map->getTileAtOffset(tempX, tempY, Map::Map::GetTileFlag::None);
					if (tempTile!=NULL)
						tempTile->setMoisture(tempTile->getMoisture()+w);
				}

				// calc gradient
				double gx=h00+h01-h10-h11;
				double gy=h00+h10-h01-h11;

				// calc next pos
				dx=(dx-gx)*Ki+gx;
				dy=(dy-gy)*Ki+gy;

				double dl=sqrt(dx*dx+dy*dy);
				if (dl<=0.01) {
					// pick random dir
					double a=Util::randFloatInInterval(0.0, 2*M_PI);
					dx=cos(a);
					dy=sin(a);
				} else {
					dx/=dl;
					dy/=dl;
				}

				double nxp=std::fmod(xp+map->getWidth()+dx, map->getWidth());
				double nyp=std::fmod(yp+map->getHeight()+dy, map->getHeight());

				assert(nxp>=0.0 && nxp<map->getWidth());
				assert(nyp>=0.0 && nyp<map->getHeight());

				// sample next height
				int nxi=floor(nxp);
				int nyi=floor(nyp);
				double nxf=nxp-nxi;
				double nyf=nyp-nyi;

				double nh00=hMap(nxi  , nyi  , DBL_MIN);
				double nh10=hMap(nxi+1, nyi  , DBL_MIN);
				double nh01=hMap(nxi  , nyi+1, DBL_MIN);
				double nh11=hMap(nxi+1, nyi+1, DBL_MIN);

				if (nh00==DBL_MIN || nh01==DBL_MIN || nh10==DBL_MIN || nh11==DBL_MIN)
					return; // TODO: think about this

				double nh=(nh00*(1-nxf)+nh10*nxf)*(1-nyf)+(nh01*(1-nxf)+nh11*nxf)*nyf;

				if (nh<seaLevelExcess)
					return;

				// if higher than current, try to deposit sediment up to neighbour height
				if (nh>=h) {
					double ds=(nh-h)+0.001f;

					if (ds>=s) {
						// deposit all sediment and stop
						ds=s;
						DEPOSIT(h)
						s=0;
						break;
					}

					DEPOSIT(h)
					s-=ds;
					v=0;
				}

				// compute transport capacity
				double dh=h-nh;
				double slope=dh;

				double q=std::max(slope, minSlope)*v*w*Kq;

				// deposit/erode (don't erode more than dh)
				double ds=s-q;
				if (ds>=0) {
					// deposit
					ds*=Kd;

					DEPOSIT(dh)
					s-=ds;
				} else {
					// erode
					ds*=-Kr;
					ds=std::min(ds, dh*0.99);

					double distfactor=1.0/(erodeRadius*erodeRadius);
					double wTotal=0.0;
					for (int y=yi-erodeRadius+1; y<=yi+erodeRadius; ++y) {
						double yo=y-yp;
						double yo2=yo*yo;
						for (int x=xi-erodeRadius+1; x<=xi+erodeRadius; ++x) {
							double xo=x-xp;
							double xo2=xo*xo;

							double w=1-(xo2+yo2)*distfactor;
							if (w<=0)
								continue;

							wTotal+=w;
						}
					}

					for (int y=yi-erodeRadius+1; y<=yi+erodeRadius; ++y) {
						double yo=y-yp;
						double yo2=yo*yo;
						for (int x=xi-erodeRadius+1; x<=xi+erodeRadius; ++x) {
							double xo=x-xp;
							double xo2=xo*xo;

							double w=1-(xo2+yo2)*distfactor;
							if (w<=0)
								continue;

							w/=wTotal;

							depositAt(x, y, -w, ds);
						}
					}

					dh-=ds;
					s+=ds;
				}

				// move to the neighbour
				v=sqrt(v*v+Kg*dh);
				w*=1-Kw;

				xp=nxp; yp=nyp;
				xi=nxi; yi=nyi;
				xf=nxf; yf=nyf;

				h=nh;
				h00=nh00;
				h10=nh10;
				h01=nh01;
				h11=nh11;
			}

			#undef DEPOSIT
		}

		double ParticleFlow::hMap(int x, int y, double unknownValue) {
			// To simplify calls to this function, allow x and y values in the interval [-mapwidth,mapwidth]
			x=(x+map->getWidth())%map->getWidth();
			y=(y+map->getHeight())%map->getHeight();

			assert(x>=0 && x<map->getWidth());
			assert(y>=0 && y<map->getHeight());

			// Grab tile.
			const MapTile *tile=map->getTileAtOffset(x, y, Map::Map::GetTileFlag::None);
			if (tile==NULL)
				return unknownValue;

			// Return height.
			return tile->getHeight();
		}

		void ParticleFlow::depositAt(int x, int y, double w, double ds) {
			// To simplify calls to this function, allow x and y values in the interval [-mapwidth,mapwidth]
			x=(x+map->getWidth())%map->getWidth();
			y=(y+map->getHeight())%map->getHeight();

			assert(x>=0 && x<map->getWidth());
			assert(y>=0 && y<map->getHeight());

			// Grab tile.
			MapTile *tile=map->getTileAtOffset(x, y, Map::Map::GetTileFlag::Dirty);
			if (tile==NULL)
				return;

			// Adjust height.
			double delta=ds*w;
			tile->setHeight(tile->getHeight()+delta);
		}

	};
};
