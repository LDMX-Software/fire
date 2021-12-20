
#include "fire/EventHeader.hpp"

namespace fire {

const std::string EventHeader::NAME = h5::Reader::EVENT_HEADER_NAME;

namespace h5 {

DataSet<EventHeader>::DataSet(EventHeader* handle)
    : AbstractDataSet<EventHeader>("events/"+EventHeader::NAME, true, handle) {
  members_.push_back(std::make_unique<DataSet<int>>(
      this->name_ + "/number", true, &(handle->number_)));
  members_.push_back(std::make_unique<DataSet<int>>(
      this->name_ + "/run", true, &(handle->run_)));
  members_.push_back(std::make_unique<DataSet<double>>(
      this->name_ + "/weight", true, &(handle->weight_)));
  members_.push_back(std::make_unique<DataSet<bool>>(
      this->name_ + "/isRealData", true, &(handle->isRealData_)));
  members_.push_back(std::make_unique<DataSet<int>>(
      this->name_ + "/timestamp", true, &(handle->timestamp_)));
}

void DataSet<EventHeader>::load(Reader& r, long unsigned int i) {
  // discovery - look through file to try to find parameters on disk
}

void DataSet<EventHeader>::save(Writer& w, long unsigned int i) {
  for (auto& m : members_) m->save(w,i);
  for (auto& [name, val] : this->handle_->parameters_) {
    if (parameters_.find(name) == parameters_.end()) {
      // dataset not created yet
      if (std::holds_alternative<int>(val)) {
        parameters_[name] = std::make_unique<DataSet<int>>(
          this->name_ + "/parameters/"+name, true, std::get_if<int>(&val));
      } else if (std::holds_alternative<float>(val)) {
        parameters_[name] = std::make_unique<DataSet<float>>(
          this->name_ + "/parameters/"+name, true, std::get_if<float>(&val));
      } 
      /// TODO enable serialization of std string
      /*
      else {
        parameters_[name] = std::make_unique<DataSet<std::string>>(
          this->name_ + "/parameters/"+name, true, std::get_if<std::string>(&val));
      }
      */
    } 
    parameters_[name]->save(w,i);
  }
}

}  // namespace fire::h5
}
