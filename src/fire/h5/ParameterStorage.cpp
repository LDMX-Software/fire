
#include "fire/ParameterStorage.hpp"

namespace fire::h5 {

DataSet<ParameterStorage>::DataSet(const std::string& name, ParameterStorage* handle)
    : AbstractDataSet<ParameterStorage>(name, true, handle) {}

void DataSet<ParameterStorage>::load(Reader& r, long unsigned int i) {
  static bool first_load{true};
  if (first_load) {
    first_load = false;
    // first load - discovery - look through file to find parameters on disk
    for (auto pname : r.list(this->name_)) {
      std::string path{this->name_+"/"+pname};
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

void DataSet<ParameterStorage>::save(Writer& w, long unsigned int i) {
  // go through all of the ParameterStorage's parameters and make a new dataset
  // if it hasn't been made yet, and then save the dataset for that parameter
  for (auto& [name, val] : this->handle_->parameters_) {
    if (parameters_.find(name) == parameters_.end()) {
      // dataset not created yet
      if (std::holds_alternative<int>(val)) {
        parameters_[name] = std::make_unique<DataSet<int>>(
          this->name_ + "/"+name, true, std::get_if<int>(&val));
      } else if (std::holds_alternative<float>(val)) {
        parameters_[name] = std::make_unique<DataSet<float>>(
          this->name_ + "/"+name, true, std::get_if<float>(&val));
      } else {
        parameters_[name] = std::make_unique<DataSet<std::string>>(
          this->name_ + "/"+name, true, std::get_if<std::string>(&val));
      }
    } 
    parameters_[name]->save(w,i);
  }
}

}  // namespace fire::h5
}
