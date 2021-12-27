
#include "fire/Processor.h"
#include "MyObject.h"

namespace mymodule {

class MyProducer : public fire::Producer {
 public:
  MyProducer(const fire::config::Parameters& ps)
    : fire::Producer(ps) {}
  ~MyProducer() = default;
  void produce(fire::Event& event) final override {
    MyObject eo;
    event.add("MyEventObject",eo);
  }
};  // MyProducer

}

DECLARE_PROCESSOR_NS(mymodule,MyProducer);
