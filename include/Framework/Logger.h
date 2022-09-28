#ifndef FRAMEWORK_LOGGER_H
#define FRAMEWORK_LOGGER_H

#include "fire/logging/Logger.h"

namespace framework::logging {
using logger = fire::logging::logger;
}

#define ldmx_log(lvl) fire_log(lvl)

#endif
