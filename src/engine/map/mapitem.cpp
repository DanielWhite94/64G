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

			// TODO: this
			return true; // ..... temporary hack
		}

		unsigned MapItem::getId(void) const {
			return id;
		}

		const char *MapItem::getName(void) const {
			return name;
		}

	};
};
