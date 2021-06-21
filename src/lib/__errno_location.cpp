#include <errno.h>

extern "C" int *__errno_location(void); // {
	// return &errno;
// }

extern "C" int *_Z17___errno_locationv(void) {
	// Well. This is something.
	return __errno_location();
}
