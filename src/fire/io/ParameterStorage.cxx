
#include "fire/io/ParameterStorage.h"

namespace fire::io {

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

Data<ParameterStorage>::Data(const std::string& path, ParameterStorage* handle)
    : AbstractData<ParameterStorage>(path, handle) {}

void Data<ParameterStorage>::load(h5::Reader& r) {
  static bool first_load{true};
  if (first_load) {
    first_load = false;
    // first load - discovery - look through file to find parameters on disk
    for (auto pname : r.list(this->path_)) {
      std::string path{this->path_+"/"+pname};
      auto type{r.getDataSetType(path).getClass()};
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

void Data<ParameterStorage>::save(Writer& w) {
  // go through all of the ParameterStorage's parameters and make a new dataset
  // if it hasn't been made yet, and then save the dataset for that parameter
  for (auto& [name, val] : this->handle_->parameters_) {
    if (parameters_.find(name) == parameters_.end()) {
      // dataset not created yet
      std::visit([&](auto && val) {
        // decltype - declares the type of the input expression
        // std::decay_t - removes any 'const' prefixes or references '&' or '*'
        attach<std::decay_t<decltype(val)>>(name);
      }, val);
    } 
    parameters_[name]->save(w);
  }
}

}  // namespace fire::h5
