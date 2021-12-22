#include "fire/StorageControl.h"

namespace fire {

StorageControl::StorageControl(const config::Parameters& ps)
    : default_keep_{ps.get<bool>("default_keep")} {
  for (const auto& listening_rule :
       ps.get<std::vector<config::Parameters>>("listening_rules")) {
    const auto& proc_regex_str = listening_rule.get<std::string>("processor");
    auto purp_regex_str = listening_rule.get<std::string>("purpose");
    // skip rules without processor pattern
    if (proc_regex_str.empty()) continue;
    // default purpose pattern is match all
    if (purp_regex_str.empty()) purp_regex_str = ".*";

    // since the types in the pair are the same, order really matters here
    // the order we put the rules into the pair needs to be the same as the
    // order used in the for-loop in addHint
    rules_.emplace_back(
        std::piecewise_construct,
        std::forward_as_tuple(proc_regex_str,
                              std::regex::extended | std::regex::nosubs),
        std::forward_as_tuple(purp_regex_str,
                              std::regex::extended | std::regex::nosubs));
  }
}

void StorageControl::resetEventState() { hints_.clear(); }

void StorageControl::addHint(Hint hint, const std::string& purpose,
                              const std::string& processor_name) {
  for (const auto& [proc_rule, purp_rule] : rules_) {
    // only count this hint if the processor and purpose
    // provided match the rule
    if (std::regex_match(processor_name, proc_rule) and
        std::regex_match(purpose, purp_rule)) {
      // store hint for later tallying
      hints_.push_back(hint);
      // leave after first rule match to avoid double-counting
      break;
    }
  }
}

bool StorageControl::keepEvent() const {
  int votesKeep(0), votesDrop(0);
  // loop over all rules and then over all hints
  for (auto hint : hints_) {
    if (hint == Hint::ShouldKeep || hint == Hint::MustKeep)
      votesKeep++;
    else if (hint == Hint::ShouldDrop || hint == Hint::MustDrop)
      votesDrop++;
  }

  // easy case
  if (!votesKeep && !votesDrop) return default_keep_;

  // harder cases
  if (votesKeep > votesDrop) return true;
  if (votesDrop > votesKeep) return false;

  // at the end, go with the default
  return default_keep_;
}
}  // namespace fire
