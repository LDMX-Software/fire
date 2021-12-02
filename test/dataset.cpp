#include <boost/test/tools/interface.hpp>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "highfive/H5Easy.hpp"
#include "fire/h5/DataSet.hpp"

// plain old data class
class Hit {
  double energy_;
  int id_;
 private:
  friend class fire::h5::DataSet<Hit>;
  void attach(fire::h5::DataSet<Hit>& set) {
    set.attach("energy",energy_);
    set.attach("id",id_);
  }
 public:
  Hit() = default;
  Hit(double e, int id) : energy_{e}, id_{id} {}
  bool operator==(Hit const& other) const {
    return energy_ == other.energy_ and id_ == other.id_;
  }
};

// class with nested class
class SpecialHit {
  int super_id_;
  Hit hit_;
 private:
  friend class fire::h5::DataSet<SpecialHit>;
  void attach(fire::h5::DataSet<SpecialHit>& set) {
    set.attach("super_id",super_id_);
    set.attach("hit",hit_);
  }
 public:
  SpecialHit() = default;
  SpecialHit(int sid, Hit h) : super_id_{sid}, hit_{h} {}
  bool operator==(SpecialHit const& other) const {
    return super_id_ == other.super_id_ and hit_ == other.hit_;
  }
}; // SpecialHit

// class with vector of nested class
class Cluster {
  int id_;
  std::vector<Hit> hits_;
 private:
  friend class fire::h5::DataSet<Cluster>;
  void attach(fire::h5::DataSet<Cluster>& set) {
    set.attach("id", id_);
    set.attach("hits",hits_);
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
};

template <typename ArbitraryDataSet, typename DataType>
bool save(ArbitraryDataSet& set, DataType const& d, fire::h5::File& f, long unsigned int i) {
  try {
    set.update(d);
    set.save(f,i);
    return true;
  } catch (std::exception const& e) {
    std::cout << e.what() << std::endl;
    return false;
  }
}

template <typename ArbitraryDataSet, typename DataType>
bool load(ArbitraryDataSet& set, DataType const& d, fire::h5::File& f, long unsigned int i) {
  try {
    set.load(f,i);
    return (d == set.get());
  } catch (std::exception const& e) {
    std::cout << e.what() << std::endl;
    return false;
  }
}

BOOST_AUTO_TEST_CASE(dataset) {
  std::string filename{"dataset.h5"};

  std::vector<double> doubles = { 1.0, 32., 69. };
  std::vector<int>    ints    = { 0, -33, 88 };
  std::vector<
    std::vector<Hit>
    > all_hits = {
      {Hit(25.,1), Hit(32.,2), Hit(1234.,10)},
      {Hit(25.,1), Hit(32.,2), Hit(23.,10), Hit(321.,69)},
      {Hit(2.,1), Hit(31.,6)}
    };


  { // Writing
    fire::h5::File f{filename, true}; 

    fire::h5::DataSet<double> double_ds("double");
    fire::h5::DataSet<int>    int_ds("int");
    fire::h5::DataSet<bool>   bool_ds("bool");
    fire::h5::DataSet<std::vector<double>> vector_double_ds("vector_double");
    fire::h5::DataSet<std::vector<int>> vector_int_ds("vector_int");
    fire::h5::DataSet<std::map<int,double>> map_int_double_ds("map_int_double");
    fire::h5::DataSet<Hit> hit_ds("hit");
    fire::h5::DataSet<std::vector<Hit>> vector_hit_ds("vector_hit");
    fire::h5::DataSet<SpecialHit> special_hit_ds("special_hit");
    fire::h5::DataSet<std::vector<SpecialHit>> vector_special_hit_ds("vector_special_hit");
    fire::h5::DataSet<Cluster> cluster_ds("cluster");
    fire::h5::DataSet<std::vector<Cluster>> vector_cluster_ds("vector_cluster");
    fire::h5::DataSet<std::map<int,Cluster>> map_cluster_ds("map_cluster");

    for (std::size_t i_entry{0}; i_entry < doubles.size(); i_entry++) {
      BOOST_CHECK(save(double_ds,doubles.at(i_entry),f,i_entry));
      BOOST_CHECK(save(int_ds,ints.at(i_entry),f,i_entry));

      bool positive{ints.at(i_entry) > 0};
      BOOST_CHECK(save(bool_ds,positive,f,i_entry));

      BOOST_CHECK(save(vector_double_ds,doubles,f,i_entry));
      BOOST_CHECK(save(vector_int_ds,ints,f,i_entry));
      std::map<int,double> map_int_double;
      for (std::size_t i{0}; i < ints.size(); i++) {
        map_int_double[ints.at(i)] = doubles.at(i);
      }
      BOOST_CHECK(save(map_int_double_ds,map_int_double,f,i_entry));

      BOOST_CHECK(save(hit_ds,all_hits[i_entry][0],f,i_entry));
      BOOST_CHECK(save(vector_hit_ds,all_hits[i_entry],f,i_entry));
      BOOST_CHECK(save(special_hit_ds,SpecialHit(0,all_hits[i_entry][0]),f,i_entry));

      std::vector<SpecialHit> sphit_vec;
      for (auto& hit : all_hits[i_entry]) sphit_vec.emplace_back(i_entry,hit);
      BOOST_CHECK(save(vector_special_hit_ds,sphit_vec,f,i_entry));

      auto c = Cluster(i_entry, all_hits.at(i_entry));
      BOOST_CHECK(save(cluster_ds,c,f,i_entry));

      std::vector<Cluster> clusters;
      clusters.emplace_back(2, all_hits.at(0));
      clusters.emplace_back(3, all_hits.at(1));
      BOOST_CHECK(save(vector_cluster_ds,clusters,f,i_entry));

      std::map<int,Cluster> map_clusters = {
        { 2, Cluster(2, all_hits.at(0)) },
        { 3, Cluster(3, all_hits.at(1)) }
      };
      BOOST_CHECK(save(map_cluster_ds,map_clusters,f,i_entry));
    }
  }

  { // Reading
    fire::h5::File f{filename};

    fire::h5::DataSet<double> double_ds("double");
    fire::h5::DataSet<int>    int_ds("int");
    fire::h5::DataSet<bool>   bool_ds("bool");
    fire::h5::DataSet<std::vector<double>> vector_double_ds("vector_double");
    fire::h5::DataSet<std::vector<int>> vector_int_ds("vector_int");
    fire::h5::DataSet<std::map<int,double>> map_int_double_ds("map_int_double");
    fire::h5::DataSet<Hit> hit_ds("hit");
    fire::h5::DataSet<std::vector<Hit>> vector_hit_ds("vector_hit");
    fire::h5::DataSet<SpecialHit> special_hit_ds("special_hit");
    fire::h5::DataSet<std::vector<SpecialHit>> vector_special_hit_ds("vector_special_hit");
    fire::h5::DataSet<Cluster> cluster_ds("cluster");
    fire::h5::DataSet<std::vector<Cluster>> vector_cluster_ds("vector_cluster");
    fire::h5::DataSet<std::map<int,Cluster>> map_cluster_ds("map_cluster");

    for (std::size_t i_entry{0}; i_entry < doubles.size(); i_entry++) {
      BOOST_CHECK(load(double_ds,doubles.at(i_entry),f,i_entry));
      BOOST_CHECK(load(int_ds,ints.at(i_entry),f,i_entry));
      bool positive{ints.at(i_entry) > 0};
      BOOST_CHECK(load(bool_ds,positive,f,i_entry));
      BOOST_CHECK(load(vector_double_ds,doubles,f,i_entry));
      BOOST_CHECK(load(vector_int_ds,ints,f,i_entry));
      std::map<int,double> map_int_double;
      for (std::size_t i{0}; i < ints.size(); i++) {
        map_int_double[ints.at(i)] = doubles.at(i);
      }
      BOOST_CHECK(load(map_int_double_ds,map_int_double,f,i_entry));
      BOOST_CHECK(load(hit_ds,all_hits[i_entry][0],f,i_entry));
      BOOST_CHECK(load(vector_hit_ds,all_hits[i_entry],f,i_entry));
      BOOST_CHECK(load(special_hit_ds,SpecialHit(0,all_hits[i_entry][0]),f,i_entry));

      std::vector<SpecialHit> sphit_vec;
      for (auto& hit : all_hits[i_entry]) sphit_vec.emplace_back(i_entry,hit);
      BOOST_CHECK(load(vector_special_hit_ds,sphit_vec,f,i_entry));

      auto c = Cluster(i_entry, all_hits.at(i_entry));
      BOOST_CHECK(load(cluster_ds,c,f,i_entry));

      std::vector<Cluster> clusters;
      clusters.emplace_back(2, all_hits.at(0));
      clusters.emplace_back(3, all_hits.at(1));
      BOOST_CHECK(load(vector_cluster_ds,clusters,f,i_entry));

      std::map<int,Cluster> map_clusters = {
        { 2, Cluster(2, all_hits.at(0)) },
        { 3, Cluster(3, all_hits.at(1)) }
      }, read_map;
      BOOST_CHECK_NO_THROW(map_cluster_ds.load(f,i_entry));
      BOOST_CHECK_NO_THROW(read_map = map_cluster_ds.get());
      for (auto const& [key,val] : map_clusters) {
        auto mit{read_map.find(key)};
        BOOST_CHECK(mit != read_map.end());
        BOOST_CHECK(mit->second == val);
      }
    }
  }
}
