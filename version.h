#pragma once

#define _VERSION_MAJOR_        0
#define _VERSION_MINOR_        1
#define _VERSION_BUILDS_       20
#define _VERSION_REVISION_     12

#define _PRODVERSION_MAJOR_    1
#define _PRODVERSION_MINOR_    0
#define _PRODVERSION_BUILDS_   0
#define _PRODVERSION_REVISION_ 0

#define _LOCAL_TEST_ENV_       0

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define _VERSION_     TOSTRING(_VERSION_MAJOR_)      \
                  "." TOSTRING(_VERSION_MINOR_)      \
                  "." TOSTRING(_VERSION_BUILDS_)     \
                  "." TOSTRING(_VERSION_REVISION_)

#define _PRODVERSION_ TOSTRING(_PRODVERSION_MAJOR_)  \
                  "." TOSTRING(_PRODVERSION_MINOR_)

#define _PROD_NAME_           "mflow"
#define _PROD_DESCRIPTION_    "tool for mitmproxy flow files"
#define _ORGANIZATION_NAME_   "Oleg V. Polivets"
#define _PROD_COPYRIGHT_      "(c) " _ORGANIZATION_NAME_ ", 2018."
