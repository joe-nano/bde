// bslmf_util.h                                                       -*-C++-*-
#ifndef INCLUDED_BSLMF_UTIL
#define INCLUDED_BSLMF_UTIL

#include <bsls_ident.h>
BSLS_IDENT("$Id: $")

//@PURPOSE: Provide low-level functions on 'bslmf' types.
//
//@CLASSES:
//  bslmf::Util: utility class providing low-level functionality
//
//@DESCRIPTION: This component defines a utility 'struct', 'bslmf::Util', that
// serves as a namespace for a suite of functions that supply low-level
// functionality for implementing portable generic facilities such as might be
// found in the C++ standard library.
//
///'bslmf::Util::forward'
///----------------------
// The function 'forward' emulates the C++ standard utility function
// 'std::forward' with the addition that on compilers that don't support
// r-value references (i.e., C++03) a 'bslmf::MovableRef<T>' is forwarded
// as a 'bslmf::MovableRef<T>'.  This operation is typically used via
// 'BSLS_COMPILERFEATURES_FORWARD' (along with
// 'BSLS_COMPILERFEATURES_FORWARD_REF') when forwarding arguments in a generic
// context.  See {Usage}.
//
///'bslmf::Util::forwardAsReference'
///---------------------------------
// The function 'forwardAsReference', like 'forward', emulates the C++ standard
// utility function 'std::forward' with the difference that on compilers that
// don't support r-value references (C++03) a 'bslmf::MovableRef<T>' is
// forwarded as 'const T&' (instead of 'bslmf::MovableRef<T>').  This operation
// is intended to be used when forwarding a 'MovableRef<T>' where that
// 'MovableRef' is being supplied to a function that does not support 'move'
// emulation, but will support true C++11 r-value references (e.g.,
// 'bdlf::BindUtil::bind').
//
///Usage
///-----
// This section illustrates intended use of this component.
//
///Example 1: Using 'bslmf::Util::forward
///--------------------------------------
// Clients should generally not use 'bslmf::Util::forward' directly, instead it
// should be used via 'BSLS_COMPILERFEATURES_FORWARD' in conjunction with
// 'BSLS_COMPILERFEATURES_FORWARD_REF'.  Here we show a simple function using
// 'BSLS_COMPILERFEATURES_FORWARD':
//..
//  template <class RESULT_TYPE>
//  struct FactoryUtil {
//
//     template <class ARG_TYPE>
//     RESULT_TYPE create(BSLS_COMPILERFEATURES_FORWARD_REF(ARG_TYPE) arg) {
//       return RESULT_TYPE(BSLS_COMPILERFEATURES_FORWARD(ARG_TYPE, arg));
//     }
//  };
//..
// Notice that 'bslmf::Util::forward' is only used in conjunction with
// 'BSLS_COMPILERFEATURES_FORWARD_REF' because, in the example above, if the
// 'create' function's parameter type was 'ARG_TYPE&& ' then it is a
// C++11-only(!) forwarding reference, and we would simply use the standard
// 'std::forward'.  Alternatively, if the parameter type was
// 'MovableRef<ARG_TYPE>' then 'arg' is *not* a forwarding-reference to be
// forwarded (certainly not in C++03).
//
///Example 2: Using 'bslmf::Util::forwardAsReference'
///--------------------------------------------------
// Suppose we had a template facility, 'MyBindUtil::bind' that does not support
// 'bslmf::MovableRef', but will accept true r-value references (on supported
// compilers).  Here we use 'forwardAsReference' to forward a supplied
// 'bslmf::MovableRef' as a true r-value reference (on supporting compilers),
// and a const reference otherwise.  Note that the definitions of 'MyFunction'
// and 'MyBindUtil' are elided and meant to represent 'bsl::function' and
// 'bdlf::BindUtil::bind' respectively:
//..
//  void doSomething(bslmf::MovableRef<Foo> value) {
//    MyFunction f = MyBindUtil::bind(
//                               bslmf::Util::forwardAsReference<Foo>(value));
//
//    //...
//
//  }
//..
// Note that because 'MyBindUtil::bind' does not support 'MovableRef', without
// 'forwardAsReference' the call to 'bind' might either fail to compile, or
// worse, bind the 'MyFunction' instance to a reference to 'value' (rather a
// new object moved-from 'value') on C++03 platforms.

#include <bslscm_version.h>

#include <bslmf_addlvaluereference.h>
#include <bslmf_addrvaluereference.h>
#include <bslmf_movableref.h>
#include <bslmf_removereference.h>

#include <bsls_compilerfeatures.h>
#include <bsls_keyword.h>

namespace BloombergLP {

namespace bslmf {

                      // ===========
                      // struct Util
                      // ===========

struct Util {
    // This struct provides several functions that are specified in the
    // <utility> header of the C++ Standard, in order to support the 'bsl'
    // library implementation without cycles into the native standard library,
    // and on platforms with only C++03 compilers available, where library
    // features may be emulated.

    // CLASS METHODS

#ifdef BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
    template <class TYPE>
    BSLS_KEYWORD_CONSTEXPR
    static TYPE&& forward(typename bsl::remove_reference<TYPE>::type&  t)
                                                         BSLS_KEYWORD_NOEXCEPT;
    template <class TYPE>
    BSLS_KEYWORD_CONSTEXPR
    static TYPE&& forward(typename bsl::remove_reference<TYPE>::type&& t)
                                                         BSLS_KEYWORD_NOEXCEPT;
#else
    template <class TYPE>
    BSLS_KEYWORD_CONSTEXPR
    static const TYPE& forward(const TYPE& t) BSLS_KEYWORD_NOEXCEPT;
    template <class TYPE>
    BSLS_KEYWORD_CONSTEXPR
    static MovableRef<TYPE> forward(MovableRef<TYPE> t) BSLS_KEYWORD_NOEXCEPT;
#endif // BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
        // Correctly forward the specified 't' argument based on the current
        // compilation environment.

#ifdef BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
    template <class TYPE>
    BSLS_KEYWORD_CONSTEXPR
    static typename bsl::add_rvalue_reference<TYPE>::type declval()
                                                         BSLS_KEYWORD_NOEXCEPT;
#else
    template <class TYPE>
    BSLS_KEYWORD_CONSTEXPR
    static typename bsl::add_lvalue_reference<TYPE>::type declval()
                                                         BSLS_KEYWORD_NOEXCEPT;
#endif // BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
        // This function has no implementation.  It exists to allow for the
        // appearance of a temporary object of the specified type that can be
        // used in unevaluated contexts.

#ifdef BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
    template <class TYPE>
    BSLS_KEYWORD_CONSTEXPR
    static TYPE&& forwardAsReference(
        typename bsl::remove_reference<TYPE>::type&  t) BSLS_KEYWORD_NOEXCEPT;
    template <class TYPE>
    BSLS_KEYWORD_CONSTEXPR
    static TYPE&& forwardAsReference(
        typename bsl::remove_reference<TYPE>::type&& t) BSLS_KEYWORD_NOEXCEPT;
#else
    template <class TYPE>
    BSLS_KEYWORD_CONSTEXPR
    static const TYPE& forwardAsReference(
                                         const TYPE& t) BSLS_KEYWORD_NOEXCEPT;
    template <class TYPE>
    BSLS_KEYWORD_CONSTEXPR
    static const TYPE& forwardAsReference(
                                    MovableRef<TYPE> t) BSLS_KEYWORD_NOEXCEPT;
#endif // BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
        // Correctly forward the specified 't' argument as a reference type
        // based on the current compilation environment.  Note that this
        // function differs from 'forward' in that when using a C++03 compiler,
        // 'MovableRef<T>' is forwarded as 'const T&' (rather than
        // 'MovableRef<T>'), which is important when forwarding to a facility
        // (e.g., 'bdlf::BindUtil::bind') which does not support
        // 'bslmf::MovableRef'.
};

// ============================================================================
//                        INLINE FUNCTION DEFINITIONS
// ============================================================================

// CLASS METHODS
#ifdef BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
template <class TYPE>
BSLS_KEYWORD_CONSTEXPR
inline
TYPE&& Util::forward(typename bsl::remove_reference<TYPE>::type& t)
                                                          BSLS_KEYWORD_NOEXCEPT
{
    return static_cast<TYPE&&>(t);
}

template <class TYPE>
BSLS_KEYWORD_CONSTEXPR
inline
TYPE&& Util::forward(typename bsl::remove_reference<TYPE>::type&& t)
                                                          BSLS_KEYWORD_NOEXCEPT
{
    return static_cast<TYPE&&>(t);
}

#else

template <class TYPE>
BSLS_KEYWORD_CONSTEXPR
inline
const TYPE& Util::forward(const TYPE& t) BSLS_KEYWORD_NOEXCEPT
{
    return t;
}

template <class TYPE>
BSLS_KEYWORD_CONSTEXPR
inline
bslmf::MovableRef<TYPE> Util::forward(bslmf::MovableRef<TYPE> t)
                                                          BSLS_KEYWORD_NOEXCEPT
{
    return t;
}
#endif // BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES

#ifdef BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES
template <class TYPE>
BSLS_KEYWORD_CONSTEXPR
inline
TYPE&& Util::forwardAsReference(typename bsl::remove_reference<TYPE>::type& t)
                                                          BSLS_KEYWORD_NOEXCEPT
{
    return static_cast<TYPE&&>(t);
}

template <class TYPE>
BSLS_KEYWORD_CONSTEXPR
inline
TYPE&& Util::forwardAsReference(typename bsl::remove_reference<TYPE>::type&& t)
                                                          BSLS_KEYWORD_NOEXCEPT
{
    return static_cast<TYPE&&>(t);
}

#else

template <class TYPE>
BSLS_KEYWORD_CONSTEXPR
inline
const TYPE& Util::forwardAsReference(const TYPE& t) BSLS_KEYWORD_NOEXCEPT
{
    return t;
}

template <class TYPE>
BSLS_KEYWORD_CONSTEXPR
inline
const TYPE& Util::forwardAsReference(bslmf::MovableRef<TYPE> t)
                                                          BSLS_KEYWORD_NOEXCEPT
{
    return bslmf::MovableRefUtil::access(t);
}
#endif // BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES

}  // close package namespace
}  // close enterprise namespace

#endif

// ----------------------------------------------------------------------------
// Copyright 2016 Bloomberg Finance L.P.
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
