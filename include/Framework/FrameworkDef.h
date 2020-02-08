/**
 * @file FrameworkDef.h 
 * @brief Class used to define commonly used variables.
 * @author Omar Moreno, SLAC National Accelerator Laboratory
 */

#ifndef FRAMEWORK_FRAMEWORKDEF_H
#define FRAMEWORK_FRAMEWORKDEF_H

/*~~~~~~~~~~~~~~~~*/
/*   C++ StdLib   */
/*~~~~~~~~~~~~~~~~*/
#include <any>
#include <map>
#include <string> 
#include <vector>

namespace ldmx { 

    /**
     * @struct Class 
     * @brief Encapsualtes the information required to create a class 
     *        of type className_ 
     */
    struct Class { 

        /// Name of the class  
        std::string className_;

        /// Name of the instance of this class described by this object
        std::string instanceName_;

        /// The parameters associated with this class
        std::map< std::string, std::any > params_;

    };

} // ldmx

#endif // FRAMEWORK_FRAMEWORKDEF_H
