
#include "fire/h5/ParameterStorage.h"

namespace fire::h5 {

void ParameterStorage::clear() {
  for (auto& [_, pval] : parameters_) {
    std::visit([&](auto && val) {
      if constexpr (std::is_same_v<std::decay_t<decltype(val)>,std::string>) {
        val.clear();
      } else {
        val = std::numeric_limits<std::decay_t<decltype(val)>>::min();
      }
    }, pval);
  }
}

DataSet<ParameterStorage>::DataSet(const std::string& name, ParameterStorage* handle)
    : AbstractDataSet<ParameterStorage>(name, handle) {}

void DataSet<ParameterStorage>::load(Reader& r) {
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
        attach<int>(pname);
      } else if (type == HighFive::DataTypeClass::Float) {
        float f{};
        this->handle_->parameters_[pname] = f;
        attach<float>(pname);
      } else {
        std::string s;
        this->handle_->parameters_[pname] = s;
        attach<std::string>(pname);
      }
    }
  }
  for (auto& [name, set] : parameters_) set->load(r);
}

void DataSet<ParameterStorage>::save(Writer& w) {
  // go through all of the ParameterStorage's parameters and make a new dataset
  // if it hasn't been made yet, and then save the dataset for that parameter
  for (auto& [name, val] : this->handle_->parameters_) {
    if (parameters_.find(name) == parameters_.end()) {
      // dataset not created yet
      std::visit([&](auto && val) {
        attach<std::decay_t<decltype(val)>>(name);
      }, val);
    } 
    parameters_[name]->save(w);
  }
}

}  // namespace fire::h5
