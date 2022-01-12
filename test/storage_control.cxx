#include <boost/test/tools/interface.hpp>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "fire/StorageControl.h"

/**
 * Test configuration of storage control object
 *
 * Checks:
 * - listens to certain processors
 * - does NOT listen to others
 * - tallys votes as expected
 * - listens to certain purposes across processors
 */
BOOST_AUTO_TEST_SUITE(storage_control)

BOOST_AUTO_TEST_CASE(no_rules) {
  fire::config::Parameters storage;
  storage.add("default_keep",true);

  fire::StorageControl sc{storage};

  // should just be default
  BOOST_TEST(sc.keepEvent() == true);

  // should still be default even if we drop from
  // a processor that is not being listened to
  sc.addHint(fire::StorageControl::Hint::MustDrop, "", "TestProc");
  BOOST_TEST(sc.keepEvent() == true);
}

BOOST_AUTO_TEST_CASE(specific_processors) {
  fire::config::Parameters storage;
  storage.add("default_keep",true);

  std::vector<fire::config::Parameters> listening_rules;
  fire::config::Parameters listening_rule;
  listening_rule.add<std::string>("processor", ".*Listen.*");
  listening_rule.add<std::string>("purpose", "");
  listening_rules.push_back(listening_rule);
  storage.add("listening_rules",listening_rules);

  fire::StorageControl sc{storage};

  // no hints, still default
  BOOST_TEST(sc.keepEvent() == true);

  // ignore processor not named matching listening rule
  sc.addHint(fire::StorageControl::Hint::MustDrop, "", "TestProc");
  BOOST_TEST(sc.keepEvent() == true);

  // listen to processor with matching name
  sc.addHint(fire::StorageControl::Hint::MustDrop, "", "ListenToMe");
  BOOST_TEST(sc.keepEvent() == false);

}

BOOST_AUTO_TEST_CASE(purposes) {
  fire::config::Parameters storage;
  storage.add("default_keep",true);

  std::vector<fire::config::Parameters> listening_rules;
  fire::config::Parameters listening_rule;
  listening_rule.add<std::string>("processor", "");
  listening_rule.add<std::string>("purpose", ".*Listen.*");
  listening_rules.push_back(listening_rule);
  storage.add("listening_rules",listening_rules);

  fire::StorageControl sc{storage};

  // no hints, still default
  BOOST_TEST(sc.keepEvent() == true);

  // ignore processor with unhelpful purposes
  sc.addHint(fire::StorageControl::Hint::MustDrop, "TestWrong", "TestProc");
  BOOST_TEST(sc.keepEvent() == true);

  // listen to processor with matching purpose
  sc.addHint(fire::StorageControl::Hint::MustDrop, "ListenToMe", "TestProc");
  BOOST_TEST(sc.keepEvent() == false);
}

BOOST_AUTO_TEST_CASE(voting) {
  fire::config::Parameters storage;
  storage.add("default_keep", false);

  std::vector<fire::config::Parameters> listening_rules;
  fire::config::Parameters listening_rule;
  listening_rule.add<std::string>("processor", "");
  listening_rule.add<std::string>("purpose", "");
  listening_rules.push_back(listening_rule);
  storage.add("listening_rules",listening_rules);

  fire::StorageControl sc{storage};

  // no hints, default
  BOOST_TEST(sc.keepEvent() == false);

  // one keep vote overrides default
  sc.addHint(fire::StorageControl::Hint::ShouldKeep, "Test", "Test");
  BOOST_TEST(sc.keepEvent() == true);

  // clear previous hints
  sc.resetEventState();

  // more keep votes, changes outcome
  // no difference between Must and Should
  sc.addHint(fire::StorageControl::Hint::MustDrop, "Test", "Test");
  sc.addHint(fire::StorageControl::Hint::MustKeep, "Test", "Test");
  sc.addHint(fire::StorageControl::Hint::ShouldKeep, "Test", "Test");
  BOOST_TEST(sc.keepEvent() == true);
}

BOOST_AUTO_TEST_SUITE(errors)

BOOST_AUTO_TEST_CASE(bad_regex) {
  fire::config::Parameters storage;
  storage.add("default_keep",true);

  std::vector<fire::config::Parameters> listening_rules;
  fire::config::Parameters listening_rule;
  listening_rule.add<std::string>("processor", "[a-b][a");
  listening_rule.add<std::string>("purpose", "");
  listening_rules.push_back(listening_rule);
  storage.add("listening_rules",listening_rules);

  BOOST_CHECK_THROW(fire::StorageControl sc{storage}, fire::Exception);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
