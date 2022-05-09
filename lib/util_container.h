#ifndef _LIB_UTIL_CONTAINER_H_
#define _LIB_UTIL_CONTAINER_H_

#include <boost/core/exchange.hpp>

#include <utility>

// AutoEraseGuard:
//
//   An object of this type (a "guard"), upon destruction,
//   erases the specified iterator from the specified container
//   unless `dismiss` has been called (i.e. the guard has been dismissed).
//
//   It is useful in order to modify a code sequence such as:
//
//     ```
//     // Version 0 (bad): Not exception-safe !
//     const auto iter1 = container1.insert (container1.end (), some_value);
//                        container2.insert (some_related_value);
//     ```
//
//   into the following:
//
//     ```
//     // Version 1 (good): Exception-safe !
//     const auto iter1 = container1.insert (container1.end (), some_value);
//     auto g = MakeAutoEraseGuard (&container1, iter1);
//                        container2.insert (some_related_value);
//     g.dismiss ();
//     ```
//
//   so that, if the second insertion fails,
//   then the first insertion is undone before the exception is propagated.
//
//   It would appear that the following variation would have the same effect:
//
//     ```
//     // Version 2 (not so good): Exception-safe, but not debugger-friendly (libstdc++-style) !
//     const auto iter1 = container1.insert (container1.end (), some_value);
//     try
//     {
//       container2.insert (some_related_value);
//     }
//     catch (...)
//     {
//       container1.erase (iter1);
//       throw;
//     }
//     ```
//
//   but there is an important difference:
//   if no exception handler (`try-catch` statement) exists (up the call stack),
//   then on many platforms, the saved core file captures the exact moment of the original `throw`
//   (and all the details of how the second insertion has failed)
//   as opposed to capturing the moment of the re-`throw`
//   (with the important details lost).

template <class Container, typename Iterator>
class AutoEraseGuard
{
 private:
    Container *pContainer;
    Iterator iterator;

 public:
    ~AutoEraseGuard ()
    {
        if (pContainer) pContainer->erase (iterator);
    }

    void swap (AutoEraseGuard &other) noexcept
    {
        using std::swap;
        swap (pContainer, other.pContainer);
        swap (iterator  , other.iterator  );
    }

    AutoEraseGuard (AutoEraseGuard &&other) noexcept
        : pContainer (boost::exchange (other.pContainer, nullptr))
        , iterator   (other.iterator)
    {}

    AutoEraseGuard &operator= (AutoEraseGuard &&other) noexcept
    {
        AutoEraseGuard tmp {std::move (other)};
        swap (tmp);
        return *this;
    }

    AutoEraseGuard (const AutoEraseGuard &other) = delete;

    AutoEraseGuard &operator= (const AutoEraseGuard &other) = delete;

 public:
    explicit AutoEraseGuard (Container *pContainer, const Iterator &iterator)
        : pContainer {pContainer}
        , iterator   {iterator}
    {}

    void dismiss () noexcept
    {
        pContainer = nullptr;
    }
};

template <class Container, typename Iterator>
void swap (AutoEraseGuard <Container, Iterator> &left, AutoEraseGuard <Container, Iterator> &rite) noexcept
{
    return left.swap (rite);
}

template <class Container, typename Iterator>
AutoEraseGuard <Container, Iterator> MakeAutoEraseGuard (Container *pContainer, const Iterator &iterator)
{
    return AutoEraseGuard <Container, Iterator> (pContainer, iterator);
}

#endif
