# Getting Started
This document is to help users become familiar with the basic workings of `fire`.
It assumes you already have a version of fire built and installed.

## Processors
Defining a new processor can be done to varying levels of complexity;
however, they fall into two distinct groups: configurable and 
non-configurable. 

### Non-Configurable
Non-configurable processors cannot be configured
at run time from the python configuration.
This makes starting them simpler even if they are slightly
less powerful.

```cpp
// MyProcessor.cpp
#include <fire/Processor.h>
class MyProcessor : public fire::Processor {
 public:
  MyProcessor(const fire::config::Parameters& ps)
   : fire::Processor(ps) {}
  ~MyProcessor() = default;
  void process(fire::Event& event) final override {
    // process event here!
  }
};
DECLARE_PROCESSOR(MyProcessor);
```

This example shows the necessary parts of a new Processor.
1. Inherits from `fire::Processor`
2. Constructor accepts configuration parameters and passes them to
   the base class
3. Destructor is defined (even if it is default)
4. Definition of the function to do the processing
5. Calling the declaration macro after class is declared.

This processor is already ready to be compiled and added
into a Python configuration. Any of the other call backs 
in this class can then be defined by your derived processor
if you see it as useful.

Let's suppose that this processor is compiled into a library
called `libMyModule.so` (perhaps with other processors or tools).
Then, we can use this processor with fire from a python configuration
file as shown below.
```py
import fire.cfg
p = fire.cfg.Process('example')
p.sequence = [
  fire.cfg.Processor('eg','MyProcessor',library='/full/path/to/libMyModule.so')
  ]
```
The full path to `libMyModule.so` only needs to be provided if it is
not accessible by `ld` (i.e. it is not in a directory listed in LD_LIBRARY_PATH
or a system directory). Moreover, if the library is accessible by `ld`
and you are on a Linux system (so that the libraries follow the naming
format of the example), you could replace the `library` argument with
`module='MyModule'` to make it easier to read. This second option is more
common for larger software sets with many modules.

You can determine if your library is accessible by the linker using
some [fancy command line nonsense](https://unix.stackexchange.com/a/282207).

### Configurable Processor
Making a processor configurable from Python is not very complicated on the
C++ side, but it introduces many complexities and nuances on the Python side. 

On the C++ side, it simply involves expanding the constructor in
order to use the passed set of parameters to define member variables.
Expanding on the example from above:

```cpp
// MyProcessor.cpp
#include <fire/Processor.h>
class MyProcessor : public fire::Processor {
  int my_parameter_;
  double my_required_parameter_;
 public:
  MyProcessor(const fire::config::Parameters& ps)
   : fire::Processor(ps) {
     my_parameter_ = ps.get<int>("my_parameter",1);
     my_required_parameter_ = ps.get<double>("my_required_parameter");
   }
  ~MyProcessor() = default;
  void process(fire::Event& event) final override {
    // process event here!

  }
};
DECLARE_PROCESSOR(MyProcessor);
```
This constructor will recieves two parameters that are configurable.
Exceptions are thrown if the parameter in Python cannot be converted 
to the passed type. For example, if `my_parameter` in Python is set 
to `2.0` instead of `2`.
1. `my_parameter_` is optional with a default value of `1`.
2. `my_required_parameter` is required - i.e. an exception will be
   thrown if a parameter with that name is not found

Now, onto the more complicated Python side.
There are three main methods for defining parameters on the Python
side of configuration. In everything below, `my_proc` is the Python
object that would be added to `p.sequence` inside of the configuration
script so that the Processor will be used during the run.

First, the base configuration class `fire.cfg.Processor` allows
the user to define parameters directly. This is helpful for small
processors that don't have an entire Python module supporting them.
```py
my_proc = fire.cfg.Processor('my_proc','MyProcessor',
                             library='/full/path/to/libMyModule.so',
                             my_parameter = 2, 
                             my_required_parameter = 3.0)
```

Next, we could wrap the code above into a function. This is helpful
for portability because now we can put this function into a Python
module that could be imported in the configuration script. Moreover,
this isolates the parameter spelling to one location so that the
the user does not have to worry about mis-spelling parameters.
```py
def MyProcessor(name, req, opt = 2) :
    return fire.cfg.Processor(name,'MyProcessor',
                              library='/full/path/to/libMyModule.so',
                              my_parameter = opt, 
                              my_required_parameter = req)

# later inside the python config
my_proc = MyProcessor('my_proc',5.0)
```

Finally, we can create a child class of the parent configuration class.
This is the most complicated method and should only be used if the
determination of parameters requires some extra Python functions.
```py
class MyProcessor(fire.cfg.Processor) :
    def __init__(name, req) :
        super().__init__(name,'MyProcessor',
                         library='/full/path/to/libMyModule.so')
        self.my_parameter = 2
        self.my_required_parameter = req

# later inside the python config
my_proc = MyProcessor('my_proc',5.0)
```
 
@note Python's variable handling is very dynamic.
 For us, this means that we need to be very careful that
 the parameters in Python are spelled the same as the parameters
 in C++. In Python, the variables provided to `fire.cfg.Processor`
 (or defined in `__init__` for the last option) need to have 
 **exactly** the same name as the parameter names requested
 in the constructor of the C++ processor.

## Data Classes
The ability of fire to handle the saving and loading of data to and from 
a file comes from the fire::io namespace.
fire is able to handle all so-called "atomic" types (types with 
[numeric limits](https://en.cppreference.com/w/cpp/types/numeric_limits)
defined and std::string, std::vector of, and std::map of these types.

This accomodates a lot of workflows, but it doesn't accomodate everything.
In order to make fire even more flexible, there is a method of interfacing
this serialization procedure with a class that you define.

Below is the `MyData` class declaration showing the minium structure 
necessary to interface with fire's serialization method.
```cpp
#include "fire/io/Data.h"
class MyData {
  friend class fire::io::Data<MyData>;
  MyData() = default;
  void clear();
  void attach(fire::io::Data<MyData>& d);
};
```

The user class has four necessary components:
1. Your class declares the the wrapping io::Data class as a `friend`.
   - This allows the io::Data class access to the (potentially private)
     methods defined below.
2. Your class has a (public or private) default constructor.
   - The default constructor may be how we initialize the data,
     so it must be defined and available to fire::io.
   - If you don't want other parts of the program using the default
     constructor, you can declare it `private`.
3. Your class has a `void clear()` method defined which resets the object
   to an "empty" or "blank" state.
   - This is used by fire to reset the data at the end of each event.
   - Similar to the default constructor, this method can be public 
     or private.
4. Your class implements a `void attach(fire::io::Data<MyData>& d)` method.
   - This method should be private since it should not be called by
     other parts of your code.
   - More detail below.

### The attach Method
This method is where you make the decision on which member variables of
your class should be stored to or read from data files and how those
variables are named. You do this using the fire::io::Data<DataType>::attach
method. This is best illustrated with an example.

```cpp
// member_one_ and member_two_ are members of MyData
void MyData::attach(fire::io::Data<MyData>& d) {
  d.attach("first_member", member_one_);
  d.attach("another_member", member_two_);
}
```

#### Important Comments
- The name of a variable on disk (the first argument) and the name
  of the variable in the class do not need to relate to each other;
  however, it is common to name them similarly so users of your data
  files aren't confused.
- The name of variables on disk cannot be the same in one `attach`
  method, but they can repeat across different classes (similar
  to member variables).
- Passing io::Data as reference (i.e. with the `&`) is necessary;
  otherwise, you would attach to a local copy and the real io::Data
  wouldn't be attached to anything.
- The members of MyData you pass to io::Data::attach can be any
  class that fire::io can handle. This includes the classes listed
  above or other classes you have defined following these rules.

### ROOT Reading
As a transitory feature, reading from ROOT files fire::io::root previously
produced by a ROOT-based serialization framework has been implemented.
In order to effectively read these ROOT files, the user must provide the
ROOT dictionaries for the classes that they wish to read. The method used
in the testing module in this repository is a good example of how to get
this done; that method involves three steps.

#### Step 1: Add ROOT macros to your class
You must include the `TObject.h` header file in order to have access to
these macros. Then use `ClassDef(<class-name>,<version>)` in the header
within the class definition. Finally, use `ClassDef(<ns>::<class-name>);`
in the source file. This lines should be wrapped by preprocessor checks
so that users compiling your library _without_ ROOT can still compile it.
For example,
```cpp
#include <fire/io/Data.h> // get fire_USE_ROOT definition
#ifdef fire_USE_ROOT
#include <TObject.h>
#endif
```
**Note**: ROOT associates the data stored in member variables with the
name of that member variable. This means that ROOT will print warnings
or event error out if new member variables are added or if member variables
change names from when the file was written with ROOT.

#### Step 2: Write a LinkDef file.
This file _must_ end in the string `LinkDef.h`. The prefix to this can
be anything that makes sense to you. The template link def is given below
with a few examples of how to list classes. This file should be alongside
any other headers in your dictionary.
```cpp
// MyEventLinkDef.h
#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link C++ nestedclasses;

// always have to list my class
#pragma link C++ class myns::MyClass+;
// include if you want to read vectors of your class
#pragma link C++ class std::vector<myns::MyClass>+;
// include if you want maps of your class 
// (key could be anything in dictionary, not just int)
#pragma link C++ class std::map<int,myns::MyClass>+;

#endif
```

#### Step 3: CMake Nonsense
ROOT has written a CMake function that can be used to attach a dictionary
compilation to an existing CMake target. It is a bit finnicky, so be careful
when deviating from the template below.
```cmake
find_package(fire REQUIRED 0.13.0)
add_library(MyEvent SHARED <list-source-files>)
target_link_libraries(MyEvent PUBLIC fire::io)
if(fire_USE_ROOT)
  target_include_directories(MyEvent PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>")
  root_generate_dictionary(MyEventDict
    <list-header-files>
    LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/include/MyEvent/MyEventLinkDef.h
    MODULE MyEvent)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libMyEVent_rdict.pcm DESTINATION lib)
endif()
```
This will include the compilation of a ROOT event dictionary if fire was built
with ROOT available.

### Schema Evolution
Oftentimes, there comes a situation where you wish to slightly modify the class
representing your data (i.e. "evolve the schema" to be fancy :star:) but you also
want to retain the ability to read data files that were produced with the past
version of the class. This can be done through modification of the attach method.

By default, the version of the class is zero, but you can increment this version
by calling the `fire_class_version` macro within your class declaration.
```cpp
class MyData {
  fire_class_version(1);
  // ... rest of declaration omitted
};
```

Then we can handle the various versions that are being read by using `version()`
to find out which version we are trying to interpret.
```cpp
void MyData::attach(fire::io::Data<MyData>& d) {
  if (d.version() < 1) {
    // handle prior version 0 here
  } else {
    // handle current version 1 here
  }  
}
```
This naturally extends to several subsequent versions and clearly shows all users
how the data is being transformed when they are reading it off disk.

@note The attach method is only called once per processing run and therefore
  it **cannot** and **should not** be used to handle more complicated data
  evolutions involving _any_ sort of calculation. If you are calculating
  a new variable from an old variable, this falls outside of the realm
  of schema evolution and needs to be done within a Processor.

Now the question becomes, how to "handle" the different versions.
This is most easily explained with a few examples.

#### New Member
Most simply, we introduce a new member variable which wasn't in prior versions.
This can be achieved by letting the serialization know that for old versions,
it shouldn't try to load anything for that member variable from disk.
```cpp
void MyData::attach(fire::io::Data<MyData>& d) {
  if (d.version() < 1) {
    using fire::io::Data<MyData>::SaveLoad;
    d.attach("new_member", new_member_, SaveLoad::SaveOnly);
  } else {
    d.attach("new_member", new_member_);
  }
}
```

#### Remove Member
Less often but still possible, we can drop a member variable but still have it
read into memory when reading a prior version.
```cpp
void MyData::attach(fire::io::Data<MyData>& d) {
  if (d.version() < 1) {
    using fire::io::Data<MyData>::SaveLoad;
    d.attach("old_member", old_member_, SaveLoad::LoadOnly);
  } else {
    // we are dropping old_member so we set it to some absurd value
    //   in later versions
    old_member_ = 9999;
  }
}
```

#### Rename
Also commonly, we want to change the name on disk for a specific member variable.
In this case, we just need to let the serialization know that old versions will be
loaded from a different location than where they are saved.
```cpp
void MyData::attach(fire::io::Data<MyData>& d) {
  if (d.version() < 1) {
    using fire::io::Data<MyData>::SaveLoad;
    d.attach("old_name", member_, SaveLoad::LoadOnly);
    d.attach("new_name", member_, SaveLoad::SaveOnly);
  } else {
    d.attach("new_name", member_);
  }
}
```
This renaming scheme is expected to be common enough that there is a helper function
defined for it. The three lines within the early-version block can be replaced by
```cpp
d.rename("old_name","new_name",member_);
```
