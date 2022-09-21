#ifndef FRAMEWORK_EXCEPTION_H
#define FRAMEWORK_EXCEPTION_H

#include "fire/exception/Exception.h"

namespace framework::exception {
using Exception = fire::Exception;
}

#define EXCEPTION_RAISE(CATEGORY, MSG) throw fire::Exception(CATEGORY, MSG);

#endif
