// bdlqq_readlockguard.h                                              -*-C++-*-
#ifndef INCLUDED_BDLQQ_READLOCKGUARD
#define INCLUDED_BDLQQ_READLOCKGUARD

#ifndef INCLUDED_BSLS_IDENT
#include <bsls_ident.h>
#endif
BSLS_IDENT("$Id: $")

//@PURPOSE: Provide a generic proctor for read synchronization objects.
//
//@CLASSES:
//  bdlqq::ReadLockGuard: automatic locking-unlocking for read access
//  bdlqq::ReadLockGuardUnlock: automatic unlocking-locking for read access
//  bdlqq::ReadLockGuardTryLock: automatic non-blocking locking-unlocking for
//                              read access
//  bdlqq::LockReadGuard: DEPRECATED
//
//@SEE_ALSO: bdlqq_lockguard, bdlqq_writelockguard, bdlqq_rwmutex
//
//@AUTHOR: Herve Bronnimann (hbronnimann)
//
//@DESCRIPTION: This component provides generic proctors to automatically lock
// and unlock an external synchronization object for reading.  The
// synchronization object can be any type (e.g., 'bdlqq::ReaderWriterLock')
// that provides the following methods:
//..
//  void lockRead();
//  void unlock();
//..
// Both 'bdlqq::ReadLockGuard' and 'bdlqq::ReadLockGuardUnlock' implement the
// "construction is acquisition, destruction is release" idiom.  During
// construction, 'bdlqq::ReadLockGuard' automatically calls 'lockRead' on the
// user-supplied object, and 'unlock' when it is destroyed (unless released).
// 'bdlqq::ReadLockGuardUnlock' does the opposite -- it invokes the 'unlock'
// method when constructed and the 'lockRead' method when destroyed.
//
// A third type of guard, 'bdlqq::ReadLockGuardTryLock', attempts to acquire a
// lock, and if acquisition succeeds, releases it upon destruction.  Since the
// acquisition is done at construction time, it is not possible to return a
// value to indicate success.  Rather, the 'bdlqq::ReadLockGuardTryLock'
// contains a pointer to the synchronization object if 'tryLock' succeeds, and
// is null otherwise.  The synchronization object can be any type (e.g.,
// 'bdlqq::Mutex' or 'bdlqq::RecursiveMutex') that provides the following
// methods:
//..
//  int tryLockRead();
//  void unlock();
//..
// Note that objects of none of these guard types assumes ownership of the
// synchronization object provided at construction.  Also note that objects of
// all of the guard types may be constructed with a null 'lock' whereby the
// constructed guard objects proctor no lock.  The destructor of each of the
// guard types has no effect if no lock is under management.
//
///Behavior of the 'release' method
///--------------------------------
// Like all BDE proctor classes, each of the three 'bdlqq::ReadLockGuard*'
// classes provides a 'release' method that terminates the proctor's management
// of any lock object that the proctor holds.  The 'release' method has *no*
// *effect* on the state of the lock object.
//
// In particular, 'bdlqq::ReadLockGuard::release' does not unlock the lock
// object under management.  If a user wants to release the lock object *and*
// unlock the lock object (because the lock is no longer required before the
// guard goes out of scope), the following idiom can be used:
//..
//  // 'guard' is an existing guard of type 'bdlqq::ReadLockGuard<my_RLock>',
//  // created in a scope that we do not control.
//
//  {
//      // ... Do work that requires the lock.
//
//      // We know that the lock is no longer needed.
//
//      my_RLock *rlock = guard.release();
//
//      // 'rlock' is no longer managed, but is *still* *locked*.
//
//      rlock->unlock();
//
//      // ... Do work that does not require the lock.
//  }
//..
//
///Usage
///-----
// Use this component to ensure that in the event of an exception or exit from
// any point in a given scope, the synchronization object will be properly
// unlocked.  The following function, 'errorProneFunc', is overly complex, not
// exception safe, and contains a bug.
//..
//  static void errorProneFunc(const my_Object *obj, my_RWLock *rwlock)
//  {
//      rwlock->lockRead();
//      if (someCondition) {
//          obj->someMethod();
//          rwlock->unlock();
//          return;
//      } else if (someOtherCondition) {
//          obj->someOtherMethod();
//          return;                      // MISTAKE! forgot to unlock rwlock
//      }
//      obj->defaultMethod();
//      rwlock->unlock();
//      return;
//  }
//..
// The function can be rewritten with a cleaner and safer implementation using
// a guard object.  The 'safeFunc' function is simpler than 'errorProneFunc',
// is exception safe, and avoids the multiple calls to unlock that can be a
// source of errors.
//..
//  static void safeFunc(const my_Object *obj, my_RWLock *rwlock)
//  {
//      bdlqq::ReadLockGuard<my_RWLock> guard(rwlock);
//      if (someCondition) {
//          obj->someMethod();
//          return;
//      } else if (someOtherCondition) {
//          obj->someOtherMethod();
//          return;                  // OK, rwlock is automatically unlocked
//      }
//      obj->defaultMethod();
//      return;
//  }
//..
// When blocking while acquiring the lock is not desirable, one may instead use
// a 'bdlqq::ReadLockGuardTryLock' in the typical following fashion:
//..
//  static int safeButNonBlockingFunc(const my_Object *obj, my_RWLock *rwlock)
//      // Perform task and return positive value if locking succeeds.
//      // Return 0 if locking fails.
//  {
//      const int RETRIES = 1; // use higher values for higher success rate
//      bdlqq::ReadLockGuardTryLock<my_RWLock> guard(rwlock, RETRIES);
//      if (guard.ptr()) { // rwlock is locked
//          if (someCondition) {
//              obj->someMethod();
//              return 2;
//          } else if (someOtherCondition) {
//              obj->someOtherMethod();
//              return 3;
//          }
//          obj->defaultMethod();
//          return 1;
//      }
//      return 0;
//  }
//..
// If the underlying lock object provides an upgrade to a lock for write (as
// does 'bdlqq::ReaderWriterLock' with the 'upgradeToWriteLock' function, for
// example), this can be safely used in conjunction with
// 'bdlqq::ReadLockGuard', as long as the same 'unlock' method is used to
// release both kinds of locks.  The following method illustrates this usage:
//..
//  static void safeUpdateFunc(my_Object *obj, my_RWLock *rwlock)
//  {
//      const my_Object *constObj = obj;
//      bdlqq::ReadLockGuard<my_RWLock> guard(rwlock);
//      if (someUpgradeCondition) {
//          rwlock->upgradeToWriteLock();
//          obj->someUpgradeMethod();
//          return;
//      } else if (someOtherCondition) {
//          constObj->someOtherMethod();
//          return;
//      }
//      constObj->defaultMethod();
//      return;
//  }
//..
// In the above code, the call to 'upgradeToWriteLock' is not necessarily
// atomic, as the upgrade may release the lock for read and be interrupted
// before getting a lock for write.  It is possible to guarantee atomicity (as
// does 'bdlqq::ReaderWriterLock' if the 'lockReadReserveWrite' function is
// used instead of 'lockRead', for example), but the standard constructor
// should not be used.  Instead, the 'lockReadReserveWrite' lock function
// should be used explicitly, and the guard constructed with an object which is
// already locked.  The following method illustrates this usage:
//..
//  static void safeAtomicUpdateFunc(my_Object *obj, my_RWLock *rwlock)
//  {
//      const my_Object *constObj = obj;
//      rwlock->lockReadReserveWrite();
//      const int PRELOCKED = 1;
//      bdlqq::ReadLockGuard<my_RWLock> guard(rwlock, PRELOCKED);
//      if (someUpgradeCondition) {
//          rwlock->upgradeToWriteLock();
//          obj->someUpgradeMethod();
//          return;
//      } else if (someOtherCondition) {
//          constObj->someOtherMethod();
//          return;
//      }
//      constObj->defaultMethod();
//      return;
//  }
//..
// Note that in the code above, the function 'rwlock->lockRead()' is never
// called, but is nevertheless required for the code to compile.
//
// Instantiations of 'bdlqq::ReadLockGuardUnlock' can be interleaved with
// instantiations of 'bdlqq::ReadLockGuard' to create both critical sections
// and regions where the lock is released.
//..
//  void f(my_RWLock *lock)
//  {
//      bdlqq::ReadLockGuard<my_RWLock> guard(lock);
//
//      // critical section here
//
//      {
//           bdlqq::ReadLockGuardUnlock<my_RWLock> guard(lock);
//
//          // mutex is unlocked here
//
//      } // lock reacquired upon destruction
//
//      // critical section here
//
//  } // lock is unlocked here
//..
// Care must be taken so as not to interleave guard objects in such a way as to
// cause an illegal sequence of calls on a lock (two sequential lock calls or
// two sequential unlock calls on a non-recursive read/write lock).

#ifndef INCLUDED_BDLSCM_VERSION
#include <bdlscm_version.h>
#endif

namespace BloombergLP {
namespace bdlqq {
                            // ===================
                            // class ReadLockGuard
                            // ===================

template <class T>
class ReadLockGuard {
    // This class template implements a proctor for acquisition and release of
    // read synchronization resources (i.e., reader locks).

    // DATA
    T *d_lock_p;  // lock proctored by this object (held, not owned)

  private:
    // NOT IMPLEMENTED
    ReadLockGuard(const ReadLockGuard<T>&);
    ReadLockGuard<T>& operator=(const ReadLockGuard<T>&);

  public:
    // CREATORS
    explicit ReadLockGuard(T *lock);
        // Create a proctor object that conditionally manages the specified
        // 'lock' (if non-zero), and invokes the 'lockRead' method on 'lock'.
        // Note that 'lock' must remain valid throughout the lifetime of this
        // proctor, or until 'release' is called.

    ReadLockGuard(T *lock, int preLockedFlag);
        // Create a proctor object that conditionally manages the specified
        // 'lock' (if non-zero) and, unless the specified 'preLockedFlag' is
        // non-zero, invokes the 'lockRead' method on 'lock'.  Note that 'lock'
        // must remain valid throughout the lifetime of this proctor, or until
        // 'release' is called.

    ~ReadLockGuard();
        // Destroy this proctor object and invoke the 'unlock' method on the
        // lock object under management by this proctor, if any.  If no lock is
        // currently being managed, this method has no effect.

    // MANIPULATORS
    T *release();
        // Return the address of the modifiable lock object under management by
        // this proctor, and release the lock from further management by this
        // proctor.  If no lock is currently being managed, return 0 with no
        // other effect.  Note that this operation does *not* unlock the lock
        // object (if any) that was under management.

    // ACCESSORS
    T *ptr() const;
        // Return the address of the modifiable lock object under management by
        // this proctor, or 0 if no lock is currently being managed.
};

                            // ===================
                            // class LockReadGuard
                            // ===================

template <class T>
class LockReadGuard : public ReadLockGuard<T> {
    // This class is DEPRECATED.  Use 'ReadLockGuard' instead.

  private:
    // NOT IMPLEMENTED
    LockReadGuard(const LockReadGuard<T>&);
    LockReadGuard<T>& operator=(const LockReadGuard<T>&);

  public:
    // CREATORS
    explicit LockReadGuard(T *lock);
        // DEPRECATED: Use 'ReadLockGuard' instead.

    LockReadGuard(T *lock, int preLockedFlag);
        // DEPRECATED: Use 'ReadLockGuard' instead.
};

                         // =========================
                         // class ReadLockGuardUnlock
                         // =========================

template <class T>
class ReadLockGuardUnlock {
    // This class template implements a proctor for release and reacquisition
    // of read synchronization resources (i.e., reader locks).

    // DATA
    T *d_lock_p;  // lock proctored by this object (held, not owned)

  private:
    // NOT IMPLEMENTED
    ReadLockGuardUnlock(const ReadLockGuardUnlock<T>&);
    ReadLockGuardUnlock<T>& operator=(const ReadLockGuardUnlock<T>&);

  public:
    // CREATORS
    explicit ReadLockGuardUnlock(T *lock);
        // Create a proctor object that conditionally manages the specified
        // 'lock' (if non-zero), and invokes the 'unlock' method on 'lock'.
        // Note that 'lock' must remain valid throughout the lifetime of this
        // proctor, or until 'release' is called.

    ReadLockGuardUnlock(T *lock, int preUnlockedFlag);
        // Create a proctor object that conditionally manages the specified
        // 'lock' (if non-zero) and, unless the specified 'preUnlockedFlag' is
        // non-zero, invokes the 'unlock' method on 'lock'.  Note that 'lock'
        // must remain valid throughout the lifetime of this proctor, or until
        // 'release' is called.

    ~ReadLockGuardUnlock();
        // Destroy this proctor object and invoke the 'lockRead' method on the
        // lock object under management by this proctor, if any.  If no lock is
        // currently being managed, this method has no effect.

    // MANIPULATORS
    T *release();
        // Return the address of the modifiable lock object under management by
        // this proctor, and release the lock from further management by this
        // proctor.  If no lock is currently being managed, return 0 with no
        // other effect.  Note that this operation does *not* lock the lock
        // object (if any) that was under management.

    // ACCESSORS
    T *ptr() const;
        // Return the address of the modifiable lock object under management by
        // this proctor, or 0 if no lock is currently being managed.
};

                         // ==========================
                         // class ReadLockGuardTryLock
                         // ==========================

template <class T>
class ReadLockGuardTryLock {
    // This class template implements a proctor for tentative acquisition and
    // release of read synchronization resources (i.e., reader locks).

    // DATA
    T *d_lock_p;  // lock proctored by this object (held, not owned)

  private:
    // NOT IMPLEMENTED
    ReadLockGuardTryLock(const ReadLockGuardTryLock<T>&);
    ReadLockGuardTryLock<T>& operator=(const ReadLockGuardTryLock<T>&);

  public:
    // CREATORS
    explicit ReadLockGuardTryLock(T *lock, int attempts = 1);
        // Create a proctor object that conditionally manages the specified
        // 'lock' (if non-zero), and invokes the 'tryLockRead' method on 'lock'
        // until the lock is acquired, or until up to the specified 'attempts'
        // have been made to acquire the lock.  The behavior is undefined
        // unless '0 < attempts'.  Note that 'lock' must remain valid
        // throughout the lifetime of this proctor, or until 'release' is
        // called.

    ~ReadLockGuardTryLock();
        // Destroy this proctor object and invoke the 'unlock' method on the
        // lock object under management by this proctor, if any.  If no lock is
        // currently being managed, this method has no effect.

    // MANIPULATORS
    T *release();
        // Return the address of the modifiable lock object under management by
        // this proctor, and release the lock from further management by this
        // proctor.  If no lock is currently being managed, return 0 with no
        // other effect.  Note that this operation does *not* unlock the lock
        // object (if any) that was under management.

    // ACCESSORS
    T *ptr() const;
        // Return the address of the modifiable lock object under management by
        // this proctor, or 0 if no lock is currently being managed.
};

}  // close package namespace

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

                            // -------------------
                            // class ReadLockGuard
                            // -------------------

// CREATORS
template <class T>
inline
bdlqq::ReadLockGuard<T>::ReadLockGuard(T *lock)
: d_lock_p(lock)
{
    if (d_lock_p) {
        d_lock_p->lockRead();
    }
}

template <class T>
inline
bdlqq::ReadLockGuard<T>::ReadLockGuard(T *lock, int preLocked)
: d_lock_p(lock)
{
    if (d_lock_p && !preLocked) {
        d_lock_p->lockRead();
    }
}

template <class T>
inline
bdlqq::ReadLockGuard<T>::~ReadLockGuard()
{
    if (d_lock_p) {
        d_lock_p->unlock();
    }
}

// MANIPULATORS
template <class T>
inline
T *bdlqq::ReadLockGuard<T>::release()
{
    T *lock  = d_lock_p;
    d_lock_p = 0;
    return lock;
}

// ACCESSORS
template <class T>
inline
T *bdlqq::ReadLockGuard<T>::ptr() const
{
    return d_lock_p;
}

                            // -------------------
                            // class LockReadGuard
                            // -------------------

// CREATORS
template <class T>
inline
bdlqq::LockReadGuard<T>::LockReadGuard(T *lock)
: ReadLockGuard<T>(lock)
{
}

template <class T>
inline
bdlqq::LockReadGuard<T>::LockReadGuard(T *lock, int preLocked)
: ReadLockGuard<T>(lock, preLocked)
{
}

                         // -------------------------
                         // class ReadLockGuardUnlock
                         // -------------------------

// CREATORS
template <class T>
inline
bdlqq::ReadLockGuardUnlock<T>::ReadLockGuardUnlock(T *lock)
: d_lock_p(lock)
{
    if (d_lock_p) {
        d_lock_p->unlock();
    }
}

template <class T>
inline
bdlqq::ReadLockGuardUnlock<T>::ReadLockGuardUnlock(T *lock, int preUnlocked)
: d_lock_p(lock)
{
    if (d_lock_p && !preUnlocked) {
        d_lock_p->unlock();
    }
}

template <class T>
inline
bdlqq::ReadLockGuardUnlock<T>::~ReadLockGuardUnlock()
{
    if (d_lock_p) {
        d_lock_p->lockRead();
    }
}

// MANIPULATORS
template <class T>
inline
T *bdlqq::ReadLockGuardUnlock<T>::release()
{
    T *lock  = d_lock_p;
    d_lock_p = 0;
    return lock;
}

// ACCESSORS
template <class T>
inline
T *bdlqq::ReadLockGuardUnlock<T>::ptr() const
{
    return d_lock_p;
}

                         // --------------------------
                         // class ReadLockGuardTryLock
                         // --------------------------

// CREATORS
template <class T>
bdlqq::ReadLockGuardTryLock<T>::ReadLockGuardTryLock(T *lock, int retries)
: d_lock_p(0)
{
    if (lock) {
        while (retries--) {
            if (!lock->tryLockRead()) {
                d_lock_p = lock;
                break;
            }
        }
    }
}

template <class T>
inline
bdlqq::ReadLockGuardTryLock<T>::~ReadLockGuardTryLock()
{
    if (d_lock_p) {
        d_lock_p->unlock();
    }
}

// MANIPULATORS
template <class T>
inline
T *bdlqq::ReadLockGuardTryLock<T>::release()
{
    T *lock  = d_lock_p;
    d_lock_p = 0;
    return lock;
}

// ACCESSORS
template <class T>
inline
T *bdlqq::ReadLockGuardTryLock<T>::ptr() const
{
    return d_lock_p;
}

}  // close enterprise namespace

#endif

// ----------------------------------------------------------------------------
// Copyright 2015 Bloomberg Finance L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------- END-OF-FILE ----------------------------------
