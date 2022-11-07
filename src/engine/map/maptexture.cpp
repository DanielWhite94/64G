#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>

#include "maptexture.h"

namespace Engine {
	namespace Map {
		MapTexture::MapTexture(unsigned gId, const char *gPath, unsigned gScale, uint8_t gMapColourR, uint8_t gMapColourG, uint8_t gMapColourB) {
			assert(gId<IdMax);
			assert(gPath!=NULL);
			assert(gScale>=1);

			id=gId;
			path=(char *)malloc(strlen(gPath)+1); // TODO: Check return.
			strcpy(path, gPath);
			scale=gScale;
			mapColourR=gMapColourR;
			mapColourG=gMapColourG;
			mapColourB=gMapColourB;
		}

		MapTexture::~MapTexture() {
			free(path);
			path=NULL;
		}

		bool MapTexture::save(const char *texturesDirPath) const {
			assert(texturesDirPath!=NULL);

			// Copy image file.
			const char *extension="png"; // TODO: Avoid hardcoding this.
			char outPath[4096]; // TODO: This better.
			sprintf(outPath, "%s/%us%ur%ug%ub%u.%s", texturesDirPath, getId(), getScale(), getMapColourR(), getMapColourG(), getMapColourB(), extension);

			int inFd=open(getImagePath(), O_RDONLY); // TODO: Check return.
			int outFd=open(outPath, O_WRONLY|O_CREAT, 0777); // TODO: Check return.

			bool result=false;
			if (inFd!=-1 && outFd!=-1) {
				off_t remaining=lseek(inFd, 0, SEEK_END);
				off_t offset=0;
				while(remaining>0) {
					ssize_t written=sendfile(outFd, inFd, &offset, remaining); // TODO: Check return.

					if (written>0)
						remaining-=written;
					else if (written==-1)
						break;
					else
						sleep(1);

					// TODO: This is all bad...
				}

				if (remaining==0)
					result=true;
			} else {
				if (inFd==-1)
					fprintf(stderr, "error: could not open input file for texture %u at '%s'\n", getId(), getImagePath());
				if (outFd==-1)
					fprintf(stderr, "error: could not open output file for texture %u at '%s'\n", getId(), outPath);
			}

			close(outFd);
			close(inFd);

			return result;
		}

		unsigned MapTexture::getId(void) const {
			return id;
		}

		const char *MapTexture::getImagePath(void) const {
			return path;
		}

		unsigned MapTexture::getScale(void) const {
			return scale;
		}

		uint8_t MapTexture::getMapColourR(void) const {
			return mapColourR;
		}

		uint8_t MapTexture::getMapColourG(void) const {
			return mapColourG;
		}

		uint8_t MapTexture::getMapColourB(void) const {
			return mapColourB;
		}
	};
};
