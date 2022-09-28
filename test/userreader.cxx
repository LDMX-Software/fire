#include <boost/test/tools/interface.hpp>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "fire/UserReader.h"

/// this filename needs to exactly match the one produced by highlevel
static std::string file_to_read = "prod_drop_async.h5";

/**
 * UserReader functionality
 *
 * can we read event objects correctly?
 */
BOOST_AUTO_TEST_SUITE(userreader)

BOOST_AUTO_TEST_CASE(noskip, *boost::unit_test::depends_on("highlevel/prod_drop_async")) {
  fire::UserReader r;
  r.open(file_to_read);

  int i{1};
  while (r.next()) {
    BOOST_CHECK(r.get<int>("keepalong") == i++);
  }
}

BOOST_AUTO_TEST_CASE(skip, *boost::unit_test::depends_on("userreader/noskip")) {
  fire::UserReader r;
  r.open(file_to_read, 3);

  int i{4};
  while (r.next()) {
    BOOST_CHECK(r.get<int>("keepalong") == i++);
  }
}

BOOST_AUTO_TEST_CASE(loop, *boost::unit_test::depends_on("userreader/skip")) {
  fire::UserReader r(true);
  r.open(file_to_read, 3);

  for (std::size_t i{4}; i < r.entries()+5; ++i) {
    BOOST_CHECK(r.next());
    std::size_t correct{i % r.entries()};
    if (correct == 0) correct = r.entries();
    // index and event number are off-by-one
    BOOST_CHECK(r.get<int>("keepalong") == correct);
  }
}

BOOST_AUTO_TEST_SUITE_END()
