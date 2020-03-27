
#ifdef __GNUC__

//----- Push conflicting macros found in <gdal/cpl_config.h (included at some point by gdal.h)
#pragma push_macro("PACKAGE_NAME")
#pragma push_macro("PACKAGE_TARNAME")
#pragma push_macro("PACKAGE_VERSION")
#pragma push_macro("PACKAGE_STRING")
#pragma push_macro("PACKAGE_BUGREPORT")
#pragma push_macro("PACKAGE_URL")

//----- Make the actual include
#include "gdal.h"

//----- Pop back our definition
#pragma pop_macro("PACKAGE_NAME")
#pragma pop_macro("PACKAGE_TARNAME")
#pragma pop_macro("PACKAGE_VERSION")
#pragma pop_macro("PACKAGE_STRING")
#pragma pop_macro("PACKAGE_BUGREPORT")
#pragma pop_macro("PACKAGE_URL")

#else // __GNUC__

#include "gdal.h"

#endif // __GNUC__
