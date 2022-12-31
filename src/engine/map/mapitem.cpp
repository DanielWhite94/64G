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

#include "mapitem.h"

namespace Engine {
	namespace Map {
		MapItem::MapItem(unsigned gId, const char *gName) {
			assert(gId<IdMax);
			assert(gName!=NULL);

			id=gId;
			name=(char *)malloc(strlen(gName)+1); // TODO: Check return.
			strcpy(name, gName);
		}

		MapItem::~MapItem() {
			free(name);
			name=NULL;
		}

		bool MapItem::save(const char *itemsDirPath) const {
			assert(itemsDirPath!=NULL);

			// Create file name
			char path[4096]; // TODO: This better.
			sprintf(path, "%s/%u", itemsDirPath, getId());

			// Save data to file (creating/truncating as required)
			FILE *fd=fopen(path, "w");
			if (fd==NULL)
				return false;

			if (fprintf(fd, "%s\n", getName())<0) {
				fclose(fd);
				return false;
			}

			fclose(fd);

			return true;
		}

		unsigned MapItem::getId(void) const {
			return id;
		}

		const char *MapItem::getName(void) const {
			return name;
		}

	};
};
