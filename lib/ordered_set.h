/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _LIB_ORDERED_SET_H_
#define _LIB_ORDERED_SET_H_

#include <boost/container/set.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/tuple/tuple.hpp>

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <utility>
#include <iterator>


//============================================================================
// EnlistedNode <T>:
//
//   An object of this type holds:
//     - two pointers (to the previous node and to the next node);
//     - a payload of type T.
//============================================================================

struct MyTag1;

template <typename T>
class EnlistedNode:
    public boost::intrusive::list_base_hook <boost::intrusive::tag <MyTag1>>
{
private:
    T _t;

public:
    explicit EnlistedNode (T &&t):
        _t {std::forward <T> (t)}
    {}

          T *adr_payload ()       noexcept { return &_t; }
    const T *adr_payload () const noexcept { return &_t; }

          T &ref_payload ()       noexcept { return _t; }
    const T &ref_payload () const noexcept { return _t; }

          T  get_payload () const          { return _t; }
};

/*
template <typename T>
bool operator== (const EnlistedNode <T> &x, const EnlistedNode <T> &y)
noexcept (noexcept (x.ref_payload () == y.ref_payload ()))
{
    return x.ref_payload () == y.ref_payload ();
}

template <typename T>
bool operator!= (const EnlistedNode <T> &x, const EnlistedNode <T> &y)
noexcept (noexcept (x.ref_payload () != y.ref_payload ()))
{
    return x.ref_payload () != y.ref_payload ();
}

template <typename T>
bool operator<  (const EnlistedNode <T> &x, const EnlistedNode <T> &y)
noexcept (noexcept (x.ref_payload () <  y.ref_payload ()))
{
    return x.ref_payload () <  y.ref_payload ();
}

template <typename T>
bool operator>  (const EnlistedNode <T> &x, const EnlistedNode <T> &y)
noexcept (noexcept (x.ref_payload () >  y.ref_payload ()))
{
    return x.ref_payload () >  y.ref_payload ();
}

template <typename T>
bool operator<= (const EnlistedNode <T> &x, const EnlistedNode <T> &y)
noexcept (noexcept (x.ref_payload () <= y.ref_payload ()))
{
    return x.ref_payload () <= y.ref_payload ();
}

template <typename T>
bool operator>= (const EnlistedNode <T> &x, const EnlistedNode <T> &y)
noexcept (noexcept (x.ref_payload () >= y.ref_payload ()))
{
    return x.ref_payload () >= y.ref_payload ();
}
*/


//============================================================================
// EnlistedComp <Comp>:
//
// If Comp is a Strict Weak Ordering
// for comparing objects of type T,
// then EnlistedComp <Comp> is a Strict Weak Ordering
// for comparing object of type EnlistedNode <T>.
//============================================================================

template <typename Comp>
class EnlistedComp
{
public:
    typedef EnlistedNode <typename Comp::first_argument_type>  first_argument_type;
    typedef EnlistedNode <typename Comp::second_argument_type> second_argument_type;
    typedef               typename Comp::result_type           result_type;

private:
    Comp _comp;

public:
    EnlistedComp () = default;

    explicit EnlistedComp (const Comp &comp):
        _comp (comp)
    {}

    result_type operator() (const first_argument_type &x, const second_argument_type &y) const
    noexcept (noexcept (_comp (x.ref_payload (), y.ref_payload ())))
    {
        return _comp (x.ref_payload (), y.ref_payload ());
    }
};


//============================================================================
// ordered_set:
//============================================================================

template
<
    typename T,
    class    Comp  = std::less <T>,
    class    Alloc = std::allocator <T>
>
class ordered_set
{
// The (non-intrusive) tree:

private:
    typedef
        EnlistedNode <T>
        MyEnlistedNode;

    typedef
        EnlistedComp <Comp>
        MyEnlistedComp;

    typedef
        typename Alloc::template rebind <MyEnlistedNode>::other
        MyEnlistedAlloc;

public:
    typedef
        boost::container::set <MyEnlistedNode, MyEnlistedComp, MyEnlistedAlloc>
        MyEnlistedTree,
        tree_type;

private:
    tree_type _tree;


// The (intrusive) list:

private:
    typedef
        boost::intrusive::list_base_hook<boost::intrusive::tag <MyTag1>>
        BaseHook1;

public:
    typedef
        boost::intrusive::list
        <
            MyEnlistedNode,
            boost::intrusive::base_hook <BaseHook1>,
            boost::intrusive::constant_time_size <false>
        >
        MyList,
        list_type;

private:
    list_type _list;


// Typedefs related to the tree type:

public:
    typedef typename tree_type::key_type               tree_key_type;
    typedef typename tree_type::value_type             tree_value_type;
    typedef typename tree_type::key_compare            tree_key_compare;
    typedef typename tree_type::value_compare          tree_value_compare;
    typedef typename tree_type::size_type              tree_size_type;
    typedef typename tree_type::difference_type        tree_difference_type;
    typedef typename tree_type::pointer                tree_pointer;
    typedef typename tree_type::const_pointer          tree_const_pointer;
    typedef typename tree_type::reference              tree_reference;
    typedef typename tree_type::const_reference        tree_const_reference;
    typedef typename tree_type::iterator               tree_iterator;
    typedef typename tree_type::const_iterator         tree_const_iterator;
    typedef typename tree_type::reverse_iterator       tree_reverse_iterator;
    typedef typename tree_type::const_reverse_iterator tree_const_reverse_iterator;


// Typedefs related to the list type:

public:
    typedef typename list_type::value_type             list_value_type;
    typedef typename list_type::size_type              list_size_type;
    typedef typename list_type::difference_type        list_difference_type;
    typedef typename list_type::pointer                list_pointer;
    typedef typename list_type::const_pointer          list_const_pointer;
    typedef typename list_type::reference              list_reference;
    typedef typename list_type::const_reference        list_const_reference;
    typedef typename list_type::iterator               list_iterator;
    typedef typename list_type::const_iterator         list_const_iterator;
    typedef typename list_type::reverse_iterator       list_reverse_iterator;
    typedef typename list_type::const_reverse_iterator list_const_reverse_iterator;


// Typedefs related to the tree type, published without prefix:

public:
    typedef tree_key_type                              key_type;
    typedef tree_value_type                            value_type;
    typedef tree_key_compare                           key_compare;
    typedef tree_value_compare                         value_compare;


// Typedefs related to the list type, published without prefix:

public:
    typedef list_size_type                             size_type;
    typedef list_difference_type                       difference_type;
    typedef list_pointer                               pointer;
    typedef list_const_pointer                         const_pointer;
    typedef list_reference                             reference;
    typedef list_const_reference                       const_reference;
    typedef list_iterator                              iterator;
    typedef list_const_iterator                        const_iterator;
    typedef list_reverse_iterator                      reverse_iterator;
    typedef list_const_reverse_iterator                const_reverse_iterator;


// Conversion from tree_iterator to list_iterator:

private:
    list_iterator       t2l_iterator (tree_iterator       titer)       noexcept
    {
        if (titer == _tree.end ())
            return _list.end ();
        else
            return _list.iterator_to (*titer);
    }

    list_const_iterator t2l_iterator (tree_const_iterator titer) const noexcept
    {
        if (titer == _tree.end ())
            return _list.end ();
        else
            return _list.iterator_to (*titer);
    }


// Conversion from list_iterator to tree_iterator:

private:
    tree_iterator        l2t_iterator (list_iterator       liter)       noexcept
    {
         if (liter == _list.end ())
             return _tree.end ();
         else
             return _tree.find (*liter);
    }

    tree_const_iterator l2t_iterator (list_const_iterator liter) const noexcept
    {
        if (liter == _list.end ())
            return _tree.end ();
        else
            return _tree.find (*liter);
    }


// init_tree:

private:
    void init_list () noexcept
    {
        BOOST_ASSERT (_list.empty ());
        std::copy (_tree.begin (), _tree.end (), std::back_inserter (_list));
    }


// sorted_iterator:

public:
    class sorted_iterator
    {
    public:
        typedef       std::bidirectional_iterator_tag  iterator_category;
        typedef       T                                value_type;
        typedef       tree_difference_type             difference_type;
        typedef const T                               *pointer;
        typedef const T                               &reference;

    private:
        tree_iterator _titer;

    private:
        friend class ordered_set;
        explicit sorted_iterator (tree_iterator titer) noexcept:
            _titer (titer)
        {}

    public:
        const T &operator*  () const noexcept { return *_titer->adr_payload (); }
        const T *operator-> () const noexcept { return  _titer->adr_payload (); }

        sorted_iterator &operator++ ()    noexcept { ++_titer; }
        sorted_iterator &operator-- ()    noexcept { --_titer; }
        sorted_iterator  operator++ (int) noexcept { const sorted_iterator saved {*this}; ++*this; return saved; }
        sorted_iterator  operator-- (int) noexcept { const sorted_iterator saved {*this}; --*this; return saved; }

        bool operator== (sorted_iterator other) noexcept { return _titer == other._titer; }
        bool operator!= (sorted_iterator other) noexcept { return ! (*this == other); }
    };

    typedef                        sorted_iterator        sorted_const_iterator;
    typedef std::reverse_iterator <sorted_iterator>       sorted_reverse_iterator;
    typedef std::reverse_iterator <sorted_const_iterator> sorted_const_reverse_iterator;


// Tree ranges:

    sorted_iterator               sorted_begin   ()       noexcept { return sorted_iterator               {_tree.begin   ()}; }
    sorted_const_iterator         sorted_begin   () const noexcept { return sorted_const_iterator         {_tree.begin   ()}; }
    sorted_const_iterator         sorted_cbegin  () const noexcept { return sorted_const_iterator         {_tree.cbegin  ()}; }
    sorted_iterator               sorted_end     ()       noexcept { return sorted_iterator               {_tree.end     ()}; }
    sorted_const_iterator         sorted_end     () const noexcept { return sorted_const_iterator         {_tree.end     ()}; }
    sorted_const_iterator         sorted_cend    () const noexcept { return sorted_const_iterator         {_tree.cend    ()}; }

    sorted_reverse_iterator       sorted_rbegin  ()       noexcept { return sorted_reverse_iterator       {_tree.rbegin  ()}; }
    sorted_const_reverse_iterator sorted_rbegin  () const noexcept { return sorted_const_reverse_iterator {_tree.rbegin  ()}; }
    sorted_const_reverse_iterator sorted_crbegin () const noexcept { return sorted_const_reverse_iterator {_tree.crbegin ()}; }
    sorted_reverse_iterator       sorted_rend    ()       noexcept { return sorted_reverse_iterator       {_tree.rend    ()}; }
    sorted_const_reverse_iterator sorted_rend    () const noexcept { return sorted_const_reverse_iterator {_tree.rend    ()}; }
    sorted_const_reverse_iterator sorted_crend   () const noexcept { return sorted_const_reverse_iterator {_tree.crend   ()}; }


// List ranges:

    list_iterator                 list_begin     ()       noexcept { return                                _list.begin   () ; }
    list_const_iterator           list_begin     () const noexcept { return                                _list.begin   () ; }
    list_const_iterator           list_cbegin    () const noexcept { return                                _list.cbegin  () ; }
    list_iterator                 list_end       ()       noexcept { return                                _list.end     () ; }
    list_const_iterator           list_end       () const noexcept { return                                _list.end     () ; }
    list_const_iterator           list_cend      () const noexcept { return                                _list.cend    () ; }

    list_reverse_iterator         list_rbegin    ()       noexcept { return                                _list.rbegin  () ; }
    list_const_reverse_iterator   list_rbegin    () const noexcept { return                                _list.rbegin  () ; }
    list_const_reverse_iterator   list_crbegin   () const noexcept { return                                _list.crbegin () ; }
    list_reverse_iterator         list_rend      ()       noexcept { return                                _list.rend    () ; }
    list_const_reverse_iterator   list_rend      () const noexcept { return                                _list.rend    () ; }
    list_const_reverse_iterator   list_crend     () const noexcept { return                                _list.crend   () ; }



// find_ex:

public:
    std::pair <sorted_iterator, list_iterator> find_ex (const T &t)
    {
        const tree_iterator titerFound = _tree.find (t);
        if (titerFound != _tree.end ())
            return std::make_pair (sorted_iterator {titerFound}, t2l_iterator (titerFound));
        else
            return std::make_pair (sorted_iterator {titerFound}, _list.end ());
    }


// find_sorted:

public:
    sorted_iterator find_sorted (const T &t)
    {
        const auto result = find_ex (t);
        return result.first;
    }


// find_list:

public:
    list_iterator find_list (const T &t)
    {
        const auto result = find_ex (t);
        return result.second;
    }


// insert_ex:

public:
    typedef boost::tuple <sorted_iterator, list_iterator, bool> insert_ex_result_type;

    insert_ex_result_type insert_ex (sorted_iterator siter_hint, list_iterator liter_pos, T &&t)
    {
        tree_value_type tt {std::forward <T> (t)};
        const tree_iterator titerFound = _tree.find (tt);
        if (titerFound == _tree.end ())
        {
            const tree_iterator tresult = _tree.insert (siter_hint._titer, std::move (tt));
            const list_iterator lresult = _list.insert (liter_pos        , *tresult);
            return boost::make_tuple (sorted_iterator {tresult}, lresult, true);
        }
        else
            return boost::make_tuple (sorted_iterator {titerFound}, t2l_iterator (titerFound), false);
    }

    insert_ex_result_type insert_ex (sorted_iterator siter_hint, T &&t)
    {
        return insert_ex (siter_hint, _list.end (), std::forward <T> (t));
    }

    insert_ex_result_type insert_ex (list_iterator iter_pos, T &&t)
    {
        return insert_ex (sorted_end (), iter_pos, std::forward <T> (t));
    }

    insert_ex_result_type insert_ex (T &&t)
    {
        return insert_ex (sorted_end (), list_end (), std::forward <T> (t));
    }


// insert_sorted:

public:
    typedef std::pair <sorted_iterator, bool> insert_sorted_result_type;

    insert_sorted_result_type insert_sorted (sorted_iterator siter_hint, list_iterator liter_pos, T &&t)
    {
        const auto result = insert_ex (siter_hint, liter_pos, std::forward <T> (t));
        return std::make_pair (boost::get <0> (result), boost::get <2> (result));
    }

    insert_sorted_result_type insert_sorted (sorted_iterator siter_hint, T &&t)
    {
        const auto result = insert_ex (siter_hint, std::forward <T> (t));
        return std::make_pair (boost::get <0> (result), boost::get <2> (result));
    }

    insert_sorted_result_type insert_sorted (list_iterator liter_pos, T &&t)
    {
        const auto result = insert_ex (liter_pos, std::forward <T> (t));
        return std::make_pair (boost::get <0> (result), boost::get <2> (result));
    }

    insert_sorted_result_type insert_sorted (T &&t)
    {
        const auto result = insert_ex (std::forward <T> (t));
        return std::make_pair (boost::get <0> (result), boost::get <2> (result));
    }


// insert_list:

public:
    typedef std::pair <list_iterator, bool> insert_list_result_type;

    insert_list_result_type insert_list (sorted_iterator siter_hint, list_iterator liter_pos, T &&t)
    {
        const auto result = insert_ex (siter_hint, liter_pos, std::forward <T> (t));
        return std::make_pair (boost::get <1> (result), boost::get <2> (result));
    }

    insert_list_result_type insert_list (sorted_iterator siter_hint, T &&t)
    {
        const auto result = insert_ex (siter_hint, std::forward <T> &&t);
        return std::make_pair (boost::get <1> (result), boost::get <2> (result));
    }

    insert_list_result_type insert_list (list_iterator liter, T &&t)
    {
        const auto result = insert_ex (liter, std::forward <T> (t));
        return std::make_pair (boost::get <1> (result), boost::get <2> (result));
    }

    insert_list_result_type insert_list (T &&t)
    {
        const auto result = insert_ex (std::forward <T> (t));
        return std::make_pair (boost::get <1> (result), boost::get <2> (result));
    }


// erase_ex:

public:
    typedef std::pair <sorted_iterator, list_iterator> erase_ex_result_type;

    erase_ex_result_type erase_ex (sorted_iterator siter)
    {
        const list_iterator liter = t2l_iterator (siter._titer);

        const tree_iterator titer_result = _tree.erase (siter);
        const list_iterator liter_result = _list.erase (liter);
        return std::make_pair (sorted_iterator {titer_result}, liter_result);
    }

    erase_ex_result_type erase_ex (list_iterator liter)
    {
        const tree_iterator titer = l2t_iterator (liter);

        const tree_iterator titer_result = _tree.erase (titer);
        const list_iterator liter_result = _list.erase (liter);
        return std::make_pair (sorted_iterator {titer_result}, liter_result);
    }


// erase_sorted:

public:
    sorted_iterator erase_sorted (sorted_iterator siter)
    {
        const auto result = erase_ex (siter);
        return result.first;
    }

    list_iterator erase_sorted (list_iterator liter)
    {
        const auto result = erase_ex (liter);
        return result.second;
    }
};


//============================================================================


#endif /* _LIB_ORDERED_SET_H_ */
