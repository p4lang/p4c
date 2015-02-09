#ifndef _BM_HANDLE_MGR_H_
#define _BM_HANDLE_MGR_H_

#include <Judy.h>

class HandleMgr {
public:
  HandleMgr()
    : handles((Pvoid_t) NULL) {}

  /* Copy constructor */
  HandleMgr(const HandleMgr& other)
    : handles((Pvoid_t) NULL) {
    int Rc_iter, Rc_set;
    Word_t index = 0;
    J1F(Rc_iter, other.handles, index);
    while(Rc_iter) {
      J1S(Rc_set, handles, index);
      J1N(Rc_iter, other.handles, index);
    }
  }

  /* Move constructor */
  HandleMgr(HandleMgr&& other) noexcept
  : handles(other.handles) {}

  ~HandleMgr() {
    Word_t bytes_freed;
    J1FA(bytes_freed, handles);
  }

  /* Copy assignment operator */
  HandleMgr &operator=(const HandleMgr &other) {    
    HandleMgr tmp(other); // re-use copy-constructor
    *this = std::move(tmp); // re-use move-assignment
    return *this;
  }
 
  /* Move assignment operator */
  HandleMgr &operator=(HandleMgr &&other) noexcept {
    // simplified move-constructor that also protects against move-to-self.
    std::swap(handles, other.handles); // repeat for all elements
    return *this;
  }

  /* Return 0 on success, -1 on failure */

  int get_handle(unsigned *handle) {
    Word_t jhandle = 0;
    int Rc;

    J1FE(Rc, handles, jhandle); // Judy1FirstEmpty()
    if(!Rc) return -1;
    J1S(Rc, handles, jhandle); // Judy1Set()
    if(!Rc) return -1;

    *handle = jhandle;

    return 0;
  }

  int release_handle(unsigned handle) {
    int Rc;
    J1U(Rc, handles, handle); // Judy1Unset()
    return Rc ? 0 : -1;
  }

  size_t size() const {
    Word_t size;
    J1C(size, handles, 0, -1);
    return size;
  }

  bool valid_handle(unsigned handle) const {
    int Rc;
    J1T(Rc, handles, handle); // Judy1Test()
    return (Rc == 1);
  }

private:
  Pvoid_t handles;
};

#endif
