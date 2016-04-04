#ifndef _ordered_map_h_
#define _ordered_map_h_

#include <functional>
#include <list>
#include <map>
#include <utility>

template <class K, class V, class COMP = std::less<K>,
          class ALLOC = std::allocator<std::pair<const K, V>>>
class ordered_map {
public:
    typedef K                           key_type;
    typedef V                           mapped_type;
    typedef std::pair<const K, V>       value_type;
    typedef COMP                        key_compare;
    typedef ALLOC                       allocator_type;
    typedef value_type                  &reference;
    typedef const value_type            &const_reference;
private:
    typedef std::list<value_type, ALLOC>                list_type;
    list_type                                           data;
public:
    typedef typename list_type::iterator                iterator;
    typedef typename list_type::const_iterator          const_iterator;
    typedef std::reverse_iterator<iterator>             reverse_iterator;
    typedef std::reverse_iterator<const_iterator>       const_reverse_iterator;

    class value_compare : std::binary_function<value_type, value_type, bool> {
    friend class ordered_map;
    protected:
        COMP    comp;
        value_compare(COMP c) : comp(c) {}
    public:
        bool operator()(const value_type &a, const value_type &b) const {
            return comp(a.first, b.first); }
    };
private:
    struct mapcmp : std::binary_function<const K*, const K*, bool> {
        COMP    comp;
        bool operator()(const K *a, const K *b) const { return comp(*a, *b); } };
    typedef std::map<const K *, iterator, mapcmp, ALLOC>  map_type;
    map_type                            data_map;
    void init_data_map() {
        data_map.clear();
        for (auto it = data.begin(); it != data.end(); it++)
            data_map.emplace(&it->first, it); }
    iterator tr_iter(typename map_type::iterator i) {
        if (i == data_map.end())
            return data.end();
        return i->second; }
    const_iterator tr_iter(typename map_type::const_iterator i) const {
        if (i == data_map.end())
            return data.end();
        return i->second; }
public:
    typedef typename map_type::size_type        size_type;

public:
    ordered_map() {}
    ordered_map(const ordered_map &a) : data(a.data) { init_data_map(); }
    ordered_map(ordered_map &&a) = default; /* move is ok? */
    ordered_map &operator=(const ordered_map &a) {
        /* std::list assignment broken by spec if elements are const... */
        if (this != &a) {
            data.clear();
            for (auto &el : a.data)
                data.push_back(el);
            init_data_map(); }
        return *this; }
    ordered_map &operator=(ordered_map &&a) = default; /* move is ok? */
    ordered_map(const std::initializer_list<value_type> &il) : data(il) { init_data_map(); }
    // FIXME add allocator and comparator ctors...

    iterator                    begin() noexcept { return data.begin(); }
    const_iterator              begin() const noexcept { return data.begin(); }
    iterator                    end() noexcept { return data.end(); }
    const_iterator              end() const noexcept { return data.end(); }
    reverse_iterator            rbegin() noexcept { return data.rbegin(); }
    const_reverse_iterator      rbegin() const noexcept { return data.rbegin(); }
    reverse_iterator            rend() noexcept { return data.rend(); }
    const_reverse_iterator      rend() const noexcept { return data.rend(); }
    const_iterator              cbegin() const noexcept { return data.cbegin(); }
    const_iterator              cend() const noexcept { return data.cend(); }
    const_reverse_iterator      crbegin() const noexcept { return data.crbegin(); }
    const_reverse_iterator      crend() const noexcept { return data.crend(); }

    bool        empty() const noexcept { return data.empty(); }
    size_type   size() const noexcept { return data_map.size(); }
    size_type   max_size() const noexcept { return data_map.max_size(); }
    bool operator==(const ordered_map &a) const { return data == a.data; }
    bool operator!=(const ordered_map &a) const { return data != a.data; }

    iterator        find(const key_type &a) { return tr_iter(data_map.find(&a)); }
    const_iterator  find(const key_type &a) const { return tr_iter(data_map.find(&a)); }
    size_type       count(const key_type &a) const { return data_map.count(&a); }
    iterator        lower_bound(const key_type &a) { return tr_iter(data_map.lower_bound(&a)); }
    const_iterator  lower_bound(const key_type &a) const {
                        return tr_iter(data_map.lower_bound(&a)); }
    iterator        upper_bound(const key_type &a) { return tr_iter(data_map.upper_bound(&a)); }
    const_iterator  upper_bound(const key_type &a) const {
                        return tr_iter(data_map.upper_bound(&a)); }
    iterator        upper_bound_pred(const key_type &a) {
                        auto ub = data_map.upper_bound(&a);
                        if (ub == data_map.begin()) return end();
                        return tr_iter(--ub); }
    const_iterator  upper_bound_pred(const key_type &a) const {
                        auto ub = data_map.upper_bound(&a);
                        if (ub == data_map.begin()) return end();
                        return tr_iter(--ub); }

    V& operator[](const K &x) {
        auto it = find(x);
        if (it == data.end()) {
            it = data.emplace(data.end(), x, V());
            data_map.emplace(&it->first, it); }
        return it->second; }
    V& operator[](K &&x) {
        auto it = find(x);
        if (it == data.end()) {
            it = data.emplace(data.end(), std::move(x), V());
            data_map.emplace(&it->first, it); }
        return it->second; }
    V& at(const K &x) {
        auto it = find(x);
        if (it == data.end()) throw std::out_of_range("ordered_map");
        return it->second; }
    const V& at(const K &x) const {
        auto it = find(x);
        if (it == data.end()) throw std::out_of_range("ordered_map");
        return it->second; }

    std::pair<iterator, bool> insert(const value_type &v) {
        auto it = find(v.first);
        if (it == data.end()) {
            it = data.insert(data.end(), v);
            data_map.emplace(&it->first, it);
            return std::make_pair(it, true); }
        return std::make_pair(it, false); }
    std::pair<iterator, bool> emplace(K &&k, V &&v) {
        auto it = find(k);
        if (it == data.end()) {
            it = data.emplace(data.end(), std::move(k), std::move(v));
            data_map.emplace(&it->first, it);
            return std::make_pair(it, true); }
        return std::make_pair(it, false); }
    template<class InputIterator> void insert(InputIterator b, InputIterator e) {
        while (b != e) insert(*b++); }

    /* should be erase(const_iterator), but glibc++ std::list::erase is broken */
    iterator erase(iterator pos) {
        data_map.erase(&pos->first);
        return data.erase(pos); }
    size_type erase(const K &k) {
        auto it = find(k);
        if (it != data.end()) {
            data_map.erase(&k);
            data.erase(it);
            return 1; }
        return 0; }

    template<class Compare> void sort(Compare comp) { data.sort(comp); }
};

#endif /* _ordered_map_h_ */
