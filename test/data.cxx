#include <boost/test/tools/interface.hpp>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <highfive/H5Easy.hpp>

#include "fire/EventHeader.h"
#include "fire/RunHeader.h"
#include "fire/config/Parameters.h"
#include "fire/io/Data.h"

// plain old data class
class Hit {
  double energy_;
  int id_;
 private:
  friend class fire::io::Data<Hit>;
  void attach(fire::io::Data<Hit>& d) {
    d.attach("energy",energy_);
    d.attach("id",id_);
  }
 public:
  Hit() = default;
  Hit(double e, int id) : energy_{e}, id_{id} {}
  bool operator==(Hit const& other) const {
    return energy_ == other.energy_ and id_ == other.id_;
  }
  void clear() {
    energy_ = 0.;
    id_ = 0;
  }
};

// class with nested class
class SpecialHit {
  int super_id_;
  Hit hit_;
 private:
  friend class fire::io::Data<SpecialHit>;
  void attach(fire::io::Data<SpecialHit>& d) {
    d.attach("super_id",super_id_);
    d.attach("hit",hit_);
  }
 public:
  SpecialHit() = default;
  SpecialHit(int sid, Hit h) : super_id_{sid}, hit_{h} {}
  bool operator==(SpecialHit const& other) const {
    return super_id_ == other.super_id_ and hit_ == other.hit_;
  }
  void clear() {
    hit_.clear();
    super_id_ = 0;
  }
}; // SpecialHit

// class with vector of nested class
class Cluster {
  int id_;
  std::vector<Hit> hits_;
 private:
  friend class fire::io::Data<Cluster>;
  void attach(fire::io::Data<Cluster>& d) {
    d.attach("id", id_);
    d.attach("hits",hits_);
  }
 public:
  Cluster() = default;
  Cluster(int id, std::vector<Hit> const& hits) : id_{id}, hits_{hits} {}
  bool operator==(Cluster const& other) const {
    if (id_ != other.id_) { return false; }
    if (hits_.size() != other.hits_.size()) { return false; }
    for (std::size_t i{0}; i < hits_.size(); i++) {
      if (not(hits_.at(i) == other.hits_.at(i))) {
        return false;
      }
    }
    return true;
  }
  void clear() {
    id_ = 0;
    hits_.clear();
  }
};

template <typename ArbitraryData, typename DataType>
bool save(ArbitraryData& h5d, DataType const& d, fire::io::Writer& f) {
  try {
    h5d.update(d);
    h5d.save(f);
    return true;
  } catch (std::exception const& e) {
    std::cout << e.what() << std::endl;
    return false;
  }
}

template <typename ArbitraryData, typename DataType>
bool load(ArbitraryData& h5d, DataType const& d, fire::io::h5::Reader& f) {
  try {
    h5d.load(f);
    return (d == h5d.get());
  } catch (std::exception const& e) {
    std::cout << e.what() << std::endl;
    return false;
  }
}

static std::string filename{"datad.h5"};
static std::string copy_file{"copy_"+filename};

static std::vector<double> doubles = { 1.0, 32., 69. };
static std::vector<int>    ints    = { 0, -33, 88 };
static std::vector<
         std::vector<Hit>
       > all_hits = {
         {Hit(25.,1), Hit(32.,2), Hit(1234.,10)},
         {Hit(25.,1), Hit(32.,2), Hit(23.,10), Hit(321.,69)},
         {Hit(2.,1), Hit(31.,6)}
       };


BOOST_AUTO_TEST_SUITE(data)

BOOST_AUTO_TEST_CASE(write) {
  fire::config::Parameters output_params;
  output_params.add("name",filename);
  output_params.add("rows_per_chunk",2);
  output_params.add("compression_level", 6);
  output_params.add("shuffle",false);
  int num_events = doubles.size(); //allow implicit conversion
  fire::io::Writer f{num_events,output_params};

  fire::EventHeader eh;
  fire::io::Data<fire::EventHeader> event_header(fire::EventHeader::NAME,&eh);
  fire::io::Data<double> double_ds("double");
  fire::io::Data<int>    int_ds("int");
  fire::io::Data<bool>   bool_ds("bool");
  fire::io::Data<std::vector<double>> vector_double_ds("vector_double");
  fire::io::Data<std::vector<int>> vector_int_ds("vector_int");
  fire::io::Data<std::map<int,double>> map_int_double_ds("map_int_double");
  fire::io::Data<Hit> hit_ds("hit");
  fire::io::Data<std::vector<Hit>> vector_hit_ds("vector_hit");
  fire::io::Data<SpecialHit> special_hit_ds("special_hit");
  fire::io::Data<std::vector<SpecialHit>> vector_special_hit_ds("vector_special_hit");
  fire::io::Data<Cluster> cluster_ds("cluster");
  fire::io::Data<std::vector<Cluster>> vector_cluster_ds("vector_cluster");
  fire::io::Data<std::map<int,Cluster>> map_cluster_ds("map_cluster");

  for (std::size_t i_entry{0}; i_entry < doubles.size(); i_entry++) {
    eh.setEventNumber(i_entry);
    // check dynamic parameters
    eh.set("istring",std::to_string(i_entry));
    eh.set("int",int(i_entry));
    eh.set("float",float(i_entry*10.));

    BOOST_CHECK(save(event_header,eh,f));
    BOOST_CHECK(save(double_ds,doubles.at(i_entry),f));
    BOOST_CHECK(save(int_ds,ints.at(i_entry),f));

    bool positive{ints.at(i_entry) > 0};
    BOOST_CHECK(save(bool_ds,positive,f));

    BOOST_CHECK(save(vector_double_ds,doubles,f));
    BOOST_CHECK(save(vector_int_ds,ints,f));
    std::map<int,double> map_int_double;
    for (std::size_t i{0}; i < ints.size(); i++) {
      map_int_double[ints.at(i)] = doubles.at(i);
    }
    BOOST_CHECK(save(map_int_double_ds,map_int_double,f));

    BOOST_CHECK(save(hit_ds,all_hits[i_entry][0],f));
    BOOST_CHECK(save(vector_hit_ds,all_hits[i_entry],f));
    BOOST_CHECK(save(special_hit_ds,SpecialHit(0,all_hits[i_entry][0]),f));

    std::vector<SpecialHit> sphit_vec;
    for (auto& hit : all_hits[i_entry]) sphit_vec.emplace_back(i_entry,hit);
    BOOST_CHECK(save(vector_special_hit_ds,sphit_vec,f));

    auto c = Cluster(i_entry, all_hits.at(i_entry));
    BOOST_CHECK(save(cluster_ds,c,f));

    std::vector<Cluster> clusters;
    clusters.emplace_back(2, all_hits.at(0));
    clusters.emplace_back(3, all_hits.at(1));
    BOOST_CHECK(save(vector_cluster_ds,clusters,f));

    std::map<int,Cluster> map_clusters = {
      { 2, Cluster(2, all_hits.at(0)) },
      { 3, Cluster(3, all_hits.at(1)) }
    };
    BOOST_CHECK(save(map_cluster_ds,map_clusters,f));
  }

  event_header.done(f);
  double_ds.done(f);
  int_ds.done(f);
  bool_ds.done(f);
  vector_double_ds.done(f);
  vector_int_ds.done(f);
  map_int_double_ds.done(f);
  hit_ds.done(f);
  vector_hit_ds.done(f);
  special_hit_ds.done(f);
  vector_special_hit_ds.done(f);
  cluster_ds.done(f);
  vector_cluster_ds.done(f);
  map_cluster_ds.done(f);

  // reader requires at least one run so that it can deduced
  // the number of runs upon construction
  fire::io::Data<fire::RunHeader> rh_d(fire::io::constants::RUN_HEADER_NAME);
  rh_d.save(f);
}

BOOST_AUTO_TEST_CASE(copy, *boost::unit_test::depends_on("data/write")) {
  fire::io::h5::Reader reader{filename};
  fire::config::Parameters output_params;
  output_params.add("name",copy_file);
  output_params.add("rows_per_chunk",2);
  output_params.add("compression_level", 6);
  output_params.add("shuffle",false);
  int num_events = doubles.size(); //allow implicit conversion
  fire::io::Writer writer{num_events,output_params};

  std::vector<std::string> objects_to_copy = {
    fire::EventHeader::NAME,
    "double",
    "int",
    "bool",
    "vector_double",
    "vector_int",
    "map_int_double",
    "hit",
    "vector_hit",
    "special_hit",
    "vector_special_hit",
    "cluster",
    "vector_cluster",
    "map_cluster"
  };

  for (std::size_t i_entry{0}; i_entry < doubles.size(); i_entry++) {
    for (const auto& obj : objects_to_copy) {
      reader.copy(i_entry, obj, writer);
    }
  }

  // reader requires at least one run so that it can deduced
  // the number of runs upon construction
  fire::io::Data<fire::RunHeader> rh_d(fire::io::constants::RUN_HEADER_NAME);
  rh_d.save(writer);

  writer.flush();

  HighFive::File f{copy_file};
  // check for existence
  for (const auto& obj : objects_to_copy) BOOST_TEST(f.exist(obj));
  // check for correctness done implicitly in the read check below
}

BOOST_AUTO_TEST_CASE(read, *boost::unit_test::depends_on("data/copy")) {
  fire::io::h5::Reader f{copy_file};

  fire::EventHeader eh;
  fire::io::Data<fire::EventHeader> event_header(fire::EventHeader::NAME,&eh);
  fire::io::Data<double> double_ds("double");
  fire::io::Data<int>    int_ds("int");
  fire::io::Data<bool>   bool_ds("bool");
  fire::io::Data<std::vector<double>> vector_double_ds("vector_double");
  fire::io::Data<std::vector<int>> vector_int_ds("vector_int");
  fire::io::Data<std::map<int,double>> map_int_double_ds("map_int_double");
  fire::io::Data<Hit> hit_ds("hit");
  fire::io::Data<std::vector<Hit>> vector_hit_ds("vector_hit");
  fire::io::Data<SpecialHit> special_hit_ds("special_hit");
  fire::io::Data<std::vector<SpecialHit>> vector_special_hit_ds("vector_special_hit");
  fire::io::Data<Cluster> cluster_ds("cluster");
  fire::io::Data<std::vector<Cluster>> vector_cluster_ds("vector_cluster");
  fire::io::Data<std::map<int,Cluster>> map_cluster_ds("map_cluster");

  for (std::size_t i_entry{0}; i_entry < doubles.size(); i_entry++) {
    event_header.load(f);

    BOOST_CHECK(eh.getEventNumber() == i_entry);
    BOOST_CHECK(eh.get<std::string>("istring") == std::to_string(i_entry));
    BOOST_CHECK(eh.get<int>("int") == i_entry);
    BOOST_CHECK(eh.get<float>("float") == 10.*i_entry);
    
    BOOST_CHECK(load(double_ds,doubles.at(i_entry),f));
    BOOST_CHECK(load(int_ds,ints.at(i_entry),f));
    bool positive{ints.at(i_entry) > 0};
    BOOST_CHECK(load(bool_ds,positive,f));
    BOOST_CHECK(load(vector_double_ds,doubles,f));
    BOOST_CHECK(load(vector_int_ds,ints,f));
    std::map<int,double> map_int_double;
    for (std::size_t i{0}; i < ints.size(); i++) {
      map_int_double[ints.at(i)] = doubles.at(i);
    }
    BOOST_CHECK(load(map_int_double_ds,map_int_double,f));

    BOOST_CHECK(load(hit_ds,all_hits[i_entry][0],f));
    BOOST_CHECK(load(vector_hit_ds,all_hits[i_entry],f));
    BOOST_CHECK(load(special_hit_ds,SpecialHit(0,all_hits[i_entry][0]),f));

    std::vector<SpecialHit> sphit_vec;
    for (auto& hit : all_hits[i_entry]) sphit_vec.emplace_back(i_entry,hit);
    BOOST_CHECK(load(vector_special_hit_ds,sphit_vec,f));

    auto c = Cluster(i_entry, all_hits.at(i_entry));
    BOOST_CHECK(load(cluster_ds,c,f));

    std::vector<Cluster> clusters;
    clusters.emplace_back(2, all_hits.at(0));
    clusters.emplace_back(3, all_hits.at(1));
    BOOST_CHECK(load(vector_cluster_ds,clusters,f));

    std::map<int,Cluster> map_clusters = {
      { 2, Cluster(2, all_hits.at(0)) },
      { 3, Cluster(3, all_hits.at(1)) }
    }, read_map;
    BOOST_CHECK_NO_THROW(map_cluster_ds.load(f));
    BOOST_CHECK_NO_THROW(read_map = map_cluster_ds.get());
    for (auto const& [key,val] : map_clusters) {
      auto mit{read_map.find(key)};
      BOOST_CHECK(mit != read_map.end());
      BOOST_CHECK(mit->second == val);
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
