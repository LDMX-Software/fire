
#include "fire/EventHeader.hpp"

namespace fire {

const std::string EventHeader::NAME = h5::Reader::EVENT_HEADER_NAME;

namespace h5 {

DataSet<EventHeader>::DataSet(EventHeader* handle)
    : AbstractDataSet<EventHeader>(Reader::EVENT_GROUP+"/"+EventHeader::NAME, true, handle) {
  members_.push_back(std::make_unique<DataSet<int>>(
      this->name_ + "/number", true, &(handle->number_)));
  members_.push_back(std::make_unique<DataSet<int>>(
      this->name_ + "/run", true, &(handle->run_)));
  members_.push_back(std::make_unique<DataSet<double>>(
      this->name_ + "/weight", true, &(handle->weight_)));
  members_.push_back(std::make_unique<DataSet<bool>>(
      this->name_ + "/isRealData", true, &(handle->isRealData_)));
  members_.push_back(std::make_unique<DataSet<long int>>(
      this->name_ + "/timestamp", true, &(handle->timestamp_)));
}

void DataSet<EventHeader>::load(Reader& r, long unsigned int i) {
  static bool first_load{true};
  for (auto& m : members_) m->load(r,i);
  if (first_load) {
    first_load = false;
    // first load - discovery - look through file to find parameters on disk
    for (auto pname : r.list(this->name_+"/parameters")) {
      std::string path{this->name_+"/parameters/"+pname};
      auto type{r.getDataSetType(path)};
      if (type == HighFive::DataTypeClass::Integer) {
        int i{};
        this->handle_->parameters_[pname] = i;
        parameters_[pname] = std::make_unique<DataSet<int>>(
            path, true, std::get_if<int>(&(this->handle_->parameters_[pname])));
      } else if (type == HighFive::DataTypeClass::Float) {
        float f{};
        this->handle_->parameters_[pname] = f;
        parameters_[pname] = std::make_unique<DataSet<float>>(
            path, true, std::get_if<float>(&(this->handle_->parameters_[pname])));
      } else {
        std::string s;
        this->handle_->parameters_[pname] = s;
        parameters_[pname] = std::make_unique<DataSet<std::string>>(
            path, true, std::get_if<std::string>(&(this->handle_->parameters_[pname])));
      }
    }
  }
  for (auto& [name, set] : parameters_) set->load(r,i);
}

void DataSet<EventHeader>::save(Writer& w, long unsigned int i) {
  for (auto& m : members_) m->save(w,i);
  // go through all of the EventHeader's parameters and make a new dataset
  // if it hasn't been made yet, and then save the dataset for that parameter
  for (auto& [name, val] : this->handle_->parameters_) {
    if (parameters_.find(name) == parameters_.end()) {
      // dataset not created yet
      if (std::holds_alternative<int>(val)) {
        parameters_[name] = std::make_unique<DataSet<int>>(
          this->name_ + "/parameters/"+name, true, std::get_if<int>(&val));
      } else if (std::holds_alternative<float>(val)) {
        parameters_[name] = std::make_unique<DataSet<float>>(
          this->name_ + "/parameters/"+name, true, std::get_if<float>(&val));
      } else {
        parameters_[name] = std::make_unique<DataSet<std::string>>(
          this->name_ + "/parameters/"+name, true, std::get_if<std::string>(&val));
      }
    } 
    parameters_[name]->save(w,i);
  }
}

}  // namespace fire::h5
}
