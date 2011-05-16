#include <algorithm>
#include <unordered_map>
#include <vector>
#include <utility>  // for std::pair

#include "MurmurHash2.h"
#include "mph_table.h"

namespace cxxmph {

// Save on repetitive typing.
#define MPH_MAP_TMPL_SPEC template <class Key, class Data, class HashFcn, class EqualKey, class Alloc>
#define MPH_MAP_CLASS_SPEC mph_map<Key, Data, HashFcn, EqualKey, Alloc>
#define MPH_MAP_METHOD_DECL(r, m) MPH_MAP_TMPL_SPEC typename MPH_MAP_CLASS_SPEC::r MPH_MAP_CLASS_SPEC::m

template <class Key, class Data, class HashFcn = std::hash<Key>, class EqualKey = std::equal_to<Key>, class Alloc = std::allocator<Data> >
class mph_map {
 public:
  typedef Key key_type;
  typedef Data data_type;
  typedef std::pair<Key, Data> value_type;
  typedef HashFcn hasher;
  typedef EqualKey key_equal;

  typedef typename std::vector<value_type>::pointer pointer;
  typedef typename std::vector<value_type>::reference reference;
  typedef typename std::vector<value_type>::const_reference const_reference;
  typedef typename std::vector<value_type>::size_type size_type;
  typedef typename std::vector<value_type>::difference_type difference_type;
  typedef typename std::vector<value_type>::iterator iterator;
  typedef typename std::vector<value_type>::const_iterator const_iterator;

  // For making macros simpler.
  typedef void void_type;
  typedef bool bool_type;
  typedef std::pair<iterator, bool> insert_return_type;

  mph_map();
  ~mph_map();

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  size_type size() const;
  bool empty() const;
  void clear();
  void erase(iterator pos);
  void erase(const key_type& k);
  std::pair<iterator, bool> insert(const value_type& x);
  iterator find(const key_type& k);
  const_iterator find(const key_type& k) const;
  data_type& operator[](const key_type &k);

  void pack() { rehash(); }

 private:
   template <typename iterator>
   struct iterator_first : public iterator {
     iterator_first(iterator it) : iterator(it) { }
      const typename iterator::value_type::first_type& operator*() const {
      return this->iterator::operator*().first;
     }
   };

   template <typename iterator>
     iterator_first<iterator> make_iterator_first(iterator it) {
     return iterator_first<iterator>(it);
   }

   void rehash();
   std::vector<value_type> values_;
   SimpleMPHTable<Key, typename seeded_hash<HashFcn>::hash_function> table_;
   // TODO(davi) optimize slack to no hold a copy of the key
   typedef typename std::unordered_map<Key, uint32_t, HashFcn, EqualKey, Alloc> slack_type;
   slack_type slack_;
};

MPH_MAP_TMPL_SPEC
bool operator==(const MPH_MAP_CLASS_SPEC& lhs, const MPH_MAP_CLASS_SPEC& rhs) {
  return lhs.values_ == rhs.values_;
}

MPH_MAP_TMPL_SPEC MPH_MAP_CLASS_SPEC::mph_map() {
  rehash();
}

MPH_MAP_TMPL_SPEC MPH_MAP_CLASS_SPEC::~mph_map() {
}

MPH_MAP_METHOD_DECL(insert_return_type, insert)(const value_type& x) {
  iterator it = find(x.first);
  if (it != end()) return std::make_pair(it, false);
  values_.push_back(x);
  slack_.insert(std::make_pair(x.first, values_.size() - 1));
  if (slack_.size() == table_.size() ||
      (slack_.size() >= 256 && table_.size() == 0)) {
     rehash();
  }
  it = find(x.first);
  return std::make_pair(it, true);
}

MPH_MAP_METHOD_DECL(void_type, rehash)() {
  if (values_.empty()) return;
  slack_type().swap(slack_);
  bool success = table_.Reset(
      make_iterator_first(values_.begin()),
      make_iterator_first(values_.end()));
  assert(success);
  std::vector<value_type> new_values(values_.size());
  for (const_iterator it = values_.begin(), end = values_.end();
       it != end; ++it) {
    size_type id = table_.index(it->first);
    assert(id < new_values.size());
    new_values[id] = *it;
  }
  values_.swap(new_values);
}

MPH_MAP_METHOD_DECL(iterator, begin)() { return values_.begin(); }
MPH_MAP_METHOD_DECL(iterator, end)() { return values_.end(); }
MPH_MAP_METHOD_DECL(const_iterator, begin)() const { return values_.begin(); }
MPH_MAP_METHOD_DECL(const_iterator, end)() const { return values_.end(); }
MPH_MAP_METHOD_DECL(bool_type, empty)() const { return values_.empty(); }

MPH_MAP_METHOD_DECL(void_type, clear)() {
  values_.clear();
  slack_.clear();
  table_.clear();
}

MPH_MAP_METHOD_DECL(void_type, erase)(iterator pos) {
  values_.erase(pos);
  rehash();
}
MPH_MAP_METHOD_DECL(void_type, erase)(const key_type& k) {
  iterator it = find(k);
  if (it == end()) return;
  erase(it);
}

MPH_MAP_METHOD_DECL(const_iterator, find)(const key_type& k) const {
  if (!slack_.empty()) {
    typename slack_type::const_iterator it = slack_.find(k);
    if (it != slack_.end()) return values_.begin() + it->second;
  }
  if (table_.size() == 0) return end();
  size_type id = table_.index(k);
  if (key_equal()(values_[id].first, k)) {
    return values_.begin() + id;
  }
  return end();
}
MPH_MAP_METHOD_DECL(iterator, find)(const key_type& k) {
  if (!slack_.empty()) {
    typename slack_type::const_iterator it = slack_.find(k);
    if (it != slack_.end()) return values_.begin() + it->second;
  }
  if (table_.size() == 0) return end();
  size_type id = table_.index(k);
  if (key_equal()(values_[id].first, k)) {
    return values_.begin() + id;
  }
  return end();
}

MPH_MAP_METHOD_DECL(data_type&, operator[])(const key_type& k) {
  return insert(std::make_pair(k, data_type())).first->second;
}

}  // namespace cxxmph
