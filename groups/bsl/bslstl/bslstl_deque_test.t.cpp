// bslstl_deque_test.t.cpp                                            -*-C++-*-
#include <bslstl_deque_test.h>

#include <bslstl_deque.h>
#include <bslstl_forwarditerator.h>
#include <bslstl_iterator.h>
#include <bslstl_randomaccessiterator.h>
#include <bslstl_vector.h>

#include <bslma_allocator.h>
#include <bslma_constructionutil.h>
#include <bslma_default.h>
#include <bslma_defaultallocatorguard.h>
#include <bslma_destructorguard.h>
#include <bslma_newdeleteallocator.h>
#include <bslma_rawdeleterproctor.h>
#include <bslma_testallocator.h>
#include <bslma_testallocatorexception.h>
#include <bslma_testallocatormonitor.h>

#include <bslmf_assert.h>
#include <bslmf_ispointer.h>
#include <bslmf_issame.h>
#include <bslmf_nestedtraitdeclaration.h>

#include <bsls_alignmentutil.h>
#include <bsls_assert.h>
#include <bsls_bsltestutil.h>
#include <bsls_compilerfeatures.h>
#include <bsls_keyword.h>
#include <bsls_libraryfeatures.h>
#include <bsls_nameof.h>
#include <bsls_objectbuffer.h>
#include <bsls_platform.h>
#include <bsls_stopwatch.h>
#include <bsls_types.h>
#include <bsls_util.h>

#include <bsltf_allocargumenttype.h>
#include <bsltf_allocemplacabletesttype.h>
#include <bsltf_argumenttype.h>
#include <bsltf_emplacabletesttype.h>
#include <bsltf_movablealloctesttype.h>
#include <bsltf_movabletesttype.h>
#include <bsltf_moveonlyalloctesttype.h>
#include <bsltf_movestate.h>
#include <bsltf_nondefaultconstructibletesttype.h>
#include <bsltf_nontypicaloverloadstesttype.h>
#include <bsltf_stdallocatoradaptor.h>
#include <bsltf_stdstatefulallocator.h>
#include <bsltf_stdtestallocator.h>
#include <bsltf_templatetestfacility.h>
#include <bsltf_testvaluesarray.h>

#include <iterator>
#include <new>         // placement 'new'
#include <stdexcept>

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
//                             TEST PLAN
// ----------------------------------------------------------------------------
// NOTICE: To reduce the compilation time, this test driver has been broken
// into 2 parts, 'bslstl_deque.t.cpp' (cases 1-11, plus the usage example), and
// 'bslstl_deque_test.cpp' (cases 12 and higher).
//
//                              Overview
//                              --------
// The class under test is a container whose interface and contract is dictated
// by the C++ standard.  In particular, the standard mandates strong exception
// safety (with full guarantee of rollback) in many cases, along with throwing
// 'std::length_error' on any request to increase the size of a deque to have
// more than 'max_size()' elements.  (Note: the value returned by 'max_size'
// depends on the (template parameter) 'VALUE_TYPE'.)  The general concerns are
// compliance, exception safety, and proper dispatching (for member function
// templates such as 'assign' and 'insert').  In addition, deque is a
// value-semantic type whose salient attributes are its size and the value of
// each element in its (ordered) sequence.  The deque container is implemented
// in the form of a class template, so its proper instantiation for several
// types is a concern.  Regarding the allocator template argument, we use
// mostly a 'bsl::allocator' together with a 'bslma::TestAllocator' mechanism,
// but we also verify the C++ standard allocator.
//
// Important states that must be covered by testing include filling "pages" (or
// "blocks") of the deque up to their page boundaries, both at the 'front'
// and the 'back', and behavior crossing those boundaries.  Therefore, the
// generator language provides a way to insert, independently, at the front and
// at the back, to fill up pages at either the front or the back.
//
// This test driver follows the standard approach for components implementing
// value-semantic containers.  We have chosen as *primary* *manipulators* the
// 'push_back' and 'push_front' methods taking rvalue references as well as the
// 'clear' method, with all three methods to be used by the generator function
// 'gg'.  Note that this choice is conditional on the implementation of
// 'clear', which clears all blocks and then sets 'd_start' to 'd_finish'.
// Additional helper functions are provided to facilitate perturbation of
// internal state (e.g., capacity and the start iterator within the start
// block).  Note that some manipulators must support aliasing, and those that
// perform memory allocation must be tested for exception neutrality via
// 'bslma::TestAllocator'.  After the mandatory sequence of cases (1-10) for
// value-semantic types (cases 5 and 10 are not implemented, as there is no
// output or BDEX streaming below 'bslstl'), we test each individual
// constructor, manipulator, and accessor in subsequent cases.  Move semantics
// (C++11) are tested within relevant test cases.
//
// Abbreviations
// -------------
// Throughout this test driver, we use:
//  T        VALUE_TYPE (template argument, no default)
//  A        ALLOCATOR (template argument, default is 'bsl::allocator<T>')
//  deque    bsl::deque<VALUE_TYPE, ALLOCATOR>
//  Args...  shorthand for a family of templates <A1>, <A1, A2>, etc.
// ----------------------------------------------------------------------------
// class bsl::deque<T, A>
// ============================================================================
// [11] TRAITS
//
// CREATORS
// [ 2] deque(const A& a = A());
// [12] deque(size_type n, const A& a = A());
// [12] deque(size_type n, const T& value, const A& a = A());
// [12] deque(ITER first, ITER last, const A& a = A());
// [ 7] deque(const deque& original, const A& = A());
// [24] deque(deque&& original);
// [24] deque(deque&& original, const A& basicAllocator);
// [30] deque(initializer_list<T>, const A& = A());
// [ 2] ~deque();
//
/// MANIPULATORS
// [ 9] deque& operator=(const deque& rhs);
// [25] deque& operator=(deque&& rhs);
// [30] deque& operator=(initializer_list<T>);
// [13] void assign(size_type n, const T& value);
// [13] void assign(ITER first, ITER last);
// [30] void assign(initializer_list<T>);
// [14] void reserve(size_type n);
// [14] void resize(size_type n);
// [14] void resize(size_type n, const T& value);
// [31] void shrink_to_fit();
// [17] void push_front(const T& value);
// [26] void push_front(T&& rvalue);
// [17] void push_back(const T& value);
// [26] void push_back(T&& rvalue);
// [28] reference emplace_front(Args&&... args);
// [28] reference emplace_back(Args&&... args);
// [29] iterator emplace(const_iterator pos, Args&&... args);
// [20] void pop_front();
// [20] void pop_back();
// [18] iterator insert(const_iterator pos, const T& value);
// [27] iterator insert(const_iterator pos, T&& rvalue);
// [18] iterator insert(const_iterator pos, size_type n, const T& value);
// [19] iterator insert(const_iterator pos, ITER first, ITER last);
// [30] iterator insert(const_iterator pos, initializer_list<T>);
// [20] iterator erase(const_iterator pos);
// [20] iterator erase(const_iterator first, const_iterator last);
// [21] void swap(deque& other);
// [ 2] void clear();
// [16] iterator begin();
// [16] iterator end();
// [16] reverse_iterator rbegin();
// [16] reverse_iterator rend();
// [15] reference operator[](size_type position);
// [15] reference at(size_type position);
// [15] reference front();
// [15] reference back();
//
// ACCESSORS
// [ 4] allocator_type get_allocator() const;
// [14] size_type max_size() const;
// [ 4] size_type size() const;
// [14] size_type capacity() const;
// [14] bool empty() const;
// [16] const_iterator begin() const;
// [16] const_iterator cbegin() const;
// [16] const_iterator end() const;
// [16] const_iterator cend() const;
// [16] const_reverse_iterator rbegin() const;
// [16] const_reverse_iterator crbegin() const;
// [16] const_reverse_iterator rend() const;
// [16] const_reverse_iterator crend() const;
// [ 4] const_reference operator[](size_type position) const;
// [ 4] const_reference at(size_type position) const;
// [15] const_reference front() const;
// [15] const_reference back() const;
//
// FREE OPERATORS
// [ 6] bool operator==(const deque& lhs, const deque& rhs);
// [ 6] bool operator!=(const deque& lhs, const deque& rhs);
// [22] bool operator< (const deque& lhs, const deque& rhs);
// [22] bool operator> (const deque& lhs, const deque& rhs);
// [22] bool operator<=(const deque& lhs, const deque& rhs);
// [22] bool operator>=(const deque& lhs, const deque& rhs);
//
// FREE FUNCTIONS
// [??] void swap(deque& a, deque& b);
// ----------------------------------------------------------------------------
// [ 1] BREATHING TEST
// [11] ALLOCATOR-RELATED CONCERNS
// [33] USAGE EXAMPLE 1
// [34] USAGE EXAMPLE 2
// [23] CONCERN: 'std::length_error' is used properly.
// [ *] CONCERN: In no case does memory come from the global allocator.
// [32] CONCERN: Methods qualified 'noexcept' in standard are so implemented.
//
// TEST APPARATUS: GENERATOR FUNCTIONS
// [ 3] int ggg(Obj *object, const char *spec, bool vF = true);
// [ 3] Obj& gg(Obj *object, const char *spec);
// [ 8] Obj   g(const char *spec);  // no longer implemented

// ============================================================================
//                     STANDARD BSL ASSERT TEST FUNCTION
// ----------------------------------------------------------------------------

namespace {

int testStatus = 0;

void aSsErT(bool condition, const char *message, int line)
{
    if (condition) {
        printf("Error " __FILE__ "(%d): %s    (failed)\n", line, message);

        if (0 <= testStatus && testStatus <= 100) {
            ++testStatus;
        }
    }
}

}  // close unnamed namespace

// ============================================================================
//               STANDARD BSL TEST DRIVER MACRO ABBREVIATIONS
// ----------------------------------------------------------------------------

#define ASSERT       BSLS_BSLTESTUTIL_ASSERT
#define ASSERTV      BSLS_BSLTESTUTIL_ASSERTV

#define LOOP_ASSERT  BSLS_BSLTESTUTIL_LOOP_ASSERT
#define LOOP0_ASSERT BSLS_BSLTESTUTIL_LOOP0_ASSERT
#define LOOP1_ASSERT BSLS_BSLTESTUTIL_LOOP1_ASSERT
#define LOOP2_ASSERT BSLS_BSLTESTUTIL_LOOP2_ASSERT
#define LOOP3_ASSERT BSLS_BSLTESTUTIL_LOOP3_ASSERT
#define LOOP4_ASSERT BSLS_BSLTESTUTIL_LOOP4_ASSERT
#define LOOP5_ASSERT BSLS_BSLTESTUTIL_LOOP5_ASSERT
#define LOOP6_ASSERT BSLS_BSLTESTUTIL_LOOP6_ASSERT

#define Q            BSLS_BSLTESTUTIL_Q   // Quote identifier literally.
#define P            BSLS_BSLTESTUTIL_P   // Print identifier and value.
#define P_           BSLS_BSLTESTUTIL_P_  // P(X) without '\n'.
#define T_           BSLS_BSLTESTUTIL_T_  // Print a tab (w/o newline).
#define L_           BSLS_BSLTESTUTIL_L_  // current Line number

#define RUN_EACH_TYPE BSLTF_TEMPLATETESTFACILITY_RUN_EACH_TYPE

// ============================================================================
//                  NEGATIVE-TEST MACRO ABBREVIATIONS
// ----------------------------------------------------------------------------

#define ASSERT_SAFE_PASS(EXPR) BSLS_ASSERTTEST_ASSERT_SAFE_PASS(EXPR)
#define ASSERT_SAFE_FAIL(EXPR) BSLS_ASSERTTEST_ASSERT_SAFE_FAIL(EXPR)
#define ASSERT_PASS(EXPR)      BSLS_ASSERTTEST_ASSERT_PASS(EXPR)
#define ASSERT_FAIL(EXPR)      BSLS_ASSERTTEST_ASSERT_FAIL(EXPR)
#define ASSERT_OPT_PASS(EXPR)  BSLS_ASSERTTEST_ASSERT_OPT_PASS(EXPR)
#define ASSERT_OPT_FAIL(EXPR)  BSLS_ASSERTTEST_ASSERT_OPT_FAIL(EXPR)

// ============================================================================
//                  PRINTF FORMAT MACRO ABBREVIATIONS
// ----------------------------------------------------------------------------

#define ZU BSLS_BSLTESTUTIL_FORMAT_ZU

// ============================================================================
//                  TARGET SPECIFIC CONFIGURATION
// ----------------------------------------------------------------------------

#if defined(BSLS_COMPILERFEATURES_SIMULATE_FORWARD_WORKAROUND)
# define BSL_DO_NOT_TEST_MOVE_FORWARDING 1
// Some compilers produce ambiguities when trying to construct our test types
// for 'emplace'-type functionality with the C++03 move-emulation.  This is a
// compiler bug triggering in lower level components, so we simply disable
// those aspects of testing, and rely on the extensive test coverage on other
// platforms.
#endif

enum {
// The following enum is set to 1 when exceptions are enabled and to 0
// otherwise.  It's here to avoid having preprocessor macros throughout.
#if defined(BDE_BUILD_TARGET_EXC)
    PLAT_EXC = 1
#else
    PLAT_EXC = 0
#endif
};

// ============================================================================
//              ADDITIONAL TEST MACROS FOR THIS TEST DRIVER
// ----------------------------------------------------------------------------

#if defined(BSLS_LIBRARYFEATURES_HAS_CPP17_BOOL_CONSTANT)
# define DECLARE_BOOL_CONSTANT(NAME, EXPRESSION)                              \
    BSLS_KEYWORD_CONSTEXPR_MEMBER bsl::bool_constant<EXPRESSION> NAME{}
    // This leading branch is the preferred version for C++17, but the feature
    // test macro is (currently) for documentation purposes only, and never
    // defined.  This is the ideal (simplest) form for such declarations:
#elif defined(BSLS_COMPILERFEATURES_SUPPORT_CONSTEXPR)
# define DECLARE_BOOL_CONSTANT(NAME, EXPRESSION)                              \
    constexpr bsl::integral_constant<bool, EXPRESSION> NAME{}
    // This is the preferred C++11 form for the definition of integral constant
    // variables.  It assumes the presence of 'constexpr' in the compiler as an
    // indication that brace-initialization and traits are available, as it has
    // historically been one of the last C++11 features to ship.
#else
# define DECLARE_BOOL_CONSTANT(NAME, EXPRESSION)                              \
    static const bsl::integral_constant<bool, EXPRESSION> NAME =              \
                 bsl::integral_constant<bool, EXPRESSION>()
    // 'bsl::integral_constant' is not an aggregate prior to C++17 extending
    // the rules, so a C++03 compiler must explicitly initialize integral
    // constant variables in a way that is unambiguously not a vexing parse
    // that declares a function instead.
#endif

// ============================================================================
//                             SWAP TEST HELPERS
// ----------------------------------------------------------------------------

namespace incorrect {

template <class TYPE>
void swap(TYPE&, TYPE&)
    // Fail.  In a successful test, this 'swap' should never be called.  It is
    // set up to be called (and fail) in the case where ADL fails to choose the
    // right 'swap' in 'invokeAdlSwap' below.
{
    ASSERT(0 && "incorrect swap called");
}

}  // close namespace incorrect

template <class TYPE>
void invokeAdlSwap(TYPE *a, TYPE *b)
    // Exchange the values of the specified '*a' and '*b' objects using the
    // 'swap' method found by ADL (Argument Dependent Lookup).
{
    using incorrect::swap;

    // A correct ADL will key off the types of '*a' and '*b', which will be of
    // our 'bsl' container type, to find the right 'bsl::swap' and not
    // 'incorrect::swap'.

    swap(*a, *b);
}

template <class TYPE>
void invokePatternSwap(TYPE *a, TYPE *b)
    // Exchange the values of the specified '*a' and '*b' objects using the
    // 'swap' method found by the recommended pattern for calling 'swap'.
{
    // Invoke 'swap' using the recommended pattern for 'bsl' clients.

    using bsl::swap;

    swap(*a, *b);
}

// The following 'using' directives must come *after* the definition of
// 'invokeAdlSwap' and 'invokePatternSwap' (above).

using namespace BloombergLP;
using bsl::deque;
using bsl::Deque_BlockLengthCalcUtil;
using bsls::NameOf;

// ============================================================================
//                  GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
// ----------------------------------------------------------------------------

// uses 'bslma' allocators
class TestTypeAlloc;
class BitwiseMoveableTestTypeAlloc;

typedef TestTypeAlloc                   TTA;
typedef BitwiseMoveableTestTypeAlloc    BMTTA;

// does not use 'bslma' allocators
class SmallTestTypeNoAlloc;
class MediumTestTypeNoAlloc;
class LargeTestTypeNoAlloc;
class BitwiseCopyableTestTypeNoAlloc;

typedef SmallTestTypeNoAlloc            SmlTT;
typedef MediumTestTypeNoAlloc           MedTT;
typedef LargeTestTypeNoAlloc            LrgTT;
typedef BitwiseCopyableTestTypeNoAlloc  BCTT;

typedef bsls::Types::IntPtr             IntPtr;
typedef bsls::Types::Int64              Int64;
typedef bsls::Types::Uint64             Uint64;

template <class T>
struct PageLength {
    // This class indicates, through an 'enum' 'k_VALUE', the number of objects
    // that can be inserted onto a page of a 'deque<T>'.  Note that the
    // effective capacity when inserting at the back is one fewer, as the last
    // slot must be occupied by a non-created object, which is the target of
    // the 'end' iterator.  This slot *is* occupied when inserting the last
    // element, but it requires allocating a fresh page with a new address for
    // the dummy one-past-the-end object.  This template must be specialized
    // with a hand-computed result for each type under test, in order to verify
    // that the internally computed page-size corresponds to the value that we
    // wish to test.
    //
    // Note that the explicit specializations of this class that defined its
    // value members must follow all the types that we wish to support in this
    // file, so that we can evaluate static assertions on the types' sizes.

    enum { k_VALUE = 16 };  // default; see specializations below
};

// CONSTANTS
const char UNINITIALIZED_VALUE = '_';
const char DEFAULT_VALUE       = 'z';
const char VA = 'A';
const char VB = 'B';
const char VC = 'C';
const char VD = 'D';
const char VE = 'E';
    // All test types have character value type.

const int NOMINAL_BLOCK_BYTES = bsl::Deque_BlockLengthCalcUtil<int>::
                                                            DEFAULT_BLOCK_SIZE;
    // Used 'int', but any type will do here, since DEFAULT_BLOCK_SIZE is
    // independent of the type.

const int MEDIUM_TEST_TYPE_SIZE = NOMINAL_BLOCK_BYTES / 2;
    // Size such that there should be only two objects in a deque block.

const int LARGE_TEST_TYPE_SIZE = NOMINAL_BLOCK_BYTES * 2;
    // Size such that there should be only a single object in a deque block.

const int NUM_PADDING = 2;
    // BLOCK_ARRAY_PADDING as defined in 'bslalg_dequeimputil'.

const int NUM_INTERNAL_STATE_TEST = 10;
    // Number of different internal states to check.

const int k_MINIMUM_PAGE_LENGTH = NOMINAL_BLOCK_BYTES;

// Define values used to initialize positional arguments for
// 'bsltf::EmplacableTestType' and 'bsltf::AllocEmplacableTestType'
// constructors.  Note, that you cannot change those values as they are used by
// 'TemplateTestFacility::getIdentifier' to map the constructed emplacable
// objects to their integer identifiers.
static const int V01 = 1;
static const int V02 = 20;
static const int V03 = 23;
static const int V04 = 44;
static const int V05 = 66;
static const int V06 = 176;
static const int V07 = 878;
static const int V08 = 8;
static const int V09 = 912;
static const int V10 = 102;

// ============================================================================
//                      GLOBAL HELPER FUNCTIONS FOR TESTING
// ----------------------------------------------------------------------------

typedef bsls::Types::Int64 Int64;

// Define DEFAULT DATA used in multiple test cases.

struct DefaultDataRow {
    int         d_line;     // source line number
    int         d_index;    // lexical order
    const char *d_spec;     // specification string, for input to 'gg' function
    const char *d_results;  // canonical equivalent spec
};

static
const DefaultDataRow DEFAULT_DATA[] = {
    //line idx  spec                 results
    //---- ---  --------             -------
    { L_,    0, "",                  "" },
    { L_,    1, "a",                 "A" },
    { L_,    2, "aa",                "AA" },
    { L_,    3, "aBCaBC",            "AABCBC" },
    { L_,    4, "aBCa",              "AABC" },
    { L_,    5, "aB",                "AB" },
    { L_,    6, "BCaB",              "ABCB" },
    { L_,    7, "BaC",               "ABC" },
    { L_,    9, "BCDa",              "ABCD" },
    { L_,   10, "BCaDE",             "ABCDE" },
    { L_,   11, "FEDCBa",            "AFEDCB" },
    { L_,   13, "BCDEFaGH",          "ABCDEFGH" },
    { L_,   14, "GCDFEabHI",         "BAGCDFEHI" },
    { L_,   15, "BFEDFGaMNOPHbIJKL", "BABFEDFGMNOPHIJKL" },
    { L_,   17, "BaD",               "ABD" },
    { L_,   19, "aC",                "AC" },
    { L_,   20, "B",                 "B" },
    { L_,   21, "BCDE",              "BCDE" },
    { L_,   22, "FEDCBc",            "CFEDCB" },
    { L_,   23, "cde",               "EDC" },
};
enum { DEFAULT_NUM_DATA = sizeof DEFAULT_DATA / sizeof *DEFAULT_DATA };

// ============================================================================
//                      GLOBAL HELPER FUNCTIONS FOR TESTING
// ----------------------------------------------------------------------------

namespace bsl {

template <class TYPE, class ALLOC>
void debugprint(const bsl::deque<TYPE,ALLOC>& v)
{
    using bsls::debugprint;

    if (v.empty()) {
        printf("<empty>");
    }
    else {
        putchar('"');
        for (size_t ii = 0; ii < v.size(); ++ii) {
            putchar(static_cast<char>(
                           bsltf::TemplateTestFacility::getIdentifier(v[ii])));
        }
        putchar('"');
    }
}

}  // close namespace bsl

template <class CONTAINER, class VALUES>
size_t verifyContainer(const CONTAINER& container,
                       const VALUES&    expectedValues,
                       size_t           expectedSize)
    // Verify the specified 'container' has the specified 'expectedSize' and
    // contains the same values as the array in the specified 'expectedValues'.
    // Return 0 if 'container' has the expected values, and a non-zero value
    // otherwise.
{
    ASSERTV(expectedSize, container.size(), expectedSize == container.size());

    if (expectedSize != container.size()) {
        return static_cast<size_t>(-1);                               // RETURN
    }

    typename CONTAINER::const_iterator it = container.cbegin();
    for (size_t i = 0; i < expectedSize; ++i) {
        ASSERTV(it != container.cend());
        ASSERTV(i, expectedValues[i], *it, expectedValues[i] == *it);

        if (bsltf::TemplateTestFacility::getIdentifier(expectedValues[i])
            != bsltf::TemplateTestFacility::getIdentifier(*it)) {
            return i + 1;                                             // RETURN
        }
        ++it;
    }
    return 0;
}

template <class CONTAINER>
size_t numMovedInto(const CONTAINER& X,
                    size_t           startIndex = 0,
                    size_t           endIndex   = 0)
{
    typedef bsltf::TemplateTestFacility TstFacility;
    typedef bsltf::MoveState            MoveState;

    int numMoved = 0;
    size_t upTo = endIndex > startIndex ? endIndex : X.size();
    for (size_t i = startIndex; i < upTo; ++i) {
        MoveState::Enum mState = TstFacility::getMovedIntoState(X[i]);
        if (MoveState::e_MOVED == mState || MoveState::e_UNKNOWN == mState) {
            ++numMoved;
        }
    }
    return numMoved;
}

template <class CONTAINER>
size_t numNotMovedInto(const CONTAINER& X,
                       size_t           startIndex = 0,
                       size_t           endIndex   = 0)
{
    typedef bsltf::TemplateTestFacility TstFacility;
    typedef bsltf::MoveState            MoveState;

    int numNotMoved = 0;
    size_t upTo = endIndex > startIndex ? endIndex : X.size();
    for (size_t i = startIndex; i < upTo; ++i) {
        MoveState::Enum mState = TstFacility::getMovedIntoState(X[i]);
        if (MoveState::e_NOT_MOVED == mState
         || MoveState::e_UNKNOWN   == mState) {
            ++numNotMoved;
        }
    }
    return numNotMoved;
}

template <class CONTAINER>
bool verifySpec(const CONTAINER& c, const char *spec)
{
    const size_t sz = c.size();
    if (strlen(spec) != sz) {
        return false;                                                 // RETURN
    }

    for (unsigned ii = 0; ii < sz; ++ii) {
        if (c[ii] != spec[ii]) {
            return false;                                             // RETURN
        }
    }

    return true;
}

// ============================================================================
//                       GLOBAL HELPER CLASSES FOR TESTING
// ----------------------------------------------------------------------------

// STATIC DATA
static bool             verbose;
static bool         veryVerbose;
static bool     veryVeryVerbose;
static bool veryVeryVeryVerbose;

static int numDefaultCtorCalls = 0;
static int numCharCtorCalls    = 0;
static int numCopyCtorCalls    = 0;
static int numAssignmentCalls  = 0;
static int numDestructorCalls  = 0;


                            // ======================
                            // class ExceptionProctor
                            // ======================

template <class OBJECT, class ALLOCATOR>
class ExceptionProctor {
    // This class provides a mechanism to verify the strong exception guarantee
    // in exception-throwing code.  On construction, this class stores a copy
    // of an object of the (template parameter) type 'OBJECT' and the address
    // of that object.  On destruction, if 'release' was not invoked, it will
    // verify the value of the object is the same as the value of the copy
    // created on construction.  This class requires that the copy constructor
    // and 'operator ==' to be tested before use.

    // DATA
    int                         d_line;      // line number at construction
    bsls::ObjectBuffer<OBJECT>  d_control;   // copy of the object being tested
    const OBJECT               *d_object_p;  // address of the original object

  private:
    // NOT IMPLEMENTED
    ExceptionProctor(const ExceptionProctor&);
    ExceptionProctor& operator=(const ExceptionProctor&);

  public:
    // CREATORS
    ExceptionProctor(const OBJECT    *object,
                     int              line,
                     const ALLOCATOR& basicAllocator = ALLOCATOR())
    : d_line(line)
    , d_object_p(object)
        // Create an exception proctor for the specified 'object' at the
        // specified 'line' number that uses the specified 'basicAllocator' to
        // supply memory for the control object.
    {
        if (d_object_p) {
            new (d_control.buffer()) OBJECT(*object, basicAllocator);
        }
    }

    ExceptionProctor(const OBJECT     *object,
                     const OBJECT&     control,
                     int               line,
                     const ALLOCATOR&  basicAllocator = ALLOCATOR())
    : d_line(line)
    , d_object_p(object)
        // Create an exception proctor for the specified 'object' at
        // the specified 'line' number using the specified 'control'
        // object.  Use the specified 'basicAllocator' to supply
        // memory for the control object.
    {
        if (d_object_p) {
            new (d_control.buffer()) OBJECT(control, basicAllocator);
        }
    }

    ExceptionProctor(const OBJECT              *object,
                     bslmf::MovableRef<OBJECT>  control,
                     int                        line)
    : d_line(line)
    , d_object_p(object)
        // Create an exception proctor for the specified 'object' at the
        // specified 'line' number using the specified 'control' object.
    {
        if (d_object_p) {
            new (d_control.buffer()) OBJECT(
                                         bslmf::MovableRefUtil::move(control));
        }
    }

    ~ExceptionProctor()
        // Destroy the exception proctor.  If the proctor was not released,
        // verify that the state of the object supplied at construction has not
        // changed.
    {
        if (d_object_p) {
            const int LINE = d_line;
            ASSERTV(LINE, d_control.object(), *d_object_p,
                    d_control.object() == *d_object_p);
            d_control.object().~OBJECT();
        }
    }

    // MANIPULATORS
    void release()
        // Release the guard from verifying the state of the object.
    {
        if (d_object_p) {
            d_control.object().~OBJECT();
        }
        d_object_p = 0;
    }
};

                               // ===================
                               // class TestTypeAlloc
                               // ===================

class TestTypeAlloc {
    // This test type contains a 'char' in some allocated storage.  It counts
    // the number of default constructions, copy constructions, assignments,
    // and destructions.  It declares the 'bslma::UsesBslmaAllocator' trait.
    // It could also have the bitwise moveable trait, but we defer that to the
    // 'BitwiseMoveableTestTypeAlloc' type.

    // DATA
    char             *d_data_p;
    bslma::Allocator *d_allocator_p;

#if defined(BDE_USE_ADDRESSOF)
    // PRIVATE ACCESSORS
    void operator&() const;     // = delete;
        // Suppress the use of address-of operator on this type.
#endif

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TestTypeAlloc, bslma::UsesBslmaAllocator);

    // CREATORS
    explicit
    TestTypeAlloc(bslma::Allocator *ba = 0)
    : d_data_p(0)
    , d_allocator_p(bslma::Default::allocator(ba))
    {
        ++numDefaultCtorCalls;
        d_data_p  = (char *)d_allocator_p->allocate(sizeof(char));
        *d_data_p = DEFAULT_VALUE;
    }

    explicit
    TestTypeAlloc(int c, bslma::Allocator *ba = 0)
    : d_data_p(0)
    , d_allocator_p(bslma::Default::allocator(ba))
    {
        ++numCharCtorCalls;
        d_data_p  = (char *)d_allocator_p->allocate(sizeof(char));
        *d_data_p = static_cast<char>(c);
    }

    TestTypeAlloc(const TestTypeAlloc& original, bslma::Allocator *ba = 0)
    : d_data_p(0)
    , d_allocator_p(bslma::Default::allocator(ba))
    {
        ++numCopyCtorCalls;
        d_data_p  = (char *)d_allocator_p->allocate(sizeof(char));
        *d_data_p = *original.d_data_p;
    }

    TestTypeAlloc(bslmf::MovableRef<TestTypeAlloc> original)
    {
        TestTypeAlloc& local = original;

        d_data_p  = (char *)d_allocator_p->allocate(sizeof(char));
        *d_data_p = *local.d_data_p;
    }

    ~TestTypeAlloc()
    {
        ++numDestructorCalls;
        *d_data_p = UNINITIALIZED_VALUE;
        d_allocator_p->deallocate(d_data_p);
        d_data_p = 0;
        d_allocator_p = 0;
    }

    // MANIPULATORS
    TestTypeAlloc& operator=(const TestTypeAlloc& rhs)
    {
        ++numAssignmentCalls;
        if (BSLS_UTIL_ADDRESSOF(rhs) != this) {
            char *newData = (char *)d_allocator_p->allocate(sizeof(char));
            *d_data_p = UNINITIALIZED_VALUE;
            d_allocator_p->deallocate(d_data_p);
            d_data_p  = newData;
            *d_data_p = *rhs.d_data_p;
        }
        return *this;
    }
        
    TestTypeAlloc& operator=(bslmf::MovableRef<TestTypeAlloc> rhs)
    {
        TestTypeAlloc& local = rhs;

        if (BSLS_UTIL_ADDRESSOF(local) != this) {
            char *newData = (char *)d_allocator_p->allocate(sizeof(char));
            *d_data_p = UNINITIALIZED_VALUE;
            d_allocator_p->deallocate(d_data_p);
            d_data_p  = newData;
            *d_data_p = *local.d_data_p;
        }
        return *this;
    }

    void setDatum(char c)
    {
        *d_data_p = c;
    }

    // ACCESSORS
    operator int() const
    {
        return static_cast<int>(*d_data_p);
    }

    char value() const
    {
        return *d_data_p;
    }

    bslma::Allocator *allocator() const
    {
        return d_allocator_p;
    }

    void print() const
    {
        if (d_data_p) {
            ASSERT(isalpha(*d_data_p));
            printf("%c (int: %d)\n", *d_data_p, (int)*d_data_p);
        } else {
            printf("VOID\n");
        }
    }
};

// FREE OPERATORS
bool operator==(const TestTypeAlloc& lhs, const TestTypeAlloc& rhs)
{
    ASSERT(isalpha(lhs.value()));
    ASSERT(isalpha(rhs.value()));

    return lhs.value() == rhs.value();
}

bool operator<(const TestTypeAlloc& lhs, const TestTypeAlloc& rhs)
{
    ASSERT(isalpha(lhs.value()));
    ASSERT(isalpha(rhs.value()));

    return lhs.value() < rhs.value();
}

// TestTypeAlloc-specific print function.
void debugprint(const TestTypeAlloc& rhs)
{
    printf("%c", rhs.value());
//    fflush(stdout);
}

                       // ==========================
                       // class SmallTestTypeNoAlloc
                       // ==========================

class SmallTestTypeNoAlloc {
    // This test type has a footprint and interface identical to
    // 'TestTypeAlloc', except that it does not use allocators.  It also counts
    // the number of default constructions, copy constructions, assignments,
    // and destructions.  It does not allocate, and thus could have the bitwise
    // copyable trait, but we defer that to the
    // 'BitwiseCopyableTestTypeNoAlloc' type.

    // DATA
    union {
        char d_char;
        char d_fill[sizeof(TestTypeAlloc)];
    } d_u;

  public:
    // CREATORS
    SmallTestTypeNoAlloc()
    {
        d_u.d_char = DEFAULT_VALUE;
        ++numDefaultCtorCalls;
    }

    explicit
    SmallTestTypeNoAlloc(int c)
    {
        d_u.d_char = static_cast<char>(c);
        ++numCharCtorCalls;
    }

    SmallTestTypeNoAlloc(const SmallTestTypeNoAlloc& original)
    {
        d_u.d_char = original.d_u.d_char;
        ++numCopyCtorCalls;
    }

    ~SmallTestTypeNoAlloc()
    {
        ++numDestructorCalls;
        d_u.d_char = '_';
    }

    // MANIPULATORS
    SmallTestTypeNoAlloc& operator=(const SmallTestTypeNoAlloc& rhs)
    {
        ++numAssignmentCalls;
        d_u.d_char = rhs.d_u.d_char;
        return *this;
    }

    // ACCESSORS
    operator int() const
    {
        return static_cast<int>(d_u.d_char);
    }

    char value() const
    {
        return d_u.d_char;
    }

    void print() const
    {
        ASSERT(isalpha(d_u.d_char));
        printf("%c (int: %d)\n", d_u.d_char, (int)d_u.d_char);
    }
};

// FREE OPERATORS
bool operator==(const SmallTestTypeNoAlloc& lhs,
                const SmallTestTypeNoAlloc& rhs)
{
    ASSERT(isalpha(lhs.value()));
    ASSERT(isalpha(rhs.value()));

    return lhs.value() == rhs.value();
}

// SmallTestTypeNoAlloc-specific print function.
void debugprint(const SmallTestTypeNoAlloc& rhs)
{
    printf("%c", rhs.value());
    fflush(stdout);
}

                       // ===========================
                       // class MediumTestTypeNoAlloc
                       // ===========================

class MediumTestTypeNoAlloc {
    // This test type has a larger footprint than 'TestTypeAlloc', so that
    // there are fewer objects in a deque's block.  It also counts the number
    // of default constructions, copy constructions, assignments, and
    // destructions.  It does not allocate, and thus could have the bitwise
    // copyable trait, but we defer that to the
    // 'BitwiseCopyableTestTypeNoAlloc' type.

    // DATA
    union {
        char d_char;
        char d_fill[MEDIUM_TEST_TYPE_SIZE];
    } d_u;

  public:
    // CREATORS
    MediumTestTypeNoAlloc()
    {
        d_u.d_char = DEFAULT_VALUE;
        ++numDefaultCtorCalls;
    }

    explicit
    MediumTestTypeNoAlloc(int c)
    {
        d_u.d_char = static_cast<char>(c);
        ++numCharCtorCalls;
    }

    MediumTestTypeNoAlloc(const MediumTestTypeNoAlloc&  original)
    {
        d_u.d_char = original.d_u.d_char;
        ++numCopyCtorCalls;
    }

    ~MediumTestTypeNoAlloc()
    {
        ++numDestructorCalls;
        d_u.d_char = '_';
    }

    // MANIPULATORS
    MediumTestTypeNoAlloc& operator=(const MediumTestTypeNoAlloc& rhs)
    {
        ++numAssignmentCalls;
        d_u.d_char = rhs.d_u.d_char;
        return *this;
    }

    // ACCESSORS
    operator int() const
    {
        return static_cast<int>(d_u.d_char);
    }

    char value() const
    {
        return d_u.d_char;
    }

    void print() const
    {
        ASSERT(isalpha(d_u.d_char));
        printf("%c (int: %d)\n", d_u.d_char, (int)d_u.d_char);
    }
};

// FREE OPERATORS
bool operator==(const MediumTestTypeNoAlloc& lhs,
                const MediumTestTypeNoAlloc& rhs)
{
    ASSERT(isalpha(lhs.value()));
    ASSERT(isalpha(rhs.value()));

    return lhs.value() == rhs.value();
}

// MediumTestTypeNoAlloc-specific print function.
void debugprint(const MediumTestTypeNoAlloc& rhs)
{
    printf("%c", rhs.value());
//    fflush(stdout);
}

                       // ==========================
                       // class LargeTestTypeNoAlloc
                       // ==========================

class LargeTestTypeNoAlloc {
    // This test type has a much larger footprint than 'TestTypeAlloc', so that
    // there are just 16 objects in a deque's block.  It also counts the number
    // of default constructions, copy constructions, assignments, and
    // destructions.  It does not allocate, and thus could have the bitwise
    // copyable trait, but we defer that to the
    // 'BitwiseCopyableTestTypeNoAlloc' type.

    // DATA
    union {
        char d_char;
        char d_fill[LARGE_TEST_TYPE_SIZE];
    } d_u;

  public:
    // CREATORS
    LargeTestTypeNoAlloc()
    {
        d_u.d_char = DEFAULT_VALUE;
        ++numDefaultCtorCalls;
    }

    explicit
    LargeTestTypeNoAlloc(int c)
    {
        d_u.d_char = static_cast<char>(c);
        ++numCharCtorCalls;
    }

    LargeTestTypeNoAlloc(const LargeTestTypeNoAlloc&  original)
    {
        d_u.d_char = original.d_u.d_char;
        ++numCopyCtorCalls;
    }

    ~LargeTestTypeNoAlloc()
    {
        ++numDestructorCalls;
        d_u.d_char = '_';
    }

    // MANIPULATORS
    LargeTestTypeNoAlloc& operator=(const LargeTestTypeNoAlloc& rhs)
    {
        ++numAssignmentCalls;
        d_u.d_char = rhs.d_u.d_char;
        return *this;
    }

    // ACCESSORS
    operator int() const
    {
        return static_cast<int>(d_u.d_char);
    }

    char value() const
    {
        return d_u.d_char;
    }

    void print() const
    {
        ASSERT(isalpha(d_u.d_char));
        printf("%c (int: %d)\n", d_u.d_char, (int)d_u.d_char);
    }
};

// FREE OPERATORS
bool operator==(const LargeTestTypeNoAlloc& lhs,
                const LargeTestTypeNoAlloc& rhs)
{
    ASSERT(isalpha(lhs.value()));
    ASSERT(isalpha(rhs.value()));

    return lhs.value() == rhs.value();
}

// LargeTestTypeNoAlloc-specific print function.
void debugprint(const LargeTestTypeNoAlloc& rhs)
{
    printf("%c", rhs.value());
//    fflush(stdout);
}

                       // ==================================
                       // class BitwiseMoveableTestTypeAlloc
                       // ==================================

class BitwiseMoveableTestTypeAlloc : public TestTypeAlloc {
    // This test type is identical to 'TestTypeAlloc' except that it has the
    // bitwise moveable trait.  All members are inherited.

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(BitwiseMoveableTestTypeAlloc,
                                   bslma::UsesBslmaAllocator);
    BSLMF_NESTED_TRAIT_DECLARATION(BitwiseMoveableTestTypeAlloc,
                                   bslmf::IsBitwiseMoveable);

    // CREATORS
    explicit
    BitwiseMoveableTestTypeAlloc(bslma::Allocator *ba = 0)
    : TestTypeAlloc(ba)
    {
    }

    explicit
    BitwiseMoveableTestTypeAlloc(int c, bslma::Allocator *ba = 0)
    : TestTypeAlloc(c, ba)
    {
    }

    BitwiseMoveableTestTypeAlloc(const BitwiseMoveableTestTypeAlloc&  original,
                                 bslma::Allocator                    *ba = 0)
    : TestTypeAlloc(original, ba)
    {
    }

    BitwiseMoveableTestTypeAlloc(
                      bslmf::MovableRef<BitwiseMoveableTestTypeAlloc> original)
    : TestTypeAlloc(static_cast<const TestTypeAlloc>(original))
    {
    }

    // MANIPULATORS
    BitwiseMoveableTestTypeAlloc& operator=(
                                       const BitwiseMoveableTestTypeAlloc& rhs)
    {
        TestTypeAlloc& thisBase = *this;

        thisBase = rhs;

        return *this;
    }

    BitwiseMoveableTestTypeAlloc& operator=(
                           bslmf::MovableRef<BitwiseMoveableTestTypeAlloc> rhs)
    {
        TestTypeAlloc& thisBase = *this;
        TestTypeAlloc& rhsBase  = rhs;

        thisBase = rhsBase;

        return *this;
    }
};

                       // ====================================
                       // class BitwiseCopyableTestTypeNoAlloc
                       // ====================================

class BitwiseCopyableTestTypeNoAlloc : public SmallTestTypeNoAlloc {
    // This test type is identical to 'SmallTestTypeNoAlloc' except that it has
    // the bitwise copyable and bitwise equality-comparable traits.  All
    // members are inherited.

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(BitwiseCopyableTestTypeNoAlloc,
                                   bsl::is_trivially_copyable);
    BSLMF_NESTED_TRAIT_DECLARATION(BitwiseCopyableTestTypeNoAlloc,
                                   bslmf::IsBitwiseEqualityComparable);

    // CREATORS
    BitwiseCopyableTestTypeNoAlloc()
    : SmallTestTypeNoAlloc()
    {
    }

    explicit
    BitwiseCopyableTestTypeNoAlloc(int c)
    : SmallTestTypeNoAlloc(c)
    {
        ++numCharCtorCalls;
    }

    BitwiseCopyableTestTypeNoAlloc(
                                const BitwiseCopyableTestTypeNoAlloc& original)
    : SmallTestTypeNoAlloc(original.value())
    {
    }

    // MANIPULATOR
    BitwiseCopyableTestTypeNoAlloc& operator=(
                                    const BitwiseCopyableTestTypeNoAlloc& rhs)
    {
        SmallTestTypeNoAlloc& thisBase = *this;

        thisBase = rhs;

        return *this;
    }
};

                               // ==============
                               // class CharList
                               // ==============

template <class TYPE>
class CharList {
    // This array class is a simple wrapper on a 'char' array offering an input
    // iterator access via the 'begin' and 'end' accessors.  The iterator is
    // specifically an *input* iterator and its value type is the (template
    // parameter) 'TYPE'.

    // DATA
    bsl::vector<TYPE> d_value;

  public:
    // TYPES
    typedef bslstl::ForwardIterator<const TYPE, const TYPE *> const_iterator;
        // Input iterator.

    // CREATORS
    CharList() {}
    explicit CharList(const bsl::vector<TYPE>& value);

    // ACCESSORS
    const TYPE& operator[](size_t index) const;
    const_iterator begin() const;
    const_iterator end() const;
};

// CREATORS
template <class TYPE>
CharList<TYPE>::CharList(const bsl::vector<TYPE>& value)
: d_value(value)
{
}

// ACCESSORS
template <class TYPE>
const TYPE&
CharList<TYPE>::operator[](size_t index) const
{
    return d_value[index];
}

template <class TYPE>
typename CharList<TYPE>::const_iterator
CharList<TYPE>::begin() const
{
    return const_iterator(d_value.begin());
}

template <class TYPE>
typename CharList<TYPE>::const_iterator
CharList<TYPE>::end() const
{
    return const_iterator(d_value.end());
}

                              // ===============
                              // class CharArray
                              // ===============

template <class TYPE>
class CharArray {
    // This array class is a simple wrapper on a deque offering an input
    // iterator access via the 'begin' and 'end' accessors.  The iterator is
    // specifically a *random-access* iterator and its value type is the
    // (template parameter) 'TYPE'.

    // DATA
    bsl::vector<TYPE>  d_value;

  public:
    // TYPES
    typedef const TYPE *const_iterator;
        // Random-access iterator.

    // CREATORS
    CharArray() {}
    explicit CharArray(const bsl::vector<TYPE>& value);

    // ACCESSORS
    const TYPE& operator[](size_t index) const;
    const_iterator begin() const;
    const_iterator end() const;
};

// CREATORS
template <class TYPE>
CharArray<TYPE>::CharArray(const bsl::vector<TYPE>& value)
: d_value(value)
{
}

// ACCESSORS
template <class TYPE>
const TYPE& CharArray<TYPE>::operator[](size_t index) const
{
    return d_value[index];
}

template <class TYPE>
typename CharArray<TYPE>::const_iterator CharArray<TYPE>::begin() const
{
    return const_iterator(d_value.begin());
}

template <class TYPE>
typename CharArray<TYPE>::const_iterator CharArray<TYPE>::end() const
{
    return const_iterator(d_value.end());
}

                              // ====================
                              // class LimitAllocator
                              // ====================

template <class ALLOC>
class LimitAllocator : public ALLOC {

  public:
    // TYPES
    typedef typename ALLOC::value_type        value_type;
    typedef typename ALLOC::pointer           pointer;
    typedef typename ALLOC::const_pointer     const_pointer;
    typedef typename ALLOC::reference         reference;
    typedef typename ALLOC::const_reference   const_reference;
    typedef typename ALLOC::size_type         size_type;
    typedef typename ALLOC::difference_type   difference_type;

    template <class OTHER_TYPE>
    struct rebind {
        // It is better not to inherit the rebind template, or else
        // 'rebind<X>::other' would be 'ALLOC::rebind<OTHER_TYPE>::other'
        // instead of 'LimitAlloc<X>'.

        typedef LimitAllocator<typename ALLOC::template
                                             rebind<OTHER_TYPE>::other > other;
    };

  private:
    // PRIVATE TYPES
    typedef ALLOC AllocBase;

    // DATA
    size_type d_limit;

  private:
    // NOT IMPLEMENTED
    LimitAllocator& operator=(const LimitAllocator&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS
    LimitAllocator()
    : d_limit(-1) {}

    explicit
    LimitAllocator(bslma::Allocator *mechanism)
    : AllocBase(mechanism)
    , d_limit(-1)
    {
    }

    explicit
    LimitAllocator(const ALLOC& alloc)
    : AllocBase((const AllocBase&)alloc)
    , d_limit(-1)
    {
    }

    ~LimitAllocator()
    {
    }

    // MANIPULATORS
    void setMaxSize(size_type maxSize)
    {
        d_limit = maxSize;
    }

    // ACCESSORS
    size_type max_size() const
    {
        return d_limit;
    }
};

namespace BloombergLP {
namespace bslmf {

template <class ALLOCATOR>
struct IsBitwiseMoveable<LimitAllocator<ALLOCATOR> >
    : IsBitwiseMoveable<ALLOCATOR>
{
};

}  // close namespace bslmf
}  // close enterprise namespace


                        // =========================
                        // class template PageLength
                        // =========================

template <>
struct PageLength<char> {
    typedef char Type;

    BSLMF_ASSERT(sizeof(Type) == 1);  // by definition, but preserves pattern

    enum { k_VALUE = 200 };
};

template <>
struct PageLength<TTA> {
    typedef TTA Type;

    BSLMF_ASSERT(sizeof(Type) == 2 * sizeof(void *));

    enum { k_VALUE = sizeof(char *) == 4 ? 25
                   : sizeof(char *) == 8 ? 16
                   : 0 };
};

template <>
struct PageLength<SmlTT> {
    typedef SmlTT Type;

    BSLMF_ASSERT(sizeof(Type) == 2 * sizeof(void *));

    enum { k_VALUE = sizeof(char *) == 4 ? 25
                   : sizeof(char *) == 8 ? 16
                   : 0 };
};

template <>
struct PageLength<MedTT> {
    typedef MedTT Type;

    BSLMF_ASSERT(sizeof(Type) < k_MINIMUM_PAGE_LENGTH &&
                 sizeof(Type) > k_MINIMUM_PAGE_LENGTH / 16);

    enum { k_VALUE = 16 };
};

template <>
struct PageLength<LrgTT> {
    typedef LrgTT Type;

    BSLMF_ASSERT(sizeof(Type) > k_MINIMUM_PAGE_LENGTH);

    enum { k_VALUE = 16 };
};

template <>
struct PageLength<BMTTA> {
    typedef BMTTA Type;

    BSLMF_ASSERT(sizeof(Type) == 2 * sizeof(void *));

    enum { k_VALUE = sizeof(char *) == 4 ? 25
                   : sizeof(char *) == 8 ? 16
                   : 0 };
};

template <>
struct PageLength<BCTT> {
    typedef BCTT Type;

    BSLMF_ASSERT(sizeof(Type) == 2 * sizeof(void *));

    enum { k_VALUE = sizeof(char *) == 4 ? 25
                   : sizeof(char *) == 8 ? 16
                   : 0 };
};

template <class ITER, class VALUE_TYPE>
struct TestMovableTypeUtil {

  public:
    static ITER findFirstNotMovedInto(ITER begin, ITER end)
    {
        typedef bsltf::TemplateTestFacility TstFacility;
        typedef bsltf::MoveState            MoveState;

        for (; begin != end; ++begin) {
            MoveState::Enum mState = TstFacility::getMovedIntoState(*begin);
            if (MoveState::e_NOT_MOVED == mState) {
                break;
            }
        }
        return begin;
    }
};

struct TestAllocatorUtil {

  public:
    // CLASS METHODS
    template <class TYPE>
    static void test(const TYPE&, const bslma::Allocator&)
    {
    }

    static void test(const bsltf::AllocEmplacableTestType& value,
                     const bslma::Allocator&               allocator)
    {
        ASSERTV(&allocator == value.arg01().allocator());
        ASSERTV(&allocator == value.arg02().allocator());
        ASSERTV(&allocator == value.arg03().allocator());
        ASSERTV(&allocator == value.arg04().allocator());
        ASSERTV(&allocator == value.arg05().allocator());
        ASSERTV(&allocator == value.arg06().allocator());
        ASSERTV(&allocator == value.arg07().allocator());
        ASSERTV(&allocator == value.arg08().allocator());
        ASSERTV(&allocator == value.arg09().allocator());
        ASSERTV(&allocator == value.arg10().allocator());
    }
};

// ============================================================================
//                       TEST DRIVER TEMPLATE
// ----------------------------------------------------------------------------

template <class TYPE, class ALLOC = bsl::allocator<TYPE> >
struct TestDriver {
    // The generating functions interpret the given 'spec' in order from left
    // to right to configure the object according to a custom language.
    // Uppercase letters '[A .. E]' correspond to arbitrary (but unique) 'char'
    // values to be appended to the 'bsl::deque<T>' object.  A tilde ('~')
    // indicates that the logical (but not necessarily physical) state of the
    // object is to be set to its initial, empty state (via the 'clear'
    // method).
    //
    // LANGUAGE SPECIFICATION:
    // -----------------------
    //
    // <SPEC>       ::= <EMPTY>   | <LIST>
    //
    // <EMPTY>      ::=
    //
    // <LIST>       ::= <ITEM>    | <ITEM><LIST>
    //
    // <ITEM>       ::= <ELEMENT> | <POP_BACK> | <CLEAR>
    //
    // <ELEMENT>    ::= 'A' | 'B' | 'C' | 'D' | 'E' | ... | 'H'
    //                                      // unique but otherwise arbitrary
    // <POP_BACK>   ::= '<'
    //
    // <CLEAR>      ::= '~'
    //
    // Spec String  Description
    // -----------  -----------------------------------------------------------
    // ""           Has no effect; leaves the object empty.
    // "A"          Append the value corresponding to A.
    // "AA"         Append two values both corresponding to A.
    // "ABC"        Append three values corresponding to A, B and C.
    // "ABC~"       Append three values corresponding to A, B and C and then
    //              remove all the elements (set array length to 0).  Note that
    //              this spec yields an object that is logically equivalent
    //              (but not necessarily identical internally) to one
    //              yielded by ("").
    // "ABC~DE"     Append three values corresponding to A, B, and C; empty
    //              the object; and append values corresponding to D and E.
    //              (Note that the resulting object has the capacity for at
    //              least three elements at the *front* and none, or the
    //              remainder of the block, at the back.)
    // "ABC<<<~DE"  Append three values corresponding to A, B, and C, then pop
    //              them back; empty the object; and append values
    //              corresponding to D and E.  (Note that the result object has
    //              a capacity for at least one more object at the back, and
    //              none at the front.)
    // ------------------------------------------------------------------------

    // CONSTANTS
    enum {START_POS  = -1 * NUM_PADDING * NOMINAL_BLOCK_BYTES /
                                                           (int) sizeof (TYPE),
        // The integer offset to the first element pointed by the first block
        // pointer of a newly created empty 'bsl::deque'.

          FINISH_POS = ((NUM_PADDING + 1) * NOMINAL_BLOCK_BYTES /
                                                      (int) sizeof (TYPE)) - 1,
        // The integer offset to the last element pointed by the last block
        // pointer of a newly created empty 'bsl::deque'.

          INCREMENT = FINISH_POS - START_POS >= NUM_INTERNAL_STATE_TEST
                    ? (FINISH_POS - START_POS) / NUM_INTERNAL_STATE_TEST
                    : 1
        // The integer increment that will be used when permutating through the
        // various internal states.
    };

    // TYPES
    typedef bsl::deque<TYPE, ALLOC>               Obj;

    typedef typename Obj::iterator                iterator;
    typedef typename Obj::const_iterator          const_iterator;
    typedef typename Obj::reverse_iterator        reverse_iterator;
    typedef typename Obj::const_reverse_iterator  const_reverse_iterator;
    typedef typename Obj::value_type              ValueType;

    typedef typename Obj::const_iterator          CIter;

    typedef bslma::ConstructionUtil               ConstrUtil;
    typedef bslmf::MovableRefUtil                 MoveUtil;

    typedef bsltf::MoveState                      MoveState;
    typedef bsltf::TemplateTestFacility           TstFacility;
    typedef bsltf::TestValuesArray<TYPE>          TestValues;

    typedef TestMovableTypeUtil<CIter, TYPE>      TstMoveUtil;

    typedef bsl::allocator_traits<ALLOC>          AllocatorTraits;

    enum AllocCategory { e_BSLMA, e_STDALLOC, e_ADAPTOR, e_STATEFUL };

    enum { k_IS_WELL_BEHAVED = bsl::is_same<TYPE,
                             bsltf::WellBehavedMoveOnlyAllocTestType>::value };

    // TEST APPARATUS

    // CLASS DATA
    static
    const AllocCategory s_allocCategory =
                        bsl::is_same<ALLOC, bsl::allocator<TYPE> >::value
                        ? e_BSLMA
                        : bsl::is_same<ALLOC,
                                       bsltf::StdAllocTestType<
                                                bsl::allocator<TYPE> > >::value
                        ? e_STDALLOC
                        : bsl::is_same<ALLOC,
                                       bsltf::StdAllocatorAdaptor<
                                                bsl::allocator<TYPE> > >::value
                        ? e_ADAPTOR
                        : e_STATEFUL;

    // CLASS METHODS
    static
    const char *allocCategoryAsStr()
    {
        return e_BSLMA == s_allocCategory ? "bslma"
                                          : e_STDALLOC == s_allocCategory
                                          ? "stdalloc"
                                          : e_ADAPTOR == s_allocCategory
                                          ? "adaptor"
                                          : e_STATEFUL == s_allocCategory
                                          ? "stateful"
                                          : "<INVALID>";
    }

    static void primaryManipulatorBack(Obj   *container,
                                       int    identifier)
        // Insert at the back of specified 'container' the value object
        // indicated by the specified 'identifier', ensuring that the overload
        // of the primary manipulator taking a modifiable rvalue is invoked
        // (rather than the one taking an lvalue).

    {
        bsls::ObjectBuffer<ValueType> buffer;
        TstFacility::emplace(buffer.address(),
                             identifier,
                             container->get_allocator());
        bslma::DestructorGuard<ValueType> guard(buffer.address());

        container->push_back(MoveUtil::move(buffer.object()));
    }

    static void primaryManipulatorFront(Obj   *container,
                                        int    identifier)
        // Insert at the front of specified 'container' the value object
        // indicated by the specified 'identifier', ensuring that the overload
        // of the primary manipulator taking a modifiable rvalue is invoked
        // (rather than the one taking an lvalue).
    {
        bsls::ObjectBuffer<ValueType> buffer;
        TstFacility::emplace(buffer.address(),
                             identifier,
                             container->get_allocator());
        bslma::DestructorGuard<ValueType> guard(buffer.address());

        container->push_front(MoveUtil::move(buffer.object()));
    }

    static int getValues(const TYPE **valuesPtr);
        // Load the specified 'valuesPtr' with the address of an array
        // containing initialized values of the (template parameter) 'TYPE',
        // and return the length of that array.

    static int ggg(Obj *object, const char *spec, bool verboseFlag = true);
        // Configure the specified 'object' according to the specified 'spec',
        // using only the primary manipulator function 'push_back' and
        // white-box manipulator 'clear'.  Optionally specify a 'false'
        // 'verboseFlag' to suppress 'spec' syntax error messages.  Return the
        // index of the first invalid character, and a negative value
        // otherwise.  Note that this function is used to implement 'gg' as
        // well as allow for verification of syntax error detection.
        //
        // The language of the grammar consists of characters in the ranges
        // 'a..z' and 'A..Z', to insert a new object to either the front or the
        // back of the deque, '*object', where lower-case indicates the front
        // and upper-case indicates the back.  A sequence of identical objects
        // can be inserted by preceding the letter with a digit, [2..0], where
        // 0 represents 10 objects.  For tests that require filling to a page
        // boundary of the deque, a sequence contained between '<' and '|' will
        // cause a sequence of insertions to fill the deque all the way to the
        // next page boundary with a an object corresponding to a supplied
        // character, where a lower-case character will fill to the front page
        // of the deque, and an upper-case character will fill the back page.
        // Padding to allow a few additional insertions before reaching the
        // page boundary is requested by a sequence of dots, where '...' would
        // mean to leave room for three more elements before reading the page
        // boundary.  Larger sequences of free slots can be requested by a
        // leading digit before a '.', where 0 indicates 10.  No more than 10
        // free slots can be requested this way.  To aid clarity, non-salient
        // whitespace is permitted to easily depict groups of operations.  If
        // the current page is already full, in the desired direction, or does
        // not have sufficient capacity to honor the requested number of free
        // slots, then an error result will be returned.
        //
        // Arbitrary sequences of elements can be pushed to the front and back
        // of a deque using 'ggg' at any point, but the fill-to-boundary
        // functionality is supported only for 'deque's in their initial
        // (default constructed) state, which is empty and a capacity
        // indicating only one page has been allocated.  Any attempt to to
        // interpret a 'spec' string containing a fill-to-end range will return
        // an error unless '*object' is in the default-constructed state.  Note
        // that the insert-point for a push in this state must also correspond
        // to the middle of the page, per a default-constructed deque.
        //
        // Examples:
        // ---------
        // "A"      // push_back a single element of configuration 'A'.
        // "a"      // push_front a single element of configuration 'A'.
        // "7A 3b"  // push_back 7 'A' objects, then push_front 3 'B' objects.
        // "A<B|"   // push_back an 'A', then fill-to-back with 'B'.
        // "A<b.|"  // push_back an 'A', then fill-to-front with 'B', leaving
        //          // space at the front for exactly one more object.
        //
        // "<A...|BCC <a...|bcc CCC7D ccc7d"
        //   Fill to the back with 'A' objects, leaving space for 3 more before
        //   an allocation would be required, then push_back a 'B' object
        //   followed by two 'C' objects.  Then, fill to the front with 'A'
        //   objects, leaving space for 3 more before an allocation would be
        //   required, then push_front a 'B' object followed by two 'C'
        //   objects, exactly filling the first page.  Then push_back a 'C'
        //   object (causing an allocation) followed by another 2 C objects,
        //   and then seven 'D' objects.  Then push_front another 'C' object,
        //   followed by two more 'C' objects and then another seven 'D'
        //   objects.
        //
        // "<A| A<A| A<A| 3A"
        //   Fill the current empty deque towards the back with 'A' objects,
        //   filling from the middle of the initial page, creating and filling
        //   two more complete pages, and finally pushed 3 'A' objects to the
        //   start of the fourth page.
        //
        // "<A| A<A| 3A ~ <A| A"
        //   Populate a default-constructed deque to the back of the first page
        //   then fill a newly allocated second page with 'A' objects, and then
        //   insert three more 'A' objects at the start of a third page.  Then
        //   clear the deque, and fill to the back of the current page.
        //   Finally push_back one more 'A' object, which should re-use one of
        //   the existing pages rather than allocate a new page.

    static Obj& gg(Obj *object, const char *spec);
        // Return, by reference, the specified 'object' with its value adjusted
        // according to the specified 'spec'.

    static bsl::vector<TYPE> gV(const char *spec);
        // Return, by value, a new vector corresponding to the specified
        // 'spec'.

    static void storeFirstNElemAddr(typename Obj::const_pointer *pointers,
                                    const Obj&                   object,
                                    size_t                       n)
    {
        size_t i = 0;
        for (CIter b = object.cbegin(); b != object.cend() && i < n; ++b) {
            pointers[i++] = bsls::Util::addressOf(*b);
        }
    }

    static
    int checkFirstNElemAddr(typename Obj::const_pointer *pointers,
                            const Obj&                   object,
                            size_t                       n)
    {
        int    count = 0;
        size_t i     = 0;
        for (CIter b = object.cbegin(); b != object.end() && i < n; ++b) {
            if (pointers[i++] != bsls::Util::addressOf(*b)) {
                ++count;
            }
        }
        return count;
    }

    static void resetMovedInto(Obj *object)
    {
        for (size_t i = 0; i < object->size(); ++i) {
            TstFacility::setMovedIntoState(BSLS_UTIL_ADDRESSOF((*object)[i]),
                                           bsltf::MoveState::e_NOT_MOVED);
        }
    }

    static void setInternalState(Obj *object, int n);
        // Using 'push_back', 'push_front', 'pop_back', and 'pop_front', set
        // the internal state of the specified 'object' by moving the start and
        // finish iterators by the specified 'n' positions.

    static void stretch(Obj         *object,
                        std::size_t  size,
                        int          identifier = int('Z'));
        // Using only primary manipulators, extend the length of the specified
        // 'object' by the specified 'size' by adding 'size' copies of the
        // value indicated by the optionally specified 'identifier'.  If
        // 'identifier' is not specified, 'Z' is used.  The resulting value is
        // not specified.

    static void stretchRemoveAll(Obj         *object,
                                 std::size_t  size,
                                 int          identifier = int('Z'));
        // Using only primary manipulators, extend the capacity of the
        // specified 'object' by the specified 'size' by adding 'size' copies
        // of the value indicated by the optionally specified 'identifier'.  If
        // 'identifier' is not specified, 'Z' is used.  The resulting value is
        // not specified.

    static int findRoomierEnd(Obj *container);
        // Determine which end of the specified '*container' has more room.
        // Return a value > 0 if it's the front, < 0 if it's the back, and
        // 0 if the two ends have the same amount of room.  Note that this
        // function modifies 'container' but restores it to its original state.

    template <class T>
    static bslmf::MovableRef<T> testArg(T& t, bsl::true_type)
    {
        return MoveUtil::move(t);
    }
    template <class T>
    static const T&             testArg(T& t, bsl::false_type)
    {
        return t;
    }

    template <int N_ARGS,
              int N01,
              int N02,
              int N03,
              int N04,
              int N05,
              int N06,
              int N07,
              int N08,
              int N09,
              int N10>
    static void testCase28a_RunTest(Obj *target);
        // Call 'emplace_back' on the specified 'target' container.  Forward
        // (template parameter) 'N_ARGS' arguments to the 'emplace_back' method
        // and ensure 1) that values are properly passed to the constructor of
        // 'value_type', 2) that the allocator is correctly configured for each
        // argument in the newly inserted element in 'target', and 3) that the
        // arguments are forwarded using copy or move semantics based on
        // integer template parameters '[N01 .. N10]'.

    template <int N_ARGS,
              int N01,
              int N02,
              int N03,
              int N04,
              int N05,
              int N06,
              int N07,
              int N08,
              int N09,
              int N10>
    static void testCase28b_RunTest(Obj *target);
        // Call 'emplace_front' on the specified 'target' container.  Forward
        // (template parameter) 'N_ARGS' arguments to the 'emplace_front'
        // method and ensure 1) that values are properly passed to the
        // constructor of 'value_type', 2) that the allocator is correctly
        // configured for each argument in the newly inserted element in
        // 'target', and 3) that the arguments are forwarded using copy or move
        // semantics based on integer template parameters '[N01 .. N10]'.

    template <int N_ARGS,
              int N01,
              int N02,
              int N03,
              int N04,
              int N05,
              int N06,
              int N07,
              int N08,
              int N09,
              int N10>
    static void testCase29a_RunTest(Obj *target, const_iterator position);
        // Call 'emplace' on the specified 'target' container at the specified
        // 'position'.  Forward (template parameter) 'N_ARGS' arguments to the
        // 'emplace' method and ensure 1) that values are properly passed to
        // the constructor of 'value_type', 2) that the allocator is correctly
        // configured for each argument in the newly inserted element in
        // 'target', and 3) that the arguments are forwarded using copy or move
        // semantics based on integer template parameters '[N01 .. N10]'.

    // TEST CASES

    static void testCase32();
        // Test 'noexcept' specifications

    template <class CONTAINER>
    static void testCaseM1Range(const CONTAINER&);
        // Performance test for methods that take a range of inputs.

    static void testCaseM1();
        // Performance test.

    static void testCase31();
        // Test 'shrink_to_fit'.

    static void testCase30();
        // Test methods that take an initializer list.

    static void testCase29a();
        // Test forwarding of arguments in 'emplace' method.

    static void testCase29();
        // Test 'emplace' other than forwarding of arguments (see 29a).

    static void testCase28a();
        // Test forwarding of arguments in 'emplace_back' method.

    static void testCase28b();
        // Test forwarding of arguments in 'emplace_front' method.

    static void testCase28();
        // Test 'emplace_front' and 'emplace_back' other than forwarding of
        // arguments (see 28a and 28b).

    static void testCase27();
        // Test 'insert' method that takes a movable ref.

    static void testCase26();
        // Test 'push_front' and 'push_back' methods that take a movable ref.

    static void testCase25_dispatch();
        // Test move-assignment operator.

    static void testCase24();
        // Test move constructor.

    static void testCase23();
        // Test proper use of 'std::length_error'.

    static void testCase22();
        // Test free comparison operators.

    template <bool PROPAGATE_ON_CONTAINER_SWAP_FLAG,
              bool OTHER_FLAGS>
    static void testCase21_propagate_on_container_swap_dispatch();
    static void testCase21_propagate_on_container_swap();
        // Test 'propagate_on_container_swap'.

    static void testCase21_dispatch();
        // Test 'swap' member.

    static void testCase20();
        // Test 'erase', 'pop_back', and 'pop_front'.

    template <class CONTAINER>
    static void testCase19(const CONTAINER&);
        // Test range 'insert' member.

    static void testCase18();
        // Test value 'insert' members.

    static void testCase17();
        // Test 'push_back' and 'push_front' members.

    static void testCase16();
        // Test iterators.

    static void testCase15();
        // Test element access.

    static void testCase14();
        // Test reserve and capacity-related methods.

    static void testCase13();
        // Test 'assign' members.

    template <class CONTAINER>
    static void testCase13Range(const CONTAINER&);
        // Test 'assign' member template.

    static void testCase12();
        // Test value constructors.

    template <class CONTAINER>
    static void testCase12Range(const CONTAINER&);
        // Test range constructor.
};

template <class TYPE>
class StdBslmaTestDriver : public TestDriver<
                            TYPE,
                            bsltf::StdAllocatorAdaptor<bsl::allocator<TYPE> > >
{
};

                               // --------------
                               // TEST APPARATUS
                               // --------------

template <class TYPE, class ALLOC>
int TestDriver<TYPE,ALLOC>::getValues(const TYPE **valuesPtr)
{
    bslma::DefaultAllocatorGuard
                                guard(&bslma::NewDeleteAllocator::singleton());

    static TYPE values[5]; // avoid DEFAULT_VALUE and UNINITIALIZED_VALUE
    values[0] = TYPE(VA);
    values[1] = TYPE(VB);
    values[2] = TYPE(VC);
    values[3] = TYPE(VD);
    values[4] = TYPE(VE);

    const int NUM_VALUES = 5;

    *valuesPtr = values;
    return NUM_VALUES;
}

template <class TYPE, class ALLOC>
int TestDriver<TYPE,ALLOC>::ggg(Obj        *object,
                                const char *spec,
                                bool        verboseFlag)
{
    BSLS_ASSERT_OPT(object);
    BSLS_ASSERT_OPT(spec);

    // In order to execute the 'spec' program, we are going to need to track a
    // little state.  Based on the known size of a page (counted in objects) we
    // can separately track the available capacity to the front and back of the
    // currently inserting blocks.  The numbers must be tracked through every
    // insertion operation.  Several characters when parsed indicate the start
    // of a command sequence, which should be parsed in full before evaluating
    // as a single command:
    //..
    //  '<'   : Record a command sequence until a '|' character
    //  digit : Repeat the next (simple) command digit times.
    //..
    // Note that a 'digit' can occur in a insert-to-boundary sequence.

    enum { SUCCESS = -1 };

    int  frontCapacity = (PageLength<TYPE>::k_VALUE - 1) / 2;
    int  backCapacity  = PageLength<TYPE>::k_VALUE - 1 - frontCapacity;
    int  repeatCount   = 1;
    int  tailPadding   = 0;
    bool applyToBack   = true;  // 'true' > 'push back', 'false' > 'push_front'
    int  fillPage      = -1;    // index of 'start range' command

    int VALUE = 0;  // 0 when in "immediate mode"; otherwise, the identifier of
                    // the object to fill-to-page-boundary with.

    // The parser below would be much simpler with C++11 lambda expressions.

    for (int i = 0; spec[i]; ++i) {
        if (' ' == spec[i]) {
            continue;   // whitespace is not salient in any parse mode
        }

        if (-1 == fillPage) {  // immediate mode, not forming a fill-page range
            if ('A' <= spec[i] && spec[i] <= 'Y') {
                for (int j = 0; j != repeatCount; ++j) {
                    primaryManipulatorBack(object, spec[i]);
                    if (backCapacity) {
                        --backCapacity;
                    }
                    else {
                        backCapacity = PageLength<TTA>::k_VALUE - 1;
                    }
                }
            }
            else if ('a' <= spec[i] && spec[i] <= 'y') {
                for (int j = 0; j != repeatCount; ++j) {
                    primaryManipulatorFront(object, toupper(spec[i]));
                    if (frontCapacity) {
                        --frontCapacity;
                    }
                    else {
                        frontCapacity = PageLength<TTA>::k_VALUE - 1;
                    }
                }
            }
            else if (1 != repeatCount) {
                if (verboseFlag) {
                    printf("Error, bad character ('%c') in spec \"%s\" at"
                           " position %d: cannot follow a repeat-count.\n",
                           spec[i], spec, i);
                }
                return i;  // Discontinue processing this spec.       // RETURN
            }
            else if ('~' == spec[i]) {
                object->clear();
                backCapacity  = PageLength<TTA>::k_VALUE / 2;
                frontCapacity = (PageLength<TYPE>::k_VALUE + 1) / 2 -1;;
            }
            else if ('<' == spec[i]) {
                fillPage = i;    // Enter fill-page mode
                // Note we cannot diagnose an at-boundary error until we know
                // whether we push at the front or the back, so wait until the
                // range direction is specified before (potentially) reporting
                // an error.
            }
            else if ('2' <= spec[i] && spec[i] <= '9') {
                repeatCount = spec[i] - '0';
            }
            else if ('0' == spec[i]) {
                repeatCount = 10;
            }
            // Any other character is invalid, although we can give stronger
            // diagnostics in some cases.
            else if ('|' == spec[i]) {
                if (verboseFlag) {
                    printf("Error, bad character ('%c') in spec \"%s\" at"
                           " position %d: terminator for an unopened range.\n",
                           spec[i], spec, i);
                }
                return i;  // Discontinue processing this spec.       // RETURN
            }
            else if ('.' == spec[i]) {
                if (verboseFlag) {
                    printf("Error, bad character ('%c') in spec \"%s\" at"
                           " position %d: padding allowed only in a range.\n",
                           spec[i], spec, i);
                }
                return i;  // Discontinue processing this spec.       // RETURN
            }
            else {
                if (verboseFlag) {
                    printf("Error, bad character ('%c') in spec \"%s\" at"
                           " position %d.\n", spec[i], spec, i);
                }
                return i;  // Discontinue processing this spec.       // RETURN
            }
        }
        else {  // forming a fill-page range
            if ('|' == spec[i]) { // execute range-fill
                if (0 == VALUE) {
                    if (verboseFlag) {
                        printf("Error, invalid fill-range in spec \"%s\""
                               " starting at position %d: no fill char.\n",
                               spec, fillPage);
                    }
                    return i;  // Discontinue processing this spec.   // RETURN
                }

                int *capacity = applyToBack ? &backCapacity : &frontCapacity;
                if (*capacity <= tailPadding) {
                    if (verboseFlag) {
                        printf("Error, invalid fill-range in spec \"%s\""
                               " starting at position %d: no room for padding."
                               "\n", spec, fillPage);
                    }
                    return i;  // Discontinue processing this spec.   // RETURN
                }

//T_; P(*capacity);
                while (*capacity != tailPadding) {
                    if (applyToBack) {
                        primaryManipulatorBack(object, VALUE);
                    }
                    else {
                        primaryManipulatorFront(object, VALUE);
                    }
                    --*capacity;
                }

                // Reset state after fill
                fillPage    = -1;
                repeatCount = 1;
                tailPadding = 0;
                VALUE       = 0;
            }
            else if ('A' <= spec[i] && spec[i] <= 'E') {
                if (0 != VALUE) {
                    if (verboseFlag) {
                        printf("Error, bad character ('%c') in spec \"%s\" at"
                               " position %d: only one fill char allowed.\n",
                               spec[i], spec, i);
                    }
                    return i;  // Discontinue processing this spec.   // RETURN
                }

                applyToBack = true;
                VALUE = spec[i];
            }
            else if ('a' <= spec[i] && spec[i] <= 'e') {
                if (0 != VALUE) {
                    if (verboseFlag) {
                        printf("Error, bad character ('%c') in spec \"%s\" at"
                               " position %d: only one fill char allowed.\n",
                               spec[i], spec, i);
                    }
                    return i;  // Discontinue processing this spec.   // RETURN
                }

                applyToBack = false;
                VALUE = toupper(spec[i]);
            }
            else if ('.' == spec[i]) { // tail padding
                if (0 == VALUE) {
                    if (verboseFlag) {
                        printf("Error, bad character ('%c') in spec \"%s\" at"
                               " position %d: padding preceding fill char.\n",
                               spec[i], spec, i);
                    }
                    return i;  // Discontinue processing this spec.   // RETURN
                }
                tailPadding += repeatCount;
            }
// TBD
#if 0  // Future extension      - allow a repeater for tail-padding.
       // Implementation detail - introduces another bool state to track.
       //     If we do not track the state-flag, then a simple trailing digit
       //     would indicate padding, without an optional '.' suffix.  Not sure
       //     if we should allow this, or pick it up as an error, but currently
       //     no expected use cases - will review again after updating the
       //     'insert' test case.
            else if ('2' <= spec[i] && spec[i] <= '9') {
                if (0 != tailPadding) {
                    if (verboseFlag) {
                        printf("Error, bad character ('%c') in spec \"%s\" at"
                               " position %d: digit cannot follow padding.\n",
                               spec[i], spec, i);
                    }
                    return i;  // Discontinue processing this spec.   // RETURN
                }
                if (0 == VALUE) {
                    if (verboseFlag) {
                        printf("Error, bad character ('%c') in spec \"%s\" at"
                               " position %d: no repeated objects in range.\n",
                               spec[i], spec, i);
                    }
                    return i;  // Discontinue processing this spec.   // RETURN
                }
                repeatCount = spec[i] - '0';
            }
            else if ('0' == spec[i]) {
                if (0 != tailPadding) {
                    if (verboseFlag) {
                        printf("Error, bad character ('%c') in spec \"%s\" at"
                               " position %d: digit cannot follow padding.\n",
                               spec[i], spec, i);
                    }
                    return i;  // Discontinue processing this spec.   // RETURN
                }
                if (0 == VALUE) {
                    if (verboseFlag) {
                        printf("Error, bad character ('%c') in spec \"%s\" at"
                               " position %d: no repeated objects in range.\n",
                               spec[i], spec, i);
                    }
                    return i;  // Discontinue processing this spec.   // RETURN
                }
                repeatCount = 10;
            }
#endif
            // We could provide nicer errors for '1', '<', and '~'
            else {
                if (verboseFlag) {
                    printf("Error, bad character ('%c') in spec \"%s\" at"
                           " position %d.\n", spec[i], spec, i);
                }
                return i;  // Discontinue processing this spec.       // RETURN
            }
        }
    }

    if (-1 != fillPage) {  // We have a range command that is not terminated.
        if (verboseFlag) {
            printf("Error, bad character ('<') in spec \"%s\" at"
                   " position %d: nonterminated fill-range.\n",
                   spec, fillPage);
        }
        return fillPage;  // Discontinue processing this spec.        // RETURN
    }

    resetMovedInto(object);

    return SUCCESS;
}

template <class TYPE, class ALLOC>
bsl::deque<TYPE,ALLOC>& TestDriver<TYPE,ALLOC>::gg(Obj        *object,
                                                   const char *spec)
{
    ASSERT(ggg(object, spec) < 0);
    return *object;
}

template <class TYPE, class ALLOC>
bsl::vector<TYPE> TestDriver<TYPE,ALLOC>::gV(const char *spec)
{
    const TYPE *VALUES;
    getValues(&VALUES);
    bsl::vector<TYPE> result;
    for (int i = 0; spec[i]; ++i) {
        if ('A' <= spec[i] && spec[i] <= 'E') {
            result.push_back(VALUES[spec[i] - 'A']);
        }
        else if ('<' == spec[i]) {
            result.pop_back();
        }
        else if ('~' == spec[i]) {
            result.clear();
        }
        else {
            ASSERT(0);
        }
   }
   return result;
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::setInternalState(Obj *object, int n)
{
    ASSERT(object);
    ASSERT(0 == object->size());
    const TYPE value = TYPE();
    if (n > 0) {
        for (; n > 0; --n) {
            object->push_back(value);
            object->pop_front();
        }
    }
    else {
        for (; n < 0; ++n) {
            object->push_front(value);
            object->pop_back();
        }
    }
    ASSERT(0 == object->size());
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::stretch(Obj         *object,
                                     std::size_t  size,
                                     int          identifier)
{
    ASSERT(object);

    const std::size_t initialSize = object->size();

    bslma::TestAllocator scratch;

    for (std::size_t i = 0; i < size; ++i) {
// TBD perhaps add an argument to stretch: front only, back only, both ends
//        if (0 == i % 2) {
            primaryManipulatorBack(object, identifier);
//        }
//        else {
//            primaryManipulatorFront(object, identifier, &scratch);
//        }
    }
    ASSERT(object->size() == initialSize + size);
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::stretchRemoveAll(Obj         *object,
                                              std::size_t  size,
                                              int          identifier)
{
    ASSERT(object);
    stretch(object, size, identifier);
    object->clear();
    ASSERT(0 == object->size());
}

template <class TYPE, class ALLOC>
int TestDriver<TYPE, ALLOC>::findRoomierEnd(Obj *container)
{
    typedef typename Obj::size_type size_type;

    const size_type cap = container->capacity();

    if (container->empty()) {
        // The container will always have room for at least one element without
        // doing any block allocations.

        container->push_back(TYPE(VA));
        const bool backGrew = container->capacity() > cap;
        container->pop_back();

        container->push_front(TYPE(VA));
        const bool frontGrew = container->capacity() > cap;
        container->pop_front();

        ASSERT(backGrew || frontGrew);

        if (backGrew) {
            if (frontGrew) {
                return 0;                                             // RETURN
            }
            return -1;                                                // RETURN
        }
        return 1;                                                     // RETURN
    }

    TYPE val = container->back();
    container->pop_back();
    const bool backShrunk = container->capacity() < cap;
    container->push_back(val);

    val = container->front();
    container->pop_front();
    const bool frontShrunk = container->capacity() < cap;
    container->push_front(val);

    ASSERT(backShrunk || frontShrunk);

    if (backShrunk) {
        if (frontShrunk) {
            return 0;                                                 // RETURN
        }
        return -1;                                                    // RETURN
    }
    return 1;
}

template <class TYPE, class ALLOC>
template <int N_ARGS,
          int N01,
          int N02,
          int N03,
          int N04,
          int N05,
          int N06,
          int N07,
          int N08,
          int N09,
          int N10>
void
TestDriver<TYPE, ALLOC>::testCase28a_RunTest(Obj *target)
{
    DECLARE_BOOL_CONSTANT(MOVE_01, N01 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_02, N02 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_03, N03 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_04, N04 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_05, N05 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_06, N06 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_07, N07 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_08, N08 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_09, N09 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_10, N10 == 1);

    bslma::TestAllocator *testAlloc = dynamic_cast<bslma::TestAllocator *>(
                                          target->get_allocator().mechanism());
    if (!testAlloc) {
        ASSERT(!"Allocator in test case 28 is not a test allocator!");
        return;                                                       // RETURN
    }
    bslma::TestAllocator& oa = *testAlloc;
    Obj& mX = *target;  const Obj& X = mX;

    bslma::TestAllocator aa("args", veryVeryVeryVerbose);
    bslma::TestAllocator scratch("scratch", veryVeryVeryVerbose);

    size_t len = X.size();

    BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {

        // Construct all arguments inside the exception test loop as the
        // exception thrown after moving only a portion of arguments leave the
        // moved arguments in a valid, but unspecified state.
        bsls::ObjectBuffer<typename TYPE::ArgType01> BUF01;
        ConstrUtil::construct(BUF01.address(), &aa, V01);
        typename TYPE::ArgType01& A01 = BUF01.object();
        bslma::DestructorGuard<typename TYPE::ArgType01> G01(&A01);

        bsls::ObjectBuffer<typename TYPE::ArgType02> BUF02;
        ConstrUtil::construct(BUF02.address(), &aa, V02);
        typename TYPE::ArgType02& A02 = BUF02.object();
        bslma::DestructorGuard<typename TYPE::ArgType02> G02(&A02);

        bsls::ObjectBuffer<typename TYPE::ArgType03> BUF03;
        ConstrUtil::construct(BUF03.address(), &aa, V03);
        typename TYPE::ArgType03& A03 = BUF03.object();
        bslma::DestructorGuard<typename TYPE::ArgType03> G03(&A03);

        bsls::ObjectBuffer<typename TYPE::ArgType04> BUF04;
        ConstrUtil::construct(BUF04.address(), &aa, V04);
        typename TYPE::ArgType04& A04 = BUF04.object();
        bslma::DestructorGuard<typename TYPE::ArgType04> G04(&A04);

        bsls::ObjectBuffer<typename TYPE::ArgType05> BUF05;
        ConstrUtil::construct(BUF05.address(), &aa, V05);
        typename TYPE::ArgType05& A05 = BUF05.object();
        bslma::DestructorGuard<typename TYPE::ArgType05> G05(&A05);

        bsls::ObjectBuffer<typename TYPE::ArgType06> BUF06;
        ConstrUtil::construct(BUF06.address(), &aa, V06);
        typename TYPE::ArgType06& A06 = BUF06.object();
        bslma::DestructorGuard<typename TYPE::ArgType06> G06(&A06);

        bsls::ObjectBuffer<typename TYPE::ArgType07> BUF07;
        ConstrUtil::construct(BUF07.address(), &aa, V07);
        typename TYPE::ArgType07& A07 = BUF07.object();
        bslma::DestructorGuard<typename TYPE::ArgType07> G07(&A07);

        bsls::ObjectBuffer<typename TYPE::ArgType08> BUF08;
        ConstrUtil::construct(BUF08.address(), &aa,  V08);
        typename TYPE::ArgType08& A08 = BUF08.object();
        bslma::DestructorGuard<typename TYPE::ArgType08> G08(&A08);

        bsls::ObjectBuffer<typename TYPE::ArgType09> BUF09;
        ConstrUtil::construct(BUF09.address(), &aa, V09);
        typename TYPE::ArgType09& A09 = BUF09.object();
        bslma::DestructorGuard<typename TYPE::ArgType09> G09(&A09);

        bsls::ObjectBuffer<typename TYPE::ArgType10> BUF10;
        ConstrUtil::construct(BUF10.address(), &aa, V10);
        typename TYPE::ArgType10& A10 = BUF10.object();
        bslma::DestructorGuard<typename TYPE::ArgType10> G10(&A10);

        ExceptionProctor<Obj, ALLOC> proctor(&X, L_, &scratch);

        switch (N_ARGS) {
          case 0: {
            mX.emplace_back();
          } break;
          case 1: {
            mX.emplace_back(testArg(A01, MOVE_01));
          } break;
          case 2: {
            mX.emplace_back(testArg(A01, MOVE_01), testArg(A02, MOVE_02));
          } break;
          case 3: {
            mX.emplace_back(testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03));
          } break;
          case 4: {
            mX.emplace_back(testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03),
                            testArg(A04, MOVE_04));
          } break;
          case 5: {
            mX.emplace_back(testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03),
                            testArg(A04, MOVE_04),
                            testArg(A05, MOVE_05));
          } break;
          case 6: {
            mX.emplace_back(testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03),
                            testArg(A04, MOVE_04),
                            testArg(A05, MOVE_05),
                            testArg(A06, MOVE_06));
          } break;
          case 7: {
            mX.emplace_back(testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03),
                            testArg(A04, MOVE_04),
                            testArg(A05, MOVE_05),
                            testArg(A06, MOVE_06),
                            testArg(A07, MOVE_07));
          } break;
          case 8: {
            mX.emplace_back(testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03),
                            testArg(A04, MOVE_04),
                            testArg(A05, MOVE_05),
                            testArg(A06, MOVE_06),
                            testArg(A07, MOVE_07),
                            testArg(A08, MOVE_08));
          } break;
          case 9: {
            mX.emplace_back(testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03),
                            testArg(A04, MOVE_04),
                            testArg(A05, MOVE_05),
                            testArg(A06, MOVE_06),
                            testArg(A07, MOVE_07),
                            testArg(A08, MOVE_08),
                            testArg(A09, MOVE_09));
          } break;
          case 10: {
            mX.emplace_back(testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03),
                            testArg(A04, MOVE_04),
                            testArg(A05, MOVE_05),
                            testArg(A06, MOVE_06),
                            testArg(A07, MOVE_07),
                            testArg(A08, MOVE_08),
                            testArg(A09, MOVE_09),
                            testArg(A10, MOVE_10));
          } break;
          default: {
            ASSERTV(!"Invalid # of args!");
          } break;
        }
        proctor.release();

        ASSERTV(len + 1, X.size(), len + 1 == X.size());

        ASSERTV(MOVE_01, A01.movedFrom(),
               MOVE_01 == (MoveState::e_MOVED == A01.movedFrom()) || 2 == N01);
        ASSERTV(MOVE_02, A02.movedFrom(),
               MOVE_02 == (MoveState::e_MOVED == A02.movedFrom()) || 2 == N02);
        ASSERTV(MOVE_03, A03.movedFrom(),
               MOVE_03 == (MoveState::e_MOVED == A03.movedFrom()) || 2 == N03);
        ASSERTV(MOVE_04, A04.movedFrom(),
               MOVE_04 == (MoveState::e_MOVED == A04.movedFrom()) || 2 == N04);
        ASSERTV(MOVE_05, A05.movedFrom(),
               MOVE_05 == (MoveState::e_MOVED == A05.movedFrom()) || 2 == N05);
        ASSERTV(MOVE_06, A06.movedFrom(),
               MOVE_06 == (MoveState::e_MOVED == A06.movedFrom()) || 2 == N06);
        ASSERTV(MOVE_07, A07.movedFrom(),
               MOVE_07 == (MoveState::e_MOVED == A07.movedFrom()) || 2 == N07);
        ASSERTV(MOVE_08, A08.movedFrom(),
               MOVE_08 == (MoveState::e_MOVED == A08.movedFrom()) || 2 == N08);
        ASSERTV(MOVE_09, A09.movedFrom(),
               MOVE_09 == (MoveState::e_MOVED == A09.movedFrom()) || 2 == N09);
        ASSERTV(MOVE_10, A10.movedFrom(),
               MOVE_10 == (MoveState::e_MOVED == A10.movedFrom()) || 2 == N10);

        const TYPE& V = X[len];

        ASSERTV(V01, V.arg01(), V01 == V.arg01() || 2 == N01);
        ASSERTV(V02, V.arg02(), V02 == V.arg02() || 2 == N02);
        ASSERTV(V03, V.arg03(), V03 == V.arg03() || 2 == N03);
        ASSERTV(V04, V.arg04(), V04 == V.arg04() || 2 == N04);
        ASSERTV(V05, V.arg05(), V05 == V.arg05() || 2 == N05);
        ASSERTV(V06, V.arg06(), V06 == V.arg06() || 2 == N06);
        ASSERTV(V07, V.arg07(), V07 == V.arg07() || 2 == N07);
        ASSERTV(V08, V.arg08(), V08 == V.arg08() || 2 == N08);
        ASSERTV(V09, V.arg09(), V09 == V.arg09() || 2 == N09);
        ASSERTV(V10, V.arg10(), V10 == V.arg10() || 2 == N10);

        TestAllocatorUtil::test(V, oa);
    } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
}

template <class TYPE, class ALLOC>
template <int N_ARGS,
          int N01,
          int N02,
          int N03,
          int N04,
          int N05,
          int N06,
          int N07,
          int N08,
          int N09,
          int N10>
void
TestDriver<TYPE, ALLOC>::testCase28b_RunTest(Obj *target)
{
    DECLARE_BOOL_CONSTANT(MOVE_01, N01 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_02, N02 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_03, N03 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_04, N04 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_05, N05 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_06, N06 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_07, N07 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_08, N08 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_09, N09 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_10, N10 == 1);

    bslma::TestAllocator *testAlloc = dynamic_cast<bslma::TestAllocator *>(
                                          target->get_allocator().mechanism());
    if (!testAlloc) {
        ASSERT(!"Allocator in test case 28 is not a test allocator!");
        return;                                                       // RETURN
    }
    bslma::TestAllocator& oa = *testAlloc;
    Obj& mX = *target;  const Obj& X = mX;

    bslma::TestAllocator aa("args", veryVeryVeryVerbose);
    bslma::TestAllocator scratch("scratch", veryVeryVeryVerbose);

    size_t len = X.size();

    BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {

        // Construct all arguments inside the exception test loop as the
        // exception thrown after moving only a portion of arguments leave the
        // moved arguments in a valid, but unspecified state.
        bsls::ObjectBuffer<typename TYPE::ArgType01> BUF01;
        ConstrUtil::construct(BUF01.address(), &aa, V01);
        typename TYPE::ArgType01& A01 = BUF01.object();
        bslma::DestructorGuard<typename TYPE::ArgType01> G01(&A01);

        bsls::ObjectBuffer<typename TYPE::ArgType02> BUF02;
        ConstrUtil::construct(BUF02.address(), &aa, V02);
        typename TYPE::ArgType02& A02 = BUF02.object();
        bslma::DestructorGuard<typename TYPE::ArgType02> G02(&A02);

        bsls::ObjectBuffer<typename TYPE::ArgType03> BUF03;
        ConstrUtil::construct(BUF03.address(), &aa, V03);
        typename TYPE::ArgType03& A03 = BUF03.object();
        bslma::DestructorGuard<typename TYPE::ArgType03> G03(&A03);

        bsls::ObjectBuffer<typename TYPE::ArgType04> BUF04;
        ConstrUtil::construct(BUF04.address(), &aa, V04);
        typename TYPE::ArgType04& A04 = BUF04.object();
        bslma::DestructorGuard<typename TYPE::ArgType04> G04(&A04);

        bsls::ObjectBuffer<typename TYPE::ArgType05> BUF05;
        ConstrUtil::construct(BUF05.address(), &aa, V05);
        typename TYPE::ArgType05& A05 = BUF05.object();
        bslma::DestructorGuard<typename TYPE::ArgType05> G05(&A05);

        bsls::ObjectBuffer<typename TYPE::ArgType06> BUF06;
        ConstrUtil::construct(BUF06.address(), &aa, V06);
        typename TYPE::ArgType06& A06 = BUF06.object();
        bslma::DestructorGuard<typename TYPE::ArgType06> G06(&A06);

        bsls::ObjectBuffer<typename TYPE::ArgType07> BUF07;
        ConstrUtil::construct(BUF07.address(), &aa, V07);
        typename TYPE::ArgType07& A07 = BUF07.object();
        bslma::DestructorGuard<typename TYPE::ArgType07> G07(&A07);

        bsls::ObjectBuffer<typename TYPE::ArgType08> BUF08;
        ConstrUtil::construct(BUF08.address(), &aa,  V08);
        typename TYPE::ArgType08& A08 = BUF08.object();
        bslma::DestructorGuard<typename TYPE::ArgType08> G08(&A08);

        bsls::ObjectBuffer<typename TYPE::ArgType09> BUF09;
        ConstrUtil::construct(BUF09.address(), &aa, V09);
        typename TYPE::ArgType09& A09 = BUF09.object();
        bslma::DestructorGuard<typename TYPE::ArgType09> G09(&A09);

        bsls::ObjectBuffer<typename TYPE::ArgType10> BUF10;
        ConstrUtil::construct(BUF10.address(), &aa, V10);
        typename TYPE::ArgType10& A10 = BUF10.object();
        bslma::DestructorGuard<typename TYPE::ArgType10> G10(&A10);

        ExceptionProctor<Obj, ALLOC> proctor(&X, L_, &scratch);

        switch (N_ARGS) {
          case 0: {
            mX.emplace_front();
          } break;
          case 1: {
            mX.emplace_front(testArg(A01, MOVE_01));
          } break;
          case 2: {
            mX.emplace_front(testArg(A01, MOVE_01), testArg(A02, MOVE_02));
          } break;
          case 3: {
            mX.emplace_front(testArg(A01, MOVE_01),
                             testArg(A02, MOVE_02),
                             testArg(A03, MOVE_03));
          } break;
          case 4: {
            mX.emplace_front(testArg(A01, MOVE_01),
                             testArg(A02, MOVE_02),
                             testArg(A03, MOVE_03),
                             testArg(A04, MOVE_04));
          } break;
          case 5: {
            mX.emplace_front(testArg(A01, MOVE_01),
                             testArg(A02, MOVE_02),
                             testArg(A03, MOVE_03),
                             testArg(A04, MOVE_04),
                             testArg(A05, MOVE_05));
          } break;
          case 6: {
            mX.emplace_front(testArg(A01, MOVE_01),
                             testArg(A02, MOVE_02),
                             testArg(A03, MOVE_03),
                             testArg(A04, MOVE_04),
                             testArg(A05, MOVE_05),
                             testArg(A06, MOVE_06));
          } break;
          case 7: {
            mX.emplace_front(testArg(A01, MOVE_01),
                             testArg(A02, MOVE_02),
                             testArg(A03, MOVE_03),
                             testArg(A04, MOVE_04),
                             testArg(A05, MOVE_05),
                             testArg(A06, MOVE_06),
                             testArg(A07, MOVE_07));
          } break;
          case 8: {
            mX.emplace_front(testArg(A01, MOVE_01),
                             testArg(A02, MOVE_02),
                             testArg(A03, MOVE_03),
                             testArg(A04, MOVE_04),
                             testArg(A05, MOVE_05),
                             testArg(A06, MOVE_06),
                             testArg(A07, MOVE_07),
                             testArg(A08, MOVE_08));
          } break;
          case 9: {
            mX.emplace_front(testArg(A01, MOVE_01),
                             testArg(A02, MOVE_02),
                             testArg(A03, MOVE_03),
                             testArg(A04, MOVE_04),
                             testArg(A05, MOVE_05),
                             testArg(A06, MOVE_06),
                             testArg(A07, MOVE_07),
                             testArg(A08, MOVE_08),
                             testArg(A09, MOVE_09));
          } break;
          case 10: {
            mX.emplace_front(testArg(A01, MOVE_01),
                             testArg(A02, MOVE_02),
                             testArg(A03, MOVE_03),
                             testArg(A04, MOVE_04),
                             testArg(A05, MOVE_05),
                             testArg(A06, MOVE_06),
                             testArg(A07, MOVE_07),
                             testArg(A08, MOVE_08),
                             testArg(A09, MOVE_09),
                             testArg(A10, MOVE_10));
          } break;
          default: {
            ASSERTV(!"Invalid # of args!");
          } break;
        }
        proctor.release();

        ASSERTV(len + 1, X.size(), len + 1 == X.size());

        ASSERTV(MOVE_01, A01.movedFrom(),
               MOVE_01 == (MoveState::e_MOVED == A01.movedFrom()) || 2 == N01);
        ASSERTV(MOVE_02, A02.movedFrom(),
               MOVE_02 == (MoveState::e_MOVED == A02.movedFrom()) || 2 == N02);
        ASSERTV(MOVE_03, A03.movedFrom(),
               MOVE_03 == (MoveState::e_MOVED == A03.movedFrom()) || 2 == N03);
        ASSERTV(MOVE_04, A04.movedFrom(),
               MOVE_04 == (MoveState::e_MOVED == A04.movedFrom()) || 2 == N04);
        ASSERTV(MOVE_05, A05.movedFrom(),
               MOVE_05 == (MoveState::e_MOVED == A05.movedFrom()) || 2 == N05);
        ASSERTV(MOVE_06, A06.movedFrom(),
               MOVE_06 == (MoveState::e_MOVED == A06.movedFrom()) || 2 == N06);
        ASSERTV(MOVE_07, A07.movedFrom(),
               MOVE_07 == (MoveState::e_MOVED == A07.movedFrom()) || 2 == N07);
        ASSERTV(MOVE_08, A08.movedFrom(),
               MOVE_08 == (MoveState::e_MOVED == A08.movedFrom()) || 2 == N08);
        ASSERTV(MOVE_09, A09.movedFrom(),
               MOVE_09 == (MoveState::e_MOVED == A09.movedFrom()) || 2 == N09);
        ASSERTV(MOVE_10, A10.movedFrom(),
               MOVE_10 == (MoveState::e_MOVED == A10.movedFrom()) || 2 == N10);

        const TYPE& V = X[0];

        ASSERTV(V01, V.arg01(), V01 == V.arg01() || 2 == N01);
        ASSERTV(V02, V.arg02(), V02 == V.arg02() || 2 == N02);
        ASSERTV(V03, V.arg03(), V03 == V.arg03() || 2 == N03);
        ASSERTV(V04, V.arg04(), V04 == V.arg04() || 2 == N04);
        ASSERTV(V05, V.arg05(), V05 == V.arg05() || 2 == N05);
        ASSERTV(V06, V.arg06(), V06 == V.arg06() || 2 == N06);
        ASSERTV(V07, V.arg07(), V07 == V.arg07() || 2 == N07);
        ASSERTV(V08, V.arg08(), V08 == V.arg08() || 2 == N08);
        ASSERTV(V09, V.arg09(), V09 == V.arg09() || 2 == N09);
        ASSERTV(V10, V.arg10(), V10 == V.arg10() || 2 == N10);

        TestAllocatorUtil::test(V, oa);
    } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
}

template <class TYPE, class ALLOC>
template <int N_ARGS,
          int N01,
          int N02,
          int N03,
          int N04,
          int N05,
          int N06,
          int N07,
          int N08,
          int N09,
          int N10>
void
TestDriver<TYPE, ALLOC>::testCase29a_RunTest(Obj *target, const_iterator pos)
{
    DECLARE_BOOL_CONSTANT(MOVE_01, N01 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_02, N02 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_03, N03 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_04, N04 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_05, N05 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_06, N06 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_07, N07 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_08, N08 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_09, N09 == 1);
    DECLARE_BOOL_CONSTANT(MOVE_10, N10 == 1);

    bslma::TestAllocator *testAlloc = dynamic_cast<bslma::TestAllocator *>(
                                          target->get_allocator().mechanism());
    if (!testAlloc) {
        ASSERT(!"Allocator in test case 29 is not a test allocator!");
        return;                                                       // RETURN
    }
    bslma::TestAllocator& oa = *testAlloc;
    Obj& mX = *target;  const Obj& X = mX;

    bslma::TestAllocator aa("args", veryVeryVeryVerbose);

    bsls::ObjectBuffer<typename TYPE::ArgType01> BUF01;
    ConstrUtil::construct(BUF01.address(), &aa, V01);
    typename TYPE::ArgType01& A01 = BUF01.object();
    bslma::DestructorGuard<typename TYPE::ArgType01> G01(&A01);

    bsls::ObjectBuffer<typename TYPE::ArgType02> BUF02;
    ConstrUtil::construct(BUF02.address(), &aa, V02);
    typename TYPE::ArgType02& A02 = BUF02.object();
    bslma::DestructorGuard<typename TYPE::ArgType02> G02(&A02);

    bsls::ObjectBuffer<typename TYPE::ArgType03> BUF03;
    ConstrUtil::construct(BUF03.address(), &aa, V03);
    typename TYPE::ArgType03& A03 = BUF03.object();
    bslma::DestructorGuard<typename TYPE::ArgType03> G03(&A03);

    bsls::ObjectBuffer<typename TYPE::ArgType04> BUF04;
    ConstrUtil::construct(BUF04.address(), &aa, V04);
    typename TYPE::ArgType04& A04 = BUF04.object();
    bslma::DestructorGuard<typename TYPE::ArgType04> G04(&A04);

    bsls::ObjectBuffer<typename TYPE::ArgType05> BUF05;
    ConstrUtil::construct(BUF05.address(), &aa, V05);
    typename TYPE::ArgType05& A05 = BUF05.object();
    bslma::DestructorGuard<typename TYPE::ArgType05> G05(&A05);

    bsls::ObjectBuffer<typename TYPE::ArgType06> BUF06;
    ConstrUtil::construct(BUF06.address(), &aa, V06);
    typename TYPE::ArgType06& A06 = BUF06.object();
    bslma::DestructorGuard<typename TYPE::ArgType06> G06(&A06);

    bsls::ObjectBuffer<typename TYPE::ArgType07> BUF07;
    ConstrUtil::construct(BUF07.address(), &aa, V07);
    typename TYPE::ArgType07& A07 = BUF07.object();
    bslma::DestructorGuard<typename TYPE::ArgType07> G07(&A07);

    bsls::ObjectBuffer<typename TYPE::ArgType08> BUF08;
    ConstrUtil::construct(BUF08.address(), &aa,  V08);
    typename TYPE::ArgType08& A08 = BUF08.object();
    bslma::DestructorGuard<typename TYPE::ArgType08> G08(&A08);

    bsls::ObjectBuffer<typename TYPE::ArgType09> BUF09;
    ConstrUtil::construct(BUF09.address(), &aa, V09);
    typename TYPE::ArgType09& A09 = BUF09.object();
    bslma::DestructorGuard<typename TYPE::ArgType09> G09(&A09);

    bsls::ObjectBuffer<typename TYPE::ArgType10> BUF10;
    ConstrUtil::construct(BUF10.address(), &aa, V10);
    typename TYPE::ArgType10& A10 = BUF10.object();
    bslma::DestructorGuard<typename TYPE::ArgType10> G10(&A10);

    const size_t                  len   = X.size();
    const typename Obj::size_type index = pos - X.cbegin();

    iterator result;

    switch (N_ARGS) {
      case 0: {
        result = mX.emplace(pos);
      } break;
      case 1: {
        result = mX.emplace(pos, testArg(A01, MOVE_01));
      } break;
      case 2: {
        result = mX.emplace(pos, testArg(A01, MOVE_01), testArg(A02, MOVE_02));
      } break;
      case 3: {
        result = mX.emplace(pos,
                            testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03));
      } break;
      case 4: {
        result = mX.emplace(pos,
                            testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03),
                            testArg(A04, MOVE_04));
      } break;
      case 5: {
        result = mX.emplace(pos,
                            testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03),
                            testArg(A04, MOVE_04),
                            testArg(A05, MOVE_05));
      } break;
      case 6: {
        result = mX.emplace(pos,
                            testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03),
                            testArg(A04, MOVE_04),
                            testArg(A05, MOVE_05),
                            testArg(A06, MOVE_06));
      } break;
      case 7: {
        result = mX.emplace(pos,
                            testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03),
                            testArg(A04, MOVE_04),
                            testArg(A05, MOVE_05),
                            testArg(A06, MOVE_06),
                            testArg(A07, MOVE_07));
      } break;
      case 8: {
        result = mX.emplace(pos,
                            testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03),
                            testArg(A04, MOVE_04),
                            testArg(A05, MOVE_05),
                            testArg(A06, MOVE_06),
                            testArg(A07, MOVE_07),
                            testArg(A08, MOVE_08));
      } break;
      case 9: {
        result = mX.emplace(pos,
                            testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03),
                            testArg(A04, MOVE_04),
                            testArg(A05, MOVE_05),
                            testArg(A06, MOVE_06),
                            testArg(A07, MOVE_07),
                            testArg(A08, MOVE_08),
                            testArg(A09, MOVE_09));
      } break;
      case 10: {
        result = mX.emplace(pos,
                            testArg(A01, MOVE_01),
                            testArg(A02, MOVE_02),
                            testArg(A03, MOVE_03),
                            testArg(A04, MOVE_04),
                            testArg(A05, MOVE_05),
                            testArg(A06, MOVE_06),
                            testArg(A07, MOVE_07),
                            testArg(A08, MOVE_08),
                            testArg(A09, MOVE_09),
                            testArg(A10, MOVE_10));
      } break;
      default: {
        ASSERTV(!"Invalid # of args!");
      } break;
    }
    ASSERTV(len + 1, X.size(), len + 1 == X.size());

    ASSERTV(MOVE_01, A01.movedFrom(),
            MOVE_01 == (MoveState::e_MOVED == A01.movedFrom()) || 2 == N01);
    ASSERTV(MOVE_02, A02.movedFrom(),
            MOVE_02 == (MoveState::e_MOVED == A02.movedFrom()) || 2 == N02);
    ASSERTV(MOVE_03, A03.movedFrom(),
            MOVE_03 == (MoveState::e_MOVED == A03.movedFrom()) || 2 == N03);
    ASSERTV(MOVE_04, A04.movedFrom(),
            MOVE_04 == (MoveState::e_MOVED == A04.movedFrom()) || 2 == N04);
    ASSERTV(MOVE_05, A05.movedFrom(),
            MOVE_05 == (MoveState::e_MOVED == A05.movedFrom()) || 2 == N05);
    ASSERTV(MOVE_06, A06.movedFrom(),
            MOVE_06 == (MoveState::e_MOVED == A06.movedFrom()) || 2 == N06);
    ASSERTV(MOVE_07, A07.movedFrom(),
            MOVE_07 == (MoveState::e_MOVED == A07.movedFrom()) || 2 == N07);
    ASSERTV(MOVE_08, A08.movedFrom(),
            MOVE_08 == (MoveState::e_MOVED == A08.movedFrom()) || 2 == N08);
    ASSERTV(MOVE_09, A09.movedFrom(),
            MOVE_09 == (MoveState::e_MOVED == A09.movedFrom()) || 2 == N09);
    ASSERTV(MOVE_10, A10.movedFrom(),
            MOVE_10 == (MoveState::e_MOVED == A10.movedFrom()) || 2 == N10);

    const TYPE& V = X[index];

    ASSERTV( result == X.begin() + index);
    ASSERTV(*result == V);

    ASSERTV(V01, V.arg01(), V01 == V.arg01() || 2 == N01);
    ASSERTV(V02, V.arg02(), V02 == V.arg02() || 2 == N02);
    ASSERTV(V03, V.arg03(), V03 == V.arg03() || 2 == N03);
    ASSERTV(V04, V.arg04(), V04 == V.arg04() || 2 == N04);
    ASSERTV(V05, V.arg05(), V05 == V.arg05() || 2 == N05);
    ASSERTV(V06, V.arg06(), V06 == V.arg06() || 2 == N06);
    ASSERTV(V07, V.arg07(), V07 == V.arg07() || 2 == N07);
    ASSERTV(V08, V.arg08(), V08 == V.arg08() || 2 == N08);
    ASSERTV(V09, V.arg09(), V09 == V.arg09() || 2 == N09);
    ASSERTV(V10, V.arg10(), V10 == V.arg10() || 2 == N10);

    TestAllocatorUtil::test(V, oa);
}

                                 // ----------
                                 // TEST CASES
                                 // ----------

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase32()
{
    // ------------------------------------------------------------------------
    // 'noexcept' SPECIFICATION
    //
    // Concerns:
    //: 1 The 'noexcept' specification has been applied to all class interfaces
    //:   required by the standard.
    //
    // Plan:
    //: 1 Apply the uniary 'noexcept' operator to expressions that mimic those
    //:   appearing in the standard and confirm that calculated boolean value
    //:   matches the expected value.
    //:
    //: 2 Since the 'noexcept' specification does not vary with the 'TYPE'
    //:   of the container, we need test for just one general type and any
    //:   'TYPE' specializations.
    //
    // Testing:
    //   CONCERN: Methods qualified 'noexcept' in standard are so implemented.
    // ------------------------------------------------------------------------

    if (verbose) {
        P(bsls::NameOf<Obj>())
        P(bsls::NameOf<TYPE>())
        P(bsls::NameOf<ALLOC>())
    }

    // N4594: page 835: 23.3.8 Class template 'deque'

    // page 835
    //..
    //  // 23.3.8.2, construct/copy/destroy:
    //  deque& operator=(deque&& x)
    //      noexcept(allocator_traits<Allocator>::is_always_equal::value);
    //  allocator_type get_allocator() const noexcept;
    //..

    {
        Obj d;    (void) d;
        Obj x;    (void) x;

        ASSERT(false
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(d = MoveUtil::move(x)));

        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(d.get_allocator()));
    }

    // page 836
    //..
    //  // iterators:
    //  iterator begin() noexcept;
    //  const_iterator begin() const noexcept;
    //  iterator end() noexcept;
    //  const_iterator end() const noexcept;
    //  reverse_iterator rbegin() noexcept;
    //  const_reverse_iterator rbegin() const noexcept;
    //  reverse_iterator rend() noexcept;
    //  const_reverse_iterator rend() const noexcept;
    //  const_iterator cbegin() const noexcept;
    //  const_iterator cend() const noexcept;
    //  const_reverse_iterator crbegin() const noexcept;
    //  const_reverse_iterator crend() const noexcept;
    //..

    {
        Obj d; const Obj& D = d;    (void) D;

        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(d.begin()));
        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(D.begin()));
        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(d.end()));
        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(D.end()));
        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(D.begin()));
        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(d.rbegin()));
        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(D.rbegin()));
        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(d.rend()));
        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(D.rend()));
        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(D.cbegin()));
        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(D.cend()));
        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(D.crbegin()));
        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(D.crend()));
    }

    // page 836
    //..
    //  // 23.3.8.3, capacity:
    //  bool empty() const noexcept;
    //  size_type size() const noexcept;
    //  size_type max_size() const noexcept;
    //..

    {
        Obj d;

        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(d.empty()));
        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(d.size()));
        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(d.max_size()));
    }

    // page 836
    //..
    //  // 23.3.8.4, modifiers:
    //  void swap(deque&)
    //  noexcept(allocator_traits<Allocator>::is_always_equal::value);
    //  void clear() noexcept;
    //..

    {
        Obj d;
        Obj x;

        ASSERT(false
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(d.swap(x)));

        ASSERT(BSLS_KEYWORD_NOEXCEPT_AVAILABLE
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(d.clear()));
    }

    // page 837
    //..
    //  // 23.3.8.5, specialized algorithms:
    //  template <class T, class Allocator>
    //  void swap(deque<T, Allocator>& x, deque<T, Allocator>& y)
    //      noexcept(noexcept(x.swap(y)));
    //..

    {
        Obj x;
        Obj y;

        ASSERT(false
            == BSLS_KEYWORD_NOEXCEPT_OPERATOR(swap(x, y)));
    }
}

template <class TYPE, class ALLOC>
template <class CONTAINER>
void TestDriver<TYPE,ALLOC>::testCaseM1Range(const CONTAINER&)
{
    // ------------------------------------------------------------------------
    // PERFORMANCE TEST (RANGE)
    //
    // Concerns:
    //: 1 That performance does not regress between versions.
    //:
    //: 2 That no surprising performance (both extremely fast or slow) is
    //:   detected, which might be indicating missed optimizations or
    //:   inadvertent loss of performance (e.g., by wrongly setting the
    //:   capacity and triggering too frequent reallocations).
    //:
    //: 3 That small "improvements" can be tested w.r.t. to performance, in a
    //:   uniform benchmark (e.g., measuring the overhead of allocating for
    //:   empty strings).
    //
    // Plan:  We follow a simple benchmark which performs the operation under
    //   timing test in a loop.  Specifically, we wish to measure the time
    //   taken by:
    //     C1) The constructors.
    //     A1) The 'assign' operation.
    //     I1) The 'insert' operation at the end.
    //     I2) The 'insert' operation at the front.
    //     I3) The 'insert' operation everywhere.
    //     E1) The 'erase' operation.
    // ------------------------------------------------------------------------

    bsls::Stopwatch t;

    // DATA INITIALIZATION (NOT TIMED)
    const TYPE         *values      = 0;
    const TYPE *const&  VALUES      = values;
    const int           NUM_VALUES  = getValues(&values);
    const int           LENGTH      = 1000;
    const int           NUM_DEQUE   = 300;

    const char         *SPECREF     = "ABCDE";
    const int           SPECREF_LEN = (int) strlen(SPECREF);
    char                SPEC[LENGTH + 1];

    for (int i = 0; i < LENGTH; ++i) {
        SPEC[i] = SPECREF[i % SPECREF_LEN];
    }
    SPEC[LENGTH] = '\0';
    CONTAINER mU(gV(SPEC));  const CONTAINER& U = mU;

    void *address = bslma::Default::defaultAllocator()->allocate(
                                        NUM_DEQUE * LENGTH * 2 * sizeof(TYPE));
    bslma::Default::defaultAllocator()->deallocate(address);

    printf("\tC1) Constructor:\n");
    {
        // bsl::deque(f, l)
        double time = 0.;
        Obj *deques[NUM_DEQUE];

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE; ++i) {
            deques[i] = new Obj(U.begin(), U.end());
        }
        time = t.elapsedTime();

        printf("\t\tbsl::deque(f,l):\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE; ++i) {
            delete deques[i];
        }
    }

    printf("\tA1) Assign:\n");
    {
        // assign(f, l)
        double time = 0.;
        Obj *deques[NUM_DEQUE];

        ASSERT(LENGTH >= NUM_DEQUE);
        // Spread out the initial lengths.
        for (int i = (LENGTH - NUM_DEQUE) / 2, j = 0;
                 i < (LENGTH + NUM_DEQUE) / 2;
                 ++i, ++j) {
            deques[j]  = new Obj(i, VALUES[i % NUM_VALUES]);
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE; ++i) {
            deques[i]->assign(U.begin(), U.end());
        }
        time = t.elapsedTime();

        printf("\t\tassign(f,l):\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE; ++i) {
            delete deques[i];
        }
    }

    printf("\tI1) Insert (at front):\n");
    {
        // insert(p, f, l)
        double time = 0.;
        Obj *deques[NUM_DEQUE];

        ASSERT(LENGTH >= NUM_DEQUE);
        // Spread out the initial lengths.
        for (int i = (LENGTH - NUM_DEQUE) / 2, j = 0;
                 i < (LENGTH + NUM_DEQUE) / 2;
                 ++i, ++j) {
            deques[j]  = new Obj(i, VALUES[i % NUM_VALUES]);
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE; ++i) {
            deques[i]->insert(deques[i]->begin(), U.begin(), U.end());
        }
        time = t.elapsedTime();

        printf("\t\tinsert(p,f,l):\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE; ++i) {
            delete deques[i];
        }
    }

    printf("\tI2) Insert (at back):\n");
    {
        // insert(p, f, l)
        double time = 0.;
        Obj *deques[NUM_DEQUE];

        ASSERT(LENGTH >= NUM_DEQUE);
        // Spread out the initial lengths.
        for (int i = (LENGTH - NUM_DEQUE) / 2, j = 0;
                 i < (LENGTH + NUM_DEQUE) / 2;
                 ++i, ++j) {
            deques[j]  = new Obj(i, VALUES[i % NUM_VALUES]);
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE; ++i) {
            deques[i]->insert(deques[i]->end(), U.begin(), U.end());
        }
        time = t.elapsedTime();

        printf("\t\tinsert(p,f,l):\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE; ++i) {
            delete deques[i];
        }
    }

    printf("\tI3) Insert (everywhere):\n");
    {
        // insert(p, f, l)
        double time = 0.;
        Obj    *deques[NUM_DEQUE];

        ASSERT(LENGTH >= NUM_DEQUE);
        // Spread out the initial lengths.
        for (int i = 0; i < NUM_DEQUE; ++i) {
            // NOTE: Uses NUM_DEQUE for length.
            deques[i] = new Obj(NUM_DEQUE, VALUES[i % NUM_VALUES]);
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE; ++i) {
            deques[i]->insert(deques[i]->begin() + i, U.begin(), U.end());
        }
        time += t.elapsedTime();

        printf("\t\tinsert(p,f,l):\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE; ++i) {
            delete deques[i];
        }
    }
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCaseM1()
{
    // ------------------------------------------------------------------------
    // PERFORMANCE TEST
    //
    // Concerns:
    //: 1 That performance does not regress between versions.
    //:
    //: 2 That no surprising performance (both extremely fast or slow) is
    //:   detected, which might be indicating missed optimizations or
    //:   inadvertent loss of performance (e.g., by wrongly setting the
    //:   capacity and triggering too frequent reallocations).
    //:
    //: 3 That small "improvements" can be tested w.r.t. to performance, in a
    //:   uniform benchmark (e.g., measuring the overhead of allocating for
    //:   empty strings).
    //
    // Plan: We follow a simple benchmark which performs the operation under
    //   timing test in a loop.  Specifically, we wish to measure the time
    //   taken by:
    //     C1) The various constructors.
    //     C2) The copy constructor.
    //     A1) The copy assignment.
    //     A2) The 'assign' operations.
    //     P1) The 'push_back' operation.
    //     P2) The 'push_front' operation.
    //     P3) The 'pop_back' operation.
    //     P4) The 'pop_front' operation.
    //     I1) The 'insert' operation in its various forms, at the front
    //     I2) The 'insert' operation in its various forms, at the back
    //     I3) The 'insert' operation in its various forms.
    //     E1) The 'erase' operation in its various forms.
    // ------------------------------------------------------------------------

    bsls::Stopwatch t;

    // DATA INITIALIZATION (NOT TIMED)
    const TYPE         *values      = 0;
    const TYPE *const&  VALUES      = values;
    const int           NUM_VALUES  = getValues(&values);
    const int           LENGTH_S    = 1000;
    const int           LENGTH_L    = 5000;
    const int           NUM_DEQUE_S = 100;
    const int           NUM_DEQUE_L = 1000;

//    void *address = bslma::Default::defaultAllocator()->allocate(
//                                  NUM_DEQUE_L * LENGTH_L * 2 * sizeof(TYPE));
//    bslma::Default::defaultAllocator()->deallocate(address);

    // C1) CONSTRUCTORS
    printf("\tC1) Constructors:\n");
    {
        // bsl::deque()
        double time = 0.;

        t.reset(); t.start();
        Obj *deques = new Obj[NUM_DEQUE_L];
        time = t.elapsedTime();

        printf("\t\tbsl::deque():\t\t%1.6fs\n", time);
        delete[] deques;
    }
    {
        // bsl::deque(n)
        double time = 0.;
        Obj *deques[NUM_DEQUE_L];

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_L; ++i) {
            deques[i] = new Obj(LENGTH_S);
        }
        time = t.elapsedTime();

        printf("\t\tbsl::deque(n):\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_L; ++i) {
            delete deques[i];
        }
    }
    {
        // bsl::deque(n,v)
        double time = 0.;
        Obj *deques[NUM_DEQUE_L];

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_L; ++i) {
            deques[i] = new Obj(LENGTH_S, VALUES[i % NUM_VALUES]);
        }
        time = t.elapsedTime();

        printf("\t\tbsl::deque(n,v):\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_L; ++i) {
            delete deques[i];
        }
    }
    // C2) COPY CONSTRUCTOR
    printf("\tC2) Copy Constructors:\n");
    {
        // bsl::deque(bsl::deque)
        double time = 0.;
        Obj  deq(LENGTH_L);
        Obj *deques[NUM_DEQUE_S];

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            deques[i] = new Obj(deq);
        }
        time = t.elapsedTime();

        printf("\t\tbsl::deque(d):\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            delete deques[i];
        }
    }

    // A1) COPY ASSIGNMENT
    printf("\tA1) Copy Assignment:\n");
    {
        // operator=(bsl::deque)
        double time = 0;
        Obj deq(LENGTH_L / 2);
        Obj *deques[NUM_DEQUE_L];

        ASSERT(LENGTH_L >= NUM_DEQUE_L);
        // Spread out the initial lengths.
        for (int i = (LENGTH_L - NUM_DEQUE_L) / 2, j = 0;
                 i < (LENGTH_L + NUM_DEQUE_L) / 2;
                 ++i, ++j) {
            deques[j] = new Obj(i, VALUES[i % NUM_VALUES]);
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_L; ++i) {
            *deques[i] = deq;
        }
        time = t.elapsedTime();

        printf("\t\toperator=(d):\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_L; ++i) {
            ASSERT(*deques[i] == deq);
            delete deques[i];
        }
    }

    // A2) ASSIGN
    printf("\tA2) Assign:\n");
    {
        // assign(n)
        double time = 0;
        Obj *deques[NUM_DEQUE_L];

        ASSERT(LENGTH_L >= NUM_DEQUE_L);
        // Spread out the initial lengths.
        for (int i = (LENGTH_L - NUM_DEQUE_L) / 2, j = 0;
                 i < (LENGTH_L + NUM_DEQUE_L) / 2;
                 ++i, ++j) {
            deques[j]  = new Obj(i, VALUES[i % NUM_VALUES]);
        }
        const TYPE val    = TYPE();
        const int  length = LENGTH_L / 2;

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_L; ++i) {
            deques[i]->assign(length, val);
        }
        time = t.elapsedTime();

        printf("\t\tassign(n):\t\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_L; ++i) {
            delete deques[i];
        }
    }

    // P1) PUSH_BACK
    printf("\tP1) Push_back:\n");
    {
        // push_back(v)
        double time = 0;
        Obj *deques[NUM_DEQUE_S];

        // Spread out the initial lengths.
        for (int i = (LENGTH_L - NUM_DEQUE_S) / 2, j = 0;
                 i < (LENGTH_L + NUM_DEQUE_S) / 2;
                 ++i, ++j) {
            deques[j] = new Obj(i, VALUES[i % NUM_VALUES]);
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            // Push in 1000 objects.
            for (int j = 0; j < LENGTH_L; ++j) {
                deques[i]->push_back(VALUES[j % NUM_VALUES]);
            }
        }
        time = t.elapsedTime();

        printf("\t\tpush_back(v):\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            delete deques[i];
        }
    }

    // P1) PUSH_FRONT
    printf("\tP2) Push_front:\n");
    {
        // push_front(v)
        double time = 0;
        Obj *deques[NUM_DEQUE_S];

        // Spread out the initial lengths.
        for (int i = (LENGTH_L - NUM_DEQUE_S) / 2, j = 0;
                 i < (LENGTH_L + NUM_DEQUE_S) / 2;
                 ++i, ++j) {
            deques[j] = new Obj(i, VALUES[i % NUM_VALUES]);
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            // Push in 1000 objects.
            for (int j = 0; j < LENGTH_L; ++j) {
                deques[i]->push_front(VALUES[j % NUM_VALUES]);
            }
        }
        time = t.elapsedTime();

        printf("\t\tpush_front(v):\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            delete deques[i];
        }
    }

    // P1) POP_BACK
    printf("\tP3) Pop_back:\n");
    {
        // pop_back()
        double time = 0;
        Obj    *deques[NUM_DEQUE_S];
        size_t  sizes[NUM_DEQUE_S];

        // Spread out the initial lengths.
        for (int i = (LENGTH_L - NUM_DEQUE_S) / 2, j = 0;
                 i < (LENGTH_L + NUM_DEQUE_S) / 2;
                 ++i, ++j) {
            deques[j] = new Obj(i, VALUES[i % NUM_VALUES]);
            sizes[j] = i;
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            // Pop out all objects.
            for (unsigned int j = 0; j < sizes[i]; ++j) {
                deques[i]->pop_back();
            }
        }
        time = t.elapsedTime();

        printf("\t\tpop_back():\t\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            delete deques[i];
        }
    }

    // P1) POP_FRONT
    printf("\tP4) Pop_front:\n");
    {
        // pop_front()
        double  time = 0;
        Obj    *deques[NUM_DEQUE_S];
        size_t  sizes[NUM_DEQUE_S];

        // Spread out the initial lengths.
        for (int i = (LENGTH_L - NUM_DEQUE_S) / 2, j = 0;
                 i < (LENGTH_L + NUM_DEQUE_S) / 2;
                 ++i, ++j) {
            deques[j] = new Obj(i, VALUES[i % NUM_VALUES]);
            sizes[j] = i;
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            // Pop out all objects.
            for (unsigned int j = 0; j < sizes[i]; ++j) {
                deques[i]->pop_front();
            }
        }
        time = t.elapsedTime();

        printf("\t\tpop_front():\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            delete deques[i];
        }
    }

    // I1) INSERT (FRONT)
    printf("\tI1) Insert (at front):\n");
    {
        // insert(p,v)
        double  time = 0;
        Obj    *deques[NUM_DEQUE_S];

        // Spread out the initial lengths.
        for (int i = (LENGTH_S - NUM_DEQUE_S) / 2, j = 0;
                 i < (LENGTH_S + NUM_DEQUE_S) / 2;
                 ++i, ++j) {
            deques[j] = new Obj(i, VALUES[i % NUM_VALUES]);
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            for (int j = 0; j < LENGTH_L; ++j) {
                deques[i]->insert(deques[i]->begin(), VALUES[i % NUM_VALUES]);
            }
        }
        time = t.elapsedTime();

        printf("\t\tinsert(p,v):\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            delete deques[i];
        }
    }
    {
        // insert(n,p,v)
        double time = 0;
        Obj  *deques[NUM_DEQUE_S];

        // Spread out the initial lengths.
        for (int i = (LENGTH_S - NUM_DEQUE_S) / 2, j = 0;
                 i < (LENGTH_S + NUM_DEQUE_S) / 2;
                 ++i, ++j) {
            deques[j] = new Obj(i, VALUES[i % NUM_VALUES]);
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            deques[i]->insert(deques[i]->begin(), LENGTH_L,
                              VALUES[i % NUM_VALUES]);
        }
        time = t.elapsedTime();

        printf("\t\tinsert(p,n,v):\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            delete deques[i];
        }
    }

    // I2) INSERT (BACK)
    printf("\tI2) Insert (at back):\n");
    {
        // insert(p,v)
        double  time = 0;
        Obj    *deques[NUM_DEQUE_S];

        // Spread out the initial lengths.
        for (int i = (LENGTH_S - NUM_DEQUE_S) / 2, j = 0;
                 i < (LENGTH_S + NUM_DEQUE_S) / 2;
                 ++i, ++j) {
            deques[j] = new Obj(i, VALUES[i % NUM_VALUES]);
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            for (int j = 0; j < LENGTH_L; ++j) {
                deques[i]->insert(deques[i]->end(), VALUES[i % NUM_VALUES]);
            }
        }
        time = t.elapsedTime();

        printf("\t\tinsert(p,v):\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            delete deques[i];
        }
    }

    {
        // insert(n,p,v)
        double time = 0;
        Obj  *deques[NUM_DEQUE_S];

        // Spread out the initial lengths.
        for (int i = (LENGTH_S - NUM_DEQUE_S) / 2, j = 0;
                 i < (LENGTH_S + NUM_DEQUE_S) / 2;
                 ++i, ++j) {
            deques[j] = new Obj(i, VALUES[i % NUM_VALUES]);
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            deques[i]->insert(deques[i]->end(), LENGTH_L,
                              VALUES[i % NUM_VALUES]);
        }
        time = t.elapsedTime();

        printf("\t\tinsert(p,n,v):\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            delete deques[i];
        }
    }

    // I3) INSERT
    printf("\tI3) Insert (everywhere):\n");
    {
        // insert(p,v)
        double  time = 0;
        Obj    *deques[NUM_DEQUE_S];
        size_t  sizes[NUM_DEQUE_S];

        // Spread out the initial lengths.
        for (int i = (LENGTH_S - NUM_DEQUE_S) / 2, j = 0;
                 i < (LENGTH_S + NUM_DEQUE_S) / 2;
                 ++i, ++j) {
            deques[j] = new Obj(i, VALUES[i % NUM_VALUES]);
            sizes[j]  = i;
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            for (unsigned int j = 0; j < sizes[i]; ++j) {
                deques[i]->insert(deques[i]->begin() + j,
                                  VALUES[i % NUM_VALUES]);
            }
        }
        time = t.elapsedTime();

        printf("\t\tinsert(p,v):\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            delete deques[i];
        }
    }

    {
        // insert(n,p,v)
        double time = 0;
        Obj    *deques[NUM_DEQUE_S];
        size_t  sizes[NUM_DEQUE_S];

        // Spread out the initial lengths.
        for (int i = (LENGTH_S - NUM_DEQUE_S) / 2, j = 0;
                 i < (LENGTH_S + NUM_DEQUE_S) / 2;
                 ++i, ++j) {
            deques[j] = new Obj(i, VALUES[i % NUM_VALUES]);
            sizes[j] = i;
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            // Insert in middle.
            deques[i]->insert(deques[i]->begin() + sizes[i] / 2, LENGTH_S,
                              VALUES[i % NUM_VALUES]);
        }
        time = t.elapsedTime();

        printf("\t\tinsert(p,n,v):\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            delete deques[i];
        }
    }

    // E1) ERASE
    printf("\tE1) Erase:\n");
    {
        // erase(p)
        double  time = 0;
        Obj    *deques[NUM_DEQUE_S];

        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            deques[i] = new Obj(LENGTH_S * 2, VALUES[i % NUM_VALUES]);
        }

        t.reset(); t.start();
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            for (int j = 0; j < LENGTH_S; ++j) {
                deques[i]->erase(deques[i]->begin() + j);
            }
        }
        time = t.elapsedTime();

        printf("\t\terase(p):\t\t\t%1.6fs\n", time);
        for (int i = 0; i < NUM_DEQUE_S; ++i) {
            delete deques[i];
        }
    }
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE, ALLOC>::testCase31()
{
    // ------------------------------------------------------------------------
    // TESTING 'shrink_to_fit'
    //
    // Concerns:
    //: 1 'shrink_to_fit' either reduces memory use or leaves it unchanged
    //:   (i.e., it never increases memory use).
    //:
    //: 2 'shrink_to_fit' either reduces capacity or leaves it unchanged
    //:   (i.e., it never increases capacity).
    //:
    //: 3 'shrink_to_fit' has no effect on object value.
    //:
    //: 4 'shrink_to_fit' does not move any elements; consequently, iterators
    //:   and references to elements remain valid.  (TBD not yet tested)
    //:
    //: 5 Following a call to 'shrink_to_fit', subsequent calls have no effect,
    //:   provided there are no intervening requests to insert or erase one or
    //:   more elements, or calls to 'reserve'.
    //:
    //: 6 'shrink_to_fit' yields the same memory use and capacity when invoked
    //:   on an empty object as when invoked on a default-constructed object.
    //:
    //: 7 All memory allocations, if any, are from the object allocator.
    //:
    //: 8 (white-box) 'shrink_to_fit' does at most one allocation.
    //:
    //: 9 'shrink_to_fit' provides the strong exception guarantee.
    //
    // Plan:
    //: 1 For an initial breathing test, call 'shrink_to_fit' on a
    //:   default-constructed object and verify that the change in capacity and
    //:   memory use is as expected.
    //:
    //: 2 Using the table-based approach, specify a set of initial container
    //:   sizes and the number of elements to erase ("delta") at the end of
    //:   each test iteration.                                   (C-1..3, 5..8)
    //:
    //:   1 For each row, 'R', from P-2, create an object, 'X', having the
    //:     specified initial size; create a second ("control") object, 'Y',
    //:     having the same initial value as 'X'.
    //:
    //:   2 Invoke 'shrink_to_fit' twice on 'X' and verify (following each
    //:     invocation) object value, capacity, and memory use are as expected.
    //:                                                             (C-1..3, 5)
    //:
    //:   3 Verify all allocations, if any, are from the object's allocator.
    //:                                                                (C-7..8)
    //:
    //:   4 Erase from 'X' (and 'Y') the number of elements specified in 'R'.
    //:
    //:   5 Repeat P-2.2..2.4 until 'X' is empty.
    //:
    //:   6 Verify that the now empty 'X' has the same capacity and memory use
    //:     as that of a shrunken default-constructed object.             (C-6)
    //:
    //: 3 Perform all calls to 'shrink_to_fit' in the presence of injected
    //:   exceptions.                                                     (C-9)
    //
    // Testing:
    //   void shrink_to_fit();
    // ------------------------------------------------------------------------

    Int64 minimumBytesInUse = 0;  // after 'shrink_to_fit' called on empty
    Int64 minimumCapacity   = 0;  //   "          "          "    "    "

    if (verbose) printf("\tTesting 'shrink_to_fit'.\n");

    if (verbose) printf("\t\tOn a default-constructed object.\n");
    {
        bslma::TestAllocator da("default", veryVeryVeryVerbose);
        bslma::TestAllocator oa("object",  veryVeryVeryVerbose);
        bslma::TestAllocator za("other",   veryVeryVeryVerbose);
        ALLOC                xoa(&oa);
        ALLOC                xza(&za);

        bslma::DefaultAllocatorGuard dag(&da);

        Obj mX(xoa);  const Obj& X = mX;

        const Obj Y(xza);  // control

        const Int64 BMEM = oa.numBytesInUse();
        const Int64 BCAP = X.capacity();

        if (veryVerbose) { printf("\t\tBEFORE: "); P_(BMEM); P(BCAP); }

        BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
            mX.shrink_to_fit();
        } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

        const Int64 AMEM = oa.numBytesInUse();
        const Int64 ACAP = X.capacity();

        if (veryVerbose) { printf("\t\tAFTER : "); P_(AMEM); P(ACAP); }

        // save minimum values for use in later testing

        minimumBytesInUse = AMEM;
        minimumCapacity   = ACAP;

        ASSERT(   X == Y);
        ASSERT(AMEM <  BMEM);
        ASSERT(ACAP <= BCAP);

        {
            bslma::TestAllocatorMonitor oam(&oa);

            BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                mX.shrink_to_fit();
            } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

            ASSERT(   X == Y);
            ASSERT(AMEM == oa.numBytesInUse());
            ASSERT(ACAP == (Int64) X.capacity());

            ASSERT(oam.isTotalSame());
        }

        ASSERT(0 == da.numBlocksTotal());
    }

    if (verbose) printf("\t\tOn objects of various sizes and capacities.\n");
    {
        const TestValues VALUES;
        const size_t     NUM_VALUES = VALUES.size();

        // Powers of 2 are not significant for deque, but we want to test
        // large numbers.

        static const struct {
            int    d_lineNum;      // source line number
            size_t d_numElements;  // initial number of elements
            int    d_delta;        // no. of elements to 'erase' each iteration
        } DATA[] = {
            //line   numElements   delta
            //----   -----------   -----
            { L_,         0,          0   },
            { L_,         1,          1   },
            { L_,         2,          1   },
            { L_,        15,          3   },
            { L_,        32,         10   },
            { L_,        99,         15   },
            { L_,       229,         25   },
            { L_,      2000,        100   }
        };
        enum { NUM_DATA = sizeof DATA / sizeof *DATA };

        for (int ti = 0; ti < NUM_DATA; ++ti) {
            const int    LINE  = DATA[ti].d_lineNum;
            const size_t NE    = DATA[ti].d_numElements;
            const size_t DELTA = DATA[ti].d_delta;

            if (veryVerbose) { T_ P_(LINE) P_(NE) P(DELTA) }

            bslma::TestAllocator da("default", veryVeryVeryVerbose);
            bslma::TestAllocator oa("object",  veryVeryVeryVerbose);
            bslma::TestAllocator za("other",   veryVeryVeryVerbose);
            ALLOC                xoa(&oa);
            ALLOC                xza(&za);

            bslma::DefaultAllocatorGuard dag(&da);

            Obj mX(xoa);  const Obj& X = mX;
            Obj mY(xza);  const Obj& Y = mY;  // control

            stretch(&mX, NE,
                    TstFacility::getIdentifier(VALUES[ti % NUM_VALUES]));
            stretch(&mY, NE,
                    TstFacility::getIdentifier(VALUES[ti % NUM_VALUES]));

            bool eraseAtFront = true;  // alternate erasing at front and back

            while (1) {
                const Int64 BALLOC = oa.numAllocations();
                const Int64 BMEM   = oa.numBytesInUse();
                const Int64 BCAP   = X.capacity();

                if (veryVerbose) {
                    printf("\t\tBEFORE: "); P_(BALLOC); P_(BMEM); P(BCAP);
                }

                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                    mX.shrink_to_fit();
                } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

                const Int64 AALLOC = oa.numAllocations();
                const Int64 AMEM   = oa.numBytesInUse();
                const Int64 ACAP   = X.capacity();

                if (veryVerbose) {
                    printf("\t\tAFTER : "); P_(AALLOC); P_(AMEM); P(ACAP);
                }

                ASSERT(   X == Y);
                ASSERT(AMEM <= BMEM);
                ASSERT(ACAP <= BCAP);

                // +2 to account for the allocation (if any) that throws in the
                // above exception testing block.

#if defined(BDE_BUILD_TARGET_EXC)
                const Int64 EXC_EXTRA = 2;
#else
                const Int64 EXC_EXTRA = 1;
#endif

                ASSERTV(BALLOC, AALLOC,
                        AALLOC == BALLOC || AALLOC == BALLOC + EXC_EXTRA);

                {
                    bslma::TestAllocatorMonitor oam(&oa);

                    BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                        mX.shrink_to_fit();
                    } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

                    ASSERT(   X == Y);
                    ASSERT(AMEM == oa.numBytesInUse());
                    ASSERT(ACAP == (Int64) X.capacity());

                    ASSERT(oam.isTotalSame());
                }

                if (X.empty()) {
                    break;
                }

                if (DELTA >= X.size()) {
                    mX.clear();
                    mY.clear();
                }
                else if (eraseAtFront) {
                    mX.erase(X.cbegin(), X.cbegin() + DELTA);
                    mY.erase(Y.cbegin(), Y.cbegin() + DELTA);
                }
                else {
                    mX.erase(X.cend() - DELTA, X.cend());
                    mY.erase(Y.cend() - DELTA, Y.cend());
                }

                ASSERT(X == Y);
                ASSERT(0 == da.numBlocksTotal());
            }
            ASSERT(X.empty() && Y.empty());
            ASSERT(0 == da.numBlocksTotal());

            ASSERT(minimumBytesInUse == oa.numBytesInUse());
            ASSERT(minimumCapacity   == (Int64) X.capacity());
        }
    }
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE, ALLOC>::testCase30()
{
    // ------------------------------------------------------------------------
    // TESTING METHODS TAKING INITIALIZER LISTS
    //
    // Concerns:
    //: 1 The methods that take an initializer list (constructor, assignment
    //:   operator, 'assign', and 'insert') simply forward to other already
    //:   tested methods.  We are interested here only in ensuring that the
    //:   forwarding is working -- not retesting already verified
    //:   functionality.
    //
    // Plan:
    //: TBD
    //
    // Testing:
    //   deque(initializer_list<T>, const A& = A());
    //   deque& operator=(initializer_list<T>);
    //   void assign(initializer_list<T>);
    //   iterator insert(const_iterator pos, initializer_list<T>);
    // ------------------------------------------------------------------------

#if defined(BSLS_COMPILERFEATURES_SUPPORT_GENERALIZED_INITIALIZERS)
    const TestValues V;

    bslma::TestAllocator da("default", veryVeryVeryVerbose);
    bslma::DefaultAllocatorGuard dag(&da);

    if (verbose) printf("\nTesting constructor with initializer lists.\n");
    {
        const struct {
            int                          d_line;      // source line number
            std::initializer_list<TYPE>  d_list;      // source list
            const char                  *d_result_p;  // expected result
        } DATA[] = {
                //line          list             result
                //----          ----             ------
                { L_,   {                  },        ""   },
                { L_,   { V[0]             },       "A"   },
                { L_,   { V[0], V[0]       },      "AA"   },
                { L_,   { V[1], V[0]       },      "BA"   },
                { L_,   { V[0], V[1], V[2] },     "ABC"   },
                { L_,   { V[0], V[1], V[0] },     "ABA"   },
        };
        enum { NUM_DATA = sizeof DATA / sizeof *DATA };

        bslma::TestAllocator oa("object", veryVeryVeryVerbose);
        ALLOC                xoa(&oa);
        bslma::TestAllocatorMonitor dam(&da);

        for (int ti = 0; ti < NUM_DATA; ++ti) {
            ASSERT(0 == oa.numBytesInUse());

            bslma::TestAllocator scratch("scratch", veryVeryVeryVerbose);
            ALLOC                xscratch(&scratch);

            Obj mY(xscratch);  const Obj& Y = gg(&mY, DATA[ti].d_result_p);

            BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                if (veryVeryVerbose) { T_ T_ Q(ExceptionTestBody) }

                Obj mX(DATA[ti].d_list, xoa);  const Obj& X = mX;

                ASSERTV(Y, X, Y == X);

                ASSERT(&oa == X.get_allocator());

            } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

            ASSERT(&scratch == Y.get_allocator());
        }
        ASSERT(dam.isTotalSame());
    }

    {
        const struct {
            int                          d_line;      // source line number
            std::initializer_list<TYPE>  d_list;      // source list
            const char                  *d_result_p;  // expected result
        } DATA[] = {
                //line          list             result
                //----          ----             ------
                { L_,   {                  },        ""   },
                { L_,   { V[0]             },       "A"   },
                { L_,   { V[0], V[0]       },      "AA"   },
                { L_,   { V[1], V[0]       },      "BA"   },
                { L_,   { V[0], V[1], V[2] },     "ABC"   },
                { L_,   { V[0], V[1], V[0] },     "ABA"   },
        };
        enum { NUM_DATA = sizeof DATA / sizeof *DATA };

        bslma::TestAllocator oa("object", veryVeryVeryVerbose);
        bslma::TestAllocatorMonitor dam(&da);

        for (int ti = 0; ti < NUM_DATA; ++ti) {
            ASSERT(0 == oa.numBytesInUse());

            bslma::TestAllocator scratch("scratch", veryVeryVeryVerbose);
            ALLOC                xscratch(&scratch);

            Obj mY(xscratch);  const Obj& Y = gg(&mY, DATA[ti].d_result_p);

            BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                if (veryVeryVerbose) { T_ T_ Q(ExceptionTestBody) }

                Obj mX(DATA[ti].d_list);  const Obj& X = mX;

                ASSERTV(Y, X, Y == X);

                ASSERT(&da == X.get_allocator());

            } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

            ASSERT(&scratch == Y.get_allocator());
        }
        ASSERT(dam.isTotalUp());
    }

    {
        const struct {
            int                          d_line;      // source line number
            std::initializer_list<TYPE>  d_list;      // source list
            const char                  *d_result_p;  // expected result
        } DATA[] = {
                //line          list             result
                //----          ----             ------
                { L_,   {                  },        ""   },
                { L_,   { V[0]             },       "A"   },
                { L_,   { V[0], V[0]       },      "AA"   },
                { L_,   { V[1], V[0]       },      "BA"   },
                { L_,   { V[0], V[1], V[2] },     "ABC"   },
                { L_,   { V[0], V[1], V[0] },     "ABA"   },
        };
        enum { NUM_DATA = sizeof DATA / sizeof *DATA };

        bslma::TestAllocator oa("object", veryVeryVeryVerbose);
        bslma::TestAllocatorMonitor dam(&da);

        for (int ti = 0; ti < NUM_DATA; ++ti) {
            ASSERT(0 == oa.numBytesInUse());

            bslma::TestAllocator scratch("scratch", veryVeryVeryVerbose);
            ALLOC                xscratch(&scratch);

            Obj mY(xscratch);  const Obj& Y = gg(&mY, DATA[ti].d_result_p);

            BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                if (veryVeryVerbose) { T_ T_ Q(ExceptionTestBody) }

                Obj mX = DATA[ti].d_list;  const Obj& X = mX;

                ASSERTV(Y, X, Y == X);

                ASSERT(&da == X.get_allocator());

            } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

            ASSERT(&scratch == Y.get_allocator());
        }
        ASSERT(dam.isTotalUp());
    }

    if (verbose) printf("\nTesting 'operator=' with initializer lists.\n");
    {
        const struct {
            int                          d_line;      // source line number
            const char                  *d_spec_p;    // target string
            std::initializer_list<TYPE>  d_list;      // source list
            const char                  *d_result_p;  // expected result
        } DATA[] = {
                //line  lhs            list                result
                //----  ------         ----                ------
                { L_,   "",       {                  },        ""   },
                { L_,   "",       { V[0]             },       "A"   },
                { L_,   "A",      {                  },        ""   },
                { L_,   "A",      { V[1]             },       "B"   },
                { L_,   "A",      { V[0], V[1]       },      "AB"   },
                { L_,   "A",      { V[1], V[2]       },      "BC"   },
                { L_,   "AB",     {                  },        ""   },
                { L_,   "AB",     { V[0], V[1], V[2] },     "ABC"   },
                { L_,   "AB",     { V[2], V[3], V[4] },     "CDE"   },
        };
        enum { NUM_DATA = sizeof DATA / sizeof *DATA };

        bslma::TestAllocator oa("object", veryVeryVeryVerbose);
        ALLOC                xoa(&oa);
        bslma::TestAllocatorMonitor dam(&da);

        for (int ti = 0; ti < NUM_DATA; ++ti) {
            ASSERT(0 == oa.numBytesInUse());

            bslma::TestAllocator scratch("scratch", veryVeryVeryVerbose);
            ALLOC                xscratch(&scratch);

            Obj mY(xscratch);  const Obj& Y = gg(&mY, DATA[ti].d_result_p);

            Obj mX(xoa);  const Obj& X = gg(&mX, DATA[ti].d_spec_p);

            BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                if (veryVeryVerbose) { T_ T_ Q(ExceptionTestBody) }

                Obj *mR = &(mX = DATA[ti].d_list);
                ASSERTV(mR, &mX, mR == &mX);
                ASSERTV(Y,    X,  Y == X);

            } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
        }
        ASSERT(dam.isTotalSame());
    }

    if (verbose) printf("\nTesting 'assign' with initializer lists.\n");
    {
        const struct {
            int                          d_line;      // source line number
            const char                  *d_spec_p;    // target string
            std::initializer_list<TYPE>  d_list;      // source list
            const char                  *d_result_p;  // expected result
        } DATA[] = {
                //line  lhs            list                result
                //----  ------         ----                ------
                { L_,   "",       {                  },        ""   },
                { L_,   "",       { V[0]             },       "A"   },
                { L_,   "A",      {                  },        ""   },
                { L_,   "A",      { V[1]             },       "B"   },
                { L_,   "A",      { V[0], V[1]       },      "AB"   },
                { L_,   "A",      { V[1], V[2]       },      "BC"   },
                { L_,   "AB",     {                  },        ""   },
                { L_,   "AB",     { V[0], V[1], V[2] },     "ABC"   },
                { L_,   "AB",     { V[2], V[3], V[4] },     "CDE"   },
        };
        enum { NUM_DATA = sizeof DATA / sizeof *DATA };

        bslma::TestAllocator oa("object", veryVeryVeryVerbose);
        ALLOC                xoa(&oa);
        bslma::TestAllocatorMonitor dam(&da);

        for (int ti = 0; ti < NUM_DATA; ++ti) {
            ASSERT(0 == oa.numBytesInUse());

            bslma::TestAllocator scratch("scratch", veryVeryVeryVerbose);
            ALLOC                xscratch(&scratch);

            Obj mY(xscratch);  const Obj& Y = gg(&mY, DATA[ti].d_result_p);

            Obj mX(xoa);  const Obj& X = gg(&mX, DATA[ti].d_spec_p);

            BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                if (veryVeryVerbose) { T_ T_ Q(ExceptionTestBody) }

                mX.assign(DATA[ti].d_list);
                ASSERTV(Y, X, Y == X);

            } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
        }
        ASSERT(dam.isTotalSame());
    }

    if (verbose) printf("\nTesting 'insert' with initializer lists.\n");
    {
        const struct {
            int                          d_line;      // source line number
            const char                  *d_spec_p;    // target string
            int                          d_pos;       // position to insert
            std::initializer_list<TYPE>  d_list;      // source list
            const char                  *d_result_p;  // expected result
        } DATA[] = {
                //line  source   pos   list                result
                //----  ------   ---   ----                ------
                { L_,   "",      -1,   {                  },        ""   },
                { L_,   "",      99,   { V[0]             },       "A"   },
                { L_,   "A",      0,   {                  },       "A"   },
                { L_,   "A",      0,   { V[1]             },      "BA"   },
                { L_,   "A",      1,   { V[1]             },      "AB"   },
                { L_,   "AB",     0,   {                  },      "AB"   },
                { L_,   "AB",     0,   { V[0], V[1]       },    "ABAB"   },
                { L_,   "AB",     1,   { V[1], V[2]       },    "ABCB"   },
                { L_,   "AB",     2,   { V[0], V[1], V[2] },   "ABABC"   },
                { L_,   "ABC",    0,   { V[3]             },    "DABC"   },
        };
        enum { NUM_DATA = sizeof DATA / sizeof *DATA };

        bslma::TestAllocator        oa("object", veryVeryVeryVerbose);
        ALLOC                       xoa(&oa);
        bslma::TestAllocatorMonitor dam(&da);

        for (int ti = 0; ti < NUM_DATA; ++ti) {
            const int LINE = DATA[ti].d_line;
            const int POS  = DATA[ti].d_pos;

            ASSERT(0 == oa.numBytesInUse());

            bslma::TestAllocator scratch("scratch", veryVeryVeryVerbose);
            ALLOC                xscratch(&scratch);

            Obj mY(xscratch);  const Obj& Y = gg(&mY, DATA[ti].d_result_p);

            Obj mX(xoa);  const Obj& X = gg(&mX, DATA[ti].d_spec_p);

            const size_t index = -1 == POS ? 0 : 99 == POS ? X.size() : POS;
            iterator result = mX.insert(X.begin() + index, DATA[ti].d_list);
            ASSERTV(LINE,  result == X.begin() + index);
            ASSERTV(LINE, X, Y, X == Y);
        }
        ASSERT(dam.isTotalSame());
    }
#endif
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase29()
{
    // ------------------------------------------------------------------------
    // TESTING 'emplace'
    //   Note that the forwarding of arguments is tested in 'testCase29a'; all
    //   other functionality is tested in this function.
    //
    // Concerns:
    //: 1 A new element is inserted at the indicated position in the container
    //:   with the relative order of the existing elements remaining unchanged.
    //:
    //: 2 The returned iterator has the expected value.
    //:
    //: 3 The internal memory management system is hooked up properly so that
    //:   *all* internally allocated memory draws from a user-supplied
    //:   allocator whenever one is specified.
    //:
    //: 4 Insertion is exception neutral w.r.t. memory allocation.
    //:
    //: 5 There is no effect on the validity of references to elements of the
    //:   container if the insertion is at the front or the back.  (TBD not yet
    //:   tested)
    //
    // Plan:
    //: 1 Using the table-based approach, specify a set of initial container
    //:   values, insertion positions, element values (for insertion), and
    //:   expected (post-insertion) container values.
    //:
    //:   1 For each row from P-1, create a container having the specified
    //:     initial value and emplace the specified element value at the
    //:     specified position.
    //:
    //:   2 Verify that the values of the container and the returned iterator
    //:     are as expected.                                            (C-1-2)
    //:
    //:   3 Verify all allocations are from the object's allocator.       (C-3)
    //:
    //: 2 Repeat P-1 under the presence of exceptions.                    (C-4)
    //
    // Testing:
    //   iterator emplace(const_iterator pos, Args&&... args);
    // ------------------------------------------------------------------------

    const TestValues VALUES;

    static const struct {
        int         d_line;       // source line number
        const char *d_spec_p;     // specification string
        int         d_pos;
        char        d_element;    // specification string
        const char *d_results_p;  // expected element values
    } DATA[] = {
        //line  spec                pos    element   results
        //----  --------------      ---    --------   -----------------
        { L_,   "",                 -1,    'Z',      "Z"                    },
        { L_,   "",                 99,    'Z',      "Z"                    },
        { L_,   "A",                 0,    'Z',      "ZA"                   },
        { L_,   "A",                 1,    'Z',      "AZ"                   },
        { L_,   "A",                99,    'Z',      "AZ"                   },
        { L_,   "AB",                0,    'B',      "BAB"                  },
        { L_,   "AB",                1,    'Z',      "AZB"                  },
        { L_,   "AB",                2,    'A',      "ABA"                  },
        { L_,   "AB",               99,    'Z',      "ABZ"                  },
        { L_,   "CAB",               0,    'A',      "ACAB"                 },
        { L_,   "CAB",               1,    'B',      "CBAB"                 },
        { L_,   "CAB",               2,    'C',      "CACB"                 },
        { L_,   "CAB",               3,    'Z',      "CABZ"                 },
        { L_,   "CAB",              99,    'Z',      "CABZ"                 },
        { L_,   "CABD",              0,    'Z',      "ZCABD"                },
        { L_,   "CABD",              1,    'Z',      "CZABD"                },
        { L_,   "CABD",              2,    'B',      "CABBD"                },
        { L_,   "CABD",              3,    'Z',      "CABZD"                },
        { L_,   "CABD",              4,    'B',      "CABDB"                },
        { L_,   "CABD",             99,    'A',      "CABDA"                },
        { L_,   "HGFEDCBA",          0,    'Z',      "ZHGFEDCBA"            },
        { L_,   "HGFEDCBA",          1,    'Z',      "HZGFEDCBA"            },
        { L_,   "HGFEDCBA",          7,    'Z',      "HGFEDCBZA"            },
        { L_,   "HGFEDCBA",          8,    'Z',      "HGFEDCBAZ"            },

        // back-end-loaded specs -- 'gg' does 'push_back' for '[A-Z]'

        { L_,   "ABCDEFGHIJKLMNOPQRSTUVWXY",
                                     0,    'Z',
               "ZABCDEFGHIJKLMNOPQRSTUVWXY"                                 },
        { L_,   "ABCDEFGHIJKLMNOPQRSTUVWXY",
                                    10,    'Z',
                "ABCDEFGHIJZKLMNOPQRSTUVWXY"                                },
        { L_,   "ABCDEFGHIJKLMNOPQRSTUVWXY",
                                    13,    'Z',
                "ABCDEFGHIJKLMZNOPQRSTUVWXY"                                },
        { L_,   "ABCDEFGHIJKLMNOPQRSTUVWXY",
                                    20,    'Z',
                "ABCDEFGHIJKLMNOPQRSTZUVWXY"                                },
        { L_,   "ABCDEFGHIJKLMNOPQRSTUVWXY",
                                    99,    'Z',
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"                                },

        // front-end-loaded specs -- 'gg' does 'push_front' for '[a-z]'

        { L_,   "yxwvutsrqponmlkjihgfedcba",
                                     0,    'Z',
               "ZABCDEFGHIJKLMNOPQRSTUVWXY"                                 },
        { L_,   "yxwvutsrqponmlkjihgfedcba",
                                    10,    'Z',
                "ABCDEFGHIJZKLMNOPQRSTUVWXY"                                },
        { L_,   "yxwvutsrqponmlkjihgfedcba",
                                    13,    'Z',
                "ABCDEFGHIJKLMZNOPQRSTUVWXY"                                },
        { L_,   "yxwvutsrqponmlkjihgfedcba",
                                    20,    'Z',
                "ABCDEFGHIJKLMNOPQRSTZUVWXY"                                },
        { L_,   "yxwvutsrqponmlkjihgfedcba",
                                    99,    'Z',
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"                                }
    };
    enum { NUM_DATA = sizeof DATA / sizeof *DATA };

    if (verbose) printf("\nTesting 'emplace' without exceptions.\n");
    {
        for (size_t ti = 0; ti < NUM_DATA; ++ti) {
            const int         LINE     = DATA[ti].d_line;
            const char *const SPEC     = DATA[ti].d_spec_p;
            const int         POS      = DATA[ti].d_pos;
            const char        ELEMENT  = DATA[ti].d_element;
            const char *const EXPECTED = DATA[ti].d_results_p;
            const size_t      SIZE     = strlen(SPEC);

            bslma::TestAllocator da("default", veryVeryVeryVerbose);
            bslma::TestAllocator oa("object",  veryVeryVeryVerbose);
            ALLOC                xoa(&oa);

            bslma::DefaultAllocatorGuard dag(&da);

            Obj mX(xoa);  const Obj& X = gg(&mX, SPEC);

            if (veryVerbose) { T_ P_(LINE) P(X) }

            // -------------------------------------------------------
            // Verify any attribute allocators are installed properly.
            // -------------------------------------------------------

            ASSERTV(LINE, xoa == X.get_allocator());

            ASSERTV(LINE, SIZE, X.size(), SIZE == X.size());

            const Int64 BB = oa.numBlocksTotal();
            const Int64 B  = oa.numBlocksInUse();

            if (veryVerbose) { printf("\t\tBEFORE: "); P_(BB); P(B); }

            const size_t index = -1 == POS ? 0 : 99 == POS ? X.size() :POS;

            CIter position = -1 == POS ? X.cbegin()
                                       : 99 == POS
                                         ? X.cend() : X.cbegin() + POS;

            iterator result = mX.emplace(position, VALUES[ELEMENT - 'A']);

            if (veryVerbose) { T_ P_(LINE) P_(ELEMENT) P(X) }

            const Int64 AA = oa.numBlocksTotal();
            const Int64 A  = oa.numBlocksInUse();

            if (veryVerbose) { printf("\t\tAFTER : "); P_(AA); P(A);}

            ASSERTV(LINE, SIZE, X.size(), SIZE + 1 == X.size());
            ASSERTV(LINE,  result == X.begin() + index);
            ASSERTV(LINE, *result == VALUES[ELEMENT - 'A']);

            TestValues exp(EXPECTED);
            ASSERTV(LINE, 0 == verifyContainer(X, exp, SIZE + 1));
        }
    }

    // There is no strong exception guarantee unless the insertion is at the
    // the front or the back, so we install the guard conditionally.

    if (verbose) printf("\nTesting 'emplace' with injected exceptions.\n");
    {
        for (size_t ti = 0; ti < NUM_DATA; ++ti) {
            const int         LINE     = DATA[ti].d_line;
            const char *const SPEC     = DATA[ti].d_spec_p;
            const int         POS      = DATA[ti].d_pos;
            const char        ELEMENT  = DATA[ti].d_element;
            const char *const EXPECTED = DATA[ti].d_results_p;
            const size_t      SIZE     = strlen(SPEC);

            if (veryVerbose) { T_ P_(LINE) P_(SPEC) P(EXPECTED) }

            bslma::TestAllocator oa("object", veryVeryVeryVerbose);
            ALLOC                xoa(&oa);

            BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                const Int64 AL = oa.allocationLimit();
                oa.setAllocationLimit(-1);

                Obj mX(xoa);  const Obj &X = gg(&mX, SPEC);

                ASSERTV(LINE, SIZE, X.size(), SIZE == X.size());

                oa.setAllocationLimit(AL);

                const size_t index =
                                    -1 == POS ? 0 : 99 == POS ? X.size() : POS;

                // The strong exception guarantee is in effect only if
                // inserting at the front or the back.

                ExceptionProctor<Obj, ALLOC> proctor(
                                          0 == index || SIZE == index ? &X : 0,
                                          LINE);

                CIter position = -1 == POS ? X.cbegin()
                                           : 99 == POS
                                             ? X.cend() : X.cbegin() + POS;

                iterator result = mX.emplace(position, VALUES[ELEMENT - 'A']);

                ASSERTV(LINE, SIZE, X.size(), SIZE + 1 == X.size());
                ASSERTV(LINE, SIZE,  result == X.begin() + index);
                ASSERTV(LINE, SIZE, *result == VALUES[ELEMENT - 'A']);

                TestValues exp(EXPECTED);
                ASSERTV(LINE, 0 == verifyContainer(X, exp, SIZE + 1));

                proctor.release();
            } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
        }
    }
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE, ALLOC>::testCase29a()
{
    // ------------------------------------------------------------------------
    // TESTING FORWARDING OF ARGUMENTS WITH 'emplace'
    //
    // Concerns:
    //: 1 'emplace' correctly forwards arguments to the constructor of the
    //:   value type, up to 10 arguments, the max number of arguments provided
    //:   for C++03 compatibility.  Note that only the forwarding of arguments
    //:   is tested in this function; all other functionality is tested in
    //:  'testCase29'.
    //:
    //: 2 'emplace' is exception neutral w.r.t. memory allocation.
    //
    // Plan:
    //: 1 This test makes material use of template method 'testCase29a_RunTest'
    //:   with the first integer template parameter indicating the number of
    //:   arguments to use, and the next 10 integer template parameters
    //:   indicating '0' for copy, '1' for move, and '2' for not-applicable
    //:   (i.e., beyond the number of arguments).  'testCase29a_RunTest' takes
    //:   two arguments: an address providing modifiable access to a container
    //:   and a 'const_iterator' indicating the desired emplacement position.
    //:
    //:   1 Create 10 argument values with their own argument-specific
    //:     allocator.
    //:
    //:   2 Based on the (first) template parameter indicating the number of
    //:     arguments to pass, call 'emplace' with the corresponding argument
    //:     values, performing an explicit move of the argument if so indicated
    //:     by the template parameter corresponding to the argument, all in the
    //:     presence of injected exceptions.
    //:
    //:   3 Verify that the argument values were passed correctly.
    //:
    //:   4 Verify that the allocator was forwarded correctly.
    //:
    //:   5 Verify that the move-state for each argument is as expected.
    //:
    //:   6 Verify that the emplaced value was inserted at the correct position
    //:     in the container.
    //:
    //:   7 Verify that the returned iterator has the expected value.
    //:
    //: 2 Create a container with it's own object-specific allocator and
    //:   populate it with four elements (see P-4).
    //:
    //: 3 Call 'testCase29a_RunTest' in various configurations:
    //:   1 For 1..10 arguments, call with the move flag set to '1' and then
    //:     with the move flag set to '0'.
    //:
    //:   2 For 1, 2, 3, and 10 arguments, call with move flags set to '0',
    //:     '1', and each move flag set independently.
    //:
    //: 4 (white-box) For each combination of template arguments, call
    //:   'testCase29a_RunTest' twice, once to emplace at 'cend() - 1' and once
    //:   to emplace at 'cbegin() + 1'.  Note that emplacements at 'cend()' and
    //:   'cbegin()' forward to the (already-tested) 'emplace_back' and
    //:   'emplace_front' methods, respectively.  Also note that there are two
    //:   additional code paths in 'emplace' according to whether the
    //:   emplacement position is nearer to 'cend()' or 'cbegin()'.
    //
    // Testing:
    //   iterator emplace(const_iterator pos, Args&&... args);
    // ------------------------------------------------------------------------

#ifndef BSL_DO_NOT_TEST_MOVE_FORWARDING
    // 'emplace' toward the back, just short of 'emplace_back'

    if (verbose)
        printf("\nTesting emplace 1..10 args, move=1 (toward back)"
               "\n------------------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);

        testCase29a_RunTest< 0,2,2,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 1,1,2,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 2,1,1,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 3,1,1,1,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 4,1,1,1,1,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 5,1,1,1,1,1,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 6,1,1,1,1,1,1,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 7,1,1,1,1,1,1,1,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 8,1,1,1,1,1,1,1,1,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 9,1,1,1,1,1,1,1,1,1,2>(&mX, X.cend() - 1);
        testCase29a_RunTest<10,1,1,1,1,1,1,1,1,1,1>(&mX, X.cend() - 1);
    }

    if (verbose)
        printf("\nTesting emplace 1..10 args, move=0 (toward back)"
               "\n------------------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);

        testCase29a_RunTest< 0,2,2,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 1,0,2,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 2,0,0,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 3,0,0,0,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 4,0,0,0,0,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 5,0,0,0,0,0,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 6,0,0,0,0,0,0,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 7,0,0,0,0,0,0,0,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 8,0,0,0,0,0,0,0,0,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 9,0,0,0,0,0,0,0,0,0,2>(&mX, X.cend() - 1);
        testCase29a_RunTest<10,0,0,0,0,0,0,0,0,0,0>(&mX, X.cend() - 1);
    }

    if (verbose) printf("\nTesting emplace with 0 args (toward back)"
                        "\n-----------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);
        testCase29a_RunTest<0,2,2,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
    }

    if (verbose) printf("\nTesting emplace with 1 args (toward back)"
                        "\n-----------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);
        testCase29a_RunTest<1,0,2,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest<1,1,2,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
    }

    if (verbose) printf("\nTesting emplace with 2 args (toward back)"
                        "\n-----------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);
        testCase29a_RunTest<2,0,0,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest<2,1,0,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest<2,0,1,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest<2,1,1,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
    }

    if (verbose) printf("\nTesting emplace with 3 args (toward back)"
                        "\n-----------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);
        testCase29a_RunTest<3,0,0,0,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest<3,1,0,0,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest<3,0,1,0,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest<3,0,0,1,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest<3,1,1,1,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
    }

    if (verbose) printf("\nTesting emplace with 10 args (toward back)"
                        "\n------------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);
        testCase29a_RunTest<10,0,0,0,0,0,0,0,0,0,0>(&mX, X.cend() - 1);
        testCase29a_RunTest<10,1,0,0,0,0,0,0,0,0,0>(&mX, X.cend() - 1);
        testCase29a_RunTest<10,0,1,0,0,0,0,0,0,0,0>(&mX, X.cend() - 1);
        testCase29a_RunTest<10,0,0,1,0,0,0,0,0,0,0>(&mX, X.cend() - 1);
        testCase29a_RunTest<10,0,0,0,1,0,0,0,0,0,0>(&mX, X.cend() - 1);
        testCase29a_RunTest<10,0,0,0,0,1,0,0,0,0,0>(&mX, X.cend() - 1);
        testCase29a_RunTest<10,0,0,0,0,0,1,0,0,0,0>(&mX, X.cend() - 1);
        testCase29a_RunTest<10,0,0,0,0,0,0,1,0,0,0>(&mX, X.cend() - 1);
        testCase29a_RunTest<10,0,0,0,0,0,0,0,1,0,0>(&mX, X.cend() - 1);
        testCase29a_RunTest<10,0,0,0,0,0,0,0,0,1,0>(&mX, X.cend() - 1);
        testCase29a_RunTest<10,0,0,0,0,0,0,0,0,0,1>(&mX, X.cend() - 1);
        testCase29a_RunTest<10,1,1,1,1,1,1,1,1,1,1>(&mX, X.cend() - 1);
    }

    // 'emplace' toward the front, just short of 'emplace_front'

    if (verbose)
        printf("\nTesting emplace 1..10 args, move=1 (toward front)"
               "\n-------------------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);

        testCase29a_RunTest< 0,2,2,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 1,1,2,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 2,1,1,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 3,1,1,1,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 4,1,1,1,1,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 5,1,1,1,1,1,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 6,1,1,1,1,1,1,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 7,1,1,1,1,1,1,1,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 8,1,1,1,1,1,1,1,1,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 9,1,1,1,1,1,1,1,1,1,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<10,1,1,1,1,1,1,1,1,1,1>(&mX, X.cbegin() + 1);
    }

    if (verbose)
        printf("\nTesting emplace 1..10 args, move=0 (toward front)"
               "\n-------------------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);

        testCase29a_RunTest< 0,2,2,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 1,0,2,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 2,0,0,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 3,0,0,0,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 4,0,0,0,0,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 5,0,0,0,0,0,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 6,0,0,0,0,0,0,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 7,0,0,0,0,0,0,0,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 8,0,0,0,0,0,0,0,0,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 9,0,0,0,0,0,0,0,0,0,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<10,0,0,0,0,0,0,0,0,0,0>(&mX, X.cbegin() + 1);
    }

    if (verbose) printf("\nTesting emplace with 0 args (toward front)"
                        "\n------------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);
        testCase29a_RunTest<0,2,2,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
    }

    if (verbose) printf("\nTesting emplace with 1 args (toward front)"
                        "\n------------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);
        testCase29a_RunTest<1,0,2,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<1,1,2,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
    }

    if (verbose) printf("\nTesting emplace with 2 args (toward front)"
                        "\n------------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);
        testCase29a_RunTest<2,0,0,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<2,1,0,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<2,0,1,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<2,1,1,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
    }

    if (verbose) printf("\nTesting emplace with 3 args (toward front)"
                        "\n------------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);
        testCase29a_RunTest<3,0,0,0,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<3,1,0,0,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<3,0,1,0,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<3,0,0,1,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<3,1,1,1,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
    }

    if (verbose) printf("\nTesting emplace with 10 args (toward front)"
                        "\n-------------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);
        testCase29a_RunTest<10,0,0,0,0,0,0,0,0,0,0>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<10,1,0,0,0,0,0,0,0,0,0>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<10,0,1,0,0,0,0,0,0,0,0>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<10,0,0,1,0,0,0,0,0,0,0>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<10,0,0,0,1,0,0,0,0,0,0>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<10,0,0,0,0,1,0,0,0,0,0>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<10,0,0,0,0,0,1,0,0,0,0>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<10,0,0,0,0,0,0,1,0,0,0>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<10,0,0,0,0,0,0,0,1,0,0>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<10,0,0,0,0,0,0,0,0,1,0>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<10,0,0,0,0,0,0,0,0,0,1>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<10,1,1,1,1,1,1,1,1,1,1>(&mX, X.cbegin() + 1);
    }
#else
    // 'emplace' toward the back, just short of 'emplace_back'

    if (verbose)
        printf("\nTesting emplace 1..10 args, move=0 (toward back)"
               "\n------------------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);

        testCase29a_RunTest< 0,2,2,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 1,0,2,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 2,0,0,2,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 3,0,0,0,2,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 4,0,0,0,0,2,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 5,0,0,0,0,0,2,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 6,0,0,0,0,0,0,2,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 7,0,0,0,0,0,0,0,2,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 8,0,0,0,0,0,0,0,0,2,2>(&mX, X.cend() - 1);
        testCase29a_RunTest< 9,0,0,0,0,0,0,0,0,0,2>(&mX, X.cend() - 1);
        testCase29a_RunTest<10,0,0,0,0,0,0,0,0,0,0>(&mX, X.cend() - 1);
    }

    // 'emplace' toward the front, just short of 'emplace_front'

    if (verbose)
        printf("\nTesting emplace 1..10 args, move=0 (toward front)"
               "\n-------------------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  const Obj& X = mX;  mX.resize(4);

        testCase29a_RunTest< 0,2,2,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 1,0,2,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 2,0,0,2,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 3,0,0,0,2,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 4,0,0,0,0,2,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 5,0,0,0,0,0,2,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 6,0,0,0,0,0,0,2,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 7,0,0,0,0,0,0,0,2,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 8,0,0,0,0,0,0,0,0,2,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest< 9,0,0,0,0,0,0,0,0,0,2>(&mX, X.cbegin() + 1);
        testCase29a_RunTest<10,0,0,0,0,0,0,0,0,0,0>(&mX, X.cbegin() + 1);
    }
#endif
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase28()
{
    // ------------------------------------------------------------------------
    // TESTING 'emplace_front(Args&&...)' AND 'emplace_back(Args&&...)'
    //   Note that the forwarding of arguments is tested in 'testCase28a' and
    //   'testCase28b'; all other functionality is tested in this function.
    //
    // Concerns:
    //: 1 For 'emplace_back', a new element is added to the back of the
    //:   container with the order of the existing elements in the container
    //:   remaining unchanged.
    //:
    //: 2 For 'emplace_front', a new element is added to the front of the
    //:   container with the order of the existing elements in the container
    //:   remaining unchanged.
    //:
    //: 3 'emplace_back' and 'emplace_front' return a reference to the inserted
    //:   element.
    //:
    //: 4 The internal memory management system is hooked up properly so that
    //:   *all* internally allocated memory draws from a user-supplied
    //:   allocator whenever one is specified.
    //:
    //: 5 'emplace_back' and 'emplace_front' provide the strong exception
    //:   guarantee.
    //:
    //: 6 There is no effect on the validity of references to elements of the
    //:   container.  (TBD not yet tested)
    //
    // Plan:
    //: 1 For 'emplace_back', create objects of varying sizes, then append an
    //:   additional element.                                       (C-1, 3..4)
    //:
    //:   1 Verify that the element was added to the back of the container and
    //:     that the contents of the container is as expected.            (C-1)
    //:
    //:   2 Verify that returned reference points to the inserted value.  (C-3)
    //:
    //:   3 Verify all allocations are from the object's allocator.       (C-4)
    //:
    //: 2 Repeat P-1 under the presence of exceptions.                    (C-5)
    //:
    //: 3 Repeat P-1..2 for 'emplace_front', but instead verify that the new
    //:   element is added to the front of the container (P-1.1).      (C-2..5)
    //
    // Testing:
    //   reference emplace_back(Args&&... args);
    //   reference emplace_front(Args&&... args);
    // ------------------------------------------------------------------------

    const TestValues VALUES;

    // testing 'emplace_back'
    {
        static const struct {
            int         d_line;       // source line number
            const char *d_spec_p;     // specification string
            char        d_element;    // specification string
            const char *d_results_p;  // expected element values
        } DATA[] = {
            //line  spec            element          results
            //----  --------------  --------         ---------------------
            { L_,   "",                 'A',         "A"                     },
            { L_,   "A",                'A',         "AA"                    },
            { L_,   "A",                'B',         "AB"                    },
            { L_,   "B",                'A',         "BA"                    },
            { L_,   "AB",               'A',         "ABA"                   },
            { L_,   "BC",               'D',         "BCD"                   },
            { L_,   "BCA",              'Z',         "BCAZ"                  },
            { L_,   "CAB",              'C',         "CABC"                  },
            { L_,   "CDAB",             'D',         "CDABD"                 },
            { L_,   "DABC",             'Z',         "DABCZ"                 },
            { L_,   "ABCDE",            'Z',         "ABCDEZ"                },
            { L_,   "EDCBA",            'E',         "EDCBAE"                },
            { L_,   "ABCDEA",           'E',         "ABCDEAE"               },
            { L_,   "ABCDEAB",          'Z',         "ABCDEABZ"              },
            { L_,   "BACDEABC",         'D',         "BACDEABCD"             },
            { L_,   "CBADEABCD",        'Z',         "CBADEABCDZ"            },
            { L_,   "CBADEABCDAB",      'B',         "CBADEABCDABB"          },
            { L_,   "CBADEABCDABC",     'Z',         "CBADEABCDABCZ"         },
            { L_,   "CBADEABCDABCDE",   'B',         "CBADEABCDABCDEB"       },
            { L_,   "CBADEABCDABCDEA",  'E',         "CBADEABCDABCDEAE"      },
            { L_,   "CBADEABCDABCDEAB", 'Z',         "CBADEABCDABCDEABZ"     },
            { L_,   "ABCDEFGHIJKLMNOPQRSTUVWXY",
                                        'Z',    "ABCDEFGHIJKLMNOPQRSTUVWXYZ" }
        };
        enum { NUM_DATA = sizeof DATA / sizeof *DATA };

        if (verbose) printf("\nTesting 'emplace_back' without exceptions.\n");
        {
            for (size_t ti = 0; ti < NUM_DATA; ++ti) {
                const int         LINE     = DATA[ti].d_line;
                const char *const SPEC     = DATA[ti].d_spec_p;
                const char        ELEMENT  = DATA[ti].d_element;
                const char *const EXPECTED = DATA[ti].d_results_p;
                const size_t      SIZE     = strlen(SPEC);

                bslma::TestAllocator da("default", veryVeryVeryVerbose);
                bslma::TestAllocator oa("object",  veryVeryVeryVerbose);
                ALLOC                xoa(&oa);

                bslma::DefaultAllocatorGuard dag(&da);

                Obj mX(xoa);  const Obj& X = gg(&mX, SPEC);

                if (veryVerbose) { T_ P_(LINE) P(X) }

                // -------------------------------------------------------
                // Verify any attribute allocators are installed properly.
                // -------------------------------------------------------

                ASSERTV(LINE, xoa == X.get_allocator());

                ASSERTV(LINE, SIZE, X.size(), SIZE == X.size());

                const TYPE& RESULT = mX.emplace_back(VALUES[ELEMENT - 'A']);

                if (veryVerbose) { T_ P_(LINE) P_(ELEMENT) P(X) }

                const TYPE *ADDRESS_OF_RESULT = bsls::Util::addressOf(RESULT);
                const TYPE *ADDRESS_OF_LAST_ELEMENT =
                                               bsls::Util::addressOf(X.back());

                ASSERTV(LINE, SIZE, X.size(), SIZE + 1  == X.size());
                ASSERTV(LINE, SIZE,
                        ADDRESS_OF_LAST_ELEMENT == ADDRESS_OF_RESULT);

                TestValues exp(EXPECTED);
                ASSERTV(LINE, 0 == verifyContainer(X, exp, SIZE + 1));
            }
        }

        if (verbose)
            printf("\nTesting 'emplace_back' with injected exceptions.\n");
        {
            for (size_t ti = 0; ti < NUM_DATA; ++ti) {
                const int         LINE     = DATA[ti].d_line;
                const char *const SPEC     = DATA[ti].d_spec_p;
                const char        ELEMENT  = DATA[ti].d_element;
                const char *const EXPECTED = DATA[ti].d_results_p;
                const size_t      SIZE     = strlen(SPEC);

                bslma::TestAllocator oa("object",  veryVeryVeryVerbose);
                ALLOC                xoa(&oa);

                Obj mX(xoa);  const Obj &X = gg(&mX, SPEC);

                ASSERTV(LINE, SIZE, X.size(), SIZE == X.size());

                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                    // 'emplace_back' provides the strong exception guarantee.

                    ExceptionProctor<Obj, ALLOC> proctor(&X, L_);

                    const TYPE& RESULT =
                                        mX.emplace_back(VALUES[ELEMENT - 'A']);

                    const TYPE *ADDRESS_OF_RESULT =
                                                 bsls::Util::addressOf(RESULT);
                    const TYPE *ADDRESS_OF_LAST_ELEMENT =
                                               bsls::Util::addressOf(X.back());

                    ASSERTV(LINE, SIZE, X.size(), SIZE + 1 == X.size());
                    ASSERTV(LINE, SIZE,
                            ADDRESS_OF_LAST_ELEMENT == ADDRESS_OF_RESULT);

                    TestValues exp(EXPECTED);
                    ASSERTV(LINE, 0 == verifyContainer(X, exp, SIZE + 1));

                    proctor.release();
                } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
            }
        }
    }

    // testing 'emplace_front'
    {
        static const struct {
            int         d_line;       // source line number
            const char *d_spec_p;     // specification string
            char        d_element;    // specification string
            const char *d_results_p;  // expected element values
        } DATA[] = {
            //line  spec            element          results
            //----  --------------  --------         ---------------------
            { L_,   "",                 'A',         "A"                     },
            { L_,   "a",                'A',         "AA"                    },
            { L_,   "a",                'B',         "BA"                    },
            { L_,   "b",                'A',         "AB"                    },
            { L_,   "ab",               'A',         "ABA"                   },
            { L_,   "bc",               'D',         "DCB"                   },
            { L_,   "bca",              'Z',         "ZACB"                  },
            { L_,   "cab",              'C',         "CBAC"                  },
            { L_,   "cdab",             'D',         "DBADC"                 },
            { L_,   "dabc",             'Z',         "ZCBAD"                 },
            { L_,   "abcde",            'Z',         "ZEDCBA"                },
            { L_,   "edcba",            'E',         "EABCDE"                },
            { L_,   "abcdea",           'E',         "EAEDCBA"               },
            { L_,   "abcdeab",          'Z',         "ZBAEDCBA"              },
            { L_,   "bacdeabc",         'D',         "DCBAEDCAB"             },
            { L_,   "cbadeabcd",        'Z',         "ZDCBAEDABC"            },
            { L_,   "cbadeabcdab",      'B',         "BBADCBAEDABC"          },
            { L_,   "cbadeabcdabc",     'Z',         "ZCBADCBAEDABC"         },
            { L_,   "cbadeabcdabcde",   'B',         "BEDCBADCBAEDABC"       },
            { L_,   "cbadeabcdabcdea",  'E',         "EAEDCBADCBAEDABC"      },
            { L_,   "cbadeabcdabcdeab", 'Z',         "ZBAEDCBADCBAEDABC"     },
            { L_,   "yxwvutsrqponmlkjihgfedcba",
                                        'Z',    "ZABCDEFGHIJKLMNOPQRSTUVWXY" }
        };
        enum { NUM_DATA = sizeof DATA / sizeof *DATA };

        if (verbose) printf("\nTesting 'emplace_front' without exceptions.\n");
        {
            for (size_t ti = 0; ti < NUM_DATA; ++ti) {
                const int         LINE     = DATA[ti].d_line;
                const char *const SPEC     = DATA[ti].d_spec_p;
                const char        ELEMENT  = DATA[ti].d_element;
                const char *const EXPECTED = DATA[ti].d_results_p;
                const size_t      SIZE     = strlen(SPEC);

                bslma::TestAllocator da("default", veryVeryVeryVerbose);
                bslma::TestAllocator oa("object",  veryVeryVeryVerbose);
                ALLOC                xoa(&oa);

                bslma::DefaultAllocatorGuard dag(&da);

                Obj mX(xoa);  const Obj& X = gg(&mX, SPEC);

                if (veryVerbose) { T_ P_(LINE) P(X) }

                // -------------------------------------------------------
                // Verify any attribute allocators are installed properly.
                // -------------------------------------------------------

                ASSERTV(LINE, xoa == X.get_allocator());

                ASSERTV(LINE, SIZE, X.size(), SIZE == X.size());

                const TYPE& RESULT = mX.emplace_front(VALUES[ELEMENT - 'A']);

                if (veryVerbose) { T_ P_(LINE) P_(ELEMENT) P(X) }

                const TYPE *ADDRESS_OF_RESULT = bsls::Util::addressOf(RESULT);
                const TYPE *ADDRESS_OF_FIRST_ELEMENT =
                                              bsls::Util::addressOf(X.front());

                ASSERTV(LINE, SIZE, X.size(), SIZE + 1 == X.size());
                ASSERTV(LINE, SIZE, X.size(),
                        ADDRESS_OF_FIRST_ELEMENT == ADDRESS_OF_RESULT);

                TestValues exp(EXPECTED);
                ASSERTV(LINE, 0 == verifyContainer(X, exp, SIZE + 1));
            }
        }

        if (verbose)
            printf("\nTesting 'emplace_front' with injected exceptions.\n");
        {
            for (size_t ti = 0; ti < NUM_DATA; ++ti) {
                const int         LINE     = DATA[ti].d_line;
                const char *const SPEC     = DATA[ti].d_spec_p;
                const char        ELEMENT  = DATA[ti].d_element;
                const char *const EXPECTED = DATA[ti].d_results_p;
                const size_t      SIZE     = strlen(SPEC);

                bslma::TestAllocator oa("object", veryVeryVeryVerbose);
                ALLOC                xoa(&oa);

                Obj mX(xoa);  const Obj &X = gg(&mX, SPEC);

                ASSERTV(LINE, SIZE, X.size(), SIZE == X.size());

                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                    // 'emplace_front' provides the strong exception guarantee.

                    ExceptionProctor<Obj, ALLOC> proctor(&X, L_);

                    const TYPE& RESULT =
                                       mX.emplace_front(VALUES[ELEMENT - 'A']);

                    const TYPE *ADDRESS_OF_RESULT =
                                                 bsls::Util::addressOf(RESULT);
                    const TYPE *ADDRESS_OF_FIRST_ELEMENT =
                                              bsls::Util::addressOf(X.front());

                    ASSERTV(LINE, SIZE, X.size(), SIZE + 1 == X.size());
                    ASSERTV(LINE, SIZE, X.size(),
                            ADDRESS_OF_FIRST_ELEMENT == ADDRESS_OF_RESULT);

                    TestValues exp(EXPECTED);
                    ASSERTV(LINE, 0 == verifyContainer(X, exp, SIZE + 1));

                    proctor.release();
                } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
            }
        }
    }
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE, ALLOC>::testCase28a()
{
    // ------------------------------------------------------------------------
    // TESTING FORWARDING OF ARGUMENTS WITH 'emplace_back'
    //
    // Concerns:
    //: 1 'emplace_back' correctly forwards arguments to the constructor of the
    //:   value type, up to 10 arguments, the max number of arguments provided
    //:   for C++03 compatibility.  Note that only the forwarding of arguments
    //:   is tested in this function; all other functionality is tested in
    //:  'testCase28'.
    //:
    //: 2 'emplace_back' provides the strong exception guarantee.
    //
    // Plan:
    //: 1 This test makes material use of template method 'testCase28a_RunTest'
    //:   with the first integer template parameter indicating the number of
    //:   arguments to use, the next 10 integer template parameters indicating
    //:   '0' for copy, '1' for move, and '2' for not-applicable (i.e., beyond
    //:   the number of arguments), and taking as the only argument an address
    //:   providing modifiable access to a container.
    //:
    //:   1 Create 10 argument values with their own argument-specific
    //:     allocator.
    //:
    //:   2 Based on the (first) template parameter indicating the number of
    //:     arguments to pass, call 'emplace_back' with the corresponding
    //:     argument values, performing an explicit move of the argument if so
    //:     indicated by the template parameter corresponding to the argument,
    //:     all in the presence of injected exceptions.
    //:
    //:   3 Verify that the argument values were passed correctly.
    //:
    //:   4 Verify that the allocator was forwarded correctly.
    //:
    //:   5 Verify that the move-state for each argument is as expected.
    //:
    //:   6 Verify that the emplaced value was appended to the back of the
    //:     container.
    //:
    //: 2 Create a container with it's own object-specific allocator.
    //:
    //: 3 Call 'testCase28a_RunTest' in various configurations:
    //:   1 For 1..10 arguments, call with the move flag set to '1' and then
    //:     with the move flag set to '0'.
    //:
    //:   2 For 1, 2, 3, and 10 arguments, call with move flags set to '0',
    //:     '1', and each move flag set independently.
    //
    // Testing:
    //   void emplace_back(Args&&... args);
    // ------------------------------------------------------------------------

#ifndef BSL_DO_NOT_TEST_MOVE_FORWARDING
    if (verbose) printf("\nTesting emplace_back 1..10 args, move=1"
                        "\n---------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28a_RunTest< 0,2,2,2,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 1,1,2,2,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 2,1,1,2,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 3,1,1,1,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 4,1,1,1,1,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 5,1,1,1,1,1,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 6,1,1,1,1,1,1,2,2,2,2>(&mX);
        testCase28a_RunTest< 7,1,1,1,1,1,1,1,2,2,2>(&mX);
        testCase28a_RunTest< 8,1,1,1,1,1,1,1,1,2,2>(&mX);
        testCase28a_RunTest< 9,1,1,1,1,1,1,1,1,1,2>(&mX);
        testCase28a_RunTest<10,1,1,1,1,1,1,1,1,1,1>(&mX);
    }

    if (verbose) printf("\nTesting emplace_back 1..10 args, move=0"
                        "\n---------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28a_RunTest< 0,2,2,2,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 1,0,2,2,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 2,0,0,2,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 3,0,0,0,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 4,0,0,0,0,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 5,0,0,0,0,0,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 6,0,0,0,0,0,0,2,2,2,2>(&mX);
        testCase28a_RunTest< 7,0,0,0,0,0,0,0,2,2,2>(&mX);
        testCase28a_RunTest< 8,0,0,0,0,0,0,0,0,2,2>(&mX);
        testCase28a_RunTest< 9,0,0,0,0,0,0,0,0,0,2>(&mX);
        testCase28a_RunTest<10,0,0,0,0,0,0,0,0,0,0>(&mX);
    }

    if (verbose) printf("\nTesting emplace_back with 0 args"
                        "\n--------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28a_RunTest<0,2,2,2,2,2,2,2,2,2,2>(&mX);
    }

    if (verbose) printf("\nTesting emplace_back with 1 args"
                        "\n--------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28a_RunTest<1,0,2,2,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest<1,1,2,2,2,2,2,2,2,2,2>(&mX);
    }

    if (verbose) printf("\nTesting emplace_back with 2 args"
                        "\n--------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28a_RunTest<2,0,0,2,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest<2,1,0,2,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest<2,0,1,2,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest<2,1,1,2,2,2,2,2,2,2,2>(&mX);
    }

    if (verbose) printf("\nTesting emplace_back with 3 args"
                        "\n--------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28a_RunTest<3,0,0,0,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest<3,1,0,0,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest<3,0,1,0,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest<3,0,0,1,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest<3,1,1,1,2,2,2,2,2,2,2>(&mX);
    }

    if (verbose) printf("\nTesting emplace_back with 10 args"
                        "\n---------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28a_RunTest<10,0,0,0,0,0,0,0,0,0,0>(&mX);
        testCase28a_RunTest<10,1,0,0,0,0,0,0,0,0,0>(&mX);
        testCase28a_RunTest<10,0,1,0,0,0,0,0,0,0,0>(&mX);
        testCase28a_RunTest<10,0,0,1,0,0,0,0,0,0,0>(&mX);
        testCase28a_RunTest<10,0,0,0,1,0,0,0,0,0,0>(&mX);
        testCase28a_RunTest<10,0,0,0,0,1,0,0,0,0,0>(&mX);
        testCase28a_RunTest<10,0,0,0,0,0,1,0,0,0,0>(&mX);
        testCase28a_RunTest<10,0,0,0,0,0,0,1,0,0,0>(&mX);
        testCase28a_RunTest<10,0,0,0,0,0,0,0,1,0,0>(&mX);
        testCase28a_RunTest<10,0,0,0,0,0,0,0,0,1,0>(&mX);
        testCase28a_RunTest<10,0,0,0,0,0,0,0,0,0,1>(&mX);
        testCase28a_RunTest<10,1,1,1,1,1,1,1,1,1,1>(&mX);
    }
#else
    if (verbose) printf("\nTesting emplace_back 1..10 args, move=0"
                        "\n---------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28a_RunTest< 0,2,2,2,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 1,0,2,2,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 2,0,0,2,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 3,0,0,0,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 4,0,0,0,0,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 5,0,0,0,0,0,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 6,0,0,0,0,0,0,2,2,2,2>(&mX);
        testCase28a_RunTest< 7,0,0,0,0,0,0,0,2,2,2>(&mX);
        testCase28a_RunTest< 8,0,0,0,0,0,0,0,0,2,2>(&mX);
        testCase28a_RunTest< 9,0,0,0,0,0,0,0,0,0,2>(&mX);
        testCase28a_RunTest<10,0,0,0,0,0,0,0,0,0,0>(&mX);
    }
#endif
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE, ALLOC>::testCase28b()
{
    // ------------------------------------------------------------------------
    // TESTING FORWARDING OF ARGUMENTS WITH 'emplace_front'
    //
    // Concerns:
    //: 1 'emplace_front' correctly forwards arguments to the constructor of
    //:   the value type, up to 10 arguments, the max number of arguments
    //:   provided for C++03 compatibility.  Note that only the forwarding of
    //:   arguments is tested in this function; all other functionality is
    //:   tested in 'testCase28'.
    //:
    //: 2 'emplace_front' provides the strong exception guarantee.
    //
    // Plan:
    //: 1 This test makes material use of template method 'testCase28b_RunTest'
    //:   with the first integer template parameter indicating the number of
    //:   arguments to use, the next 10 integer template parameters indicating
    //:   '0' for copy, '1' for move, and '2' for not-applicable (i.e., beyond
    //:   the number of arguments), and taking as the only argument an address
    //:   providing modifiable access to a container.
    //:
    //:   1 Create 10 argument values with their own argument-specific
    //:     allocator.
    //:
    //:   2 Based on the (first) template parameter indicating the number of
    //:     arguments to pass, call 'emplace_front' with the corresponding
    //:     argument values, performing an explicit move of the argument if so
    //:     indicated by the template parameter corresponding to the argument,
    //:     all in the presence of injected exceptions.
    //:
    //:   3 Verify that the argument values were passed correctly.
    //:
    //:   4 Verify that the allocator was forwarded correctly.
    //:
    //:   5 Verify that the move-state for each argument is as expected.
    //:
    //:   6 Verify that the emplaced value was prepended to the front of the
    //:     container.
    //:
    //: 2 Create a container with it's own object-specific allocator.
    //:
    //: 3 Call 'testCase28b_RunTest' in various configurations:
    //:   1 For 1..10 arguments, call with the move flag set to '1' and then
    //:     with the move flag set to '0'.
    //:
    //:   2 For 1, 2, 3, and 10 arguments, call with move flags set to '0',
    //:     '1', and each move flag set independently.
    //
    // Testing:
    //   void emplace_front(Args&&... args);
    // ------------------------------------------------------------------------

#ifndef BSL_DO_NOT_TEST_MOVE_FORWARDING
    if (verbose) printf("\nTesting emplace_front 1..10 args, move=1"
                        "\n----------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28b_RunTest< 0,2,2,2,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 1,1,2,2,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 2,1,1,2,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 3,1,1,1,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 4,1,1,1,1,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 5,1,1,1,1,1,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 6,1,1,1,1,1,1,2,2,2,2>(&mX);
        testCase28b_RunTest< 7,1,1,1,1,1,1,1,2,2,2>(&mX);
        testCase28b_RunTest< 8,1,1,1,1,1,1,1,1,2,2>(&mX);
        testCase28b_RunTest< 9,1,1,1,1,1,1,1,1,1,2>(&mX);
        testCase28b_RunTest<10,1,1,1,1,1,1,1,1,1,1>(&mX);
    }

    if (verbose) printf("\nTesting emplace_front 1..10 args, move=0"
                        "\n----------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28b_RunTest< 0,2,2,2,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 1,0,2,2,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 2,0,0,2,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 3,0,0,0,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 4,0,0,0,0,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 5,0,0,0,0,0,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 6,0,0,0,0,0,0,2,2,2,2>(&mX);
        testCase28b_RunTest< 7,0,0,0,0,0,0,0,2,2,2>(&mX);
        testCase28b_RunTest< 8,0,0,0,0,0,0,0,0,2,2>(&mX);
        testCase28b_RunTest< 9,0,0,0,0,0,0,0,0,0,2>(&mX);
        testCase28b_RunTest<10,0,0,0,0,0,0,0,0,0,0>(&mX);
    }

    if (verbose) printf("\nTesting emplace_front with 0 args"
                        "\n---------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28b_RunTest<0,2,2,2,2,2,2,2,2,2,2>(&mX);
    }

    if (verbose) printf("\nTesting emplace_front with 1 args"
                        "\n---------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28b_RunTest<1,0,2,2,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest<1,1,2,2,2,2,2,2,2,2,2>(&mX);
    }

    if (verbose) printf("\nTesting emplace_front with 2 args"
                        "\n---------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28b_RunTest<2,0,0,2,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest<2,1,0,2,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest<2,0,1,2,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest<2,1,1,2,2,2,2,2,2,2,2>(&mX);
    }

    if (verbose) printf("\nTesting emplace_front with 3 args"
                        "\n---------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28b_RunTest<3,0,0,0,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest<3,1,0,0,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest<3,0,1,0,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest<3,0,0,1,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest<3,1,1,1,2,2,2,2,2,2,2>(&mX);
    }

    if (verbose) printf("\nTesting emplace_front with 10 args"
                        "\n----------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28b_RunTest<10,0,0,0,0,0,0,0,0,0,0>(&mX);
        testCase28b_RunTest<10,1,0,0,0,0,0,0,0,0,0>(&mX);
        testCase28b_RunTest<10,0,1,0,0,0,0,0,0,0,0>(&mX);
        testCase28b_RunTest<10,0,0,1,0,0,0,0,0,0,0>(&mX);
        testCase28b_RunTest<10,0,0,0,1,0,0,0,0,0,0>(&mX);
        testCase28b_RunTest<10,0,0,0,0,1,0,0,0,0,0>(&mX);
        testCase28b_RunTest<10,0,0,0,0,0,1,0,0,0,0>(&mX);
        testCase28b_RunTest<10,0,0,0,0,0,0,1,0,0,0>(&mX);
        testCase28b_RunTest<10,0,0,0,0,0,0,0,1,0,0>(&mX);
        testCase28b_RunTest<10,0,0,0,0,0,0,0,0,1,0>(&mX);
        testCase28b_RunTest<10,0,0,0,0,0,0,0,0,0,1>(&mX);
        testCase28b_RunTest<10,1,1,1,1,1,1,1,1,1,1>(&mX);
    }
#else
    if (verbose) printf("\nTesting emplace_front 1..10 args, move=0"
                        "\n----------------------------------------\n");
    {
        bslma::TestAllocator oa("object", veryVeryVeryVerbose);

        Obj mX(&oa);  mX.resize(4);

        testCase28b_RunTest< 0,2,2,2,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 1,0,2,2,2,2,2,2,2,2,2>(&mX);
        testCase28a_RunTest< 2,0,0,2,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 3,0,0,0,2,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 4,0,0,0,0,2,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 5,0,0,0,0,0,2,2,2,2,2>(&mX);
        testCase28b_RunTest< 6,0,0,0,0,0,0,2,2,2,2>(&mX);
        testCase28b_RunTest< 7,0,0,0,0,0,0,0,2,2,2>(&mX);
        testCase28b_RunTest< 8,0,0,0,0,0,0,0,0,2,2>(&mX);
        testCase28b_RunTest< 9,0,0,0,0,0,0,0,0,0,2>(&mX);
        testCase28b_RunTest<10,0,0,0,0,0,0,0,0,0,0>(&mX);
    }
#endif
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase27()
{
    // ------------------------------------------------------------------------
    // TESTING 'iterator insert(const_iterator pos, T&&)'
    //
    // Concerns:
    //: 1 A new element is inserted at the indicated position in the container
    //:   with the relative order of the existing elements remaining unchanged.
    //:
    //: 2 The returned iterator has the expected value.
    //:
    //: 3 The newly inserted item is move-inserted.
    //:
    //: 4 Internal memory management system is hooked up properly so that *all*
    //:   internally allocated memory draws from a user-supplied allocator
    //:   whenever one is specified.
    //:
    //: 5 Insertion is exception neutral w.r.t. memory allocation.
    //:
    //: 6 There is no effect on the validity of references to elements of the
    //:   container if the insertion is at the front or the back.  (TBD not yet
    //:   tested)
    //
    // Plan:
    //: 1 Using the table-based approach, specify a set of initial container
    //:   values, insertion positions, element values (for insertion), and
    //:   expected (post-insertion) container values.
    //:
    //:   1 For each row from P-1, create a container having the specified
    //:     initial value and move-insert the specified element value at the
    //:     specified position.
    //:
    //:   2 Verify that the values of the container and the returned iterator
    //:     are as expected.                                            (C-1-2)
    //:
    //:   3 Verify that the moved-into state for the new element is MOVED.(C-3)
    //:
    //:   4 Verify all allocations are from the object's allocator.       (C-4)
    //:
    //: 2 Repeat P-1 under the presence of exception                      (C-5)
    //
    // Testing:
    //   iterator insert(const_iterator pos, T&& rvalue);
    // ------------------------------------------------------------------------

    if (verbose) printf("TC 27: %s\n", bsls::NameOf<TYPE>().name());

    const TestValues VALUES;

    static const struct {
        int         d_line;       // source line number
        const char *d_spec_p;     // specification string
        int         d_pos;
        char        d_element;    // specification string
        const char *d_results_p;  // expected element values
    } DATA[] = {
        //line  spec                pos    element   results
        //----  --------------      ---    --------   -----------------
        { L_,   "",                 -1,    'Z',      "Z"                    },
        { L_,   "",                 99,    'Z',      "Z"                    },
        { L_,   "A",                -1,    'Z',      "ZA"                   },
        { L_,   "A",                 0,    'Z',      "ZA"                   },
        { L_,   "A",                 1,    'Z',      "AZ"                   },
        { L_,   "A",                99,    'Z',      "AZ"                   },
        { L_,   "AB",               -1,    'B',      "BAB"                  },
        { L_,   "AB",                0,    'B',      "BAB"                  },
        { L_,   "AB",                1,    'Z',      "AZB"                  },
        { L_,   "AB",                2,    'A',      "ABA"                  },
        { L_,   "AB",               99,    'Z',      "ABZ"                  },
        { L_,   "CAB",              -1,    'Z',      "ZCAB"                 },
        { L_,   "CAB",               0,    'A',      "ACAB"                 },
        { L_,   "CAB",               1,    'B',      "CBAB"                 },
        { L_,   "CAB",               2,    'C',      "CACB"                 },
        { L_,   "CAB",               3,    'Z',      "CABZ"                 },
        { L_,   "CAB",              99,    'Z',      "CABZ"                 },
        { L_,   "CABD",             -1,    'A',      "ACABD"                },
        { L_,   "CABD",              0,    'Z',      "ZCABD"                },
        { L_,   "CABD",              1,    'Z',      "CZABD"                },
        { L_,   "CABD",              2,    'B',      "CABBD"                },
        { L_,   "CABD",              3,    'Z',      "CABZD"                },
        { L_,   "CABD",              4,    'B',      "CABDB"                },
        { L_,   "CABD",             99,    'A',      "CABDA"                },
        { L_,   "HGFEDCBA",          0,    'Z',      "ZHGFEDCBA"            },
        { L_,   "HGFEDCBA",          1,    'Z',      "HZGFEDCBA"            },
        { L_,   "HGFEDCBA",          7,    'Z',      "HGFEDCBZA"            },
        { L_,   "HGFEDCBA",          8,    'Z',      "HGFEDCBAZ"            },

        // back-end-loaded specs -- 'gg' does 'push_back' for '[A-Z]'

        { L_,   "ABCDEFGHIJKLMNOPQRSTUVWXY",
                                     0,    'Z',
               "ZABCDEFGHIJKLMNOPQRSTUVWXY"                                 },
        { L_,   "ABCDEFGHIJKLMNOPQRSTUVWXY",
                                    10,    'Z',
                "ABCDEFGHIJZKLMNOPQRSTUVWXY"                                },
        { L_,   "ABCDEFGHIJKLMNOPQRSTUVWXY",
                                    13,    'Z',
                "ABCDEFGHIJKLMZNOPQRSTUVWXY"                                },
        { L_,   "ABCDEFGHIJKLMNOPQRSTUVWXY",
                                    20,    'Z',
                "ABCDEFGHIJKLMNOPQRSTZUVWXY"                                },
        { L_,   "ABCDEFGHIJKLMNOPQRSTUVWXY",
                                    99,    'Z',
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"                                },

        // front-end-loaded specs -- 'gg' does 'push_front' for '[a-z]'

        { L_,   "yxwvutsrqponmlkjihgfedcba",
                                     0,    'Z',
               "ZABCDEFGHIJKLMNOPQRSTUVWXY"                                 },
        { L_,   "yxwvutsrqponmlkjihgfedcba",
                                    10,    'Z',
                "ABCDEFGHIJZKLMNOPQRSTUVWXY"                                },
        { L_,   "yxwvutsrqponmlkjihgfedcba",
                                    13,    'Z',
                "ABCDEFGHIJKLMZNOPQRSTUVWXY"                                },
        { L_,   "yxwvutsrqponmlkjihgfedcba",
                                    20,    'Z',
                "ABCDEFGHIJKLMNOPQRSTZUVWXY"                                },
        { L_,   "yxwvutsrqponmlkjihgfedcba",
                                    99,    'Z',
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"                                }
    };
    const Int64 NUM_DATA = sizeof DATA / sizeof *DATA;

    if (verbose) printf("\nTesting 'insert' without exceptions.\n");
    {
        for (size_t ti = 0; ti < NUM_DATA; ++ti) {
            const int         LINE     = DATA[ti].d_line;
            const char *const SPEC     = DATA[ti].d_spec_p;
            const int         POS      = DATA[ti].d_pos;
            const char        ELEMENT  = DATA[ti].d_element;
            const char *const EXPECTED = DATA[ti].d_results_p;
            const size_t      SIZE     = strlen(SPEC);

            for (char cfg = 'a'; cfg <= 'b'; ++cfg) {
                const char CONFIG = cfg;

                bslma::TestAllocator da("default",   veryVeryVeryVerbose);
                bslma::TestAllocator oa("object",    veryVeryVeryVerbose);
                bslma::TestAllocator za("different", veryVeryVeryVerbose);
                ALLOC                xoa(&oa);

                bslma::DefaultAllocatorGuard dag(&da);

                Obj mX(xoa);  const Obj& X = gg(&mX, SPEC);

                bslma::TestAllocator *valAllocator = 0;
                switch (cfg) {
                  case 'a': {
                    // inserted value has same allocator
                    valAllocator = &oa;
                  } break;
                  case 'b': {
                    // inserted value has different allocator
                    valAllocator = &za;
                  } break;
                  default: {
                    ASSERTV(CONFIG, !"Bad allocator config.");
                  } return;                                           // RETURN
                }
                bslma::TestAllocator& sa = *valAllocator;
                ALLOC                 xsa(&sa);

                if (veryVerbose) { T_ P_(LINE) P_(CONFIG) P(X) }

                // -------------------------------------------------------
                // Verify any attribute allocators are installed properly.
                // -------------------------------------------------------

                ASSERTV(LINE, CONFIG, xoa == X.get_allocator());

                ASSERTV(LINE, CONFIG, SIZE, X.size(), SIZE == X.size());

                bsls::ObjectBuffer<ValueType> buffer;
                ValueType *valptr = buffer.address();
                TstFacility::emplace(valptr, ELEMENT, xsa);

                ASSERTV(SIZE, numNotMovedInto(X), SIZE == numNotMovedInto(X));

                const size_t index =
                                    -1 == POS ? 0 : 99 == POS ? X.size() : POS;

                CIter position = -1 == POS ? X.cbegin()
                                           : 99 == POS
                                             ? X.cend() : X.cbegin() + POS;

                iterator result = mX.insert(position, MoveUtil::move(*valptr));

                MoveState::Enum mState =
                                       TstFacility::getMovedFromState(*valptr);
                bslma::DestructionUtil::destroy(valptr);

                if (veryVerbose) { T_ P_(LINE) P_(CONFIG) P_(ELEMENT) P(X) }

                if (k_IS_WELL_BEHAVED && &oa != &sa) {
                    ASSERTV(mState, MoveState::e_NOT_MOVED == mState);
                }
                else {
                    ASSERTV(mState, MoveState::e_UNKNOWN   == mState
                                 || MoveState::e_MOVED     == mState);
                }

                ASSERTV(LINE, CONFIG, SIZE, X.size(), SIZE + 1 == X.size());
                ASSERTV(LINE, CONFIG,  result == X.begin() + index);
                ASSERTV(LINE, CONFIG, *result == VALUES[ELEMENT - 'A']);

                TestValues exp(EXPECTED);
                ASSERTV(LINE, CONFIG, 0 == verifyContainer(X, exp, SIZE + 1));
            }
        }
    }

    // There is no strong exception guarantee unless the insertion is at the
    // the front or the back, so we install the guard conditionally.

    if (verbose) printf("\nTesting 'insert' with injected exceptions.\n");
    {
        for (size_t ti = 0; ti < NUM_DATA; ++ti) {
            const int         LINE     = DATA[ti].d_line;
            const char *const SPEC     = DATA[ti].d_spec_p;
            const int         POS      = DATA[ti].d_pos;
            const char        ELEMENT  = DATA[ti].d_element;
            const char *const EXPECTED = DATA[ti].d_results_p;
            const size_t      SIZE     = strlen(SPEC);

            for (char cfg = 'a'; cfg <= 'b'; ++cfg) {
                const char CONFIG = cfg;

                bslma::TestAllocator da("default",   veryVeryVeryVerbose);
                bslma::TestAllocator oa("object",    veryVeryVeryVerbose);
                bslma::TestAllocator za("different", veryVeryVeryVerbose);
                ALLOC                xoa(&oa);

                bslma::DefaultAllocatorGuard dag(&da);

                bslma::TestAllocator *valAllocator = 0;
                switch (cfg) {
                  case 'a': {
                    // inserted value has same allocator
                    valAllocator = &oa;
                  } break;
                  case 'b': {
                    // inserted value has different allocator
                    valAllocator = &za;
                  } break;
                  default: {
                    ASSERTV(CONFIG, !"Bad allocator config.");
                  } return;                                           // RETURN
                }
                bslma::TestAllocator& sa = *valAllocator;
                ALLOC                 xsa(&sa);

                bslma::TestAllocator scratch("scratch", veryVeryVeryVerbose);
                ALLOC                xscratch(&scratch);

                const size_t index = -1 == POS ? 0 : 99 == POS ? SIZE : POS;

                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                    const Int64 AL = oa.allocationLimit();
                    oa.setAllocationLimit(-1);
                    Obj mX(xoa);  const Obj &X = gg(&mX, SPEC);

                    ASSERTV(LINE, SIZE, X.size(), SIZE == X.size());

                    bsls::ObjectBuffer<ValueType> buffer;
                    ValueType *valptr = buffer.address();
                    TstFacility::emplace(valptr, ELEMENT, xsa);
                    bslma::DestructorGuard<ValueType> guard(valptr);

                    oa.setAllocationLimit(AL);

                    Obj mZ(xscratch);  const Obj& Z = gg(&mZ, SPEC); (void) Z;

                    // The strong exception guarantee is in effect only if
                    // inserting at the front or the back.

                    ExceptionProctor<Obj, ALLOC> proctor(
                                          0 == index || SIZE == index ? &X : 0,
                                          MoveUtil::move(mZ),
                                          LINE);

                    CIter position = -1 == POS ? X.cbegin()
                                               : 99 == POS
                                                 ? X.cend() : X.cbegin() + POS;

                    iterator result = mX.insert(position,
                                                MoveUtil::move(*valptr));

                    proctor.release();

                    ASSERTV(LINE, SIZE, X.size(), SIZE + 1 == X.size());
                    ASSERTV(LINE,  result == X.begin() + index);
                    ASSERTV(LINE, *result == VALUES[ELEMENT - 'A']);

                    TestValues exp(EXPECTED);
                    ASSERTV(LINE, 0 == verifyContainer(X, exp, SIZE + 1));
                } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
            }
        }
    }
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase26()
{
    // ------------------------------------------------------------------------
    // TESTING 'push_front(T&&)' AND 'push_back(T&&)'
    //
    // Concerns:
    //: 1 For 'push_back', a new element is added to the back of the container
    //:   with the order of the existing elements in the container remaining
    //:   unchanged.
    //:
    //: 2 For 'push_front', a new element is added to the front of the
    //:   container with the order of the existing elements in the container
    //:   remaining unchanged.
    //:
    //: 3 The newly inserted item is move-inserted.
    //:
    //: 4 The internal memory management system is hooked up properly so that
    //:   *all* internally allocated memory draws from a user-supplied
    //:   allocator whenever one is specified.
    //:
    //: 5 'push_back' and 'push_front' provide the strong exception guarantee.
    //:
    //: 6 There is no effect on the validity of references to elements of the
    //:   container.  (TBD not yet tested)
    //
    // Plan:
    //: 1 For 'push_back', create objects of varying sizes, then append an
    //:   additional element.                                       (C-1, 3..4)
    //:
    //:   1 Verify that the element was added to the back of the container and
    //:     that the contents of the container is as expected.            (C-1)
    //:
    //:   2 Verify that the moved-into state for the new element is MOVED.(C-3)
    //:
    //:   3 Verify all allocations are from the object's allocator.       (C-4)
    //:
    //: 2 Repeat P-1 under the presence of exceptions.                    (C-5)
    //:
    //: 3 Repeat P-1..2 for 'push_front', but instead verify that the new
    //:   element is added to the front of the container (P-1.1).      (C-2..5)
    //
    // Testing:
    //   void push_front(T&& rvalue);
    //   void push_back(T&& rvalue);
    // ------------------------------------------------------------------------

    const char *name = bsls::NameOf<TYPE>();

    if (verbose) printf("TC 26: %s\n", name);

    const TestValues VALUES;

    // testing 'push_back'
    {
        static const struct {
            int         d_line;       // source line number
            const char *d_spec_p;     // specification string
            char        d_element;    // specification string
            const char *d_results_p;  // expected element values
        } DATA[] = {
            //line  spec            element          results
            //----  --------------  --------         ---------------------
            { L_,   "",                 'A',         "A"                     },
            { L_,   "A",                'A',         "AA"                    },
            { L_,   "A",                'B',         "AB"                    },
            { L_,   "B",                'A',         "BA"                    },
            { L_,   "AB",               'A',         "ABA"                   },
            { L_,   "BC",               'D',         "BCD"                   },
            { L_,   "BCA",              'Z',         "BCAZ"                  },
            { L_,   "CAB",              'C',         "CABC"                  },
            { L_,   "CDAB",             'D',         "CDABD"                 },
            { L_,   "DABC",             'Z',         "DABCZ"                 },
            { L_,   "ABCDE",            'Z',         "ABCDEZ"                },
            { L_,   "EDCBA",            'E',         "EDCBAE"                },
            { L_,   "ABCDEA",           'E',         "ABCDEAE"               },
            { L_,   "ABCDEAB",          'Z',         "ABCDEABZ"              },
            { L_,   "BACDEABC",         'D',         "BACDEABCD"             },
            { L_,   "CBADEABCD",        'Z',         "CBADEABCDZ"            },
            { L_,   "CBADEABCDAB",      'B',         "CBADEABCDABB"          },
            { L_,   "CBADEABCDABC",     'Z',         "CBADEABCDABCZ"         },
            { L_,   "CBADEABCDABCDE",   'B',         "CBADEABCDABCDEB"       },
            { L_,   "CBADEABCDABCDEA",  'E',         "CBADEABCDABCDEAE"      },
            { L_,   "CBADEABCDABCDEAB", 'Z',         "CBADEABCDABCDEABZ"     },
            { L_,   "ABCDEFGHIJKLMNOPQRSTUVWXY",
                                        'Z',    "ABCDEFGHIJKLMNOPQRSTUVWXYZ" }
        };
        const IntPtr NUM_DATA = sizeof DATA / sizeof *DATA;

        if (verbose) printf("\nTesting 'push_back' without exceptions.\n");
        {
            for (size_t ti = 0; ti < NUM_DATA; ++ti) {
                const int         LINE     = DATA[ti].d_line;
                const char *const SPEC     = DATA[ti].d_spec_p;
                const char        ELEMENT  = DATA[ti].d_element;
                const char *const EXPECTED = DATA[ti].d_results_p;
                const size_t      SIZE     = strlen(SPEC);

                for (char cfg = 'a'; cfg <= 'b'; ++cfg) {
                    const char CONFIG = cfg;

                    bslma::TestAllocator da("default",   veryVeryVeryVerbose);
                    bslma::TestAllocator oa("object",    veryVeryVeryVerbose);
                    bslma::TestAllocator za("different", veryVeryVeryVerbose);
                    ALLOC                xoa(&oa);

                    bslma::DefaultAllocatorGuard dag(&da);

                    Obj mX(xoa);  const Obj& X = gg(&mX, SPEC);

                    bslma::TestAllocator *valAllocator = 0;
                    switch (cfg) {
                      case 'a': {
                        // inserted value has same allocator
                        valAllocator = &oa;
                      } break;
                      case 'b': {
                        // inserted value has different allocator
                        valAllocator = &za;
                      } break;
                      default: {
                        ASSERTV(CONFIG, !"Bad allocator config.");
                      } return;                                       // RETURN
                    }
                    bslma::TestAllocator& sa = *valAllocator;
                    ALLOC                 xsa(&sa);
                    if (veryVerbose) { T_ P_(LINE) P_(CONFIG) P(X) }

                    // -------------------------------------------------------
                    // Verify any attribute allocators are installed properly.
                    // -------------------------------------------------------

                    ASSERTV(LINE, CONFIG, xoa == X.get_allocator());

                    ASSERTV(LINE, CONFIG, SIZE, X.size(), SIZE == X.size());

                    bsls::ObjectBuffer<ValueType> buffer;
                    ValueType *valptr = buffer.address();
                    TstFacility::emplace(valptr, ELEMENT, xsa);

                    ASSERTV(SIZE, numNotMovedInto(X),
                            SIZE == numNotMovedInto(X));

                    mX.push_back(MoveUtil::move(*valptr));

                    MoveState::Enum mState =
                                       TstFacility::getMovedFromState(*valptr);
                    bslma::DestructionUtil::destroy(valptr);

                    if (veryVerbose) { T_ P_(LINE) P_(CONFIG) P_(ELEMENT) P(X)}

                    if (k_IS_WELL_BEHAVED && &sa != &oa) {
                        ASSERTV(mState, CONFIG, name,
                                             MoveState::e_NOT_MOVED == mState);
                    }
                    else {
                        ASSERTV(mState, CONFIG, name,
                                               MoveState::e_UNKNOWN == mState
                                            || MoveState::e_MOVED   == mState);
                    }

                    if (0 != SIZE) {
                        ASSERTV(SIZE, numNotMovedInto(X),
                                SIZE == numNotMovedInto(X, 0, SIZE));
                    }

                    ASSERTV(LINE, CONFIG, SIZE, X.size(),
                            SIZE + 1 == X.size());

                    TestValues exp(EXPECTED);
                    ASSERTV(LINE, CONFIG,
                            0 == verifyContainer(X, exp, SIZE + 1));
                }
            }
        }

        if (verbose)
            printf("\nTesting 'push_back' with injected exceptions.\n");
        {
            for (size_t ti = 0; ti < NUM_DATA; ++ti) {
                const int         LINE     = DATA[ti].d_line;
                const char *const SPEC     = DATA[ti].d_spec_p;
                const char        ELEMENT  = DATA[ti].d_element;
                const char *const EXPECTED = DATA[ti].d_results_p;
                const size_t      SIZE     = strlen(SPEC);

                for (char cfg = 'a'; cfg <= 'b'; ++cfg) {
                    const char CONFIG = cfg;

                    bslma::TestAllocator da("default",   veryVeryVeryVerbose);
                    bslma::TestAllocator oa("object",    veryVeryVeryVerbose);
                    bslma::TestAllocator za("different", veryVeryVeryVerbose);
                    ALLOC                xoa(&oa);

                    bslma::DefaultAllocatorGuard dag(&da);

                    Obj mX(xoa);  const Obj& X = gg(&mX, SPEC);

                    bslma::TestAllocator *valAllocator = 0;
                    switch (cfg) {
                      case 'a': {
                        // inserted value has same allocator
                        valAllocator = &oa;
                      } break;
                      case 'b': {
                        // inserted value has different allocator
                        valAllocator = &za;
                      } break;
                      default: {
                        ASSERTV(CONFIG, !"Bad allocator config.");
                      } return;                                       // RETURN
                    }
                    bslma::TestAllocator& sa = *valAllocator;
                    ALLOC                 xsa(&sa);

                    ASSERTV(LINE, SIZE, X.size(), SIZE == X.size());

                    bslma::TestAllocator scratch("scratch",
                                                 veryVeryVeryVerbose);
                    ALLOC                xscratch(&scratch);

                    BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                        Obj mZ(xscratch);  const Obj& Z = gg(&mZ, SPEC);
                        ASSERTV(Z, X, Z == X);

                        // 'push_back' provides the strong exception guarantee.

                        ExceptionProctor<Obj, ALLOC>
                                           proctor(&X, MoveUtil::move(mZ), L_);

                        bsls::ObjectBuffer<ValueType> buffer;
                        ValueType *valptr = buffer.address();
                        TstFacility::emplace(valptr, ELEMENT, xsa);
                        bslma::DestructorGuard<ValueType> guard(valptr);

                        mX.push_back(MoveUtil::move(*valptr));

                        ASSERTV(LINE, SIZE, X.size(), SIZE + 1 == X.size());

                        TestValues exp(EXPECTED);
                        ASSERTV(LINE, 0 == verifyContainer(X, exp, SIZE + 1));

                        proctor.release();
                    } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
                }
            }
        }
    }

    // testing 'push_front'
    {
        static const struct {
            int         d_line;       // source line number
            const char *d_spec_p;     // specification string
            char        d_element;    // specification string
            const char *d_results_p;  // expected element values
        } DATA[] = {
            //line  spec            element          results
            //----  --------------  --------         ---------------------
            { L_,   "",                 'A',         "A"                     },
            { L_,   "a",                'A',         "AA"                    },
            { L_,   "a",                'B',         "BA"                    },
            { L_,   "b",                'A',         "AB"                    },
            { L_,   "ab",               'A',         "ABA"                   },
            { L_,   "bc",               'D',         "DCB"                   },
            { L_,   "bca",              'Z',         "ZACB"                  },
            { L_,   "cab",              'C',         "CBAC"                  },
            { L_,   "cdab",             'D',         "DBADC"                 },
            { L_,   "dabc",             'Z',         "ZCBAD"                 },
            { L_,   "abcde",            'Z',         "ZEDCBA"                },
            { L_,   "edcba",            'E',         "EABCDE"                },
            { L_,   "abcdea",           'E',         "EAEDCBA"               },
            { L_,   "abcdeab",          'Z',         "ZBAEDCBA"              },
            { L_,   "bacdeabc",         'D',         "DCBAEDCAB"             },
            { L_,   "cbadeabcd",        'Z',         "ZDCBAEDABC"            },
            { L_,   "cbadeabcdab",      'B',         "BBADCBAEDABC"          },
            { L_,   "cbadeabcdabc",     'Z',         "ZCBADCBAEDABC"         },
            { L_,   "cbadeabcdabcde",   'B',         "BEDCBADCBAEDABC"       },
            { L_,   "cbadeabcdabcdea",  'E',         "EAEDCBADCBAEDABC"      },
            { L_,   "cbadeabcdabcdeab", 'Z',         "ZBAEDCBADCBAEDABC"     },
            { L_,   "yxwvutsrqponmlkjihgfedcba",
                                        'Z',    "ZABCDEFGHIJKLMNOPQRSTUVWXY" }
        };
        const size_t NUM_DATA = sizeof DATA / sizeof *DATA;

        if (verbose) printf("\nTesting 'push_front' without exceptions.\n");
        {
            for (size_t ti = 0; ti < NUM_DATA; ++ti) {
                const int         LINE     = DATA[ti].d_line;
                const char *const SPEC     = DATA[ti].d_spec_p;
                const char        ELEMENT  = DATA[ti].d_element;
                const char *const EXPECTED = DATA[ti].d_results_p;
                const size_t      SIZE     = strlen(SPEC);

                for (char cfg = 'a'; cfg <= 'b'; ++cfg) {
                    const char CONFIG = cfg;

                    bslma::TestAllocator da("default",   veryVeryVeryVerbose);
                    bslma::TestAllocator oa("object",    veryVeryVeryVerbose);
                    bslma::TestAllocator za("different", veryVeryVeryVerbose);
                    ALLOC                xoa(&oa);

                    bslma::DefaultAllocatorGuard dag(&da);

                    Obj mX(xoa);  const Obj& X = gg(&mX, SPEC);

                    bslma::TestAllocator *valAllocator = 0;
                    switch (cfg) {
                      case 'a': {
                        // inserted value has same allocator
                        valAllocator = &oa;
                      } break;
                      case 'b': {
                        // inserted value has different allocator
                        valAllocator = &za;
                      } break;
                      default: {
                        ASSERTV(CONFIG, !"Bad allocator config.");
                      } return;                                       // RETURN
                    }
                    bslma::TestAllocator& sa = *valAllocator;
                    ALLOC                 xsa(&sa);

                    if (veryVerbose) { T_ P_(LINE) P_(CONFIG) P(X) }

                    // -------------------------------------------------------
                    // Verify any attribute allocators are installed properly.
                    // -------------------------------------------------------

                    ASSERTV(LINE, CONFIG, &oa == X.get_allocator());

                    ASSERTV(LINE, CONFIG, SIZE, X.size(), SIZE == X.size());

                    bsls::ObjectBuffer<ValueType> buffer;
                    ValueType *valptr = buffer.address();
                    TstFacility::emplace(valptr, ELEMENT, xsa);

                    ASSERTV(SIZE, numNotMovedInto(X),
                            SIZE == numNotMovedInto(X));

                    mX.push_front(MoveUtil::move(*valptr));

                    MoveState::Enum mState =
                                       TstFacility::getMovedFromState(*valptr);
                    bslma::DestructionUtil::destroy(valptr);

                    if (veryVerbose) { T_ P_(LINE) P_(CONFIG) P_(ELEMENT) P(X)}

                    if (k_IS_WELL_BEHAVED && &sa != &oa) {
                        ASSERTV(mState, CONFIG, name,
                                             MoveState::e_NOT_MOVED == mState);
                    }
                    else {
                        ASSERTV(mState, CONFIG, name,
                                               MoveState::e_UNKNOWN == mState
                                            || MoveState::e_MOVED   == mState);
                    }

                    ASSERTV(SIZE, numNotMovedInto(X),
                                  SIZE == numNotMovedInto(X, 1, SIZE + 1));

                    ASSERTV(LINE, CONFIG,
                            SIZE, X.size(), SIZE + 1 == X.size());

                    TestValues exp(EXPECTED);
                    ASSERTV(LINE, CONFIG,
                            0 == verifyContainer(X, exp, SIZE + 1));
                }
            }
        }

        if (verbose)
            printf("\nTesting 'push_front' with injected exceptions.\n");
        {
            for (size_t ti = 0; ti < NUM_DATA; ++ti) {
                const int         LINE     = DATA[ti].d_line;
                const char *const SPEC     = DATA[ti].d_spec_p;
                const char        ELEMENT  = DATA[ti].d_element;
                const char *const EXPECTED = DATA[ti].d_results_p;
                const size_t      SIZE     = strlen(SPEC);

                for (char cfg = 'a'; cfg <= 'b'; ++cfg) {
                    const char CONFIG = cfg;

                    bslma::TestAllocator da("default",   veryVeryVeryVerbose);
                    bslma::TestAllocator oa("object",    veryVeryVeryVerbose);
                    bslma::TestAllocator za("different", veryVeryVeryVerbose);
                    ALLOC                xoa(&oa);

                    bslma::DefaultAllocatorGuard dag(&da);

                    Obj mX(xoa);  const Obj& X = gg(&mX, SPEC);

                    bslma::TestAllocator *valAllocator = 0;
                    switch (cfg) {
                      case 'a': {
                        // inserted value has same allocator
                        valAllocator = &oa;
                      } break;
                      case 'b': {
                        // inserted value has different allocator
                        valAllocator = &za;
                      } break;
                      default: {
                        ASSERTV(CONFIG, !"Bad allocator config.");
                      } return;                                       // RETURN
                    }
                    bslma::TestAllocator& sa = *valAllocator;
                    ALLOC                 xsa(&sa);

                    ASSERTV(LINE, SIZE, X.size(), SIZE == X.size());

                    bslma::TestAllocator scratch("scratch",
                                                 veryVeryVeryVerbose);
                    ALLOC                xscratch(&scratch);

                    BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                        Obj mZ(xscratch);  const Obj& Z = gg(&mZ, SPEC);
                        ASSERTV(Z, X, Z == X);

                        // 'push_front' provides the strong exception
                        // guarantee.

                        ExceptionProctor<Obj, ALLOC>
                                           proctor(&X, MoveUtil::move(mZ), L_);

                        bsls::ObjectBuffer<ValueType> buffer;
                        ValueType *valptr = buffer.address();
                        TstFacility::emplace(valptr, ELEMENT, xsa);
                        bslma::DestructorGuard<ValueType> guard(valptr);

                        mX.push_front(MoveUtil::move(*valptr));

                        ASSERTV(LINE, SIZE, X.size(), SIZE + 1 == X.size());

                        TestValues exp(EXPECTED);
                        ASSERTV(LINE, 0 == verifyContainer(X, exp, SIZE + 1));

                        proctor.release();
                    } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
                }
            }
        }
    }
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase25_dispatch()
{
    // ------------------------------------------------------------------------
    // TESTING MOVE-ASSIGNMENT OPERATOR
    //
    // Concerns:
    //  TBD: the test does not yet cover the case where allocator propagation
    //       is enabled for move assignment (hard-coded to 'false') -- i.e.,
    //       parts of C-5..6 are currently not addressed.
    //
    //: 1 The signature and return type are standard.
    //:
    //: 2 The reference returned is to the target object (i.e., '*this').
    //:
    //: 3 The move-assignment operator can change the value of a modifiable
    //:   target object to that of any source object.
    //:
    //: 4 The object has its internal memory management system hooked up
    //:   properly so that *all* internally allocated memory draws from a
    //:   user-supplied allocator whenever one is specified.
    //:
    //: 5 If allocator propagation is not enabled for move-assignment, the
    //:   allocator address held by the target object is unchanged; otherwise,
    //:   the allocator address held by the target object is changed to that of
    //:   the source.
    //:
    //: 6 If allocator propagation is enabled for move-assignment, any memory
    //:   allocation from the original target allocator is released after the
    //:   operation has completed.
    //:
    //: 7 All elements in the target object are either move-assigned to or
    //:   destroyed.
    //:
    //: 8 The source object is left in a valid but unspecified state, and the
    //:   allocator address held by the original object is unchanged.
    //:
    //: 9 Subsequent changes to or destruction of the original object have no
    //:   effect on the move-assigned object and vice-versa.
    //:
    //:10 Assigning a source object having the default-constructed value
    //:   allocates no memory; assigning a value to a target object in the
    //:   default state does not allocate or deallocate any memory.
    //:
    //:11 Every object releases any allocated memory at destruction.
    //:
    //:12 Any memory allocation is exception neutral.
    //:
    //:13 Assigning an object to itself behaves as expected (alias-safety).
    //:
    // Plan:
    //: 1 Use the address of 'operator=' to initialize a member-function
    //:   pointer having the appropriate signature and return type for the
    //:   move-assignment operator defined in this component.             (C-1)
    //:
    //: 2 Iterate over a set of object values with substantial and varied
    //:   differences, ordered by increasing length, and create for each a
    //:   control object representing the source of the assignment, with its
    //:   own scratch allocator.
    //:
    //: 3 Iterate again over the same set of object values and create an object
    //:   representing the target of the assignment, with its own unique object
    //:   allocator.
    //:
    //: 4 In a loop consisting of two iterations, create a source object (a
    //:   copy of the control object in P-1) with 1) a different allocator than
    //:   that of target and 2) the same allocator as that of the target,
    //:
    //: 5 Call the move-assignment operator in the presence of exceptions
    //:   during memory allocations (using a 'bslma::Allocator' and varying
    //:   its allocation limit) and verify the following:                (C-12)
    //:
    //:   1 The address of the return value is the same as that of the target
    //:     object.                                                       (C-2)
    //:
    //:   2 The object being assigned to has the same value as that of the
    //:     source object before assignment (i.e., the control object).   (C-3)
    //:
    //:   3 CONTAINER-SPECIFIC NOTE: none
    //:
    //:   4 If the source and target objects use the same allocator, ensure
    //:     that there is no net increase in memory use from the common
    //:     allocator.  Also consider the following cases:
    //:
    //:     1 If the source object is empty, confirm that there are no bytes
    //:       currently in use from the common allocator.                (C-10)
    //:
    //:     2 If the target object is empty, confirm that there was no memory
    //:       change in memory usage.                                    (C-10)
    //:
    //:   5 If the source and target objects use different allocators, ensure
    //:     that each element in the source object is move-inserted into the
    //:     target object.                                                (C-7)
    //:
    //:   6 Ensure that the source, target, and control objects continue to
    //:     have the correct allocators and that all memory allocations come
    //:     from the appropriate allocator.                               (C-4)
    //:
    //:   7 Manipulate the source object (after assignment) to ensure that it
    //:     is in a valid state, destroy it, and then manipulate the target
    //:     object to ensure that it is in a valid state.                 (C-8)
    //:
    //:   8 Verify all memory is released when the source and target objects
    //:     are destroyed.                                               (C-11)
    //:
    //: 6 Use a test allocator installed as the default allocator to verify
    //:   that no memory is ever allocated from the default allocator.
    //
    // Testing:
    //   deque& operator=(deque&& rhs);
    // ------------------------------------------------------------------------

    const char *name = bsls::NameOf<TYPE>();

    if (verbose) printf("TC 25: %s\n", name);

    // Since this function is called with a variety of template arguments, it
    // is necessary to infer some things about our template arguments in order
    // to print a meaningful banner.

    const bool isPropagate =
                AllocatorTraits::propagate_on_container_move_assignment::value;
    const bool otherTraitsSet =
                AllocatorTraits::propagate_on_container_copy_assignment::value;

    // We can print the banner now:

    if (verbose) printf("%sTESTING MOVE ASSIGN '%s' OTHER:%c PROP:%c"
                                                                " ALLOC: %s\n",
                        veryVerbose ? "\n" : "",
                        NameOf<TYPE>().name(), otherTraitsSet ? 'T' : 'F',
                        isPropagate ? 'T' : 'F',
                        allocCategoryAsStr());

    // Assign the address of the function to a variable.
    {
        typedef Obj& (Obj::*OperatorMAg)(bslmf::MovableRef<Obj>);

        OperatorMAg op = &Obj::operator=;
        (void) op;  // quash potential compiler warning
    }

    // Create a test allocator and install it as the default.

    bslma::TestAllocator doa("default",   veryVeryVeryVerbose);
    bslma::TestAllocator ooa("object",    veryVeryVeryVerbose);
    bslma::TestAllocator zoa("other",     veryVeryVeryVerbose);
    bslma::TestAllocator soa("scratch",   veryVeryVeryVerbose);
    bslma::TestAllocator foa("footprint", veryVeryVeryVerbose);
    bslma::DefaultAllocatorGuard dag(&doa);

    ALLOC da(&doa);
    ALLOC oa(&ooa);
    ALLOC za(&zoa);
    ALLOC sa(&soa);

    // Check remaining properties of allocator to make sure they all match
    // 'otherTraitsSet'.

    BSLMF_ASSERT(otherTraitsSet ==
                          AllocatorTraits::propagate_on_container_swap::value);
    ASSERT((otherTraitsSet ? sa : da) ==
                   AllocatorTraits::select_on_container_copy_construction(sa));

    // Use a table of distinct object values and expected memory usage.

    enum { NUM_DATA                         = DEFAULT_NUM_DATA };
    const  DefaultDataRow (&DATA)[NUM_DATA] = DEFAULT_DATA;

    // Testing function signature

    {
        typedef Obj& (Obj::*OperatorMoveAssign)(bslmf::MovableRef<Obj>);
        OperatorMoveAssign oma = &Obj::operator=;    (void) oma;
    }

    Obj szc(sa);    const Obj& SZC = szc;
    primaryManipulatorBack(&szc, 'Z');
    const TYPE& zValue = SZC.front();

    // Create first object.
    for (int ti = 0; ti < NUM_DATA; ++ti) {
        const char *const SPEC1   = DATA[ti].d_spec;
        const size_t      LENGTH1 = strlen(SPEC1);

        Obj  mZZ(sa);  const Obj&  ZZ = gg(&mZZ,  SPEC1);

        // Ensure the first row of the table contains the default-constructed
        // value.
        if (0 == ti) {
            ASSERTV(SPEC1, Obj(sa), ZZ, Obj(sa) == ZZ);
        }

        // Create second object.
        for (int tj = 0; tj < NUM_DATA; ++tj) {
            const char *const SPEC2   = DATA[tj].d_spec;
            const size_t      LENGTH2 = strlen(SPEC2);
            for (char cfg = 'a'; cfg <= 'b'; ++cfg) {
                const char CONFIG = cfg;  // how we specify the allocator

                if (veryVerbose) { T_ P_(SPEC1); P_(SPEC2); P(CONFIG); }

                Obj *objPtr = new (foa) Obj(oa);

                Obj& mX = *objPtr;  const Obj& X = gg(&mX, SPEC2);

                Obj *srcPtr = 0;
                ALLOC&                ra  = 'a' == CONFIG ? za  : oa;
                bslma::TestAllocator& roa = 'a' == CONFIG ? zoa : ooa;

                const bool empty = 0 == ZZ.size();

                typename Obj::const_pointer pointers[2];

                Int64 oaBase;
                Int64 zaBase;

                int numPasses = 0;
                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(ooa) {
                    ++numPasses;

                    Int64 al = ooa.allocationLimit();
                    ooa.setAllocationLimit(-1);

                    srcPtr = new (foa) Obj(ra);
                    bslma::RawDeleterProctor<Obj, bslma::Allocator>
                                                         proctor(srcPtr, &foa);
                    gg(srcPtr, SPEC1);

                    Obj& mZ = *srcPtr;  const Obj& Z = mZ;
                    ASSERT(ZZ == Z);

                    // TBD: add exception guard for mX here

                    ASSERTV(SPEC1, SPEC2, Z, X, (Z == X) == (ti == tj));

                    storeFirstNElemAddr(pointers, Z,
                                        sizeof pointers / sizeof *pointers);

                    if (veryVeryVerbose) { T_ T_ Q(ExceptionTestBody) }

                    oaBase = ooa.numAllocations();
                    zaBase = zoa.numAllocations();

                    ooa.setAllocationLimit(al);

                    Obj *mR = &(mX = MoveUtil::move(mZ));
                    ASSERTV(SPEC1, SPEC2, mR, &mX, mR == &mX);

                    // Verify the value of the object.
                    ASSERTV(SPEC1, SPEC2,  X,  ZZ,  X ==  ZZ);

                    proctor.release();
                } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

                Obj& mZ = *srcPtr;  const Obj& Z = mZ;

                ASSERTV(SPEC1, SPEC2, &ra == &oa, X, LENGTH1 == X.size());

                // CONTAINER-SPECIFIC NOTE: For 'deque', if the allocators
                // differ, the source object is left with the same number of
                // elements but with each element in the "moved-from" state
                // (whatever that is); otherwise, the source object has the
                // same number of elements as the destination object had (and
                // vice versa).

                if (&roa == &ooa || isPropagate) {
                    // same allocator

                    // 1. no allocations from the (common) object allocator
                    ASSERTV(SPEC1, SPEC2, ooa.numAllocations() == oaBase);

                    // 2. unchanged address(es) of contained element(s)
                    ASSERT(0 == checkFirstNElemAddr(pointers, X,
                                          sizeof pointers / sizeof *pointers));

                    // 3. CONTAINER-SPECIFIC NOTE: 'src' & 'dst' "swap" lengths
                    ASSERTV(SPEC1, SPEC2, &ra == &oa, Z,
                                       (&ra == &oa ? LENGTH2 : 0) == Z.size());

                    // 4. nothing from the other allocator
                    ASSERTV(SPEC1, SPEC2, isPropagate ||
                                               zaBase == zoa.numAllocations());
                }
                else {
                    // 1. each element in original move-inserted
                    typename Obj::const_iterator exp =
                             k_IS_WELL_BEHAVED && e_STATEFUL != s_allocCategory
                                                         ? X.begin() : X.end();
                    typename Obj::const_iterator result = 
                       TstMoveUtil::findFirstNotMovedInto(X.begin(), X.end());
                                        
                    ASSERTV(SPEC1, SPEC2, result - X.begin(), NameOf<TYPE>(),
                                            allocCategoryAsStr(), result - exp,
                                                                exp == result);

                    // 2. CONTAINER-SPECIFIC NOTE: orig obj with same length
                    ASSERTV(SPEC1, SPEC2, &ra != &oa, Z, LENGTH1 == Z.size());

                    // 3. additional memory checks
                    ASSERTV(SPEC1, SPEC2, &ra == &oa,
                            empty || oaBase < ooa.numAllocations());
                    ASSERTV(SPEC1, SPEC2, zoa.numAllocations() == zaBase);
                }

                // Verify that 'X', 'Z', and 'ZZ' have correct allocator.
                ASSERTV(SPEC1, SPEC2, sa == ZZ.get_allocator());
                ASSERTV(SPEC1, SPEC2, (isPropagate ? ra : oa) ==
                                             X.get_allocator());
                ASSERTV(SPEC1, SPEC2, ra ==  Z.get_allocator());

                // Manipulate source object 'Z' to ensure it is in a valid
                // state and is independent of 'X'.

                const size_t zAfterSize = Z.size();

                primaryManipulatorBack(&mZ, 'Z');
                ASSERTV(SPEC1, SPEC2, Z, zAfterSize + 1 == Z.size());
                ASSERTV(SPEC1, SPEC2, zValue == Z.back());
                ASSERTV(SPEC1, SPEC2, Z, ZZ, Z != ZZ);
                ASSERTV(SPEC1, SPEC2, X, ZZ, X == ZZ);

                primaryManipulatorFront(&mZ, 'Z');
                ASSERTV(SPEC1, SPEC2, Z, zAfterSize + 2 == Z.size());
                ASSERTV(SPEC1, SPEC2, zValue == Z.front());
                ASSERTV(SPEC1, SPEC2, X, ZZ, X == ZZ);

                foa.deleteObject(srcPtr);

                ASSERTV(SPEC1, SPEC2, roa.numBlocksInUse(),
                                       empty || ((&ra == &oa || isPropagate) ==
                                                  (0 < roa.numBlocksInUse())));

                // Verify subsequent manipulation of target object 'X'.

                primaryManipulatorBack(&mX, 'Z');
                ASSERTV(SPEC1, SPEC2, LENGTH1 + 1, X.size(),
                        LENGTH1 + 1 == X.size());
                ASSERTV(SPEC1, SPEC2, zValue == X[LENGTH1]);
                ASSERTV(SPEC1, SPEC2, X, ZZ, X != ZZ);

                primaryManipulatorFront(&mX, 'Z');
                ASSERTV(SPEC1, SPEC2, LENGTH1 + 2, X.size(),
                        LENGTH1 + 2 == X.size());
                ASSERTV(SPEC1, SPEC2, zValue == X[0]);

                foa.deleteObject(objPtr);

                ASSERTV(SPEC1, SPEC2, ooa.numBlocksInUse(),
                        0 == ooa.numBlocksInUse());
                ASSERTV(SPEC1, SPEC2, zoa.numBlocksInUse(),
                        0 == zoa.numBlocksInUse());
            }
        }

        // self-assignment

        {
            Obj mZ(oa);   const Obj& Z  = gg(&mZ,  SPEC1);

            ASSERTV(SPEC1, ZZ, Z, ZZ == Z);

            bslma::TestAllocatorMonitor oam(&ooa), sam(&soa);

            BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(ooa) {
                if (veryVeryVerbose) { T_ T_ Q(ExceptionTestBody) }

                Obj *mR = &(mZ = MoveUtil::move(mZ));
                ASSERTV(SPEC1, ZZ,  Z, ZZ ==  Z);
                ASSERTV(SPEC1, mR, &Z, mR == &Z);
            } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

            ASSERTV(SPEC1, oa == Z.get_allocator());

            ASSERTV(SPEC1, sam.isTotalSame());
            ASSERTV(SPEC1, oam.isTotalSame());
        }

        // Verify all object memory is released on destruction.

        ASSERTV(SPEC1, ooa.numBlocksInUse(), 0 == ooa.numBlocksInUse());
    }

    ASSERTV(e_STATEFUL == s_allocCategory || 0 == doa.numBlocksTotal());
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase24()
{
    // ------------------------------------------------------------------------
    // TESTING MOVE CONSTRUCTOR
    //
    // Concerns:
    //: 1 The newly created object has the same value (using the equality
    //:   operator) as that of the original object before the call.
    //:
    //: 2 All internal representations of a given value can be used to create a
    //:   new object of equivalent value.
    //:
    //: 3 The allocator is propagated to the newly created object if (and only
    //:   if) no allocator is specified in the call to the move constructor.
    //:
    //: 4 A constant-time move, with no additional memory allocations or
    //:   deallocations, is performed when no allocator or the same allocator
    //:   as that of the original object is passed to the move constructor.
    //:
    //: 5 A linear operation, where each element is move-inserted into the
    //:   newly created object, is performed when a '0' or an allocator that is
    //:   different than that of the original object is explicitly passed to
    //:   the move constructor.
    //:
    //: 6 The original object is always left in a valid state; the allocator
    //:   address held by the original object is unchanged.
    //:
    //: 7 Subsequent changes to or destruction of the original object have no
    //:   effect on the move-constructed object and vice-versa.
    //:
    //: 8 The object has its internal memory management system hooked up
    //:   properly so that *all* internally allocated memory draws from a
    //:   user-supplied allocator whenever one is specified.
    //:
    //: 9 Every object releases any allocated memory at destruction.
    //
    //:10 Any memory allocation is exception neutral.
    //:
    // Plan:
    //: 1 Specify a set, 'V', of object values with substantial and varied
    //:   differences, ordered by increasing length, to be used sequentially in
    //:   the following tests; for each entry, create a control object.   (C-2)
    //:
    //: 2 Call the move constructor to create the container in all relevant use
    //:   cases involving the allocator: 1) no allocator passed in, 2) a '0' is
    //    explicitly passed in as the allocator argument, 3) the same allocator
    //:   as that of the original object is explicitly passed in, and 4) a
    //:   different allocator than that of the original object is passed in.
    //:
    //: 3 For each of the object values (P-1) and for each configuration (P-2),
    //:   verify the following:
    //:
    //:   1 Verify the newly created object has the same value as that of the
    //:     original object before the call to the move constructor (control
    //:     value).                                                       (C-1)
    //:
    //:   2 CONTAINER-SPECIFIC NOTE: none
    //:
    //:   3 Where a constant-time move is expected, ensure that no memory was
    //:     allocated, that element addresses did not change, and that the
    //:     original object is left in the default state.         (C-3..5, C-7)
    //:
    //:   4 Where a linear-time move is expected, ensure that the move
    //:     constructor was called for each element.                   (C-6..7)
    //:
    //:   5 CONTAINER-SPECIFIC NOTE: none
    //:
    //:   6 Ensure that the new, original, and control objects continue to have
    //:     the correct allocator and that all memory allocations come from the
    //:     appropriate allocator.                                    (C-3,C-9)
    //:
    //:   7 Manipulate the original object (after the move construction) to
    //:     ensure it is in a valid state, destroy it, and then manipulate the
    //:     newly created object to ensure that it is in a valid state.   (C-8)
    //:
    //:   8 Verify all memory is released when the object is destroyed.  (C-11)
    //;
    //: 4 Perform tests in the presence of exceptions during memory allocations
    //:   using a 'bslma::TestAllocator' and varying its *allocation* *limit*.
    //:                                                                  (C-10)
    //
    // Testing:
    //  deque(deque&& original);
    //  deque(deque&& original, const A& basicAllocator);
    // ------------------------------------------------------------------------

    if (verbose) printf("TC 24: %s\n", bsls::NameOf<TYPE>().name());

    const TestValues VALUES;

    static const char *SPECS[] = {
        "",
        "A",
        "BC",
        "CDE",
        "DEAB",
        "EABCD",
        "ABCDEAB",
        "ABCDEABC",
        "ABCDEABCD",
        "ABCDEABCDEABCDE",
        "ABCDEABCDEABCDEA",
        "ABCDEABCDEABCDEAB",
        "ABCDEFGHIJKLMNOPQRSTUVWXY"
    };
    enum { NUM_SPECS = sizeof SPECS / sizeof *SPECS };

    if (verbose) printf("\nTesting both versions of move constructor.\n");
    {
        for (int ti = 0; ti < NUM_SPECS; ++ti) {
            const char *const SPEC   = SPECS[ti];
            const size_t      LENGTH = strlen(SPEC);

            if (verbose) {
                printf("\nFor an object of length " ZU ":\n", LENGTH);
                P(SPEC);
            }

            // Create control object 'ZZ' with the scratch allocator.

            bslma::TestAllocator scratch("scratch", veryVeryVeryVerbose);
            ALLOC                xscratch(&scratch);

            Obj mZZ(xscratch);  const Obj& ZZ = gg(&mZZ, SPEC);

            static bool firstFlag = true;
            if (firstFlag) {
                ASSERTV(ti, Obj(), ZZ, Obj() == ZZ);
                firstFlag = false;
            }

            for (char cfg = 'a'; cfg <= 'd'; ++cfg) {

                const char           CONFIG = cfg;  // how we specify
                                                    // the allocator
                bslma::TestAllocator fa("footprint", veryVeryVeryVerbose);
                bslma::TestAllocator da("default",   veryVeryVeryVerbose);
                bslma::TestAllocator sa("supplied",  veryVeryVeryVerbose);
                bslma::TestAllocator za("different", veryVeryVeryVerbose);
                ALLOC                xsa(&sa);
                ALLOC                xza(&za);

                // Create source object 'Z'.
                Obj *srcPtr = new (fa) Obj(xsa);
                Obj& mZ = *srcPtr;  const Obj& Z = gg(&mZ, SPEC);

                typename Obj::const_pointer pointers[2];
                storeFirstNElemAddr(pointers,
                                    Z,
                                    sizeof pointers / sizeof *pointers);

                bslma::DefaultAllocatorGuard dag(&da);

                bslma::TestAllocatorMonitor oam(&da), sam(&sa);

                Obj                  *objPtr;
                bslma::TestAllocator *objAllocatorPtr;
                bslma::TestAllocator *othAllocatorPtr;

                bool empty = 0 == ZZ.size();

                switch (CONFIG) {
                  case 'a': {
                    oam.reset(&sa);
                    objPtr = new (fa) Obj(MoveUtil::move(mZ));
                    objAllocatorPtr = &sa;
                    othAllocatorPtr = &da;
                  } break;
                  case 'b': {
                    oam.reset(&da);
                    objPtr = new (fa) Obj(MoveUtil::move(mZ), ALLOC(0));
                    objAllocatorPtr = &da;
                    othAllocatorPtr = &za;
                  } break;
                  case 'c': {
                    oam.reset(&sa);
                    objPtr = new (fa) Obj(MoveUtil::move(mZ), xsa);
                    objAllocatorPtr = &sa;
                    othAllocatorPtr = &da;
                  } break;
                  case 'd': {
                    oam.reset(&za);
                    objPtr = new (fa) Obj(MoveUtil::move(mZ), xza);
                    objAllocatorPtr = &za;
                    othAllocatorPtr = &da;
                  } break;
                  default: {
                    ASSERTV(CONFIG, !"Bad allocator config.");
                  } return;                                           // RETURN
                }

                bslma::TestAllocator& oa = *objAllocatorPtr;
                bslma::TestAllocator& noa = *othAllocatorPtr;
                ALLOC                 xoa(&oa);

                Obj& mX = *objPtr;  const Obj& X = mX;
                // Verify the value of the object.
                ASSERTV(SPEC, CONFIG, X == ZZ);

                // CONTAINER-SPECIFIC NOTE: For 'deque', if the allocators
                // differ, the original object is left with the same number of
                // elements but with each element in the "moved-from" state
                // (whatever that is); otherwise, the original object is left
                // empty.

                if (&sa == &oa) {
                    // 1. some memory allocation
                    ASSERTV(SPEC, CONFIG, &sa == &oa, oam.isTotalUp());

                    // 2. unchanged address(es) of contained element(s)
                    ASSERT(0 == checkFirstNElemAddr(pointers, X,
                                          sizeof pointers / sizeof *pointers));

                    // 3. original object left empty
                    ASSERTV(SPEC, CONFIG, &sa == &oa, Z, 0 == Z.size());
                }
                else {
                    // 1. each element in original move-inserted
                    typename Obj::const_iterator exp =
                                       k_IS_WELL_BEHAVED ? X.begin() : X.end();
                                        
                    ASSERTV(SPEC, CONFIG, exp ==
                       TstMoveUtil::findFirstNotMovedInto(X.begin(), X.end()));

                    // 2. original object left with same size
                    ASSERTV(SPEC, CONFIG, &sa == &oa, Z, LENGTH == Z.size());

                    // 3. additional memory checks
                    ASSERTV(SPEC, CONFIG,
                            &sa == &oa, oam.isTotalUp() || empty);
                }

                // Verify that 'X', 'Z', and 'ZZ' have the correct allocator.
                ASSERTV(SPEC, CONFIG, &scratch == ZZ.get_allocator());
                ASSERTV(SPEC, CONFIG,      &sa ==  Z.get_allocator());
                ASSERTV(SPEC, CONFIG,      &oa ==  X.get_allocator());

                // Verify no allocation from the non-object allocator and that
                // object allocator is hooked up.
                ASSERTV(SPEC, CONFIG, 0 == noa.numBlocksTotal());
                ASSERTV(SPEC, CONFIG, 0 < oa.numBlocksTotal() || empty);

                // Manipulate source object 'Z' to ensure it is in a valid
                // state and is independent of 'X'.

                primaryManipulatorBack(&mZ, 'Z');
                if (&sa == &oa) {
                    ASSERTV(SPEC, CONFIG, Z, 1 == Z.size());
                    ASSERTV(SPEC, CONFIG, VALUES['Z' - 'A'] == Z[0]);
                }
                else {
                    ASSERTV(SPEC, CONFIG, Z, LENGTH + 1 == Z.size());
                    ASSERTV(SPEC, CONFIG, VALUES['Z' - 'A'] == Z[LENGTH]);
                }
                ASSERTV(SPEC, CONFIG, Z, ZZ, Z != ZZ);
                ASSERTV(SPEC, CONFIG, X, ZZ, X == ZZ);

                primaryManipulatorFront(&mZ, 'Z');
                if (&sa == &oa) {
                    ASSERTV(SPEC, CONFIG, Z, 2 == Z.size());
                }
                else {
                    ASSERTV(SPEC, CONFIG, Z, LENGTH + 2 == Z.size());
                }
                ASSERTV(SPEC, CONFIG, VALUES['Z' - 'A'] == Z[0]);

                fa.deleteObject(srcPtr);

                ASSERTV(SPEC, CONFIG, X, ZZ, X == ZZ);

                // Verify subsequent manipulation of new object 'X'.

                primaryManipulatorBack(&mX, 'Z');
                ASSERTV(SPEC, LENGTH + 1 == X.size());
                ASSERTV(SPEC, VALUES['Z' - 'A'] == X[LENGTH]);
                ASSERTV(SPEC, X != ZZ);

                primaryManipulatorFront(&mX, 'Z');
                ASSERTV(SPEC, LENGTH + 2 == X.size());
                ASSERTV(SPEC, VALUES['Z' - 'A'] == X[0]);

                fa.deleteObject(objPtr);
                // Verify all memory is released on object destruction.
                ASSERTV(SPEC, 0 == fa.numBlocksInUse());
                ASSERTV(SPEC, 0 == da.numBlocksInUse());
                ASSERTV(SPEC, 0 == sa.numBlocksInUse());
                ASSERTV(SPEC, 0 == za.numBlocksInUse());
            }
        }
    }
    if (verbose)
        printf("\nTesting move constructor with injected exceptions.\n");
#if defined(BDE_BUILD_TARGET_EXC)
    {
        for (int ti = 0; ti < NUM_SPECS; ++ti) {
            const char *const SPEC   = SPECS[ti];
            const size_t      LENGTH = strlen(SPEC);

            bslma::TestAllocator scratch("scratch", veryVeryVeryVerbose);
            ALLOC                xscratch(&scratch);

            Obj mZZ(xscratch);  const Obj& ZZ = gg(&mZZ, SPEC);

            if (verbose) {
                printf("\nFor an object of length " ZU ":\n", LENGTH);
                P(SPEC);
            }

            bslma::TestAllocator oa("object",    veryVeryVeryVerbose);
            bslma::TestAllocator za("different", veryVeryVeryVerbose);
            ALLOC                xoa(&oa);
            ALLOC                xza(&za);

            const Int64 BB = oa.numBlocksTotal();
            const Int64  B = oa.numBlocksInUse();

            if (veryVerbose) {
                printf("\t\t\t\tBefore Creation: "); P_(BB); P(B);
            }

            BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                Obj mWW(xza);  const Obj& WW = gg(&mWW, SPEC);

                const Obj X(MoveUtil::move(mWW), xoa);
                if (veryVerbose) {
                    printf("\t\t\tException Case  :\n");
                    printf("\t\t\t\tObj : "); P(X);
                }
                ASSERTV(SPEC, ZZ == X);
                ASSERTV(SPEC, WW.size(), LENGTH == WW.size());
                ASSERTV(SPEC, WW.get_allocator() != X.get_allocator());
            } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

            const Int64 AA = oa.numBlocksTotal();
            const Int64  A = oa.numBlocksInUse();

            if (veryVerbose) {
                printf("\t\t\t\tAfter Creation: "); P_(AA); P(A);
            }
        }
    }
#endif  // BDE_BUILD_TARGET_EXC
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase23()
{
    // ------------------------------------------------------------------------
    // TESTING 'std::length_error'
    //
    // Concerns:
    //   1) That any call to a constructor, 'assign', 'push_back' or 'insert'
    //      that would result in a value exceeding 'max_size()' throws
    //      'std::length_error'.
    //   2) That the 'max_size()' taken into consideration is that of the
    //      allocator, and not an absolute constant.
    //   3) That the value of the deque is unchanged if an exception is
    //      thrown.
    //   4) That integer overflows are correctly handled when 'length_error'
    //      exceeds 'Obj::max_size()' (which is the largest representable
    //      size_type).
    //
    // Plan:
    //   For concern 2, we use an allocator wrapper that provides the same
    //   functionality as 'ALLOC' but changes the return value of 'max_size()'
    //   to a 'limit' value settable at runtime.  Note that the operations
    //   throw unless 'length <= limit'.
    //
    //   Construct objects with value large enough that the constructor throws.
    //   For 'assign', 'insert', 'push_back', we construct a small (non-empty)
    //   object, and use the corresponding function to request an increase in
    //   size that is guaranteed to result in a value exceeding 'max_size()'.
    //
    // Testing:
    //   CONCERN: 'std::length_error' is used properly.
    // ------------------------------------------------------------------------

    bslma::TestAllocator oa("object", veryVeryVeryVerbose);

    const TYPE DEFAULT_VALUE = TYPE(::DEFAULT_VALUE);

    LimitAllocator<ALLOC> a(&oa);
    a.setMaxSize((size_t)-1);

    const int LENGTH = 32;
    typedef bsl::deque<TYPE,LimitAllocator<ALLOC> > LimitObj;

    LimitObj mY(LENGTH, DEFAULT_VALUE);  // does not throw
    const LimitObj& Y = mY;

#ifdef BDE_BUILD_TARGET_EXC
    if (verbose) printf("\nConstructor 'deque(n, a = A())'.\n");

    for (int limit = LENGTH - 2; limit <= LENGTH + 2; ++limit) {
        bool exceptionCaught = false;
        a.setMaxSize(limit);

        if (veryVerbose)
            printf("\tWith max_size() equal to limit = %d\n", limit);

        try {
            LimitObj mX(LENGTH, a);  // test here
        }
        catch (const std::length_error& e) {
            if (veryVerbose) {
                printf("\t\tCaught std::length_error(\"%s\").\n", e.what());
            }
            exceptionCaught = true;
        }
        catch (std::exception& e) {
            ASSERT(0);
            if (veryVerbose) {
                printf("\t\tCaught std::exception(%s).\n", e.what());
            }
        }
        catch (...) {
            ASSERT(0);
            if (veryVerbose) {
                printf("\t\tCaught unknown exception.\n");
            }
        }
        LOOP2_ASSERT(limit, exceptionCaught,
                     (limit < LENGTH) == exceptionCaught);
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBytesInUse());

    if (verbose) printf("\nConstructor 'deque(n, T x, a = A())'.\n");

    for (int limit = LENGTH - 2; limit <= LENGTH + 2; ++limit) {
        bool exceptionCaught = false;
        a.setMaxSize(limit);

        if (veryVerbose)
            printf("\tWith max_size() equal to limit = %d\n", limit);

        try {
            LimitObj mX(LENGTH, DEFAULT_VALUE, a);  // test here
        }
        catch (const std::length_error& e) {
            if (veryVerbose) {
                printf("\t\tCaught std::length_error(\"%s\").\n", e.what());
            }
            exceptionCaught = true;
        }
        catch (std::exception& e) {
            ASSERT(0);
            if (veryVerbose) {
                printf("\t\tCaught std::exception(%s).\n", e.what());
            }
        }
        catch (...) {
            ASSERT(0);
            if (veryVerbose) {
                printf("\t\tCaught unknown exception.\n");
            }
        }
        LOOP2_ASSERT(limit, exceptionCaught,
                     (limit < LENGTH) == exceptionCaught);
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBytesInUse());

    if (verbose) printf("\nConstructor 'deque<Iter>(f, l, a = A())'.\n");

    for (int limit = LENGTH - 2; limit <= LENGTH + 2; ++limit) {
        bool exceptionCaught = false;
        a.setMaxSize(limit);

        if (veryVerbose)
            printf("\tWith max_size() equal to limit = %d\n", limit);

        try {
            LimitObj mX(Y.begin(), Y.end(), a);  // test here
        }
        catch (const std::length_error& e) {
            if (veryVerbose) {
                printf("\t\tCaught std::length_error(\"%s\").\n", e.what());
            }
            exceptionCaught = true;
        }
        catch (std::exception& e) {
            ASSERT(0);
            if (veryVerbose) {
                printf("\t\tCaught std::exception(%s).\n", e.what());
            }
        }
        catch (...) {
            ASSERT(0);
            if (veryVerbose) {
                printf("\t\tCaught unknown exception.\n");
            }
        }
        LOOP2_ASSERT(limit, exceptionCaught,
                     (limit < LENGTH) == exceptionCaught);
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBytesInUse());

    if (verbose) printf("\nWith 'resize'.\n");
    {
        for (int limit = LENGTH - 2; limit <= LENGTH + 2; ++limit) {
            bool exceptionCaught = false;
            a.setMaxSize(limit);

            if (veryVerbose)
                printf("\tWith max_size() equal to limit = %d\n", limit);

            try {
                LimitObj mX(a);

                mX.resize(LENGTH);
            }
            catch (const std::length_error& e) {
                if (veryVerbose) {
                    printf("\t\tCaught std::length_error(\"%s\").\n",
                           e.what());
                }
                exceptionCaught = true;
            }
            catch (std::exception& e) {
                ASSERT(0);
                if (veryVerbose) {
                    printf("\t\tCaught std::exception(%s).\n", e.what());
                }
            }
            catch (...) {
                ASSERT(0);
                if (veryVerbose) {
                    printf("\t\tCaught unknown exception.\n");
                }
            }
            LOOP2_ASSERT(limit, exceptionCaught,
                         (limit < LENGTH) == exceptionCaught);
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBytesInUse());

    if (verbose) printf("\nWith 'resize' using a value.\n");
    {
        for (int limit = LENGTH - 2; limit <= LENGTH + 2; ++limit) {
            bool exceptionCaught = false;
            a.setMaxSize(limit);

            if (veryVerbose)
                printf("\tWith max_size() equal to limit = %d\n", limit);

            try {
                LimitObj mX(a);

                mX.resize(LENGTH, DEFAULT_VALUE);
            }
            catch (const std::length_error& e) {
                if (veryVerbose) {
                    printf("\t\tCaught std::length_error(\"%s\").\n",
                           e.what());
                }
                exceptionCaught = true;
            }
            catch (std::exception& e) {
                ASSERT(0);
                if (veryVerbose) {
                    printf("\t\tCaught std::exception(%s).\n", e.what());
                }
            }
            catch (...) {
                ASSERT(0);
                if (veryVerbose) {
                    printf("\t\tCaught unknown exception.\n");
                }
            }
            LOOP2_ASSERT(limit, exceptionCaught,
                         (limit < LENGTH) == exceptionCaught);
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBytesInUse());

    if (verbose) printf("\nWith 'assign'.\n");

    for (int assignMethod = 0; assignMethod <= 1; ++assignMethod) {

        if (veryVerbose) {
            switch (assignMethod) {
              case 0: printf("\tWith assign(n, T x).\n"); break;
              case 1: printf("\tWith assign<Iter>(f, l).\n"); break;
              default: ASSERT(0);
            };
        }

        for (int limit = LENGTH - 2; limit <= LENGTH + 2; ++limit) {
            bool exceptionCaught = false;
            a.setMaxSize(limit);

            if (veryVerbose)
                printf("\t\tWith max_size() equal to limit = %d\n", limit);

            try {
                LimitObj mX(a);

                switch (assignMethod) {
                  case 0: {
                    mX.assign(LENGTH, DEFAULT_VALUE);
                  } break;
                  case 1: {
                    mX.assign(Y.begin(), Y.end());
                  } break;
                  default: ASSERT(0);
                };
            }
            catch (const std::length_error& e) {
                if (veryVerbose) {
                    printf("\t\tCaught std::length_error(\"%s\").\n",
                           e.what());
                }
                exceptionCaught = true;
            }
            catch (std::exception& e) {
                ASSERT(0);
                if (veryVerbose) {
                    printf("\t\tCaught std::exception(%s).\n", e.what());
                }
            }
            catch (...) {
                ASSERT(0);
                if (veryVerbose) {
                    printf("\t\tCaught unknown exception.\n");
                }
            }
            LOOP2_ASSERT(limit, exceptionCaught,
                         (limit < LENGTH) == exceptionCaught);
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBytesInUse());

    if (verbose) printf("\nWith 'insert'.\n");

    for (int insertMethod = 0; insertMethod <= 3; ++insertMethod) {

        if (verbose) {
            switch (insertMethod) {
              case 0: printf("\tWith push_back(c).\n");          break;
              case 1: printf("\tWith insert(p, T x).\n");        break;
              case 2: printf("\tWith insert(p, n, T x).\n");     break;
              case 3: printf("\tWith insert<Iter>(p, f, l).\n"); break;
              default: ASSERT(0);
            };
        }

        for (int limit = LENGTH - 2; limit <= LENGTH + 2; ++limit) {
            bool exceptionCaught = false;
            a.setMaxSize(limit);

            if (veryVerbose)
                printf("\t\tWith max_size() equal to limit = %d\n", limit);

            try {
                LimitObj mX(a);  const LimitObj& X = mX;

                switch (insertMethod) {
                  case 0: {
                    for (int i = 0; i < LENGTH; ++i) {
                        mX.push_back(Y[i]);
                    }
                  } break;
                  case 1: {
                    for (int i = 0; i < LENGTH; ++i) {
                        mX.insert(X.begin(), DEFAULT_VALUE);
                    }
                  } break;
                  case 2: {
                    mX.insert(X.begin(), LENGTH, DEFAULT_VALUE);
                  } break;
                  case 3: {
                    mX.insert(X.begin(), Y.begin(), Y.end());
                  } break;
                  default: ASSERT(0);
                };
            }
            catch (const std::length_error& e) {
                if (veryVerbose) {
                    printf("\t\t\tCaught std::length_error(\"%s\").\n",
                           e.what());
                }
                exceptionCaught = true;
            }
            catch (std::exception& e) {
                ASSERT(0);
                if (veryVerbose) {
                    printf("\t\t\tCaught std::exception(%s).\n", e.what());
                }
            }
            catch (...) {
                ASSERT(0);
                if (veryVerbose) {
                    printf("\t\t\tCaught unknown exception.\n");
                }
            }
            LOOP2_ASSERT(limit, exceptionCaught,
                         (limit < LENGTH) == exceptionCaught);
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBytesInUse());

    const int PADDING = 16;
    const Obj X;

    if (verbose) printf("\nTesting requests for '(size_t) -1' elements with "
                        "default allocator.\n");

    if (verbose) printf("\nConstructor 'deque(n, T x, a = A())'"
                        " and 'max_size()' equal to %llu.\n",
                        (Uint64)X.max_size());
    {
        bool exceptionCaught = false;

        if (veryVerbose) {
            size_t minus2 = (size_t) -2;
            printf("\tWith max_size() equal to %llu.\n", (Uint64)minus2);
        }

        try {
            Obj mX(-1, DEFAULT_VALUE);  // test here
        }
        catch (const std::length_error& e) {
            if (veryVerbose) {
                printf("\t\tCaught std::length_error(\"%s\").\n", e.what());
            }
            exceptionCaught = true;
        }
        catch (std::exception& e) {
            ASSERT(0);
            if (veryVerbose) {
                printf("\t\tCaught std::exception(%s).\n", e.what());
            }
        }
        catch (...) {
            ASSERT(0);
            if (veryVerbose) {
                printf("\t\tCaught unknown exception.\n");
            }
        }
        ASSERT(exceptionCaught);
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBytesInUse());

    if (verbose) printf("\nWith 'reserve/resize' and"
                        " 'max_size()' equal to %llu.\n",
                        (Uint64)X.max_size());

    for (int capacityMethod = 0; capacityMethod <= 2; ++capacityMethod)
    {
        bool exceptionCaught = false;

        if (verbose) {
            switch (capacityMethod) {
              case 0: printf("\tWith reserve(n).\n");        break;
              case 1: printf("\tWith resize(n).\n");         break;
              case 2: printf("\tWith resize(n, T x).\n");    break;
              default: ASSERT(0);
            };
        }

        try {
            Obj mX;

            switch (capacityMethod) {
              case 0:  mX.reserve((size_t)-1);                  break;
              case 1:  mX.resize((size_t)-1);                   break;
              case 2:  mX.resize((size_t)-1, DEFAULT_VALUE);    break;
              default: ASSERT(0);
            };
        }
        catch (const std::length_error& e) {
            if (veryVerbose) {
                printf("\t\tCaught std::length_error(\"%s\").\n", e.what());
            }
            exceptionCaught = true;
        }
        catch (std::exception& e) {
            if (veryVerbose) {
                printf("\t\tCaught std::exception(%s).\n", e.what());
            }
            ASSERT(0);
        }
        catch (...) {
            if (veryVerbose) {
                printf("\t\tCaught unknown exception.\n");
            }
            ASSERT(0);
        }
        ASSERT(exceptionCaught);
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBytesInUse());

    if (verbose) printf("\nTesting requests for 'X.max_size() + n' elements "
                        "with 'insert' and 'max_size()' equal to %llu.\n",
                        (Uint64)X.max_size());

    for (int insertMethod = 0; insertMethod <= 1; insertMethod += 2) {

        if (verbose) {
            switch (insertMethod) {
              case 0: printf("\tWith insert(pos, n, C c).\n");        break;
              case 1: printf("\tWith insert(p, n, C c).\n");          break;
              default: ASSERT(0);
            };
        }

        for (int limit = 1; limit <= 5; ++limit) {
            bool exceptionCaught = false;

            if (veryVerbose)
                printf("\t\tCreating string of length 'max_size()' plus %d.\n",
                       limit);

            try {
                Obj mX(PADDING, DEFAULT_VALUE, a);  const Obj& X = mX;

                const size_t LENGTH = X.max_size() - PADDING + limit;
                switch (insertMethod) {
                  case 0: {
                    mX.insert(mX.begin(), LENGTH, DEFAULT_VALUE);
                  } break;
                  default: ASSERT(0);
                };
            }
            catch (const std::length_error& e) {
                if (veryVerbose) {
                    printf("\t\t\tCaught std::length_error(\"%s\").\n",
                           e.what());
                }
                exceptionCaught = true;
            }
            catch (std::exception& e) {
                ASSERT(0);
                if (veryVerbose) {
                    printf("\t\t\tCaught std::exception(%s).\n", e.what());
                }
            }
            catch (...) {
                ASSERT(0);
                if (veryVerbose) {
                    printf("\t\t\tCaught unknown exception.\n");
                }
            }
            LOOP2_ASSERT(limit, exceptionCaught,
                         (limit < LENGTH) == exceptionCaught);
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBytesInUse());
#endif
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase22()
{
    // ------------------------------------------------------------------------
    // TESTING FREE COMPARISON OPERATORS
    //
    // Concerns:
    //   1) 'operator<' returns the lexicographic comparison on two arrays.
    //   2) 'operator>', 'operator<=', and 'operator>=' are correctly tied to
    //      'operator<'.
    //   3) That traits get selected properly.
    //
    // Plan:
    //   For a variety of 'deque's of different sizes and different values,
    //   test that the comparison returns as expected.  Note that capacity is
    //   not of concern here, the implementation specifically uses only
    //   'begin()', 'end()', and 'size()'.
    //
    // Testing:
    //   bool operator< (const deque& lhs, const deque& rhs);
    //   bool operator> (const deque& lhs, const deque& rhs);
    //   bool operator<=(const deque& lhs, const deque& rhs);
    //   bool operator>=(const deque& lhs, const deque& rhs);
    // ------------------------------------------------------------------------

    static const char *SPECS[] = {
        "",
        "A",
        "AA",
        "AAA",
        "AAAA",
        "AAAAA",
        "AAAAAA",
        "AAAAAAA",
        "AAAAAAAA",
        "AAAAAAAAA",
        "AAAAAAAAAA",
        "AAAAAAAAAAA",
        "AAAAAAAAAAAA",
        "AAAAAAAAAAAAA",
        "AAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAA",
        "AAAAAB",
        "AAAAABA",
        "AAAAABAA",
        "AAAAABAAA",
        "AAAAABAAAA",
        "AAAAABAAAAA",
        "AAAAABAAAAAA",
        "AAAAABAAAAAAA",
        "AAAAABAAAAAAAA",
        "AAAAABAAAAAAAAA",
        "AAAAB",
        "AAAABAAAAAA",
        "AAAABAAAAAAA",
        "AAAABAAAAAAAA",
        "AAAABAAAAAAAAA",
        "AAAABAAAAAAAAAA",
        "AAAB",
        "AAABA",
        "AAABAA",
        "AAABAAAAAA",
        "AAB",
        "AABA",
        "AABAA",
        "AABAAA",
        "AABAAAAAA",
        "AB",
        "ABA",
        "ABAA",
        "ABAAA",
        "ABAAAAAA",
        "B",
        "BA",
        "BAA",
        "BAAA",
        "BAAAA",
        "BAAAAA",
        "BAAAAAA",
        "BB",
        0  // null string required as last element
    };

    if (verbose) printf("\nCompare each pair of similar and different"
                        " values (u, v) in S X S \n.");
    {
        // Create first object.
        for (int si = 0; SPECS[si]; ++si) {
            const char *const U_SPEC = SPECS[si];

            Obj mU;  const Obj& U = gg(&mU, U_SPEC);

            if (veryVerbose) {
                T_; T_; P_(U_SPEC); P(U);
            }

            // Create second object.
            for (int sj = 0; SPECS[sj]; ++sj) {
                const char *const V_SPEC = SPECS[sj];

                Obj mV;  const Obj& V = gg(&mV, V_SPEC);

                if (veryVerbose) {
                    T_; T_; P_(V_SPEC); P(V);
                }

                const bool isLess = si < sj;
                const bool isLessEq = si <= sj;
                LOOP2_ASSERT(si, sj,  isLess   == (U < V));
                LOOP2_ASSERT(si, sj, !isLessEq == (U > V));
                LOOP2_ASSERT(si, sj,  isLessEq == (U <= V));
                LOOP2_ASSERT(si, sj, !isLess   == (U >= V));
            }
        }
    }
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase21_dispatch()
{
    // ------------------------------------------------------------------------
    // SWAP MEMBER AND FREE FUNCTIONS
    //   Ensure that, when member and free 'swap' are implemented, we can
    //   exchange the values of any two objects that use the same
    //   allocator.
    //
    // Concerns:
    //: 1 Both functions exchange the values of the (two) supplied objects.
    //:
    //: 2 Both functions have standard signatures and return types.
    //:
    //: 3 Using either function to swap an object with itself does not
    //:   affect the value of the object (alias-safety).
    //:
    //: 4 If the two objects being swapped uses the same allocator, neither
    //:   function allocates memory from any allocator and the allocator
    //:   address held by both objects is unchanged.
    //:
    //: 5 If the two objects being swapped uses different allocators and
    //:   'AllocatorTraits::propagate_on_container_swap' is an alias to
    //:   'false_type', then both function may allocate memory and the
    //:   allocator address held by both object is unchanged.
    //:
    //: 6 If the two objects being swapped uses different allocators and
    //:   'AllocatorTraits::propagate_on_container_swap' is an alias to
    //:   'true_type', then no memory will be allocated and the allocators will
    //:   also be swapped.
    //:
    //: 7 Both functions provides the strong exception guarantee w.t.r. to
    //:   memory allocation .
    //:
    //: 8 The free 'swap' function is discoverable through ADL (Argument
    //:   Dependent Lookup).
    //
    // Plan:
    //: 1 Use the addresses of the 'swap' member and free functions defined
    //:   in this component to initialize, respectively, member-function
    //:   and free-function pointers having the appropriate signatures and
    //:   return types.  (C-2)
    //:
    //: 2 Create a 'bslma::TestAllocator' object, and install it as the
    //:   default allocator (note that a ubiquitous test allocator is
    //:   already installed as the global allocator).
    //:
    //: 3 Using the table-driven technique:
    //:
    //:   1 Specify a set of (unique) valid object values (one per row) in
    //:     terms of their individual attributes, including (a) first, the
    //:     default value, (b) boundary values corresponding to every range
    //:     of values that each individual attribute can independently
    //:     attain, and (c) values that should require allocation from each
    //:     individual attribute that can independently allocate memory.
    //:
    //:   2 Additionally, provide a (tri-valued) column, 'MEM', indicating
    //:     the expectation of memory allocation for all typical
    //:     implementations of individual attribute types: ('Y') "Yes",
    //:     ('N') "No", or ('?') "implementation-dependent".
    //:
    //: 4 For each row 'R1' in the table of P-3:  (C-1, 3..7)
    //:
    //:   1 Create a 'bslma::TestAllocator' object, 'oa'.
    //:
    //:   2 Use the value constructor and 'oa' to create a modifiable
    //:     'Obj', 'mW', having the value described by 'R1'; also use the
    //:     copy constructor and a "scratch" allocator to create a 'const'
    //:     'Obj' 'XX' from 'mW'.
    //:
    //:   3 Use the member and free 'swap' functions to swap the value of
    //:     'mW' with itself; verify, after each swap, that:  (C-3..4)
    //:
    //:     1 The value is unchanged.  (C-3)
    //:
    //:     2 The allocator address held by the object is unchanged.  (C-4)
    //:
    //:     3 There was no additional object memory allocation.  (C-4)
    //:
    //:   4 For each row 'R2' in the table of P-3:  (C-1, 4)
    //:
    //:     1 Use the copy constructor and 'oa' to create a modifiable
    //:       'Obj', 'mX', from 'XX' (P-4.2).
    //:
    //:     2 Use the value constructor and 'oa' to create a modifiable
    //:       'Obj', 'mY', and having the value described by 'R2'; also use
    //:       the copy constructor to create, using a "scratch" allocator,
    //:       a 'const' 'Obj', 'YY', from 'Y'.
    //:
    //:     3 Use, in turn, the member and free 'swap' functions to swap
    //:       the values of 'mX' and 'mY'; verify, after each swap, that:
    //:       (C-1..2)
    //:
    //:       1 The values have been exchanged.  (C-1)
    //:
    //:       2 The common object allocator address held by 'mX' and 'mY'
    //:         is unchanged in both objects.  (C-4)
    //:
    //:       3 There was no additional object memory allocation.  (C-4)
    //:
    //:     5 Use the value constructor and 'oaz' to a create a modifiable
    //:       'Obj' 'mZ', having the value described by 'R2'; also use the copy
    //:       constructor to create, using a "scratch" allocator, a const
    //:       'Obj', 'ZZ', from 'Z'.
    //:
    //:     6 Use the member and free 'swap' functions to swap the values of
    //:       'mX' and 'mZ' respectively (when
    //:       AllocatorTraits::propagate_on_container_swap is an alias to
    //:       false_type) under the presence of exception; verify, after each
    //:       swap, that:  (C-1, 5, 7)
    //:
    //:       1 If exception occurred during the swap, both values are
    //:         unchanged.  (C-7)
    //
    //:       2 If no exception occurred, the values have been exchanged.
    //:         (C-1)
    //:
    //:       3 The common object allocator address held by 'mX' and 'mZ' is
    //:         unchanged in both objects.  (C-5)
    //:
    //:       4 Temporary memory were allocated from 'oa' if 'mZ' is not empty,
    //:         and temporary memory were allocated from 'oaz' if 'mX' is not
    //:         empty.  (C-5)
    //:
    //:     7 Create a new object allocator, 'oap'.
    //:
    //:     8 Use the value constructor and 'oap' to create a modifiable 'Obj'
    //:       'mP', having the value described by 'R2'; also use the copy
    //:       constructor to create, using a "scratch" allocator, a const
    //:       'Obj', 'PP', from 'P.
    //:
    //:     9 Manually change 'AllocatorTraits::propagate_on_container_swap' to
    //:       be an alias to 'true_type' (Instead of this manual step, use an
    //:       allocator that enables propagate_on_container_swap when
    //:       AllocatorTraits supports it) and use the member and free 'swap'
    //:       functions to swap the values 'mX' and 'mZ' respectively; verify,
    //:       after each swap, that: (C-1, 6)
    //:
    //:       1 The values have been exchanged.  (C-1)
    //:
    //:       2 The allocators addresses have been exchanged.  (C-6)
    //:
    //:       3 There was no additional object memory allocation.  (C-6)
    //:
    //:    7 Create a new object allocator, 'oap'.
    //:
    //:    8 Use the value constructor and 'oap' to create a modifiable 'Obj'
    //:      'mP', having the value described by 'R2'; also use the copy
    //:      constructor to create, using a "scratch" allocator, a const 'Obj',
    //:      'PP', from 'P.
    //:
    //:    9 Manually change 'AllocatorTraits::propagate_on_container_swap' to
    //:      be an alias to 'true_type' (Instead of this manual step, use an
    //:      allocator that enables propagate_on_container_swap when
    //:      AllocatorTraits supports it) and use the member and free 'swap'
    //:      functions to swap the values 'mX' and 'mZ' respectively; verify,
    //:      after each swap, that:  (C-1, 6)
    //:
    //:      1 The values have been exchanged.  (C-1)
    //:
    //:      2 The allocators addresses have been exchanged.  (C-6)
    //:
    //:      3 There was no additional object memory allocation.  (C-6)
    //:
    //: 5 Verify that the free 'swap' function is discoverable through ADL:
    //:   (C-8)
    //:
    //:   1 Create a set of attribute values, 'A', distinct from the values
    //:     corresponding to the default-constructed object, choosing
    //:     values that allocate memory if possible.
    //:
    //:   2 Create a 'bslma::TestAllocator' object, 'oa'.
    //:
    //:   3 Use the default constructor and 'oa' to create a modifiable
    //:     'Obj' 'mX' (having default attribute values); also use the copy
    //:     constructor and a "scratch" allocator to create a 'const' 'Obj'
    //:     'XX' from 'mX'.
    //:
    //:   4 Use the value constructor and 'oa' to create a modifiable 'Obj'
    //:     'mY' having the value described by the 'Ai' attributes; also
    //:     use the copy constructor and a "scratch" allocator to create a
    //:     'const' 'Obj' 'YY' from 'mY'.
    //:
    //:   5 Use the 'invokeAdlSwap' helper function template to swap the
    //:     values of 'mX' and 'mY', using the free 'swap' function defined
    //:     in this component, then verify that:  (C-8)
    //:
    //:     1 The values have been exchanged.  (C-1)
    //:
    //:     2 There was no additional object memory allocation.  (C-4)
    //
    // Testing:
    //   void swap(multiset& other);
    //   void swap(multiset<K, C, A>& a, multiset<K, C, A>& b);
    // ------------------------------------------------------------------------

    // Since this function is called with a variety of template arguments, it
    // is necessary to infer some things about our template arguments in order
    // to print a meaningful banner.
    const bool isPropagate =
                           AllocatorTraits::propagate_on_container_swap::value;
    const bool otherTraitsSet =
                AllocatorTraits::propagate_on_container_copy_assignment::value;

    // We can print the banner now:

    if (verbose) printf("%sTESTING SWAP '%s' OTHER:%c PROP:%c ALLOC: %s\n",
                        veryVerbose ? "\n" : "",
                        NameOf<TYPE>().name(), otherTraitsSet ? 'T' : 'F',
                        isPropagate ? 'T' : 'F',
                        allocCategoryAsStr());

    // Assign the address of each function to a variable.
    {
        typedef void (Obj::*funcPtr)(Obj&);
        typedef void (*freeFuncPtr)(Obj&, Obj&);

        // Verify that the signatures and return types are standard.

        funcPtr     memberSwap = &Obj::swap;
        freeFuncPtr freeSwap   = bsl::swap;

        (void) memberSwap;  // quash potential compiler warnings
        (void) freeSwap;
    }

    // Create a test allocator and install it as the default.

    bslma::TestAllocator doa("default",   veryVeryVeryVerbose);
    bslma::TestAllocator ooa("object",    veryVeryVeryVerbose);
    bslma::TestAllocator zoa("other",     veryVeryVeryVerbose);
    bslma::TestAllocator soa("scratch",   veryVeryVeryVerbose);
    bslma::TestAllocator foa("footprint", veryVeryVeryVerbose);
    bslma::DefaultAllocatorGuard dag(&doa);

    ALLOC da(&doa);
    ALLOC oa(&ooa);
    ALLOC za(&zoa);
    ALLOC sa(&soa);

    // Check remaining properties of allocator to make sure they all match
    // 'otherTraitsSet'.

    BSLMF_ASSERT(otherTraitsSet ==
               AllocatorTraits::propagate_on_container_move_assignment::value);
    ASSERT((otherTraitsSet ? sa : da) ==
                   AllocatorTraits::select_on_container_copy_construction(sa));

    // Use a table of distinct object values and expected memory usage.

    enum { NUM_DATA                         = DEFAULT_NUM_DATA };
    const  DefaultDataRow (&DATA)[NUM_DATA] = DEFAULT_DATA;

    if (veryVerbose) {
        printf("Testing swap with matching allocs, no exceptions.\n");
    }

    for (int ti = 0; ti < NUM_DATA; ++ti) {
        const char *SPEC1 = DATA[ti].d_spec;

        Obj mW(oa);     const Obj& W  = gg(&mW,  SPEC1);
        Obj mXX(sa);    const Obj& XX = gg(&mXX, SPEC1);

        if (veryVerbose) { printf("noexcep: src: "); P_(SPEC1) P(XX) }

        // Ensure the first row of the table contains the default-constructed
        // value.

        if (0 == ti) {
            ASSERTV(SPEC1, Obj(sa), W, Obj(sa) == W);
        }

        for (int member = 0; member < 2; ++member) {
            bslma::TestAllocatorMonitor oam(&ooa);

            if (member) {
                mW.swap(mW);    // member 'swap'
            }
            else {
                swap(mW, mW);   // free function 'swap'
            }

            ASSERTV(SPEC1, XX, W, XX == W);
            ASSERTV(SPEC1, oa == W.get_allocator());
            ASSERTV(SPEC1, oam.isTotalSame());
        }

        for (int tj = 0; tj < NUM_DATA; ++tj) {
            const char *const SPEC2   = DATA[tj].d_spec;

            Obj mYY(sa);   const Obj& YY = gg(&mYY, SPEC2);

            if (veryVerbose) { printf("noexcep: src: "); P_(SPEC1) P(YY) }

            ASSERT((ti == tj) == (XX == YY));

            for (int member = 0; member < 2; ++member) {
                Obj mX(oa);    const Obj& X  = gg(&mX,  SPEC1);
                Obj mY(oa);    const Obj& Y  = gg(&mY,  SPEC2);

                if (veryVerbose) {
                    T_ printf("before: "); P_(X) P(Y);
                }

                ASSERT(XX == X && YY == Y);

                if (veryVerbose) { T_ P_(SPEC2) P_(X) P_(Y) P(YY) }

                bslma::TestAllocatorMonitor oam(&ooa);

                if (member) {
                    mX.swap(mY);    // member 'swap'
                }
                else {
                    swap(mX, mY);   // free function 'swap'
                }

                ASSERTV(SPEC1, SPEC2, YY, X, YY == X);
                ASSERTV(SPEC1, SPEC2, XX, Y, XX == Y);
                ASSERTV(SPEC1, SPEC2, oa == X.get_allocator());
                ASSERTV(SPEC1, SPEC2, oa == Y.get_allocator());
                ASSERTV(SPEC1, SPEC2, oam.isTotalSame());
            }
        }
    }

    if (veryVerbose) {
        printf("Testing swap, non-matching, with injected exceptions.\n");
    }

    for (int ti = 0; ti < NUM_DATA; ++ti) {
        const char *const SPEC1  = DATA[ti].d_spec;
        const size_t      LENGTH1 = strlen(DATA[ti].d_results);

        Obj mXX(sa);    const Obj& XX = gg(&mXX, SPEC1);

        for (int tj = 0; tj < NUM_DATA; ++tj) {
            const char *const SPEC2   = DATA[tj].d_spec;
            const size_t      LENGTH2 = strlen(DATA[tj].d_results);

            if (4 < LENGTH2) {
                continue;    // time consuming, skip (it's O(LENGTH2^2))
            }

            Obj mYY(sa);   const Obj& YY = gg(&mYY, SPEC2);

            ASSERT((ti == tj) == (XX == YY));

            for (int member = 0; member < 2; ++member) {
                int numPasses = 0;
                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(ooa) {
                    ++numPasses;

                    Int64 al = ooa.allocationLimit();
                    ooa.setAllocationLimit(-1);

                    Obj mX(oa);    const Obj& X = gg(&mX, SPEC1);
                    Obj mZ(za);    const Obj& Z = gg(&mZ, SPEC2);

                    if (veryVerbose) {
                        T_ printf("before: "); P_(X) P(Z);
                    }

                    ASSERT(XX == X && YY == Z);

                    ooa.setAllocationLimit(al);
                    bslma::TestAllocatorMonitor oam(&ooa);
                    bslma::TestAllocatorMonitor oazm(&zoa);

                    if (member) {
                        mX.swap(mZ);     // member 'swap'
                    }
                    else {
                        swap(mX, mZ);    // free function 'swap'
                    }

                    ooa.setAllocationLimit(-1);

                    if (veryVerbose) {
                        T_ printf("after:  "); P_(X) P(Z);
                    }

                    ASSERTV(SPEC1, SPEC2, YY, X, YY == X);
                    ASSERTV(SPEC1, SPEC2, XX, Z, XX == Z);
                    ASSERTV(SPEC1, SPEC2, (isPropagate ? za : oa) ==
                                                            X.get_allocator());
                    ASSERTV(SPEC1, SPEC2, (isPropagate ? oa : za) ==
                                                            Z.get_allocator());

                    ASSERTV(SPEC1, SPEC2, !isPropagate == oam.isTotalUp());
                    ASSERT(!PLAT_EXC ||   !isPropagate == (1 < numPasses));
                    ASSERTV(SPEC1, SPEC2, !isPropagate == oazm.isTotalUp());

                    ASSERTV(SPEC1, SPEC2, (LENGTH1 != LENGTH2 && !isPropagate)
                                 || (oam.isInUseSame() && oazm.isInUseSame()));
                } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
            }
        }
    }

    {
        // 'A' values: Should cause memory allocation if possible.

        Obj mX(oa);     const Obj& X  = gg(&mX,  "DD");
        Obj mXX(sa);    const Obj& XX = gg(&mXX, "DD");

        Obj mY(oa);     const Obj& Y  = gg(&mY,  "ABC");
        Obj mYY(sa);    const Obj& YY = gg(&mYY, "ABC");

        if (veryVerbose) printf(
              "Invoke free 'swap' function in a context where ADL is used.\n");

        if (veryVerbose) { T_ P_(X) P(Y) }

        bslma::TestAllocatorMonitor oam(&ooa);

        invokeAdlSwap(&mX, &mY);

        ASSERTV(YY, X, YY == X);
        ASSERTV(XX, Y, XX == Y);
        ASSERT(oam.isTotalSame());

        if (veryVerbose) { T_ P_(X) P(Y) }

        if (veryVerbose) printf("Invoke std BDE pattern 'swap' function.\n");

        invokePatternSwap(&mX, &mY);

        ASSERTV(YY, X, XX == X);
        ASSERTV(XX, Y, YY == Y);
        ASSERT(oam.isTotalSame());

        if (veryVerbose) { T_ P_(X) P(Y) }
    }

    ASSERTV(e_STATEFUL == s_allocCategory || 0 == doa.numBlocksTotal());
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase20()
{
    // ------------------------------------------------------------------------
    // TESTING ERASE
    //
    // Concerns:
    //: 1 That the resulting value is correct.
    //:
    //: 2 That erasing a suffix of the array never allocates, and thus never
    //:   throws.  In particular, 'pop_back()' and 'erase(..., X.end())' do not
    //:   throw.
    //:
    //: 3 That erasing is exception neutral w.r.t. memory allocation.
    //:
    //: 4 That no memory is leaked.
    //
    // Plan:
    //   For the 'erase' methods, the concerns are simply to cover the full
    //   range of possible indices and numbers of elements.  We build a deque
    //   with a variable size and capacity, and remove a variable element or
    //   number of elements from it, and verify that size, capacity, and value
    //   are as expected:
    //      - Without exceptions, and computing the number of allocations.
    //      - In the presence of exceptions during memory allocations using a
    //        'bslma::TestAllocator' and varying its *allocation* *limit*, but
    //        not computing the number of allocations or checking on the value
    //        in case an exception is thrown (it is enough to verify that all
    //        the elements have been destroyed indirectly by making sure that
    //        there are no memory leaks).
    //   For concern 2, we verify that the number of allocations is as
    //   expected:
    //      - length of the tail (last element erased to last element) if the
    //        type uses a 'bslma' allocator, and is not moveable.
    //      - 0 otherwise.
    //
    // Testing:
    //   void pop_front();
    //   void pop_back();
    //   iterator erase(const_iterator pos);
    //   iterator erase(const_iterator first, const_iterator last);
    // ------------------------------------------------------------------------

    bslma::TestAllocator oa("object", veryVeryVeryVerbose);
    ALLOC                xoa(&oa);

    const TestValues VALUES;
    const int        NUM_VALUES = static_cast<int>(VALUES.size());

    static const struct {
        int d_lineNum;  // source line number
        int d_length;   // expected length
    } DATA[] = {
        //line  length
        //----  ------
        { L_,        0   },
        { L_,        1   },
        { L_,        2   },
        { L_,        3   },
        { L_,        4   },
        { L_,        5   },
        { L_,        6   },
        { L_,        7   },
        { L_,        8   },
        { L_,        9   },
        { L_,       11   },
        { L_,       12   },
        { L_,       14   },
        { L_,       15   },
        { L_,       16   },
        { L_,       17   }
    };
    enum { NUM_DATA = sizeof DATA / sizeof *DATA };

    if (verbose) printf("\nTesting 'pop_back' on non-empty deques.\n");
    {
        for (int i = 1; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;
            const size_t LENGTH      = INIT_LENGTH - 1;

            for (int l = i; l < NUM_DATA; ++l) {
                const size_t INIT_CAP = DATA[l].d_length;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\tWith initial "); P_(INIT_LENGTH); P(INIT_CAP);
                }

                Obj mX(xoa);  const Obj& X = mX;

                stretch(&mX, INIT_LENGTH);
                mX.reserve(INIT_CAP);

                unsigned k = 0;
                for (k = 0; k < INIT_LENGTH; ++k) {
                    bsls::ObjectBuffer<TYPE> buffer;
                    TstFacility::emplace(buffer.address(),
                                         'A' + k % NUM_VALUES,
                                         bslma::Default::defaultAllocator());
                    bslma::DestructorGuard<TYPE> guard(buffer.address());

                    mX[k] = MoveUtil::move(buffer.object());
                }

                const Int64 BB = oa.numBlocksTotal();
                const Int64  B = oa.numBlocksInUse();

                if (veryVerbose) {
                    printf("\t\tBEFORE: "); P_(BB); P(B);
                }

                mX.pop_back();

                const Int64 AA = oa.numBlocksTotal();
                const Int64  A = oa.numBlocksInUse();

                if (veryVerbose) {
                    printf("\t\tAFTER : "); P_(AA); P(A);
                    T_; T_; P_(X); P(X.capacity());
                }

                LOOP3_ASSERT(INIT_LINE, INIT_LENGTH, INIT_CAP,
                             LENGTH == X.size());
                for (k = 0; k < LENGTH; ++k) {
                    LOOP4_ASSERT(INIT_LINE, i, l, k,
                                 VALUES[k % NUM_VALUES] == X[k]);
                }
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());

#ifdef BDE_BUILD_TARGET_EXC
    if (verbose) printf("\tWith exceptions.\n");
    {
        for (int i = 1; i < NUM_DATA; ++i) {
            const int    LINE   = DATA[i].d_lineNum;
            const size_t LENGTH = DATA[i].d_length;

            for (int l = i; l < NUM_DATA; ++l) {
                const size_t CAP = DATA[l].d_length;
                ASSERT(LENGTH <= CAP);

                Obj mX(xoa);

                stretch(&mX, LENGTH);
                mX.reserve(CAP);

                if (veryVerbose) {
                    printf("\t\tWith initial "); P_(LENGTH); P(CAP);
                }

                bool exceptionCaught = false;
                try {
                    mX.pop_back();
                }
                catch (...) {
                    exceptionCaught = true;
                }
                LOOP_ASSERT(LINE, !exceptionCaught);
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());
#endif

    if (verbose) printf("\nTesting 'pop_front' on non-empty deques.\n");
    {
        for (int i = 1; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;
            const size_t LENGTH      = INIT_LENGTH - 1;

            for (int l = i; l < NUM_DATA; ++l) {
                const size_t INIT_CAP = DATA[l].d_length;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\tWith initial "); P_(INIT_LENGTH); P(INIT_CAP);
                }

                Obj mX(xoa);  const Obj& X = mX;

                stretch(&mX, INIT_LENGTH);
                mX.reserve(INIT_CAP);

                unsigned k = 0;
                for (k = 0; k < INIT_LENGTH; ++k) {
                    bsls::ObjectBuffer<TYPE> buffer;
                    TstFacility::emplace(buffer.address(),
                                         'A' + k % NUM_VALUES,
                                         bslma::Default::defaultAllocator());
                    bslma::DestructorGuard<TYPE> guard(buffer.address());

                    mX[k] = MoveUtil::move(buffer.object());
                }

                const Int64 BB = oa.numBlocksTotal();
                const Int64  B = oa.numBlocksInUse();

                if (veryVerbose) {
                    printf("\t\tBEFORE: "); P_(BB); P(B);
                }

                mX.pop_front();

                const Int64 AA = oa.numBlocksTotal();
                const Int64  A = oa.numBlocksInUse();

                if (veryVerbose) {
                    printf("\t\tAFTER : "); P_(AA); P(A);
                    T_; T_; P_(X); P(X.capacity());
                }

                LOOP3_ASSERT(INIT_LINE, INIT_LENGTH, INIT_CAP,
                             LENGTH == X.size());

                for (k = 0; k < LENGTH; ++k) {
                    LOOP4_ASSERT(INIT_LINE, i, l, k,
                                 VALUES[(k+1) % NUM_VALUES] == X[k]);
                }
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());

#ifdef BDE_BUILD_TARGET_EXC
    if (verbose) printf("\tWith exceptions.\n");
    {
        for (int i = 1; i < NUM_DATA; ++i) {
            const int    LINE   = DATA[i].d_lineNum;
            const size_t LENGTH = DATA[i].d_length;

            for (int l = i; l < NUM_DATA; ++l) {
                const size_t CAP = DATA[l].d_length;
                ASSERT(LENGTH <= CAP);

                Obj mX(xoa);

                stretch(&mX, LENGTH);
                mX.reserve(CAP);

                if (veryVerbose) {
                    printf("\t\tWith initial "); P_(LENGTH); P(CAP);
                }

                bool exceptionCaught = false;
                try {
                    mX.pop_front();
                }
                catch (...) {
                    exceptionCaught = true;
                }
                LOOP_ASSERT(LINE, !exceptionCaught);
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());
#endif

    if (verbose) printf("\nTesting 'erase(pos)' on non-empty deques.\n");
    {
        for (int i = 1; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;
            const size_t LENGTH      = INIT_LENGTH - 1;

            for (int l = i; l < NUM_DATA; ++l) {
                const size_t INIT_CAP = DATA[l].d_length;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\tWith initial "); P_(INIT_LENGTH); P(INIT_CAP);
                }

                for (size_t j = 0; j < INIT_LENGTH; ++j) {
                    const size_t POS = j;

                    Obj mX(xoa);  const Obj& X = mX;

                    stretch(&mX, INIT_LENGTH);
                    mX.reserve(INIT_CAP);

                    unsigned m = 0;
                    for (m = 0; m < INIT_LENGTH; ++m) {
                        bsls::ObjectBuffer<TYPE> buffer;
                        TstFacility::emplace(
                                           buffer.address(),
                                           'A' + m % NUM_VALUES,
                                           bslma::Default::defaultAllocator());
                        bslma::DestructorGuard<TYPE> guard(buffer.address());

                        mX[m] = MoveUtil::move(buffer.object());
                    }

                    if (veryVerbose) {
                        printf("\t\tErase one element at "); P(POS);
                    }

                    const Int64 BB = oa.numBlocksTotal();
                    const Int64  B = oa.numBlocksInUse();

                    if (veryVerbose) {
                        printf("\t\t\tBEFORE: "); P_(BB); P(B);
                    }

                    mX.erase(X.begin() + POS);

                    const Int64 AA = oa.numBlocksTotal();
                    const Int64  A = oa.numBlocksInUse();

                    if (veryVerbose) {
                        printf("\t\t\tAFTER : "); P_(AA); P(A);
                        T_; T_; T_; P_(X); P(X.capacity());
                    }

                    LOOP4_ASSERT(INIT_LINE, INIT_LENGTH, INIT_CAP, POS,
                                 LENGTH == X.size());

                    for (m = 0; m < POS; ++m) {
                        LOOP5_ASSERT(INIT_LINE, INIT_LENGTH, INIT_CAP, POS, m,
                                     VALUES[m % NUM_VALUES] == X[m]);
                    }
                    for (; m < LENGTH; ++m) {
                        LOOP5_ASSERT(INIT_LINE, INIT_LENGTH, INIT_CAP, POS, m,
                                     VALUES[(m + 1) % NUM_VALUES] == X[m]);
                    }
                }
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());

    if (verbose) printf("\tWith exceptions.\n");
    {
        for (int i = 1; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;
            const size_t LENGTH      = INIT_LENGTH - 1;

            for (int l = i; l < NUM_DATA; ++l) {
                const size_t INIT_CAP = DATA[l].d_length;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\t\tWith initial "); P_(INIT_LENGTH); P(INIT_CAP);
                }

                for (size_t j = 0; j < INIT_LENGTH; ++j) {
                    const size_t POS = j;

                    BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                        const int AL = (int) oa.allocationLimit();
                        oa.setAllocationLimit(-1);

                        Obj mX(xoa);  const Obj& X = mX;

                        stretch(&mX, INIT_LENGTH);
                        mX.reserve(INIT_CAP);

                        unsigned m = 0;
                        for (m = 0; m < INIT_LENGTH; ++m) {
                            bsls::ObjectBuffer<TYPE> buffer;
                            TstFacility::emplace(
                                           buffer.address(),
                                           'A' + m % NUM_VALUES,
                                           bslma::Default::defaultAllocator());
                            bslma::DestructorGuard<TYPE>
                                                       guard(buffer.address());

                            mX[m] = MoveUtil::move(buffer.object());
                        }

                        oa.setAllocationLimit(AL);

                        mX.erase(X.begin() + POS);  // test erase here

                        for (m = 0; m < POS; ++m) {
                            LOOP5_ASSERT(
                                      INIT_LINE, INIT_LENGTH, INIT_CAP, POS, m,
                                      VALUES[m % NUM_VALUES] == X[m]);
                        }
                        for (; m < LENGTH; ++m) {
                            LOOP5_ASSERT(
                                      INIT_LINE, INIT_LENGTH, INIT_CAP, POS, m,
                                      VALUES[(m + 1) % NUM_VALUES] == X[m]);
                        }
                    } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
                }
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());

    if (verbose) printf("\nTesting 'erase(first, last)'.\n");
    {
        for (int i = 1; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;

            for (int l = i; l < NUM_DATA; ++l) {
                const size_t INIT_CAP = DATA[l].d_length;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\tWith initial "); P_(INIT_LENGTH); P(INIT_CAP);
                }

                for (size_t j = 0; j <  INIT_LENGTH; ++j) {
                for (size_t k = j; k <= INIT_LENGTH; ++k) {
                    const size_t BEGIN_POS    = j;
                    const size_t END_POS      = k;
                    const int    NUM_ELEMENTS = (int) (END_POS - BEGIN_POS);
                    const size_t LENGTH       = INIT_LENGTH - NUM_ELEMENTS;

                    Obj mX(xoa);  const Obj& X = mX;

                    stretch(&mX, INIT_LENGTH);
                    mX.reserve(INIT_CAP);

                    unsigned m = 0;
                    for (m = 0; m < INIT_LENGTH; ++m) {
                        bsls::ObjectBuffer<TYPE> buffer;
                        TstFacility::emplace(
                                           buffer.address(),
                                           'A' + m % NUM_VALUES,
                                           bslma::Default::defaultAllocator());
                        bslma::DestructorGuard<TYPE> guard(buffer.address());

                        mX[m] = MoveUtil::move(buffer.object());
                    }

                    if (veryVerbose) {
                        printf("\t\tErase elements between ");
                        P_(BEGIN_POS); P(END_POS);
                    }

                    const Int64 BB = oa.numBlocksTotal();
                    const Int64  B = oa.numBlocksInUse();

                    if (veryVerbose) {
                        printf("\t\t\tBEFORE: "); P_(BB); P(B);
                    }

                    mX.erase(X.begin() + BEGIN_POS, X.begin() + END_POS);

                    const Int64 AA = oa.numBlocksTotal();
                    const Int64  A = oa.numBlocksInUse();

                    if (veryVerbose) {
                        printf("\t\t\t\tAFTER : "); P_(AA); P(A);
                        T_; T_; T_; P_(X); P(X.capacity());
                    }

                    LOOP4_ASSERT(INIT_LINE, INIT_LENGTH, INIT_CAP,
                                 NUM_ELEMENTS, LENGTH == X.size());

                    for (m = 0; m < BEGIN_POS; ++m) {
                        LOOP5_ASSERT(INIT_LINE, LENGTH, BEGIN_POS, END_POS, m,
                                     VALUES[m % NUM_VALUES] == X[m]);
                    }
                    for (; m < LENGTH; ++m) {
                        LOOP5_ASSERT(
                              INIT_LINE, LENGTH, BEGIN_POS, END_POS, m,
                              VALUES[(m + NUM_ELEMENTS) % NUM_VALUES] == X[m]);
                    }
                }
                }
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());

    if (verbose) printf("\tWith exceptions.\n");
    {
        for (int i = 1; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;

            for (int l = i; l < NUM_DATA; ++l) {
                const size_t INIT_CAP = DATA[l].d_length;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\t\tWith initial "); P_(INIT_LENGTH); P(INIT_CAP);
                }

                for (size_t j = 0; j <  INIT_LENGTH; ++j) {
                for (size_t k = j; k <= INIT_LENGTH; ++k) {
                    const size_t BEGIN_POS    = j;
                    const size_t END_POS      = k;
                    const int    NUM_ELEMENTS = (int) (END_POS - BEGIN_POS);
                    const size_t LENGTH       = INIT_LENGTH - NUM_ELEMENTS;

                    if (veryVerbose) {
                        printf("\t\t\tErase elements between ");
                        P_(BEGIN_POS); P(END_POS);
                    }

                    BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                        const int AL = (int) oa.allocationLimit();
                        oa.setAllocationLimit(-1);

                        Obj mX(xoa);  const Obj& X = mX;

                        stretch(&mX, INIT_LENGTH);
                        mX.reserve(INIT_CAP);

                        unsigned m = 0;
                        for (m = 0; m < INIT_LENGTH; ++m) {
                            bsls::ObjectBuffer<TYPE> buffer;
                            TstFacility::emplace(
                                           buffer.address(),
                                           'A' + m % NUM_VALUES,
                                           bslma::Default::defaultAllocator());
                            bslma::DestructorGuard<TYPE>
                                                       guard(buffer.address());

                            mX[m] = MoveUtil::move(buffer.object());
                        }

                        oa.setAllocationLimit(AL);

                        mX.erase(X.begin() + BEGIN_POS, X.begin() + END_POS);
                                                             // test erase here

                        for (m = 0; m < BEGIN_POS; ++m) {
                            LOOP5_ASSERT(INIT_LINE, INIT_LENGTH, INIT_CAP,
                                         END_POS, m,
                                               VALUES[m % NUM_VALUES] == X[m]);
                        }
                        for (; m < LENGTH; ++m) {
                            LOOP5_ASSERT(INIT_LINE, INIT_LENGTH, INIT_CAP,
                                         END_POS, m,
                              VALUES[(m + NUM_ELEMENTS) % NUM_VALUES] == X[m]);
                        }
                    } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
                }
                }
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());
}

template <class TYPE, class ALLOC>
template <class CONTAINER>
void TestDriver<TYPE,ALLOC>::testCase19(const CONTAINER&)
{
    // ------------------------------------------------------------------------
    // TESTING INPUT-RANGE INSERTION
    //
    // We have the following concerns:
    //   1) That the resulting deque value is correct.
    //   2) That the initial range is correctly imported and then moved if the
    //      initial 'FWD_ITER' is an input iterator.
    //   3) That the resulting capacity is correctly set up if the initial
    //      'FWD_ITER' is a random-access iterator.
    //   4) That existing elements are moved without copy-construction if the
    //      bitwise-moveable trait is present.
    //   5) That insertion is exception neutral w.r.t. memory allocation.
    //   6) The internal memory management system is hooked up properly
    //      so that *all* internally allocated memory draws from a
    //      user-supplied allocator whenever one is specified.
    //
    // Plan:
    //   For insertion we will create objects of varying sizes with different
    //   'value' as argument.  Perform the above tests:
    //      - Using 'CONTAINER::const_iterator'.
    //      - Without exceptions, and compute the number of allocations.
    //      - In the presence of exceptions during memory allocations using
    //        a 'bslma::TestAllocator' and varying its *allocation* *limit*,
    //        but do not compute the number of allocations.
    //   and use basic accessors to verify
    //      - size
    //      - capacity
    //      - element value at each index position { 0 .. length - 1 }.
    //   In addition, the number of allocations should reflect proper internal
    //   memory management: the number of allocations should equal the sum of
    //      - NUM_ELEMENTS + (INIT_LENGTH - POS) if the type uses an allocator
    //        and is not bitwise-moveable,  0 otherwise
    //      - 1 if there is a change in capacity, 0 otherwise
    //      - 1 if the type uses an allocator and the value is an alias.
    //      -
    //   For concern 4, we test with a bitwise-moveable type that the only
    //   reallocations are for the new elements plus one if the deque
    //   undergoes a reallocation (capacity changes).
    //
    // Testing:
    //   iterator insert(const_iterator pos, ITER first, ITER last);
    // ------------------------------------------------------------------------

    bslma::TestAllocator oa("object", veryVeryVeryVerbose);
    ALLOC                xoa(&oa);

    const TYPE DEFAULT_VALUE = TYPE(::DEFAULT_VALUE);

    const TYPE         *values     = 0;
    const TYPE *const&  VALUES     = values;
    const int           NUM_VALUES = getValues(&values);

    static const struct {
        int d_lineNum;  // source line number
        int d_length;   // expected length
    } DATA[] = {
        //line  length
        //----  ------
        { L_,        0   },
        { L_,        1   },
        { L_,        2   },
        { L_,        3   },
        { L_,        4   },
        { L_,        5   },
        { L_,        6   },
        { L_,        7   },
        { L_,        8   },
        { L_,        9   },
        { L_,       11   },
        { L_,       12   },
        { L_,       14   },
        { L_,       15   },
        { L_,       16   },
        { L_,       17   }
    };
    enum { NUM_DATA = sizeof DATA / sizeof *DATA };

    static const struct {
        int         d_lineNum;  // source line number
        const char *d_spec_p;   // container spec
    } U_DATA[] = {
        //line  spec                            length
        //----  ----                            ------
        { L_,   ""                        }, // 0
        { L_,   "A"                       }, // 1
        { L_,   "AB"                      }, // 2
        { L_,   "ABC"                     }, // 3
        { L_,   "ABCD"                    }, // 4
        { L_,   "ABCDE"                   }, // 5
        { L_,   "ABCDEAB"                 }, // 7
        { L_,   "ABCDEABC"                }, // 8
        { L_,   "ABCDEABCD"               }, // 9
        { L_,   "ABCDEABCDEABCDE"         }, // 15
        { L_,   "ABCDEABCDEABCDEA"        }, // 16
        { L_,   "ABCDEABCDEABCDEAB"       }  // 17
    };
    enum { NUM_U_DATA = sizeof U_DATA / sizeof *U_DATA };

    if (verbose) printf("\tUsing 'CONTAINER::const_iterator'.\n");
    {
        for (int i = 0; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;

            for (int l = i; l < NUM_DATA; ++l) {
                const size_t INIT_CAP = DATA[l].d_length;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\t\tWith initial value of ");
                    P_(INIT_LENGTH); P_(INIT_CAP);
                    printf("using default value.\n");
                }

                for (int ti = 0; ti < NUM_U_DATA; ++ti) {
                    const int     LINE         = U_DATA[ti].d_lineNum;
                    const char   *SPEC         = U_DATA[ti].d_spec_p;
                    const int     NUM_ELEMENTS = (int) strlen(SPEC);
                    const size_t  LENGTH       = INIT_LENGTH + NUM_ELEMENTS;

                    CONTAINER mU(gV(SPEC));  const CONTAINER& U = mU;

                    for (size_t j = 0; j <= INIT_LENGTH; ++j) {
                        const size_t POS = j;

                        Obj mX(INIT_LENGTH, xoa); const Obj& X = mX;
                        mX.reserve(INIT_CAP);

                        size_t k;
                        for (k = 0; k < INIT_LENGTH; ++k) {
                            mX[k] = VALUES[k % NUM_VALUES];
                        }

                        if (veryVerbose) {
                            printf("\t\t\tInsert "); P_(NUM_ELEMENTS);
                            printf("at "); P_(POS);
                            printf("using "); P(SPEC);
                        }

                        const Int64 BB = oa.numBlocksTotal();
                        const Int64  B = oa.numBlocksInUse();

                        if (veryVerbose) {
                            printf("\t\t\t\tBEFORE: "); P_(BB); P_(B); P(mX);
                        }

                        iterator result =
                                mX.insert(X.begin() + POS, U.begin(), U.end());

                        const Int64 AA = oa.numBlocksTotal();
                        const Int64  A = oa.numBlocksInUse();

                        if (veryVerbose) {
                            printf("\t\t\t\tAFTER : "); P_(AA); P_(A); P(mX);
                            T_; T_; T_; T_; P_(X); P(X.capacity());
                        }

                        LOOP4_ASSERT(INIT_LINE, LINE, i, j,
                                     LENGTH == X.size());
                        LOOP4_ASSERT(INIT_LINE, LINE, i, j,
                                     X.begin() + POS == result);

                        size_t m;
                        for (k = 0; k < POS; ++k) {
                            LOOP4_ASSERT(INIT_LINE, LINE, j, k,
                                         VALUES[k % NUM_VALUES] == X[k]);
                        }
                        for (m = 0; k < POS + NUM_ELEMENTS; ++k, ++m) {
                            LOOP5_ASSERT(INIT_LINE, LINE, j, k, m,
                                         U[m] == X[k]);
                        }
                        for (m = POS; k < LENGTH; ++k, ++m) {
                            LOOP5_ASSERT(INIT_LINE, LINE, j, k, m,
                                         VALUES[m % NUM_VALUES] == X[k]);
                        }
                    }
                }
                ASSERT(0 == oa.numMismatches());
                ASSERT(0 == oa.numBlocksInUse());
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());

    if (verbose) printf("\tWith exceptions.\n");
    {
        for (int i = 0; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;

            if (4 < INIT_LENGTH && NUM_DATA-1 != i) {
                continue;
            }

            for (int l = i; l < NUM_DATA; ++l) {
                const size_t INIT_CAP = DATA[l].d_length;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\t\tWith initial value of ");
                    P_(INIT_LENGTH); P_(INIT_CAP);
                    printf("using default value.\n");
                }

                for (int ti = 0; ti < NUM_U_DATA; ++ti) {
                    const int     LINE         = U_DATA[ti].d_lineNum;
                    const char   *SPEC         = U_DATA[ti].d_spec_p;
                    const int     NUM_ELEMENTS = (int) strlen(SPEC);
                    const size_t  LENGTH       = INIT_LENGTH + NUM_ELEMENTS;

                    if (4 < NUM_ELEMENTS && NUM_U_DATA-1 != ti) {
                        continue;
                    }

                    CONTAINER mU(gV(SPEC));  const CONTAINER& U = mU;

                    for (size_t j = 0; j <= INIT_LENGTH; ++j) {
                        const size_t POS = j;

                        if (veryVerbose) {
                            printf("\t\t\tInsert "); P_(NUM_ELEMENTS);
                            printf("at "); P_(POS);
                            printf("using "); P(SPEC);
                        }

                        BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                            const int AL = (int) oa.allocationLimit();
                            oa.setAllocationLimit(-1);

                            Obj mX(INIT_LENGTH, DEFAULT_VALUE, xoa);
                            mX.reserve(INIT_CAP);
                            const Obj& X = mX;

                            oa.setAllocationLimit(AL);

                            if (veryVerbose) {
                                printf("\t\t\tBefore "); P(mX);
                            }

                            iterator result =
                                mX.insert(X.begin() + POS, U.begin(), U.end());
                                                         // test insertion here

                            if (veryVerbose) {
                                printf("\t\t\tAfter "); P(mX);
                            }

                            if (veryVerbose) {
                                T_; T_; T_; P_(X); P(X.capacity());
                            }

                            LOOP4_ASSERT(INIT_LINE, LINE, i, j,
                                         LENGTH == X.size());
                            LOOP4_ASSERT(INIT_LINE, LINE, i, j,
                                         X.begin() + POS == result);

                            size_t k;
                            for (k = 0; k < POS; ++k) {
                                LOOP5_ASSERT(INIT_LINE, LINE, i, j, k,
                                             DEFAULT_VALUE == X[k]);
                            }
                            for (; k < POS + NUM_ELEMENTS; ++k) {
                                LOOP5_ASSERT(INIT_LINE, LINE, i, j, k,
                                             U[k - POS] == X[k]);
                            }
                            for (; k < LENGTH; ++k) {
                                LOOP5_ASSERT(INIT_LINE, LINE, i, j, k,
                                             DEFAULT_VALUE == X[k]);
                            }
                        } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
                    }
                }
                ASSERT(0 == oa.numMismatches());
                ASSERT(0 == oa.numBlocksInUse());
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase18()
{
    // ------------------------------------------------------------------------
    // TESTING VALUE INSERTION
    //
    // We have the following concerns:
    //   1) That the resulting deque value is correct.
    //   2) That the 'insert' return (if any) value is a valid iterator, even
    //      when the deque underwent a reallocation.
    //   3) That the resulting capacity is correctly set up.
    //   4) That existing elements are moved without copy-construction if the
    //      bitwise-moveable trait is present.
    //   5) That insertion is exception neutral w.r.t. memory allocation.
    //   6) The internal memory management system is hooked up properly
    //      so that *all* internally allocated memory draws from a
    //      user-supplied allocator whenever one is specified.
    //   7) The move 'push_back' and 'insert' move the value, capacity, and
    //      allocator correctly, and without performing any allocation.
    //   8) That inserting a 'const T& value' that is a reference to an element
    //      of the deque does not suffer from aliasing problems.
    //
    // Plan:
    //   For insertion we will create objects of varying sizes and capacities
    //   containing default values, and insert a distinct 'value' at various
    //   positions, or a variable number of copies of this value.  Perform the
    //   above tests:
    //      - Without exceptions, and compute the number of allocations.
    //      - In the presence of exceptions during memory allocations using
    //        a 'bslma::TestAllocator' and varying its *allocation* *limit*,
    //        but do not compute the number of allocations.
    //   and use basic accessors to verify the resulting
    //      - size
    //      - capacity
    //      - element value at each index position { 0 .. length - 1 }.
    //   In addition, the number of allocations should reflect proper internal
    //   memory management: the number of allocations should equal the sum of
    //      - NUM_ELEMENTS + (INIT_LENGTH - POS) if the type uses an allocator
    //        and is not bitwise-moveable,  0 otherwise
    //      - 1 if there a change in capacity, 0 otherwise
    //      - 1 if the type uses an allocator and the value is an alias.
    //
    //   For concerns 4 and 7, we test with a bitwise-moveable type that the
    //   only allocation for a move 'push_back' or 'insert' is the one for the
    //   deque reallocation (if capacity changes; all elements are moved), and
    //   for insertion the only reallocations should be for the new elements
    //   plus one if the deque undergoes a reallocation (capacity changes).
    //
    //   For concern 8, we insert an element of some deque where all the
    //   values are distinct into the same deque, taking care of the cases
    //   where the reference is before or after the position of insertion, and
    //   that the deque undergoes a reallocation or not (i.e., capacity
    //   changes or not).  We verify that the value is as expected, i.e.,
    //   identical to t it would be if the value had not been aliased.
    //
    // Testing:
    //   iterator insert(const_iterator pos, const T& value);
    //   iterator insert(const_iterator pos, size_type n, const T& value);
    // ------------------------------------------------------------------------

    bslma::TestAllocator oa("object", veryVeryVeryVerbose);
    ALLOC                xoa(&oa);

    const TYPE DEFAULT_VALUE = TYPE(::DEFAULT_VALUE);

    const TYPE         *values     = 0;
    const TYPE *const&  VALUES     = values;
    const int           NUM_VALUES = getValues(&values);

    static const struct {
        int d_lineNum;  // source line number
        int d_length;   // expected length
    } DATA[] = {
        //line  length
        //----  ------
        { L_,        0   },
        { L_,        1   },
        { L_,        2   },
        { L_,        3   },
        { L_,        4   },
        { L_,        5   },
        { L_,        6   },
        { L_,        7   },
        { L_,        8   },
        { L_,        9   },
        { L_,       11   },
        { L_,       12   },
        { L_,       14   },
        { L_,       15   },
        { L_,       16   },
        { L_,       17   }
    };
    enum { NUM_DATA = sizeof DATA / sizeof *DATA };

    if (verbose) printf("\nTesting 'insert'.\n");

    if (verbose) printf("\tUsing a single 'value'.\n");
    {
        for (int i = 0; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;
            const TYPE   VALUE       = VALUES[i % NUM_VALUES];
            const size_t LENGTH      = INIT_LENGTH + 1;

            for (int l = i; l < NUM_DATA; ++l) {
                const size_t INIT_CAP = DATA[l].d_length;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\t\tWith initial value of ");
                    P_(INIT_LENGTH); P_(INIT_CAP);
                    printf("using default value.\n");
                }

                for (size_t j = 0; j <= INIT_LENGTH; ++j) {
                    const size_t POS = j;

                    Obj mX(INIT_LENGTH, xoa);  const Obj& X = mX;
                    mX.reserve(INIT_CAP);

                    size_t k;
                    for (k = 0; k < INIT_LENGTH; ++k) {
                        mX[k] = VALUES[k % NUM_VALUES];
                    }

                    if (veryVerbose) {
                        printf("\t\t\tInsert with "); P_(LENGTH);
                        printf(" at "); P_(POS);
                        printf(" using "); P(VALUE);
                    }

                    const Int64 BB = oa.numBlocksTotal();
                    const Int64  B = oa.numBlocksInUse();

                    if (veryVerbose) {
                        printf("\t\t\t\tBEFORE: "); P_(BB); P(B);
                        T_; T_; T_; T_; P_(X); P(X.capacity());
                    }

                    iterator result = mX.insert(X.begin() + POS, VALUE);

                    const Int64 AA = oa.numBlocksTotal();
                    const Int64  A = oa.numBlocksInUse();

                    if (veryVerbose) {
                        printf("\t\t\t\tAFTER : "); P_(AA); P(A);
                        T_; T_; T_; T_; P_(X); P(X.capacity());
                    }

                    LOOP3_ASSERT(INIT_LINE, i, j, LENGTH == X.size());
                    LOOP3_ASSERT(INIT_LINE, i, j, X.begin() + POS == result);

                    for (k = 0; k < POS; ++k) {
                        LOOP4_ASSERT(INIT_LINE, LENGTH, POS, k,
                                     VALUES[k % NUM_VALUES] == X[k]);
                    }
                    LOOP3_ASSERT(INIT_LINE, LENGTH, POS, VALUE == X[POS]);
                    for (++k; k < LENGTH; ++k) {
                        LOOP4_ASSERT(INIT_LINE, LENGTH, POS, k,
                                     VALUES[(k - 1) % NUM_VALUES] == X[k]);
                    }
                }
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());

    if (verbose) printf("\tUsing 'n' copies of 'value'.\n");
    {
        for (int i = 0; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;

            for (int l = i; l < NUM_DATA; ++l) {
                const size_t INIT_CAP = DATA[l].d_length;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\t\tWith initial value of ");
                    P_(INIT_LENGTH); P_(INIT_CAP);
                    printf("using default value.\n");
                }

                for (int ti = 0; ti < NUM_DATA; ++ti) {
                    const int    LINE         = DATA[ti].d_lineNum;
                    const int    NUM_ELEMENTS = DATA[ti].d_length;
                    const TYPE   VALUE        = VALUES[ti % NUM_VALUES];
                    const size_t LENGTH       = INIT_LENGTH + NUM_ELEMENTS;

                    for (size_t j = 0; j <= INIT_LENGTH; ++j) {
                        const size_t POS = j;

                        Obj mX(INIT_LENGTH, xoa); const Obj& X = mX;
                        mX.reserve(INIT_CAP);

                        size_t k;
                        for (k = 0; k < INIT_LENGTH; ++k) {
                            mX[k] = VALUES[k % NUM_VALUES];
                        }

                        if (veryVerbose) {
                            printf("\t\t\tInsert "); P_(NUM_ELEMENTS);
                            printf("at "); P_(POS);
                            printf("using "); P(VALUE);
                        }

                        const Int64 BB = oa.numBlocksTotal();
                        const Int64  B = oa.numBlocksInUse();

                        if (veryVerbose) {
                            printf("\t\t\t\tBEFORE: "); P_(BB); P(B);
                        }

                        iterator result =
                               mX.insert(X.begin() + POS, NUM_ELEMENTS, VALUE);

                        const Int64 AA = oa.numBlocksTotal();
                        const Int64  A = oa.numBlocksInUse();

                        if (veryVerbose) {
                            printf("\t\t\t\tAFTER : "); P_(AA); P(A);
                            T_; T_; T_; T_; P_(X); P(X.capacity());
                        }

                        LOOP4_ASSERT(INIT_LINE, LINE, i, j,
                                     LENGTH == X.size());
                        LOOP4_ASSERT(INIT_LINE, LINE, i, j,
                                     X.begin() + POS == result);

                        size_t m = 0;
                        for (k = 0; k < POS; ++k) {
                            LOOP4_ASSERT(INIT_LINE, LINE, j, k,
                                         VALUES[k % NUM_VALUES] == X[k]);
                        }
                        for (; k < POS + NUM_ELEMENTS; ++k) {
                            LOOP4_ASSERT(INIT_LINE, LINE, j, k,
                                         VALUE == X[k]);
                        }
                        for (m = POS; k < LENGTH; ++k, ++m) {
                            LOOP5_ASSERT(INIT_LINE, LINE, j, k, m,
                                         VALUES[m % NUM_VALUES] == X[k]);
                        }
                    }
                }
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());

    if (verbose) printf("\tWith exceptions.\n");
    {
        for (int i = 0; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;

            if (4 < INIT_LENGTH && NUM_DATA-1 != i) {
                continue;
            }

            for (int l = i; l < NUM_DATA; ++l) {
                const size_t INIT_CAP = DATA[l].d_length;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\t\tWith initial value of ");
                    P_(INIT_LENGTH); P_(INIT_CAP);
                    printf("using default value.\n");
                }

                for (int ti = 0; ti < NUM_DATA; ++ti) {
                    const int    LINE         = DATA[ti].d_lineNum;
                    const size_t NUM_ELEMENTS = DATA[ti].d_length;
                    const TYPE   VALUE        = VALUES[ti % NUM_VALUES];
                    const size_t LENGTH       = INIT_LENGTH + NUM_ELEMENTS;

                    if (4 < NUM_ELEMENTS && NUM_DATA-1 != ti) {
                        continue;
                    }

                    for (size_t j = 0; j <= INIT_LENGTH; ++j) {
                        const size_t POS = j;

                        BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                            const int AL = (int) oa.allocationLimit();
                            oa.setAllocationLimit(-1);

                            Obj mX(INIT_LENGTH, DEFAULT_VALUE, xoa);
                            mX.reserve(INIT_CAP);
                            const Obj& X = mX;

                            oa.setAllocationLimit(AL);

                            if (veryVerbose) {
                                T_; T_; T_; P_(X); P_(X.capacity()); P(POS);
                            }

                            iterator result =
                               mX.insert(X.begin() + POS, NUM_ELEMENTS, VALUE);
                                                         // test insertion here

                            if (veryVerbose) {
                                T_; T_; T_; printf("After: "); P_(X);
                            }

                            LOOP4_ASSERT(INIT_LINE, LINE, i, j,
                                         LENGTH == X.size());
                            LOOP4_ASSERT(INIT_LINE, LINE, i, j,
                                         X.begin() + POS == result);

                            size_t k;
                            for (k = 0; k < POS; ++k) {
                                LOOP5_ASSERT(INIT_LINE, LINE, i, j, k,
                                             DEFAULT_VALUE == X[k]);
                            }
                            for (; k < POS + NUM_ELEMENTS; ++k) {
                                LOOP5_ASSERT(INIT_LINE, LINE, i, j, k,
                                             VALUE == X[k]);
                            }
                            for (; k < LENGTH; ++k) {
                                LOOP5_ASSERT(INIT_LINE, LINE, i, j, k,
                                             DEFAULT_VALUE == X[k]);
                            }
                        } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
                    }
                }
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());

    if (verbose) printf("\tTesting aliasing concerns.\n");
    {
        for (int i = 0; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;
            const size_t LENGTH      = INIT_LENGTH + 1;

            for (int l = i; l < NUM_DATA; ++l) {
                const size_t INIT_CAP = DATA[l].d_length;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\t\tWith initial value of ");
                    P_(INIT_LENGTH); P_(INIT_CAP);
                    printf("using distinct (cyclic) values.\n");
                }

                for (size_t j = 0; j <= INIT_LENGTH; ++j) {
                    const size_t POS = j;

                    for (size_t h = 0; h < INIT_LENGTH; ++h) {
                        const size_t INDEX = h;

                        Obj mX(INIT_LENGTH, DEFAULT_VALUE, xoa);
                        mX.reserve(INIT_CAP);
                        const Obj& X = mX;

                        for (size_t k = 0; k < INIT_LENGTH; ++k) {
                            mX[k] = VALUES[k % NUM_VALUES];
                        }

                        Obj mY(X);  const Obj& Y = mY;  // control

                        if (veryVerbose) {
                            printf("\t\t\tInsert with "); P_(LENGTH);
                            printf(" at "); P_(POS);
                            printf(" using value at "); P_(INDEX);
                            printf("\n");
                        }

                        mY.insert(Y.begin() + POS, X[INDEX]);  // control
                        mX.insert(X.begin() + POS, X[INDEX]);

                        if (veryVerbose) {
                            T_; T_; T_; T_; P(X);
                            T_; T_; T_; T_; P(Y);
                        }

                        LOOP5_ASSERT(INIT_LINE, i, INIT_CAP, POS, INDEX,
                                     X == Y);
                    }
                }
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase17()
{
    // ------------------------------------------------------------------------
    // TESTING PUSH_FRONT & PUSH_BACK
    //
    // We have the following concerns:
    //   1) That the resulting deque value is correct.
    //   2) That the resulting capacity is correctly set up.
    //   3) That insertion is exception neutral w.r.t. memory allocation.
    //   4) The internal memory management system is hooked up properly
    //      so that *all* internally allocated memory draws from a
    //      user-supplied allocator whenever one is specified.
    //   5) The initial internal state of the deque will not affect the
    //      correctness of the operation.
    //
    // Plan:
    //   For insertion we will create objects of varying sizes and capacities
    //   containing default values, and insert a distinct 'value' using
    //   'push_back' and 'push_front'.  Perform the above test:
    //      - Without exceptions, and compute the number of allocations.
    //      - In the presence of exceptions during memory allocations using
    //        a 'bslma::TestAllocator' and varying its *allocation* *limit*,
    //        but do not compute the number of allocations.
    //   and use basic accessors to verify the resulting
    //      - size
    //      - element value at each index position { 0 .. length - 1 }.
    //
    //   For concern 5, 'setInternalState' will be used to permutate the
    //   internal state of the deque.
    //
    // Testing:
    //   void push_front(const T& value);
    //   void push_back(const T& value);
    // ------------------------------------------------------------------------

    bslma::TestAllocator oa("object", veryVeryVeryVerbose);
    ALLOC                xoa(&oa);

    const TYPE DEFAULT_VALUE = TYPE(::DEFAULT_VALUE);

    const TYPE         *values     = 0;
    const TYPE *const&  VALUES     = values;
    const int           NUM_VALUES = getValues(&values);

    static const struct {
        int d_lineNum;  // source line number
        int d_length;   // expected length
    } DATA[] = {
        //line  length
        //----  ------
        { L_,        0   },
        { L_,        1   },
        { L_,        2   },
        { L_,        3   },
        { L_,        4   },
        { L_,        5   },
        { L_,        6   },
        { L_,        7   },
        { L_,        8   },
        { L_,        9   },
        { L_,       11   },
        { L_,       12   },
        { L_,       14   },
        { L_,       15   },
        { L_,       16   },
        { L_,       17   }
    };
    enum { NUM_DATA = sizeof DATA / sizeof *DATA };

    if (verbose) printf("\nTesting 'push_back'.\n");
    {
        for (int i = 0; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;
            const TYPE   VALUE       = VALUES[i % NUM_VALUES];
            const size_t LENGTH      = INIT_LENGTH + 1;

            for (size_t j = INIT_LENGTH; j <= LENGTH; ++j) {
                const size_t INIT_CAP = j;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\t\tWith initial value of ");
                    P_(INIT_LENGTH); P_(INIT_CAP);
                    printf("using default value.\n");
                }

                Obj mX(INIT_LENGTH, DEFAULT_VALUE, xoa);
                mX.reserve(INIT_CAP);
                const Obj& X = mX;

                if (veryVerbose) {
                    printf("\t\t\t'push_back' using "); P(VALUE);
                }

                TYPE mV(VALUE);

                const Int64 BB = oa.numBlocksTotal();
                const Int64  B = oa.numBlocksInUse();

                if (veryVerbose) {
                    printf("\t\t\t\tBEFORE: "); P_(BB); P(B);
                }

                mX.push_back(mV);

                const Int64 AA = oa.numBlocksTotal();
                const Int64  A = oa.numBlocksInUse();

                if (veryVerbose) {
                    printf("\t\t\t\tAFTER : "); P_(AA); P(A);
                    T_; T_; T_; T_; P_(X); P(X.capacity());
                }

                LOOP3_ASSERT(INIT_LINE, i, INIT_CAP, LENGTH == X.size());

                size_t k = 0;
                for (k = 0; k < INIT_LENGTH; ++k) {
                    LOOP4_ASSERT(INIT_LINE, INIT_CAP, LENGTH, k,
                                 DEFAULT_VALUE == X[k]);
                }
                LOOP3_ASSERT(INIT_LINE, INIT_CAP, INIT_LENGTH,
                             VALUE == X[INIT_LENGTH]);
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());

    if (verbose) printf("\nTesting 'push_front'.\n");
    {
        for (int i = 0; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;
            const TYPE   VALUE       = VALUES[i % NUM_VALUES];
            const size_t LENGTH      = INIT_LENGTH + 1;

            for (size_t j = INIT_LENGTH; j <= LENGTH; ++j) {
                const size_t INIT_CAP = j;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\t\tWith initial value of ");
                    P_(INIT_LENGTH); P_(INIT_CAP);
                    printf("using default value.\n");
                }

                Obj mX(INIT_LENGTH, DEFAULT_VALUE, xoa);
                mX.reserve(INIT_CAP);
                const Obj& X = mX;

                if (veryVerbose) {
                    printf("\t\t\t'push_front' using "); P(VALUE);
                }

                TYPE mV(VALUE);

                const Int64 BB = oa.numBlocksTotal();
                const Int64  B = oa.numBlocksInUse();

                if (veryVerbose) {
                    printf("\t\t\t\tBEFORE: "); P_(BB); P(B);
                }

                mX.push_front(mV);

                const Int64 AA = oa.numBlocksTotal();
                const Int64  A = oa.numBlocksInUse();

                if (veryVerbose) {
                    printf("\t\t\t\tAFTER : "); P_(AA); P(A);
                    T_; T_; T_; T_; P_(X); P(X.capacity());
                }

                LOOP3_ASSERT(INIT_LINE, i, INIT_CAP, LENGTH == X.size());

                size_t k = 0;
                LOOP3_ASSERT(INIT_LINE, INIT_CAP, INIT_LENGTH,
                             VALUE == X[k]);
                for (k = 1; k < LENGTH; ++k) {
                    LOOP4_ASSERT(INIT_LINE, INIT_CAP, LENGTH, k,
                                 DEFAULT_VALUE == X[k]);
                }
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());

    if (verbose) printf("\tTesting 'push_back' with exceptions.\n");
    {
        for (int i = 0; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;
            const size_t LENGTH      = INIT_LENGTH + 1;
            const TYPE   VALUE       = VALUES[i % NUM_VALUES];

            for (size_t l = INIT_LENGTH; l < LENGTH; ++l) {
                const size_t INIT_CAP = l;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\t\tWith initial value of ");
                    P_(INIT_LENGTH); P_(INIT_CAP);
                    printf("using default value.\n");
                }

                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                    const int AL = (int) oa.allocationLimit();
                    oa.setAllocationLimit(-1);

                    Obj mX(INIT_LENGTH, DEFAULT_VALUE, xoa);
                    mX.reserve(INIT_CAP);
                    const Obj& X = mX;

                    oa.setAllocationLimit(AL);

                    if (veryVerbose) {
                        T_; T_; T_; printf("BEFORE: "); P_(X); P(X.capacity());
                    }

                    mX.push_back(VALUE);

                    if (veryVerbose) {
                        T_; T_; T_; printf("AFTER : "); P_(X); P(X.capacity());
                    }

                    LOOP3_ASSERT(INIT_LINE, i, l, LENGTH == X.size());

                    size_t k;
                    for (k = 0; k < INIT_LENGTH; ++k) {
                        LOOP4_ASSERT(INIT_LINE, i, l, k,
                                     DEFAULT_VALUE == X[k]);
                    }
                    LOOP3_ASSERT(INIT_LINE, i, l, VALUE == X[k]);
                } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());

    if (verbose) printf("\tTesting 'push_front' with exceptions.\n");
    {
        for (int i = 0; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;
            const size_t LENGTH      = INIT_LENGTH + 1;
            const TYPE   VALUE       = VALUES[i % NUM_VALUES];

            for (size_t l = INIT_LENGTH; l <= LENGTH; ++l) {
                const size_t INIT_CAP = l;
                ASSERT(INIT_LENGTH <= INIT_CAP);

                if (veryVerbose) {
                    printf("\t\tWith initial value of ");
                    P_(INIT_LENGTH); P_(INIT_CAP);
                    printf("using default value.\n");
                }

                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                    const int AL = (int) oa.allocationLimit();
                    oa.setAllocationLimit(-1);

                    Obj mX(INIT_LENGTH, DEFAULT_VALUE, xoa);
                    mX.reserve(INIT_CAP);
                    const Obj& X = mX;

                    oa.setAllocationLimit(AL);

                    if (veryVerbose) {
                        T_; T_; T_; printf("BEFORE: "); P_(X); P(X.capacity());
                    }

                    mX.push_front(VALUE);

                    if (veryVerbose) {
                        T_; T_; T_; printf("AFTER : "); P_(X); P(X.capacity());
                    }

                    LOOP3_ASSERT(INIT_LINE, i, l, LENGTH == X.size());

                    size_t k = 0;
                    LOOP3_ASSERT(INIT_LINE, i, l, VALUE == X[k]);
                    for (k = 1; k < LENGTH; ++k) {
                        LOOP4_ASSERT(INIT_LINE, i, l, k,
                                     DEFAULT_VALUE == X[k]);
                    }
                } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
            }
        }
    }
    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase16()
{
    // ------------------------------------------------------------------------
    // TESTING ITERATORS
    //
    // Concerns:
    //: 1 That 'begin' and 'end' return mutable iterators for a reference to a
    //:   modifiable deque, and non-mutable iterators otherwise.
    //:
    //: 2 That the range '[begin(), end())' equals the value of the deque.
    //:
    //: 3 Same concerns with 'rbegin' and 'rend'.
    //:
    //: 4 That 'iterator' is a pointer to 'TYPE'.
    //:
    //: 5 That 'const_iterator' is a pointer to 'const TYPE'.
    //:
    //: 6 That 'reverse_iterator' and 'const_reverse_iterator' are implemented
    //:   by the (fully-tested) 'bslstl::ReverseIterator' over a pointer to
    //:   'TYPE' or 'const TYPE'.
    //
    // Plan:
    //   For 1--3, for each value given by variety of specifications of
    //   different lengths, create a test deque with this value, and access
    //   each element in sequence and in reverse sequence, both as a modifiable
    //   reference (setting it to a default value, then back to its original
    //   value, and as a non-modifiable reference.
    //
    //   For 4--6, use 'bsl::is_same' to assert the identity of iterator
    //   types.  Note that these concerns let us get away with other concerns
    //   such as testing that 'iter[i]' and 'iter + i' advance 'iter' by the
    //   correct number 'i' of positions, and other concern about traits,
    //   because 'bslstl::IteratorTraits' and 'bslstl::ReverseIterator' have
    //   already been fully tested in the 'bslstl_iterator' component.
    //
    // Testing:
    //   iterator begin();
    //   iterator end();
    //   reverse_iterator rbegin();
    //   reverse_iterator rend();
    //   const_iterator begin() const;
    //   const_iterator cbegin() const;
    //   const_iterator end() const;
    //   const_iterator cend() const;
    //   const_reverse_iterator rbegin() const;
    //   const_reverse_iterator crbegin() const;
    //   const_reverse_iterator rend() const;
    //   const_reverse_iterator crend() const;
    // ------------------------------------------------------------------------

    bslma::TestAllocator oa("object", veryVeryVeryVerbose);
    ALLOC                xoa(&oa);

    const TYPE DEFAULT_VALUE = TYPE();
    const int  BLOCK_LENGTH  = Deque_BlockLengthCalcUtil<TYPE>::BLOCK_LENGTH;

    static const struct {
        int         d_lineNum;  // source line number
        const char *d_spec_p;   // initial
    } DATA[] = {
        { L_,  ""                },
        { L_,  "A"               },
        { L_,  "ABC"             },
        { L_,  "ABCD"            },
        { L_,  "ABCDE"           },
        { L_,  "ABCDEAB"         },
        { L_,  "ABCDEABC"        },
        { L_,  "ABCDEABCD"       }
    };
    enum { NUM_DATA = sizeof DATA / sizeof *DATA };

    if (verbose) printf("Testing 'iterator', 'begin', and 'end',"
                        " and 'const' variants.\n");
    {
        ASSERT(1 == (bsl::is_same<iterator,
                        bslstl::RandomAccessIterator<TYPE,
                            bslalg::DequeIterator<TYPE, BLOCK_LENGTH>
                                                                  > >::value));
        ASSERT(1 == (bsl::is_same<const_iterator,
                     bslstl::RandomAccessIterator<const TYPE,
                        bslalg::DequeIterator<TYPE, BLOCK_LENGTH>
                                                                  > >::value));

        for (int ti = 0; ti < NUM_DATA; ++ti) {
            const int     LINE   = DATA[ti].d_lineNum;
            const char   *SPEC   = DATA[ti].d_spec_p;
            const size_t  LENGTH = strlen(SPEC);

            Obj mX(xoa);  const Obj& X = gg(&mX, SPEC);

            Obj mY(X);  const Obj& Y = mY;  // control

            if (verbose) { P_(LINE); P(SPEC); }

            size_t i = 0;
            for (iterator iter = mX.begin(); iter != mX.end(); ++iter, ++i) {
                LOOP_ASSERT(LINE, Y[i] == *iter);
                *iter = DEFAULT_VALUE;
                LOOP_ASSERT(LINE, DEFAULT_VALUE == *iter);
                mX[i] = Y[i];
            }
            LOOP_ASSERT(LINE, LENGTH == i);

            LOOP_ASSERT(LINE, Y == X);

            i = 0;
            for (const_iterator iter = X.begin(); iter != X.end();
                                                                 ++iter, ++i) {
                LOOP2_ASSERT(LINE, i, Y[i] == *iter);
            }
            LOOP_ASSERT(LINE, LENGTH == i);

            i = 0;
            for (const_iterator iter = X.cbegin(); iter != X.cend();
                                                                 ++iter, ++i) {
                LOOP2_ASSERT(LINE, i, Y[i] == *iter);
            }
            LOOP_ASSERT(LINE, LENGTH == i);
        }
    }

    if (verbose) printf("Testing 'reverse_iterator', 'rbegin', and 'rend',"
                        " and 'const' variants.\n");
    {
        ASSERT(1 == (bsl::is_same<reverse_iterator,
                                   bsl::reverse_iterator<iterator> >::value));
        ASSERT(1 == (bsl::is_same<const_reverse_iterator,
                              bsl::reverse_iterator<const_iterator> >::value));

        for (int ti = 0; ti < NUM_DATA; ++ti) {
            const int     LINE   = DATA[ti].d_lineNum;
            const char   *SPEC   = DATA[ti].d_spec_p;
            const int     LENGTH = (int) strlen(SPEC);

            Obj mX(xoa);  const Obj& X = gg(&mX, SPEC);

            Obj mY(X);  const Obj& Y = mY;  // control

            if (verbose) { P_(LINE); P(SPEC); }

            int i = LENGTH - 1;
            for (reverse_iterator riter = mX.rbegin(); riter != mX.rend();
                                                                ++riter, --i) {
                LOOP_ASSERT(LINE, Y[i] == *riter);
                *riter = DEFAULT_VALUE;
                LOOP_ASSERT(LINE, DEFAULT_VALUE == *riter);
                mX[i] = Y[i];
            }
            LOOP_ASSERT(LINE, -1 == i);

            LOOP_ASSERT(LINE, Y == X);

            i = LENGTH - 1;
            for (const_reverse_iterator riter = X.rbegin(); riter != X.rend();
                                                                ++riter, --i) {
                LOOP_ASSERT(LINE, Y[i] == *riter);
            }
            LOOP_ASSERT(LINE, -1 == i);

            i = LENGTH - 1;
            for (const_reverse_iterator riter = X.crbegin();
                                            riter != X.crend(); ++riter, --i) {
                LOOP_ASSERT(LINE, Y[i] == *riter);
            }
            LOOP_ASSERT(LINE, -1 == i);
        }
    }
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase15()
{
    // ------------------------------------------------------------------------
    // TESTING ELEMENT ACCESS
    //
    // Concerns:
    //   1) That 'v[x]', as well as 'v.front()' and 'v.back()', allow to modify
    //      its indexed element when 'v' is an lvalue, but must not modify its
    //      indexed element when it is an rvalue.
    //   2) That 'v.at(pos)' returns 'v[x]' or throws if 'pos == v.size())'.
    //   3) That 'v.front()' is identical to 'v[0]' and 'v.back()' the same as
    //      'v[v.size() - 1]'.
    //
    // Plan:
    //   For each value given by variety of specifications of different
    //   lengths, create a test deque with this value, and access each element
    //   (front, back, at each position) both as a modifiable reference
    //   (setting it to a default value, then back to its original value, and
    //   as a non-modifiable reference.  Verify that 'at' throws
    //   'std::out_of_range' when accessing the past-the-end element.
    //
    // Testing:
    //   reference operator[](size_type position);
    //   reference at(size_type position);
    //   reference front();
    //   reference back();
    //   const_reference front() const;
    //   const_reference back() const;
    // ------------------------------------------------------------------------

    bslma::TestAllocator oa("object", veryVeryVeryVerbose);
    ALLOC                xoa(&oa);

    const TYPE DEFAULT_VALUE = TYPE();

    static const struct {
        int         d_lineNum;  // source line number
        const char *d_spec_p;   // initial
    } DATA[] = {
        { L_,  ""                },
        { L_,  "A"               },
        { L_,  "ABC"             },
        { L_,  "ABCD"            },
        { L_,  "ABCDE"           },
        { L_,  "ABCDEAB"         },
        { L_,  "ABCDEABC"        },
        { L_,  "ABCDEABCD"       }
    };
    enum { NUM_DATA = sizeof DATA / sizeof *DATA };

    if (verbose) printf("\tWithout exception.\n");
    {
        for (int ti = 0; ti < NUM_DATA; ++ti) {
            const int     LINE   = DATA[ti].d_lineNum;
            const char   *SPEC   = DATA[ti].d_spec_p;
            const size_t  LENGTH = strlen(SPEC);

            Obj mX(xoa);  const Obj& X = gg(&mX, SPEC);

            Obj mY(X);  const Obj& Y = mY;  // control

            if (verbose) { P_(LINE); P(SPEC); }

            if (LENGTH) {
                LOOP_ASSERT(LINE, TYPE(SPEC[0]) == X.front());
                mX.front() = DEFAULT_VALUE;
                LOOP_ASSERT(LINE, DEFAULT_VALUE == X.front());
                mX[0] = Y[0];

                LOOP_ASSERT(LINE, X[LENGTH - 1] == X.back());
                mX.back() = DEFAULT_VALUE;
                LOOP_ASSERT(LINE, DEFAULT_VALUE == X.back());
                mX[LENGTH - 1] = Y[LENGTH - 1];
            }

            LOOP_ASSERT(LINE, Y == X);

            for (size_t j = 0; j < LENGTH; ++j) {
                LOOP_ASSERT(LINE, TYPE(SPEC[j]) == X[j]);
                mX[j] = DEFAULT_VALUE;
                LOOP_ASSERT(LINE, DEFAULT_VALUE == X[j]);
                mX.at(j) = Y[j];
                LOOP_ASSERT(LINE, TYPE(SPEC[j]) == X.at(j));
            }
        }
    }

#ifdef BDE_BUILD_TARGET_EXC
    if (verbose) printf("\tWith exception.\n");
    {
        for (int ti = 0; ti < NUM_DATA; ++ti) {
            const int     LINE   = DATA[ti].d_lineNum;
            const char   *SPEC   = DATA[ti].d_spec_p;
            const size_t  LENGTH = strlen(SPEC);

            Obj mX(xoa);  const Obj& X = gg(&mX, SPEC);

            Obj mY(X);  const Obj& Y = mY;  // control

            bool outOfRangeCaught = false;
            try {
                mX.at(LENGTH) = DEFAULT_VALUE;
            }
            catch (const std::out_of_range& ex) {
                outOfRangeCaught = true;
            }
            LOOP_ASSERT(LINE, Y == X);
            LOOP_ASSERT(LINE, outOfRangeCaught);
        }
    }
#endif
}

template <class TYPE, bool = bslma::UsesBslmaAllocator<TYPE>::value>
struct TypeAllocatesForDefault {
    static int test()
    {
        return 0;
    }
};

template <class TYPE>
struct TypeAllocatesForDefault<TYPE, true> {
    static int test()
    {
        bslma::TestAllocator queryAllocator("query allocate on default");
        const TYPE defaultObject(&queryAllocator);
        return static_cast<int>(queryAllocator.numAllocations());
    }
};

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase14()
{
    // ------------------------------------------------------------------------
    // TESTING CAPACITY
    //   Testing methods related to 'capacity' and 'size', such as 'reserve',
    //   'resize', 'max_size', and 'empty'.  Note that the standard does not
    //   provide the strong exception safety guarantee (rollback when an
    //   exception is thrown) for 'resize', nor does the BDE 'reserve'
    //   extension.
    //
    // Concerns:
    //: 1 That 'v.reserve(n)' reserves sufficient capacity in 'v' to hold 'n'
    //:   elements without reallocation, but does not change the value of 'v'.
    //:   In addition, if 'v.reserve(n)' allocates, it must allocate for a
    //:   capacity of exactly 'n' elements.
    //:
    //: 2 That 'v.resize(n, value)' changes the size of 'v' to 'n', appending
    //:   elements having value 'value' if 'n' is larger than the current size.
    //:
    //: 3 That existing elements are moved without copy-construction if the
    //:   bitwise-moveable trait is present.
    //:
    //: 4 That 'reserve' and 'resize' are exception-neutral.  Note that there
    //:   is no strong exception-safety guarantee on these methods.
    //:
    //: 5 That the accessors such as 'capacity' and 'empty' return the correct
    //:   value.
    //
    // Plan:
    //: 1 For deque 'v' having various initial capacities, call 'v.reserve(n)'
    //:   for various values of 'n'.  Verify that sufficient capacity is
    //:   allocated by filling 'v' with 'n' elements.  Perform each test in the
    //:   standard 'bslma' exception-testing macro block.
    //
    // Testing:
    //   void reserve(size_type n);
    //   void resize(size_type n);
    //   void resize(size_type n, const T& value);
    //   size_type max_size() const;
    //   size_type capacity() const;
    //   bool empty() const;
    // ------------------------------------------------------------------------

    typedef typename Obj::size_type size_type;

    bslma::TestAllocator  oa("object", veryVeryVeryVerbose);
    ALLOC                 xoa(&oa);
    ASSERT(0 == oa.numBytesInUse());

    static const size_t EXTEND[] = {
        0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17
    };
    enum { NUM_EXTEND = sizeof EXTEND / sizeof *EXTEND };

    const int PAGE_LENGTH = PageLength<TYPE>::k_VALUE;
    BSLMF_ASSERT(PAGE_LENGTH >= 7);
    const int BLOCKS_LENGTH = 5;     // Length of blocks array for a
                                     // default-constructed empty deque.

    static const size_t DATA[] = {
        0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17
    };
    enum { NUM_DATA = sizeof DATA / sizeof *DATA };

    const int FS = (PAGE_LENGTH - 1) / 2;    // front space
    const int BS = PAGE_LENGTH - 1 - FS;     // back space

    struct {
        int         d_line;
        const char *d_spec_p;
        int         d_size;
    } SPECS[] = {
        // Must not be empty

        { L_, "A",            1 },
        { L_, "<B|<a|",       PAGE_LENGTH - 1 },
        { L_, "<B|",          BS },
        { L_, "<a|",          FS },
        { L_, "<B...|<a...|", PAGE_LENGTH - 6 - 1 },
        { L_, "<a...|",       FS - 3 },
        { L_, "<B...|",       BS - 3 }, };
    enum { NUM_SPECS = sizeof SPECS / sizeof *SPECS };

    const TYPE         *values     = 0;
    const TYPE *const&  TEST_VALUES     = values;
    const int           NUM_TEST_VALUES = getValues(&values);
    (void) NUM_TEST_VALUES;

    Int64 numAllocsPerItem;
    if (verbose) printf("\tTesting 'max_size', init numAllocsPerItem.\n");
    {
        // This is the maximum value.  Any larger value would be cause for
        // potential bugs.

        Obj mX(xoa);  const Obj& X = mX;
        ASSERT(~(size_t)0 / sizeof(TYPE) >= X.max_size());

        Int64 initBlocks = oa.numBlocksInUse();
        mX.push_back(TEST_VALUES[0]);
        numAllocsPerItem = oa.numBlocksInUse() - initBlocks;
    }

    if (veryVerbose) { P_(PAGE_LENGTH); P_(FS); P_(BS); P(numAllocsPerItem); }

    if (verbose) printf("\tTesting 'capacity, pushing at both ends'.\n");
    {
        Obj mX(xoa);  const Obj& X = mX;
        const size_type INITIAL_CAPACITY = X.capacity();

        // An empty deque allocates the first page of memory for elements, so
        // capacity should be non-zero.

        ASSERTV(INITIAL_CAPACITY, 0 < INITIAL_CAPACITY);

        // insert up to initial capacity, one item at a time.
        //  alternately push_back/push_front, watching capacity
        //  (want to set the situation where one push decrements by one,
        //  pushing at the other end has no effect, but a second push at the
        //  other end decrements by one.  Verify that no memory is allocated
        //  until capacity is 0, then one allocation when pushing at each end.
        //  Also make sure that capacity remains zero after first allocating
        //  push, and a big jump after pushing at the other end.

        size_type lastCapacity = INITIAL_CAPACITY;

        // Capacity is size plus minimum number of pushes at either end before
        // the container must reallocate.  As we don't know if the front or the
        // back of the deque has a lower threshold, we will push at both ends,
        // expecting size to grow by 2, but capacity to grow by only one.  For
        // the insertion where size equals capacity, we know we have exhausted
        // that end of the deque and should push only at the other end, until
        // it too is full.  We can spot this by first popping the last inserted
        // element so that free capacity at the full end is now 1, and we can
        // spot 1 dropping to 0 when the opposite end is given its last push.
        // Finally, push that initial element one last time to truly full the
        // deque, and confirm that, for an element type that does not allocate,
        // no allocations have yet occurred.  Then push one element onto either
        // end and observe that allocation occurs, and capacity grows.  Note
        // that we have no guarantee that the new capacity will be the old
        // capacity plus one, as re-allocation may leave the container with a
        // better balanced spread of elements starting and ending in the middle
        // of blocks in the new arrangement.

        const Int64 initBlocks = oa.numBlocksInUse();
        Int64 newBlocks;
        size_type iterations  = 0;
        ASSERT(X.empty());
        ASSERT(2 == initBlocks);   // first block + array of block pointers

        // Note that 'findRoomierEnd' may both deallocate and allocate.

        mX.push_back(TEST_VALUES[2]);
        typename Obj::const_iterator it = mX.begin();

        int room;
        while ((room = findRoomierEnd(&mX)),
                                        0 != room || X.capacity() > X.size()) {
            if (0 < room) {
                // More room in front.

                mX.push_front(TEST_VALUES[0]);
            }
            else {
                // Symmetrical or more room in back.

                mX.push_back(TEST_VALUES[1]);
            }
            newBlocks = oa.numBlocksInUse();
            ASSERTV(newBlocks <= numAllocsPerItem * (int) X.size() +
                                             + initBlocks + BLOCKS_LENGTH - 1);

            const size_type NEW_CAPACITY = X.capacity();
            ASSERTV(NEW_CAPACITY,   lastCapacity,
                    NEW_CAPACITY == lastCapacity ||
                    NEW_CAPACITY == lastCapacity + 1);

            lastCapacity = NEW_CAPACITY;
            ++iterations;
            ASSERT(X.size() == iterations + 1);
        }
        ASSERTV(X.capacity(), X.size(), X.capacity() == X.size());
        ASSERTV(TEST_VALUES[2] == *it);    // iterator not invalidated

        // After loop, the deque should be full to touching both ends, which
        // means there will be one space open at the back end.

        ASSERTV(PAGE_LENGTH, BLOCKS_LENGTH, iterations,
                                  BLOCKS_LENGTH * PAGE_LENGTH - 1 == X.size());
        ASSERT(0 == room);

        // There should be exactly 'BLOCKS_LENGTH' blocks in the deque, plus
        // the array of pointers to blocks.

        ASSERT(numAllocsPerItem * (Int64) X.size() + BLOCKS_LENGTH + 1
               == oa.numBlocksInUse());

        // Now verify that the next push forces an allocation.  Need to
        // recreate the situation twice, once for 'push_back' and a second time
        // for 'push_front'.

        // Investigate all 4 push/pop sequences:
        //   push_back / pop_back
        //   push_back / pop_front
        //   push_front/ pop_front
        //   push_front/ pop_back
    }

    if (verbose) printf("\tTesting 'capacity, fill back before front'.\n");
    {
        Obj mX(xoa);  const Obj& X = mX;
        const size_type INITIAL_CAPACITY = X.capacity();

        // An empty deque allocates the first page of memory for elements, so
        // capacity should be non-zero.

        ASSERTV(INITIAL_CAPACITY, 0 < INITIAL_CAPACITY);

        // insert up to initial capacity, one item at a time.
        //  alternately push_back/push_front, watching capacity
        //  (want to set the situation where one push decrements by one,
        //  pushing at the other end has no effect, but a second push at the
        //  other end decrements by one.  Verify that no memory is allocated
        //  until capacity is 0, then one allocation when pushing at each end.
        //  Also make sure that capacity remains zero after first allocating
        //  push, and a big jump after pushing at the other end.

        size_type lastCapacity = INITIAL_CAPACITY;

        // Capacity is size plus minimum number of pushes at either end before
        // the container must reallocate.  As we don't know if the front or the
        // back of the deque has a lower threshold, we will push at both ends,
        // expecting size to grow by 2, but capacity to grow by only one.  For
        // the insertion where size equals capacity, we know we have exhausted
        // that end of the deque and should push only at the other end, until
        // it too is full.  We can spot this by first popping the last inserted
        // element so that free capacity at the full end is now 1, and we can
        // spot 1 dropping to 0 when the opposite end is given its last push.
        // Finally, push that initial element one last time to truly full the
        // deque, and confirm that, for an element type that does not allocate,
        // no allocations have yet occurred.  Then push one element onto either
        // end and observe that allocation occurs, and capacity grows.  Note
        // that we have no guarantee that the new capacity will be the old
        // capacity plus one, as re-allocation may leave the container with a
        // better balanced spread of elements starting and ending in the middle
        // of blocks in the new arrangement.

        const Int64 initBlocks = oa.numBlocksInUse();
        Int64 newBlocks;
        bool      backIsFull  = 0 == INITIAL_CAPACITY;
        bool      frontIsFull = 0 == INITIAL_CAPACITY;
        size_type iterations  = 0;

        ASSERT(2 == initBlocks);   // first block + array of block pointers

        mX.push_back(TEST_VALUES[2]);
        typename Obj::const_iterator it = mX.begin();

        while (!backIsFull) {
            mX.push_back(TEST_VALUES[0]);
            const size_type BACK_CAPACITY  = X.capacity();

            // 'push_back' does not affect 'capacity' if less room at the
            // back, otherwise it should grow by exactly one.
            ASSERTV(BACK_CAPACITY,   lastCapacity,
                    BACK_CAPACITY == lastCapacity ||
                    BACK_CAPACITY == lastCapacity + 1);

            newBlocks = oa.numBlocksInUse();
            ASSERT(newBlocks <= numAllocsPerItem * (Int64) X.size() +
                                         initBlocks + (BLOCKS_LENGTH - 1) / 2);

            backIsFull = X.capacity() == X.size();
            if (backIsFull) {
                // Make sure we can spot the front filling up.
                mX.pop_back();
            }

            lastCapacity = X.capacity();

            if (++iterations > 2 * INITIAL_CAPACITY) {
                // An early abort to avoid an infinite loop if the test is
                // failing.

                break;
            }
        }

        iterations = 0;   // Reset safety-valve counter

        while (!frontIsFull) {
            mX.push_front(TEST_VALUES[1]);
            const size_type FRONT_CAPACITY = X.capacity();
            ASSERTV(FRONT_CAPACITY,   lastCapacity,
                    FRONT_CAPACITY == lastCapacity ||
                    FRONT_CAPACITY == lastCapacity + 1);

            frontIsFull  = X.capacity() == X.size();
            if (frontIsFull) {
                // Restore the previously popped element to fill deque.

                mX.push_back(TEST_VALUES[1]);
            }

            newBlocks = oa.numBlocksInUse();
            ASSERT(newBlocks <= numAllocsPerItem * (Int64) X.size() +
                                               initBlocks + BLOCKS_LENGTH - 1);

            // Note that there is potential when inserting into an empty
            // container that the very first insertion may reduce capacity in
            // both directions, so a subsequent insert reduces capacity again.

            lastCapacity = X.capacity();

            if (++iterations > 2 * INITIAL_CAPACITY) {
                // An early abort to avoid an infinite loop if the test is
                // failing.

                break;
            }
        }

        // After loop, capacity should have been exhausted
        ASSERTV(X.capacity(), X.size(), X.capacity() == X.size());
        ASSERT(TEST_VALUES[2] == *it);        // iterator still valid
        ASSERT(0 == findRoomierEnd(&mX));

        ASSERTV(BLOCKS_LENGTH * PAGE_LENGTH - 1 == X.size());

        // There should be exactly 'BLOCKS_LENGTH' blocks in the deque, plus
        // the array of pointers to blocks.

        ASSERT(numAllocsPerItem * (Int64) X.size() +
                                     BLOCKS_LENGTH + 1 == oa.numBlocksInUse());
    }

    if (verbose) printf("\tTesting 'capacity, fill front before back'.\n");
    {
        Obj mX(xoa);  const Obj& X = mX;
        const size_type INITIAL_CAPACITY = X.capacity();

        // An empty deque allocates the first page of memory for elements, so
        // capacity should be non-zero.

        ASSERTV(INITIAL_CAPACITY, 0 < INITIAL_CAPACITY);

        // insert up to initial capacity, one item at a time.
        //  alternately push_back/push_front, watching capacity
        //  (want to set the situation where one push decrements by one,
        //  pushing at the other end has no effect, but a second push at the
        //  other end decrements by one.  Verify that no memory is allocated
        //  until capacity is 0, then one allocation when pushing at each end.
        //  Also make sure that capacity remains zero after first allocating
        //  push, and a big jump after pushing at the other end.

        size_type lastCapacity = INITIAL_CAPACITY;

        // Capacity is size plus minimum number of pushes at either end before
        // the container must reallocate.  As we don't know if the front or the
        // back of the deque has a lower threshold, we will push at both ends,
        // expecting size to grow by 2, but capacity to grow by only one.  For
        // the insertion where size equals capacity, we know we have exhausted
        // that end of the deque and should push only at the other end, until
        // it too is full.  We can spot this by first popping the last inserted
        // element so that free capacity at the full end is now 1, and we can
        // spot 1 dropping to 0 when the opposite end is given its last push.
        // Finally, push that initial element one last time to truly full the
        // deque, and confirm that, for an element type that does not allocate,
        // no allocations have yet occurred.  Then push one element onto either
        // end and observe that allocation occurs, and capacity grows.  Note
        // that we have no guarantee that the new capacity will be the old
        // capacity plus one, as re-allocation may leave the container with a
        // better balanced spread of elements starting and ending in the middle
        // of blocks in the new arrangement.

        const Int64 initBlocks = oa.numBlocksInUse();
        Int64 newBlocks;
        bool      backIsFull  = 0 == INITIAL_CAPACITY;
        bool      frontIsFull = 0 == INITIAL_CAPACITY;
        size_type iterations  = 0;

        mX.push_front(TEST_VALUES[2]);
        typename Obj::const_iterator it = mX.begin();

        while (!frontIsFull) {
            mX.push_front(TEST_VALUES[0]);
            const size_type FRONT_CAPACITY = X.capacity();

            // 'push_front' does not affect 'capacity' if less room at the
            // front, otherwise it should grow by exactly one.

            ASSERTV(FRONT_CAPACITY,   lastCapacity,
                    FRONT_CAPACITY == lastCapacity ||
                    FRONT_CAPACITY == lastCapacity + 1);

            newBlocks = oa.numBlocksInUse();
            ASSERT(newBlocks <= numAllocsPerItem * (Int64) X.size() +
                                         initBlocks + (BLOCKS_LENGTH - 1) / 2);

            frontIsFull  = X.capacity() == X.size();
            if (frontIsFull) {
                // Make sure we can spot the back filling up in the next step.

                mX.pop_front();
            }

            // Note that there is potential when inserting into an empty
            // container that the very first insertion may reduce capacity in
            // both directions, so a subsequent insert reduces capacity again.

            lastCapacity = X.capacity();

            if (++iterations > 2 * INITIAL_CAPACITY) {
                // An early abort to avoid an infinite loop if the test is
                // failing.

                break;
            }
        }

        iterations = 0;   // Reset safety-valve counter

        while (!backIsFull) {
            mX.push_back(TEST_VALUES[1]);
            const size_type BACK_CAPACITY  = X.capacity();

            // 'push_back' does not affect 'capacity' if less room at the
            // back, otherwise it should grow by exactly one.

            ASSERTV(BACK_CAPACITY,   lastCapacity,
                    BACK_CAPACITY == lastCapacity ||
                    BACK_CAPACITY == lastCapacity + 1);

            backIsFull = X.capacity() == X.size();
            if (backIsFull) {
                // Restore the previously popped element to fill deque.

                mX.push_front(TEST_VALUES[0]);
            }

            newBlocks = oa.numBlocksInUse();
            ASSERT(newBlocks <= numAllocsPerItem * (Int64) X.size() +
                                               initBlocks + BLOCKS_LENGTH - 1);

            lastCapacity = X.capacity();

            if (++iterations > 2 * INITIAL_CAPACITY) {
                // An early abort to avoid an infinite loop if the test is
                // failing.

                break;
            }
        }

        // After loop, capacity should have been exhausted

        ASSERTV(X.capacity(), X.size(), X.capacity() == X.size());
        ASSERT(TEST_VALUES[2] == *it);        // iterator still valid

        ASSERT(0 == findRoomierEnd(&mX));

        ASSERTV(BLOCKS_LENGTH * PAGE_LENGTH - 1 == X.size());

        // There should be exactly 'BLOCKS_LENGTH' blocks in the deque, plus
        // the array of pointers to blocks.

        ASSERT(numAllocsPerItem * (int) X.size() +
                                     BLOCKS_LENGTH + 1 == oa.numBlocksInUse());
    }

    if (verbose) printf("\tTesting 'reserve', 'capacity' and 'empty'.\n");
    for (int ti = 0; ti < NUM_SPECS; ++ti) {
        const int     LINE = SPECS[ti].d_line;
        const char   *SPEC = SPECS[ti].d_spec_p;
        const size_t  SZ   = SPECS[ti].d_size;

        for (int tj = 0; tj < NUM_EXTEND; ++tj) {
            const size_t NE = EXTEND[tj];

            Obj mX(xoa);  const Obj& X = mX;
            ASSERT(X.empty());
            ASSERTV(LINE, 0 > ggg(&mX, SPEC));
            ASSERTV(LINE, X.size(), SZ, X.size() == SZ);
            ASSERTV(SZ <= PAGE_LENGTH);
            ASSERT((0 == SZ) == X.empty());

            typename Obj::iterator it = mX.begin();
            TYPE v = *it;
            TYPE *pv = BSLS_UTIL_ADDRESSOF(*it);

            const size_t preCap = X.capacity();

            Int64 numBlocksBefore = oa.numBlocksInUse();
            int throwCount = -1;
            Obj mY(X);

            BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                ExceptionProctor<Obj, ALLOC> proctor(&mX, X, L_);
                ASSERT(X.capacity() == preCap);
                ++throwCount;

                mX.reserve(NE);
            } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END
            size_t postCap = X.capacity();
            ASSERT((NE <= preCap - SZ) == (postCap == preCap));
            ASSERT(X == mY);    // capacity may have changed, but not salient
            LOOP_ASSERT(ti, SZ == X.size());
            ASSERT((0 == SZ) == X.empty());

            Int64 numBlocksAfter = oa.numBlocksInUse();
            if (NE <= preCap - SZ) {
                ASSERT(numBlocksBefore == numBlocksAfter);
                ASSERT(postCap == preCap);
                ASSERT(0 == throwCount);
                ASSERT(v == *it);                  // iterators not invalidated
            }
            else {
                ASSERT(numBlocksBefore < numBlocksAfter);
                ASSERT(postCap > preCap);
                ASSERT(0 < throwCount);
            }
            ASSERT(postCap - SZ >= NE);
            size_t space = postCap - SZ;

            it = mX.begin();
            v = *it;

            // Make symmetrical.

            int room;
            while (0 != (room = findRoomierEnd(&mX))) {
                if (0 < room) {
                    mX.push_front(TEST_VALUES[1]);
                }
                else if (0 > room) {
                    mX.push_back( TEST_VALUES[0]);
                }
            }
            ASSERT(0 == findRoomierEnd(&mX));
            space = X.capacity() - X.size();
            mY = X;

            // Adding 'postCap' more items to each of both ends will not
            // invalidate iterators, and will only alloc the amount necessary
            // for the items themselves.

            numBlocksBefore = oa.numBlocksInUse();
            for (size_t tk = 0; tk < space; ++tk) {
                mX.push_back( TEST_VALUES[2]);
                mX.push_front(TEST_VALUES[3]);
            }
            numBlocksAfter = oa.numBlocksInUse();
            const size_t distToV = it - mX.begin();
            ASSERT(v == *it);                      // iterators not invalidated
            ASSERT(!space == (X == mY));
            ASSERTV(LINE, numBlocksAfter,
                                      numBlocksBefore, space, numAllocsPerItem,
                    numBlocksAfter == numBlocksBefore + BLOCKS_LENGTH - 1 +
                                         2 * (Int64) space * numAllocsPerItem);
            ASSERTV(X.capacity(), 0 == X.capacity() - X.size());
            ASSERT(0 == findRoomierEnd(&mX));
            ASSERTV(X.capacity() == X.size());
            ASSERT(X.size() % PAGE_LENGTH == PAGE_LENGTH - 1);

            // Verify the layout of the deque.  Items in the same block will
            // be adjacent.  Since we're using the test allocator, we know that
            // allocated segments begin and end with padding, so items in
            // different blocks can't be adjacent.

            for (size_t szB = X.size(), ii = 0; ii < szB; ++ii) {
                const size_t stop =  (szB < ii + PAGE_LENGTH
                                    ? szB : ii + PAGE_LENGTH) - 1;
                size_t jj = ii;
                for (; jj < stop; ++jj) {
                    // adjacent; same block
                    ASSERT(BSLS_UTIL_ADDRESSOF(X[jj]) + 1 ==
                           BSLS_UTIL_ADDRESSOF(X[jj + 1]));
                }
                ii = jj;
                ASSERT(ii + 1 == szB ||
                           BSLS_UTIL_ADDRESSOF(X[ii]) + 1 !=
                           BSLS_UTIL_ADDRESSOF(X[ii + 1]));
                                              // not adjacent; different blocks
            }

            // Pushing one more item to each end will cause a grow of one page
            // on either end, will invalidate iterators.  With the current imp
            // of iterators, comparing 'it' with 'itB' below with '!=' won't
            // work, because '==' and '!=' just look at the 'd_value_p' field,
            // while it's the 'd_block_p' field that will change, so we have to
            // use 'memcmp' to look at the guts of the whole iterator to see
            // that the old iterator is no longer valid.

            ASSERT(X.size() >= 2);
            ASSERT((1 < PAGE_LENGTH) ==     // same block
                 (BSLS_UTIL_ADDRESSOF(X[1]) == BSLS_UTIL_ADDRESSOF(X[0]) + 1));
            size_t preSuperCap = X.size();
            numBlocksBefore = oa.numBlocksInUse();
            mX.push_back( TEST_VALUES[0]);
            mX.push_front(TEST_VALUES[1]);
            numBlocksAfter = oa.numBlocksInUse();
            typename Obj::iterator itB = mX.begin() + distToV + 1;
            ASSERT(v ==   *itB);
            ASSERT(pv == BSLS_UTIL_ADDRESSOF(*itB));       // hasn't moved
            ASSERT(0 != memcmp(&it, &itB, sizeof(it)));    // iterators changed
            ASSERT(BSLS_UTIL_ADDRESSOF(X[1]) !=
                   BSLS_UTIL_ADDRESSOF(X[0]) + 1);         // different blocks
            ASSERT(numBlocksAfter ==
                                   numBlocksBefore + 2 * numAllocsPerItem + 2);
            size_t postSuperCap = X.size() + 2 * (X.capacity() - X.size());
            ASSERT(postSuperCap >= preSuperCap + 2 * PAGE_LENGTH);

            mX.clear();
            ASSERT(X.empty());
        }
    }

    if (verbose) printf("\tTesting 'resize'.\n");
    const typename Obj::value_type DEFAULT_VALUE = typename Obj::value_type();
    for (int ti = 0; ti < NUM_DATA; ++ti) {
        const size_t NE = DATA[ti];

        for (int ei = 0; ei < NUM_EXTEND; ++ei) {
            const size_t CAP   = EXTEND[ei];
            const size_t DELTA = NE > CAP ? NE - CAP : 0;

            if (veryVeryVerbose)
                printf("LINE = %d, ti = %d, ei = %d\n", L_, ti, ei);

            BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
              const int AL = (int) oa.allocationLimit();
              oa.setAllocationLimit(-1);

              Obj mX(xoa);  const Obj& X = mX;

              stretchRemoveAll(&mX, CAP);
              LOOP_ASSERT(ti, X.empty());
              LOOP_ASSERT(ti, 0   == X.size());
              LOOP_ASSERT(ti, CAP <= X.capacity());

              Int64 numBlocksBefore = oa.numBlocksInUse();

              oa.setAllocationLimit(AL);

              mX.resize(NE);  // test here

              LOOP_ASSERT(ti, NE == X.size());
              LOOP_ASSERT(ti, NE <= X.capacity());
              Int64 numBlocksAfter = oa.numBlocksInUse();

              ASSERTV(NE, DELTA, numBlocksAfter, numBlocksBefore,
                      NE > PAGE_LENGTH / 2 - 1 ||
                                numBlocksAfter == (int) NE * numAllocsPerItem +
                                                              numBlocksBefore);

              for (size_t j = 0; j < NE; ++j) {
                  LOOP2_ASSERT(ti, j, DEFAULT_VALUE == X[j]);
              }

              typename Obj::iterator it;
              if (NE) it = mX.begin();

              numBlocksBefore = oa.numBlocksInUse();

              // This will add 1 block, but it will not reallocate 'd_blocks'.

              mX.resize(NE + PAGE_LENGTH);  // test here

              numBlocksAfter = oa.numBlocksInUse();

              ASSERTV(NE, PAGE_LENGTH, numBlocksAfter, numBlocksAfter,
                      numBlocksBefore, numBlocksAfter ==
                         1 + PAGE_LENGTH * numAllocsPerItem + numBlocksBefore);

              // Iterators not invalidated.

              typename Obj::iterator itB;
              if (NE) itB = mX.begin();
              ASSERT(0 == memcmp(&it, &itB, sizeof(it)));

              for (size_t j = 0; j < NE + PAGE_LENGTH; ++j) {
                  LOOP2_ASSERT(ti, j, DEFAULT_VALUE == X[j]);
              }

              it = mX.begin();

              numBlocksBefore = oa.numBlocksInUse();

              // This will add 1 block, but it will not reallocate 'd_blocks'.

              mX.resize(NE + 4 * PAGE_LENGTH);  // test here

              numBlocksAfter = oa.numBlocksInUse();

              ASSERTV(NE, PAGE_LENGTH, numBlocksAfter, numBlocksAfter,
                  numBlocksBefore, numBlocksAfter ==
                     3 + 3 * PAGE_LENGTH * numAllocsPerItem + numBlocksBefore);

              // Iterators invalidated.

              itB = mX.begin();
              ASSERT(0 != memcmp(&it, &itB, sizeof(it)));

              for (size_t j = 0; j < NE + 4 * PAGE_LENGTH; ++j) {
                  LOOP2_ASSERT(ti, j, DEFAULT_VALUE == X[j]);
              }
            } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

            ASSERT(0 == oa.numMismatches());
            ASSERT(0 == oa.numBlocksInUse());
        }
    }

    if (verbose) printf("\tTesting 'resize' with value.\n");
    for (int ti = 0; ti < NUM_DATA; ++ti) {
        const size_t NE = DATA[ti];
        const TYPE   TV = TEST_VALUES[1];

        for (int ei = 0; ei < NUM_EXTEND; ++ei) {
            const size_t CAP   = EXTEND[ei];
            const size_t DELTA = NE > CAP ? NE - CAP : 0;

            if (veryVeryVerbose)
                printf("LINE = %d, ti = %d, ei = %d\n", L_, ti, ei);

            BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
              const int AL = (int) oa.allocationLimit();
              oa.setAllocationLimit(-1);

              Obj mX(xoa);  const Obj& X = mX;

              stretchRemoveAll(&mX, CAP);
              LOOP_ASSERT(ti, X.empty());
              LOOP_ASSERT(ti, 0   == X.size());
              LOOP_ASSERT(ti, CAP <= X.capacity());

              Int64 numBlocksBefore = oa.numBlocksInUse();

              oa.setAllocationLimit(AL);

              mX.resize(NE, TV);  // test here

              LOOP_ASSERT(ti, NE == X.size());
              LOOP_ASSERT(ti, NE <= X.capacity());
              Int64 numBlocksAfter = oa.numBlocksInUse();

              ASSERTV(NE, DELTA, numBlocksAfter, numBlocksBefore,
                      NE > PAGE_LENGTH / 2 - 1 ||
                      numBlocksAfter == (Int64) NE * numAllocsPerItem +
                                                              numBlocksBefore);

              for (size_t j = 0; j < NE; ++j) {
                  LOOP2_ASSERT(ti, j, TV == X[j]);
              }

              typename Obj::iterator it;
              if (NE) it = mX.begin();

              numBlocksBefore = oa.numBlocksInUse();

              // This will add 1 block, but it will not reallocate 'd_blocks'.

              mX.resize(NE + PAGE_LENGTH, TV);  // test here

              numBlocksAfter = oa.numBlocksInUse();

              ASSERTV(NE, PAGE_LENGTH, numBlocksAfter,
                      numBlocksBefore, numBlocksAfter ==
                         1 + PAGE_LENGTH * numAllocsPerItem + numBlocksBefore);

              // Iterators not invalidated.

              typename Obj::iterator itB;
              if (NE) itB = mX.begin();
              ASSERT(0 == memcmp(&it, &itB, sizeof(it)));

              for (size_t j = 0; j < NE + PAGE_LENGTH; ++j) {
                  LOOP2_ASSERT(ti, j, TV == X[j]);
              }

              it = mX.begin();

              numBlocksBefore = oa.numBlocksInUse();

              // This will add 1 block, but it will not reallocate 'd_blocks'.

              mX.resize(NE + 4 * PAGE_LENGTH, TV);  // test here

              numBlocksAfter = oa.numBlocksInUse();

              ASSERTV(NE, PAGE_LENGTH, numBlocksAfter, numBlocksAfter,
                 numBlocksBefore, numBlocksAfter ==
                     3 + 3 * PAGE_LENGTH * numAllocsPerItem + numBlocksBefore);

              // Iterators invalidated.

              itB = mX.begin();
              ASSERT(0 != memcmp(&it, &itB, sizeof(it)));

              for (size_t j = 0; j < NE + 4 * PAGE_LENGTH; ++j) {
                  LOOP2_ASSERT(ti, j, TV == X[j]);
              }
            } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

            ASSERT(0 == oa.numMismatches());
            ASSERT(0 == oa.numBlocksInUse());
        }
    }

    ASSERT(0 == oa.numMismatches());
    ASSERT(0 == oa.numBlocksInUse());
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase13()
{
    // ------------------------------------------------------------------------
    // TESTING 'assign'
    //
    // Concerns:
    //   The concerns are the same as for the constructor with the same
    //   signature (case 12), except that the implementation is different, and
    //   in addition the previous value must be freed properly.
    //
    // Plan:
    //   For the assignment we will create objects of varying sizes containing
    //   default values for type T, and then assign different 'value'.  Perform
    //   the above tests:
    //    - With various initial values before the assignment.
    //    - In the presence of exceptions during memory allocations using
    //        a 'bslma::TestAllocator' and varying its *allocation* *limit*.
    //   and use basic accessors to verify
    //      - size
    //      - capacity
    //      - element value at each index position { 0 .. length - 1 }.
    //   Note that we relax the concerns about memory consumption, since this
    //   is implemented as 'erase + insert', and insert will be tested more
    //   completely in test case 17.
    //
    // Testing:
    //   void assign(size_type n, const T& value);
    // ------------------------------------------------------------------------

    bslma::TestAllocator  oa("object", veryVeryVeryVerbose);
    ALLOC                 xoa(&oa);

    const TYPE          *values     = 0;
    const TYPE *const&   VALUES     = values;
    const int            NUM_VALUES = getValues(&values);

    if (verbose) printf("\nTesting initial-length assignment.\n");
    {
        static const struct {
            int d_lineNum;  // source line number
            int d_length;   // expected length
        } DATA[] = {
            //line  length
            //----  ------
            { L_,        0   },
            { L_,        1   },
            { L_,        2   },
            { L_,        3   },
            { L_,        4   },
            { L_,        5   },
            { L_,        6   },
            { L_,        7   },
            { L_,        8   },
            { L_,        9   },
            { L_,       11   },
            { L_,       12   },
            { L_,       14   },
            { L_,       15   },
            { L_,       16   },
            { L_,       17   }
        };
        enum { NUM_DATA = sizeof DATA / sizeof *DATA };

        if (verbose) printf("\tUsing 'n' copies of 'value'.\n");
        {
            for (int i = 0; i < NUM_DATA; ++i) {
                const int    INIT_LINE   = DATA[i].d_lineNum;
                const size_t INIT_LENGTH = DATA[i].d_length;

                if (veryVerbose) {
                    printf("\t\tWith initial value of "); P_(INIT_LENGTH);
                    printf("using default value.\n");
                }

                Obj mX(INIT_LENGTH, xoa);  const Obj& X = mX;

                for (int ti = 0; ti < NUM_DATA; ++ti) {
                    const int    LINE   = DATA[ti].d_lineNum;
                    const size_t LENGTH = DATA[ti].d_length;
                    const TYPE   VALUE  = VALUES[ti % NUM_VALUES];

                    if (veryVerbose) {
                        printf("\t\tAssign "); P_(LENGTH);
                        printf(" using "); P(VALUE);
                    }

                    mX.assign(LENGTH, VALUE);

                    if (veryVerbose) {
                        T_; T_; T_; P_(X); P(X.capacity());
                    }

                    LOOP4_ASSERT(INIT_LINE, LINE, i, ti, LENGTH == X.size());

                    for (size_t j = 0; j < LENGTH; ++j) {
                        LOOP5_ASSERT(INIT_LINE, LINE, i, ti, j, VALUE == X[j]);
                    }
                }
            }
            ASSERT(0 == oa.numMismatches());
            ASSERT(0 == oa.numBlocksInUse());
        }

        if (verbose) printf("\tWith exceptions.\n");
        {
            for (int i = 0; i < NUM_DATA; ++i) {
                const int    INIT_LINE   = DATA[i].d_lineNum;
                const size_t INIT_LENGTH = DATA[i].d_length;

                if (veryVerbose) {
                    printf("\t\tWith initial value of "); P_(INIT_LENGTH);
                    printf("using default value.\n");
                }

                for (int ti = 0; ti < NUM_DATA; ++ti) {
                    const int    LINE   = DATA[ti].d_lineNum;
                    const size_t LENGTH = DATA[ti].d_length;
                    const TYPE   VALUE  = VALUES[ti % NUM_VALUES];

                    if (veryVerbose) {
                        printf("\t\tAssign "); P_(LENGTH);
                        printf(" using "); P(VALUE);
                    }

                    BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                        const int AL = (int) oa.allocationLimit();
                        oa.setAllocationLimit(-1);

                        Obj mX(INIT_LENGTH, xoa);  const Obj& X = mX;
                        ExceptionProctor<Obj, ALLOC> proctor(&mX, Obj(), L_);

                        oa.setAllocationLimit(AL);

                        mX.assign(LENGTH, VALUE);  // test here
                        proctor.release();

                        if (veryVerbose) {
                            T_; T_; T_; P_(X); P(X.capacity());
                        }

                        LOOP4_ASSERT(INIT_LINE, LINE, i, ti,
                                     LENGTH == X.size());

                        for (size_t j = 0; j < LENGTH; ++j) {
                            LOOP4_ASSERT(INIT_LINE, ti, i, j, VALUE == X[j]);
                        }
                    } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

                    ASSERT(0 == oa.numMismatches());
                    ASSERT(0 == oa.numBlocksInUse());
                }
            }
        }
    }
}

template <class TYPE, class ALLOC>
template <class CONTAINER>
void TestDriver<TYPE,ALLOC>::testCase13Range(const CONTAINER&)
{
    // ------------------------------------------------------------------------
    // TESTING RANGE 'assign'
    //
    // Concerns:
    //   The concerns are the same as for the constructor with the same
    //   signature (case 12), except that the implementation is different, and
    //   in addition the previous value must be freed properly.
    //
    // Plan:
    //   For the assignment we will create objects of varying sizes containing
    //   default values for type T, and then assign different 'value' as
    //   argument.  Perform the above tests:
    //    - Using 'CONTAINER::const_iterator'.
    //    - In the presence of exceptions during memory allocations using
    //        a 'bslma::TestAllocator' and varying its *allocation* *limit*.
    //   and use basic accessors to verify
    //      - size
    //      - capacity
    //      - element value at each index position { 0 .. length - 1 }.
    //   Note that we relax the concerns about memory consumption, since this
    //   is implemented as 'erase + insert', and insert will be tested more
    //   completely in test case 17.
    //
    // Testing:
    //   void assign(ITER first, ITER last);
    // ------------------------------------------------------------------------

    bslma::TestAllocator  oa("object", veryVeryVeryVerbose);
    ALLOC                 xoa(&oa);

    const TYPE          *values     = 0;
    const TYPE *const&   VALUES     = values;
    const int            NUM_VALUES = getValues(&values);

    static const struct {
        int d_lineNum;  // source line number
        int d_length;   // expected length
    } DATA[] = {
        //line  length
        //----  ------
        { L_,        0   },
        { L_,        1   },
        { L_,        2   },
        { L_,        3   },
        { L_,        4   },
        { L_,        5   },
        { L_,        6   },
        { L_,        7   },
        { L_,        8   },
        { L_,        9   },
        { L_,       11   },
        { L_,       12   },
        { L_,       14   },
        { L_,       15   },
        { L_,       16   },
        { L_,       17   }
    };
    enum { NUM_DATA = sizeof DATA / sizeof *DATA };

    static const struct {
        int         d_lineNum;  // source line number
        const char *d_spec_p;   // container spec
    } U_DATA[] = {
        //line  spec                            length
        //----  ----                            ------
        { L_,   ""                        }, // 0
        { L_,   "A"                       }, // 1
        { L_,   "AB"                      }, // 2
        { L_,   "ABC"                     }, // 3
        { L_,   "ABCD"                    }, // 4
        { L_,   "ABCDE"                   }, // 5
        { L_,   "ABCDEAB"                 }, // 7
        { L_,   "ABCDEABC"                }, // 8
        { L_,   "ABCDEABCD"               }, // 9
        { L_,   "ABCDEABCDEABCDE"         }, // 15
        { L_,   "ABCDEABCDEABCDEA"        }, // 16
        { L_,   "ABCDEABCDEABCDEAB"       }  // 17
    };
    enum { NUM_U_DATA = sizeof U_DATA / sizeof *U_DATA };

    if (verbose) printf("\tUsing 'CONTAINER::const_iterator'.\n");
    {
        for (int i = 0; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;

            if (veryVerbose) {
                printf("\t\tWith initial value of "); P_(INIT_LENGTH);
                printf("using default value.\n");
            }

            Obj mX(INIT_LENGTH, VALUES[i % NUM_VALUES], xoa);
            const Obj& X = mX;

            for (int ti = 0; ti < NUM_U_DATA; ++ti) {
                const int     LINE   = U_DATA[ti].d_lineNum;
                const char   *SPEC   = U_DATA[ti].d_spec_p;
                const size_t  LENGTH = strlen(SPEC);

                CONTAINER mU(gV(SPEC));  const CONTAINER& U = mU;

                if (veryVerbose) {
                    printf("\t\tAssign "); P_(LENGTH);
                    printf(" using "); P(SPEC);
                }

                mX.assign(U.begin(), U.end());

                if (veryVerbose) {
                    T_; T_; T_; P_(X); P(X.capacity());
                }

                LOOP4_ASSERT(INIT_LINE, LINE, i, ti, LENGTH == X.size());

                Obj mY;  const Obj& Y = gg(&mY, SPEC);
                for (size_t j = 0; j < LENGTH; ++j) {
                    LOOP5_ASSERT(INIT_LINE, LINE, i, ti, j, Y[j] == X[j]);
                }
            }
        }
        ASSERT(0 == oa.numMismatches());
        ASSERT(0 == oa.numBlocksInUse());
    }

    if (verbose) printf("\tWith exceptions.\n");
    {
        for (int i = 0; i < NUM_DATA; ++i) {
            const int    INIT_LINE   = DATA[i].d_lineNum;
            const size_t INIT_LENGTH = DATA[i].d_length;

            if (veryVerbose) {
                printf("\t\tWith initial value of "); P_(INIT_LENGTH);
                printf("using default value.\n");
            }

            for (int ti = 0; ti < NUM_U_DATA; ++ti) {
                const int     LINE   = U_DATA[ti].d_lineNum;
                const char   *SPEC   = U_DATA[ti].d_spec_p;
                const size_t  LENGTH = strlen(SPEC);

                CONTAINER mU(gV(SPEC));  const CONTAINER& U = mU;

                if (veryVerbose) {
                    printf("\t\tAssign "); P_(LENGTH);
                    printf(" using "); P(SPEC);
                }

                Obj mY;  const Obj& Y = gg(&mY, SPEC);

                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                    const int AL = (int) oa.allocationLimit();
                    oa.setAllocationLimit(-1);

                    Obj mX(INIT_LENGTH, xoa);  const Obj& X = mX;
                    ExceptionProctor<Obj, ALLOC> proctor(&mX, Obj(), L_);

                    oa.setAllocationLimit(AL);

                    mX.assign(U.begin(), U.end());  // test here
                    proctor.release();

                    if (veryVerbose) {
                        T_; T_; T_; P_(X); P(X.capacity());
                    }

                    LOOP4_ASSERT(INIT_LINE, LINE, i, ti, LENGTH == X.size());

                    for (size_t j = 0; j < LENGTH; ++j) {
                        LOOP5_ASSERT(INIT_LINE, LINE, i, ti, j, Y[j] == X[j]);
                    }
                } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

                LOOP_ASSERT(oa.numMismatches(),  0 == oa.numMismatches());
                LOOP_ASSERT(oa.numBlocksInUse(), 0 == oa.numBlocksInUse());
            }
        }
    }
}

template <class TYPE, class ALLOC>
void TestDriver<TYPE,ALLOC>::testCase12()
{
    // ------------------------------------------------------------------------
    // TESTING INITIAL-LENGTH CONSTRUCTORS
    //   We have the following concerns:
    //    1) The initial value is correct.
    //    2) The initial capacity is correctly set up.
    //    3) The constructor is exception neutral w.r.t. memory allocation.
    //    4) The internal memory management system is hooked up properly
    //       so that *all* internally allocated memory draws from a
    //       user-supplied allocator whenever one is specified.
    //
    // Plan:
    //   For the constructor we will create objects of varying sizes with
    //   different 'value' as argument.  Test first with the default value
    //   for type T, and then test with different values.  Perform the above
    //   tests:
    //    - With and without passing in an allocator.
    //    - In the presence of exceptions during memory allocations using
    //        a 'bslma::TestAllocator' and varying its *allocation* *limit*.
    //    - Where the object is constructed with an object allocator, and
    //        neither of global and default allocator is used to supply memory.
    //   and use basic accessors to verify
    //      - size
    //      - capacity
    //      - element value at each index position { 0 .. length - 1 }.
    //
    // Testing:
    //   deque(size_type n, const A& a = A());
    //   deque(size_type n, const T& value, const A& a = A());
    // ------------------------------------------------------------------------

    bslma::TestAllocator oa("object", veryVeryVeryVerbose);
    ALLOC                xoa(&oa);

    const TYPE           DEFAULT_VALUE = TYPE();

    const TYPE          *values     = 0;
    const TYPE *const&   VALUES     = values;
    const int            NUM_VALUES = getValues(&values);

    if (verbose) printf("\nTesting initial-length ctor "
                        "with (default) initial value.\n");
    {
        static const struct {
            int d_lineNum;  // source line number
            int d_length;   // expected length
        } DATA[] = {
            //line  length
            //----  ------
            { L_,        0   },
            { L_,        1   },
            { L_,        2   },
            { L_,        3   },
            { L_,        4   },
            { L_,        5   },
            { L_,        6   },
            { L_,        7   },
            { L_,        8   },
            { L_,        9   },
            { L_,       11   },
            { L_,       12   },
            { L_,       14   },
            { L_,       15   },
            { L_,       16   },
            { L_,       17   },
            { L_,       31   },
            { L_,       32   },
            { L_,       33   },
            { L_,       63   },
            { L_,       64   },
            { L_,       65   }
        };
        enum { NUM_DATA = sizeof DATA / sizeof *DATA };

        if (verbose) printf("\tWithout passing in an allocator, "
                            "using default value.\n");
        {
            for (int ti = 0; ti < NUM_DATA; ++ti) {
                const int    LINE   = DATA[ti].d_lineNum;
                const size_t LENGTH = DATA[ti].d_length;

                if (verbose) { printf("\t\tCreating object of "); P(LENGTH); }

                Obj mX(LENGTH);  const Obj& X = mX;

                if (veryVerbose) {
                    T_; T_; P_(X); P(X.capacity());
                }

                LOOP2_ASSERT(LINE, ti, LENGTH == X.size());

                for (size_t j = 0; j < LENGTH; ++j) {
                    LOOP3_ASSERT(LINE, ti, j, DEFAULT_VALUE == X[j]);
                }
            }
        }

        if (verbose) printf("\tWithout passing in an allocator, "
                            "using non-default values.\n");
        {
            for (int ti = 0; ti < NUM_DATA; ++ti) {
                const int    LINE   = DATA[ti].d_lineNum;
                const size_t LENGTH = DATA[ti].d_length;
                const TYPE   VALUE  = VALUES[ti % NUM_VALUES];

                if (verbose) {
                    printf("\t\tCreating object of "); P_(LENGTH);
                    printf("using "); P(VALUE);
                }

                Obj mX(LENGTH, VALUE);  const Obj& X = mX;

                if (veryVerbose) {
                    T_; T_; P_(X); P(X.capacity());
                }

                LOOP2_ASSERT(LINE, ti, LENGTH == X.size());

                for (size_t j = 0; j < LENGTH; ++j) {
                    LOOP3_ASSERT(LINE, ti, j, VALUE == X[j]);
                }
            }
        }

        if (verbose) printf("\tWith passing in an allocator, "
                            "using default value.\n");
        {
            for (int ti = 0; ti < NUM_DATA; ++ti) {
                const int    LINE   = DATA[ti].d_lineNum;
                const size_t LENGTH = DATA[ti].d_length;

                if (verbose) { printf("\t\tCreating object of "); P(LENGTH); }

                const Int64 BB = oa.numBlocksTotal();
                const Int64  B = oa.numBlocksInUse();

                if (veryVerbose) { printf("\t\t\tBEFORE: "); P_(BB); P(B); }

                Obj mX(LENGTH, DEFAULT_VALUE, xoa);
                const Obj& X = mX;

                const Int64 AA = oa.numBlocksTotal();
                const Int64  A = oa.numBlocksInUse();

                if (veryVerbose) {
                    printf("\t\t\tAFTER : "); P_(AA); P(A);
                    T_; T_; T_; P_(X); P(X.capacity());
                }

                LOOP2_ASSERT(LINE, ti, LENGTH == X.size());

                for (size_t j = 0; j < LENGTH; ++j) {
                    LOOP3_ASSERT(LINE, ti, j, DEFAULT_VALUE == X[j]);
                }
            }
        }

        if (verbose) printf("\tWith passing in an allocator, "
                            "using non-default value.\n");
        {
            for (int ti = 0; ti < NUM_DATA; ++ti) {
                const int    LINE   = DATA[ti].d_lineNum;
                const size_t LENGTH = DATA[ti].d_length;
                const TYPE   VALUE  = VALUES[ti % NUM_VALUES];

                if (verbose) {
                    printf("\t\tCreating object of "); P_(LENGTH);
                    printf("using "); P(VALUE);
                }

                Obj mX(LENGTH, VALUE, xoa);
                const Obj& X = mX;

                if (veryVerbose) {
                    T_; T_; P_(X); P(X.capacity());
                }

                LOOP2_ASSERT(LINE, ti, LENGTH == X.size());

                for (size_t j = 0; j < LENGTH; ++j) {
                    LOOP3_ASSERT(LINE, ti, j, VALUE == X[j]);
                }
            }
        }

        if (verbose) printf("\tWith passing an allocator and checking for "
                            "allocation exceptions using default value.\n");
        {
            for (int ti = 0; ti < NUM_DATA; ++ti) {
                const int    LINE   = DATA[ti].d_lineNum;
                const size_t LENGTH = DATA[ti].d_length;

                if (verbose) { printf("\t\tCreating object of "); P(LENGTH); }

                const Int64 BB = oa.numBlocksTotal();
                const Int64  B = oa.numBlocksInUse();

                if (veryVerbose) { printf("\t\tBEFORE: "); P_(BB); P(B);}

                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                    Obj mX(LENGTH, DEFAULT_VALUE, xoa);
                    const Obj& X = mX;

                    if (veryVerbose) {
                        T_; T_; P_(X); P(X.capacity());
                    }

                    LOOP2_ASSERT(LINE, ti, LENGTH == X.size());

                    for (size_t j = 0; j < LENGTH; ++j) {
                        LOOP3_ASSERT(LINE, ti, j, DEFAULT_VALUE == X[j]);
                    }

                } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

                const Int64 AA = oa.numBlocksTotal();
                const Int64  A = oa.numBlocksInUse();

                if (veryVerbose) { printf("\t\tAFTER : "); P_(AA); P(A);}
            }
        }

        if (verbose)
            printf("\tWith passing an allocator and checking for "
                   "allocation exceptions using non-default value.\n");
        {
            for (int ti = 0; ti < NUM_DATA; ++ti) {
                const int    LINE   = DATA[ti].d_lineNum;
                const size_t LENGTH = DATA[ti].d_length;
                const TYPE   VALUE  = VALUES[ti % NUM_VALUES];

                if (verbose) {
                    printf("\t\tCreating object of "); P_(LENGTH);
                    printf("using "); P(VALUE);
                }

                const Int64 BB = oa.numBlocksTotal();
                const Int64  B = oa.numBlocksInUse();

                if (veryVerbose) { printf("\t\tBEFORE: "); P_(BB); P(B);}

                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                    Obj mX(LENGTH, VALUE, xoa);
                    const Obj& X = mX;

                    if (veryVerbose) {
                        T_; T_; P_(X); P(X.capacity());
                    }

                    LOOP2_ASSERT(LINE, ti, LENGTH == X.size());

                    for (size_t j = 0; j < LENGTH; ++j) {
                        LOOP3_ASSERT(LINE, ti, j, VALUE == X[j]);
                    }

                } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

                const Int64 AA = oa.numBlocksTotal();
                const Int64  A = oa.numBlocksInUse();

                if (veryVerbose) { printf("\t\tAFTER : "); P_(AA); P(A);}

                LOOP2_ASSERT(LINE, ti, 0 == oa.numBlocksInUse());
            }
        }

        if (verbose) printf("\tAllocators hooked up properly when using "
                            "default value constructors.\n");
        {
            for (int ti = 0; ti < NUM_DATA; ++ti) {
                const int    LINE   = DATA[ti].d_lineNum;
                const size_t LENGTH = DATA[ti].d_length;
                (void) LINE;

                bslma::TestAllocator sa("supplied", veryVeryVeryVerbose);
                ALLOC                xsa(&sa);
                bslma::TestAllocator da("default", veryVeryVeryVerbose);

                bslma::DefaultAllocatorGuard dag(&da);

                if (verbose) {
                    printf("\t\tCreating object of "); P(LENGTH);
                }

                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(sa) {
                    ASSERT(0 == da.numBytesTotal());
                    ASSERT(0 == sa.numBytesInUse());

                    Obj x(LENGTH, DEFAULT_VALUE, xsa);

                    ASSERT(0 == da.numBytesInUse());
                    ASSERT(0 != sa.numBytesInUse());
                } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

                ASSERT(0 == da.numBytesInUse());
                ASSERT(0 == sa.numBytesInUse());
            }
        }

        if (verbose) printf("\tAllocators hooked up properly when using "
                            "non-default value constructors.\n");
        {
            for (int ti = 0; ti < NUM_DATA; ++ti) {
                const int    LINE   = DATA[ti].d_lineNum;
                const size_t LENGTH = DATA[ti].d_length;
                const TYPE   VALUE  = VALUES[ti % NUM_VALUES];
                (void) LINE;

                bslma::TestAllocator sa("supplied", veryVeryVeryVerbose);
                ALLOC                xsa(&sa);
                bslma::TestAllocator da("default", veryVeryVeryVerbose);

                bslma::DefaultAllocatorGuard dag(&da);

                if (verbose) {
                    printf("\t\tCreating object of "); P_(LENGTH);
                    printf("using "); P(VALUE);
                }

                BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(sa) {
                    ASSERT(0 == da.numBytesInUse());
                    ASSERT(0 == sa.numBytesInUse());

                    Obj x(LENGTH, VALUE, xsa);

                    ASSERT(0 == da.numBytesInUse());
                    ASSERT(0 != sa.numBytesInUse());
                } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

                ASSERT(0 == da.numBytesInUse());
                ASSERT(0 == sa.numBytesInUse());
            }
        }
    }
}

template <class TYPE, class ALLOC>
template <class CONTAINER>
void TestDriver<TYPE,ALLOC>::testCase12Range(const CONTAINER&)
{
    // ------------------------------------------------------------------------
    // TESTING RANGE CONSTRUCTOR
    //   We have the following concerns:
    //    1) That the initial value is correct.
    //    2) That the initial range is correctly imported and then moved if the
    //       initial 'FWD_ITER' is an input iterator.
    //    2) That the initial capacity is correctly set up if the initial
    //       'FWD_ITER' is a random-access iterator.
    //    3) That the constructor is exception neutral w.r.t. memory
    //       allocation.
    //    4) That the internal memory management system is hooked up properly
    //       so that *all* internally allocated memory draws from a
    //       user-supplied allocator whenever one is specified.
    //
    // Plan:
    //   We will create objects of varying sizes and capacities containing
    //   default values, and insert a range containing distinct values as
    //   argument.  Perform the above tests:
    //    - Using 'CONTAINER::const_iterator'.
    //    - With and without passing in an allocator.
    //    - In the presence of exceptions during memory allocations using
    //        a 'bslma::TestAllocator' and varying its *allocation* *limit*.
    //   and use basic accessors to verify
    //      - size
    //      - capacity
    //      - element value at each index position { 0 .. length - 1 }.
    //
    // Testing:
    //   deque(ITER first, ITER last, const A& a = A());
    // ------------------------------------------------------------------------

    bslma::TestAllocator oa("object", veryVeryVeryVerbose);
    ALLOC                xoa(&oa);

    static const struct {
        int         d_lineNum;  // source line number
        const char *d_spec_p;   // initial
    } DATA[] = {
        { L_,  ""                },
        { L_,  "A"               },
        { L_,  "AB"              },
        { L_,  "ABC"             },
        { L_,  "ABCD"            },
        { L_,  "ABCDE"           },
        { L_,  "ABCDEAB"         },
        { L_,  "ABCDEABC"        },
        { L_,  "ABCDEABCD"       }
    };
    enum { NUM_DATA = sizeof DATA / sizeof *DATA };

    if (verbose) printf("\tWithout passing in an allocator.\n");
    {
        for (int ti = 0; ti < NUM_DATA; ++ti) {
            const int     LINE   = DATA[ti].d_lineNum;
            const char   *SPEC   = DATA[ti].d_spec_p;
            const size_t  LENGTH = strlen(SPEC);

            if (verbose) {
                printf("\t\tCreating object of "); P_(LENGTH);
                printf("using "); P(SPEC);
            }

            CONTAINER mU(gV(SPEC));  const CONTAINER& U = mU;

            Obj mX(U.begin(), U.end());  const Obj& X = mX;

            if (veryVerbose) {
                T_; T_; P_(X); P(X.capacity());
            }

            LOOP2_ASSERT(LINE, ti, LENGTH == X.size());
            LOOP2_ASSERT(LINE, ti, LENGTH <= X.capacity());

            Obj mY;  const Obj& Y = gg(&mY, SPEC);
            for (size_t j = 0; j < LENGTH; ++j) {
                LOOP3_ASSERT(LINE, ti, j, Y[j] == X[j]);
            }
        }
    }

    if (verbose) printf("\tWith passing in an allocator.\n");
    {
        for (int ti = 0; ti < NUM_DATA; ++ti) {
            const int     LINE   = DATA[ti].d_lineNum;
            const char   *SPEC   = DATA[ti].d_spec_p;
            const size_t  LENGTH = strlen(SPEC);

            if (verbose) { printf("\t\tCreating object "); P(SPEC); }

            CONTAINER mU(gV(SPEC));  const CONTAINER& U = mU;
            Obj mY;  const Obj& Y = gg(&mY, SPEC);

            const Int64 BB = oa.numBlocksTotal();
            const Int64  B = oa.numBlocksInUse();

            Obj        mX(U.begin(), U.end(), xoa);
            const Obj& X = mX;

            const Int64 AA = oa.numBlocksTotal();
            const Int64  A = oa.numBlocksInUse();

            if (veryVerbose) {
                T_; T_; P_(X); P(X.capacity());
                T_; T_; P_(AA - BB); P(A - B);
            }

            LOOP2_ASSERT(LINE, ti, LENGTH == X.size());
            LOOP2_ASSERT(LINE, ti, LENGTH <= X.capacity());

            for (size_t j = 0; j < LENGTH; ++j) {
                LOOP3_ASSERT(LINE, ti, j, Y[j] == X[j]);
            }
        }
    }

    if (verbose) printf("\tWith passing an allocator and checking for "
                        "allocation exceptions.\n");
    {
        for (int ti = 0; ti < NUM_DATA; ++ti) {
            const int     LINE   = DATA[ti].d_lineNum;
            const char   *SPEC   = DATA[ti].d_spec_p;
            const size_t  LENGTH = strlen(SPEC);

            if (verbose) {
                printf("\t\tCreating object of "); P_(LENGTH);
                printf("using "); P(SPEC);
            }

            CONTAINER mU(gV(SPEC));  const CONTAINER& U = mU;
            Obj mY;  const Obj& Y = gg(&mY, SPEC);

            const Int64 BB = oa.numBlocksTotal();
            const Int64  B = oa.numBlocksInUse();

            if (veryVerbose) { printf("\t\tBEFORE: "); P_(BB); P(B);}

            BSLMA_TESTALLOCATOR_EXCEPTION_TEST_BEGIN(oa) {
                Obj mX(U.begin(), U.end(), xoa);

                const Obj& X = mX;

                if (veryVerbose) {
                    T_; T_; P_(X); P(X.capacity());
                }

                LOOP2_ASSERT(LINE, ti, LENGTH == X.size());
                LOOP2_ASSERT(LINE, ti, LENGTH <= X.capacity());

                for (size_t j = 0; j < LENGTH; ++j) {
                    LOOP3_ASSERT(LINE, ti, j, Y[j] == X[j]);
                }

            } BSLMA_TESTALLOCATOR_EXCEPTION_TEST_END

            const Int64 AA = oa.numBlocksTotal();
            const Int64  A = oa.numBlocksInUse();

            if (veryVerbose) { printf("\t\tAFTER : "); P_(AA); P(A);}

            LOOP2_ASSERT(LINE, ti, 0 == oa.numBlocksInUse());
        }
    }
}



template <class TYPE>
struct MetaTestDriver {
    // This 'struct' is to be called by the 'RUN_EACH_TYPE' macro, and the
    // functions within it dispatch to functions in 'TestDriver' instantiated
    // with different types of allocator.

    typedef bsl::allocator<TYPE>            BAP;
    typedef bsltf::StdAllocatorAdaptor<BAP> SAA;

    static void testCase25();
        // Test move-assign.

    static void testCase21();
        // Test member and free 'swap'.
};

template <class TYPE>
void MetaTestDriver<TYPE>::testCase25()
{
    // Limit the # of tests if we're in C++11 to avoid running out of compiler
    // memory.

    // The low-order bit of the identifier specifies whether the fourth boolean
    // argument of the stateful allocator, which indicates propagate on move
    // assign, is set.

    typedef bsltf::StdStatefulAllocator<TYPE, false, false, false, false> A00;
    typedef bsltf::StdStatefulAllocator<TYPE, false, false, false, true>  A01;
#if !defined(BSLS_COMPILERFEATURES_SUPPORT_GENERALIZED_INITIALIZERS)
    typedef bsltf::StdStatefulAllocator<TYPE, true,  true,  true,  false> A10;
    typedef bsltf::StdStatefulAllocator<TYPE, true,  true,  true,  true>  A11;
#endif

    if (verbose) printf("\n");

    TestDriver<TYPE, BAP>::testCase25_dispatch();

    TestDriver<TYPE, A00>::testCase25_dispatch();
    TestDriver<TYPE, A01>::testCase25_dispatch();

#if !defined(BSLS_COMPILERFEATURES_SUPPORT_GENERALIZED_INITIALIZERS)
    TestDriver<TYPE, SAA>::testCase25_dispatch();

    TestDriver<TYPE, A10>::testCase25_dispatch();
    TestDriver<TYPE, A11>::testCase25_dispatch();
#endif
}

template <class TYPE>
void MetaTestDriver<TYPE>::testCase21()
{
    // Limit the # of tests if we're in C++11 to avoid running out of compiler
    // memory.

    // The low-order bit of the identifier specifies whether the third boolean
    // argument of the stateful allocator, which indicates propagate on
    // container swap, is set.

    typedef bsltf::StdStatefulAllocator<TYPE, false, false, false, false> A00;
    typedef bsltf::StdStatefulAllocator<TYPE, false, false, true,  false> A01;
#if !defined(BSLS_COMPILERFEATURES_SUPPORT_GENERALIZED_INITIALIZERS)
    typedef bsltf::StdStatefulAllocator<TYPE, true,  true,  false, true>  A10;
    typedef bsltf::StdStatefulAllocator<TYPE, true,  true,  true,  true>  A11;
#endif

    if (verbose) printf("\n");

    TestDriver<TYPE, BAP>::testCase21_dispatch();

    TestDriver<TYPE, A00>::testCase21_dispatch();
    TestDriver<TYPE, A01>::testCase21_dispatch();

#if !defined(BSLS_COMPILERFEATURES_SUPPORT_GENERALIZED_INITIALIZERS)
    TestDriver<TYPE, SAA>::testCase21_dispatch();

    TestDriver<TYPE, A10>::testCase21_dispatch();
    TestDriver<TYPE, A11>::testCase21_dispatch();
#endif
}

// ============================================================================
//                              MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int test = argc > 1 ? atoi(argv[1]) : 0;

                verbose = argc > 2;
            veryVerbose = argc > 3;
        veryVeryVerbose = argc > 4;
    veryVeryVeryVerbose = argc > 5;

    bslma::TestAllocator defaultAllocator("default", veryVeryVeryVerbose);
    bslma::DefaultAllocatorGuard dag(&defaultAllocator);

    bslma::TestAllocator globalAllocator("global", veryVeryVeryVerbose);
    bslma::Default::setGlobalAllocator(&globalAllocator);

    // Verify sanity of DEFAULT_DATA

    {
        typedef TestDriver<int> TD;

        const size_t NUM_DATA                  = DEFAULT_NUM_DATA;
        const DefaultDataRow (&DATA)[NUM_DATA] = DEFAULT_DATA;

        bslma::TestAllocator sa("scratch");

        TD::Obj mX(&sa); const TD::Obj& X = mX;
        TD::Obj mY(&sa); const TD::Obj& Y = mY;

        for (unsigned ii = 0; ii < NUM_DATA; ++ii) {
            const int   INDEX1  = DATA[ii].d_index;
            const char *SPEC1   = DATA[ii].d_spec;
            const char *RESULT1 = DATA[ii].d_results;

            ASSERT(!ii || DATA[ii-1].d_index <= INDEX1);
            ASSERT(strlen(SPEC1) == strlen(RESULT1));

            mX.clear();
            TD::gg(&mX, SPEC1);
            ASSERTV(ii, SPEC1, RESULT1, verifySpec(X, RESULT1));

            mY.clear();
            TD::gg(&mY, RESULT1);
            ASSERTV(ii, SPEC1, RESULT1, X == Y);

            for (unsigned jj = 0; jj < NUM_DATA; ++jj) {
                ASSERT((INDEX1 == DATA[jj].d_index) ==
                                         !strcmp(RESULT1, DATA[jj].d_results));
            }
        }
    }

    printf("TEST " __FILE__ " CASE %d\n", test);

    switch (test) { case 0:  // Zero is always the leading case.
      case 32: {
        // --------------------------------------------------------------------
        // 'noexcept' SPECIFICATION
        // --------------------------------------------------------------------
        if (verbose) printf("\n" "'noexcept' SPECIFICATION" "\n"
                                 "========================" "\n");

        TestDriver<int>::testCase32();

      } break;
      case 31: {
        // --------------------------------------------------------------------
        // TESTING 'shrink_to_fit'
        // --------------------------------------------------------------------

        if (verbose) printf("TESTING 'shrink_to_fit'\n"
                            "=======================\n");

        RUN_EACH_TYPE(TestDriver,
                      testCase31,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_REGULAR);

        RUN_EACH_TYPE(TestDriver,
                      testCase31,
                      bsltf::MoveOnlyAllocTestType,
                      bsltf::WellBehavedMoveOnlyAllocTestType);

         RUN_EACH_TYPE(StdBslmaTestDriver,
                      testCase31,
                      bsltf::StdAllocTestType<bsl::allocator<int> >,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_PRIMITIVE);
      } break;
      case 30: {
        // --------------------------------------------------------------------
        // TESTING INITIALIZER LIST FUNCTIONS
        // --------------------------------------------------------------------

        RUN_EACH_TYPE(TestDriver,
                      testCase30,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_REGULAR);

        RUN_EACH_TYPE(StdBslmaTestDriver,
                      testCase30,
                      bsltf::StdAllocTestType<bsl::allocator<int> >,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_PRIMITIVE);
     } break;
      case 29: {
        // --------------------------------------------------------------------
        // TESTING 'emplace'
        // --------------------------------------------------------------------

        if (verbose) printf("\nTesting Value Emplacement"
                            "\n=========================\n");

#if !defined(BSLS_PLATFORM_CMP_SUN) || !defined(BSLS_PLATFORM_CMP_GNU)
# if !defined(u_LIMIT_EMPLACE_TESTS)
        RUN_EACH_TYPE(TestDriver,
                      testCase29,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_REGULAR,
                      bsltf::NonDefaultConstructibleTestType);

        RUN_EACH_TYPE(TestDriver,
                      testCase29a,
                      bsltf::EmplacableTestType,
                      bsltf::AllocEmplacableTestType);
# else
        RUN_EACH_TYPE(TestDriver,
                      testCase29,
                      signed char,
                      bsltf::TemplateTestFacility::ObjectPtr,
                      bsltf::AllocTestType,
                      bsltf::NonDefaultConstructibleTestType);

        RUN_EACH_TYPE(TestDriver,
                      testCase29a,
                      bsltf::AllocEmplacableTestType);

        Q(Emplace Tests Limited);
# endif
#else
        Q(Emplace Tests Prevented);
#endif
        RUN_EACH_TYPE(StdBslmaTestDriver,
                      testCase29,
                      bsltf::StdAllocTestType<bsl::allocator<int> >,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_PRIMITIVE);

      } break;
      case 28: {
        // --------------------------------------------------------------------
        // TESTING 'emplace_front' AND 'emplace_back'
        // --------------------------------------------------------------------

        if (verbose) printf("\nTesting Value Emplacement"
                            "\n=========================\n");

#if !defined(BSLS_PLATFORM_CMP_SUN) || !defined(BSLS_PLATFORM_CMP_GNU)
# if !defined(u_LIMIT_EMPLACE_TESTS)
        RUN_EACH_TYPE(TestDriver,
                      testCase28,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_REGULAR,
                      bsltf::NonDefaultConstructibleTestType);

        RUN_EACH_TYPE(StdBslmaTestDriver,
                      testCase28,
                      bsltf::StdAllocTestType<bsl::allocator<int> >,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_PRIMITIVE);

        // 'emplace_back'

        RUN_EACH_TYPE(TestDriver,
                      testCase28a,
                      bsltf::EmplacableTestType,
                      bsltf::AllocEmplacableTestType);

        // 'emplace_front'

        RUN_EACH_TYPE(TestDriver,
                      testCase28b,
                      bsltf::EmplacableTestType,
                      bsltf::AllocEmplacableTestType);
# else
        RUN_EACH_TYPE(TestDriver,
                      testCase28,
                      signed char,
                      bsltf::TemplateTestFacility::ObjectPtr,
                      bsltf::AllocTestType,
                      bsltf::NonDefaultConstructibleTestType);

        // 'emplace_back'

        RUN_EACH_TYPE(TestDriver,
                      testCase28a,
                      bsltf::AllocEmplacableTestType);

        // 'emplace_front'

        RUN_EACH_TYPE(TestDriver,
                      testCase28b,
                      bsltf::AllocEmplacableTestType);

        Q(Emplace Tests Limited);
# endif
#else
        Q(Emplace Tests Prevented);
#endif
      } break;
      case 27: {
        // --------------------------------------------------------------------
        // TESTING 'insert' ON MOVABLE VALUES
        // --------------------------------------------------------------------

        RUN_EACH_TYPE(TestDriver,
                      testCase27,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_REGULAR);

        RUN_EACH_TYPE(TestDriver,
                      testCase27,
                      bsltf::MoveOnlyAllocTestType,
                      bsltf::WellBehavedMoveOnlyAllocTestType);

        RUN_EACH_TYPE(StdBslmaTestDriver,
                      testCase27,
                      bsltf::StdAllocTestType<bsl::allocator<int> >,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_PRIMITIVE);

      } break;
      case 26: {
        // --------------------------------------------------------------------
        // TESTING 'push_front' AND 'push_back' ON MOVABLE VALUES
        // --------------------------------------------------------------------

        RUN_EACH_TYPE(TestDriver,
                      testCase26,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_REGULAR);

        RUN_EACH_TYPE(TestDriver,
                      testCase26,
                      bsltf::MoveOnlyAllocTestType,
                      bsltf::WellBehavedMoveOnlyAllocTestType);

        RUN_EACH_TYPE(StdBslmaTestDriver,
                      testCase26,
                      bsltf::StdAllocTestType<bsl::allocator<int> >,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_PRIMITIVE);

      } break;
      case 25: {
        // --------------------------------------------------------------------
        // TESTING MOVE-ASSIGNMENT OPERATOR
        // --------------------------------------------------------------------

        RUN_EACH_TYPE(MetaTestDriver,
                      testCase25,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_REGULAR);

        RUN_EACH_TYPE(MetaTestDriver,
                      testCase25,
                      bsltf::MoveOnlyAllocTestType,
                      bsltf::WellBehavedMoveOnlyAllocTestType);
      } break;
      case 24: {
        // --------------------------------------------------------------------
        // TESTING MOVE CONSTRUCTOR
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING MOVE CONSTRUCTOR"
                            "\n========================\n");

        RUN_EACH_TYPE(TestDriver,
                      testCase24,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_REGULAR);

        RUN_EACH_TYPE(TestDriver,
                      testCase24,
                      bsltf::MoveOnlyAllocTestType,
                      bsltf::WellBehavedMoveOnlyAllocTestType);

        RUN_EACH_TYPE(StdBslmaTestDriver,
                      testCase24,
                      bsltf::StdAllocTestType<bsl::allocator<int> >,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_PRIMITIVE);

      } break;
      case 23: {
        // --------------------------------------------------------------------
        // TESTING EXCEPTIONS
        //
        // Testing:
        //   CONCERN: 'std::length_error' is used properly.
        // --------------------------------------------------------------------

        if (verbose) printf("TESTING EXCEPTIONS\n"
                            "==================\n");

        TestDriver<TTA>::testCase23();

      } break;
      case 22: {
        // --------------------------------------------------------------------
        // TESTING FREE COMPARISON OPERATORS
        //
        // Testing:
        //   bool operator< (const deque& lhs, const deque& rhs);
        //   bool operator> (const deque& lhs, const deque& rhs);
        //   bool operator<=(const deque& lhs, const deque& rhs);
        //   bool operator>=(const deque& lhs, const deque& rhs);
        // --------------------------------------------------------------------

        if (verbose) printf("TESTING FREE COMPARISON OPERATORS\n"
                            "=================================\n");

        if (verbose) printf("\n... with 'char'.\n");
        TestDriver<char>::testCase22();

        if (verbose) printf("\n... with 'TestTypeAlloc'.\n");
        TestDriver<TTA>::testCase22();

      } break;
      case 21: {
        // --------------------------------------------------------------------
        // TESTING SWAP
        //
        // Testing:
        //   void swap(deque&);
        //   void swap(deque&, deque&);
        // --------------------------------------------------------------------

        if (verbose) printf("TESTING SWAP\n"
                            "============\n");

        RUN_EACH_TYPE(MetaTestDriver,
                      testCase21,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_REGULAR);

        RUN_EACH_TYPE(MetaTestDriver,
                      testCase21,
                      bsltf::MoveOnlyAllocTestType,
                      bsltf::WellBehavedMoveOnlyAllocTestType);
      } break;
      case 20: {
        // --------------------------------------------------------------------
        // TESTING ERASE AND POP_BACK|POP_FRONT
        //
        // Testing:
        //   void pop_front();
        //   void pop_back();
        //   iterator erase(const_iterator pos);
        //   iterator erase(const_iterator first, const_iterator last);
        // --------------------------------------------------------------------

        if (verbose) printf("TESTING ERASE AND POP_BACK|POP_FRONT\n"
                            "====================================\n");

        // TBD Want to add an instrumented sanity test for 'pop_back'.  By
        // inspection, it looks like 'pop_back' destroys the item *preceding*
        // the last item, potentially after deallocating the trailing page if
        // the last item was the first item on a data page.

        RUN_EACH_TYPE(TestDriver,
                      testCase20,
                      char,
                      TTA,
                      SmlTT,
                      MedTT,
                      LrgTT,
                      BMTTA,
                      BCTT);

        RUN_EACH_TYPE(TestDriver,
                      testCase20,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_REGULAR);

        RUN_EACH_TYPE(TestDriver,
                      testCase20,
                      bsltf::MoveOnlyAllocTestType,
                      bsltf::WellBehavedMoveOnlyAllocTestType);

        RUN_EACH_TYPE(StdBslmaTestDriver,
                      testCase20,
                      bsltf::StdAllocTestType<bsl::allocator<int> >,
                      BSLTF_TEMPLATETESTFACILITY_TEST_TYPES_PRIMITIVE);

      } break;
      case 19: {
        // --------------------------------------------------------------------
        // TESTING INPUT-RANGE INSERTION
        //
        // Testing:
        //   iterator insert(const_iterator pos, ITER first, ITER last);
        // --------------------------------------------------------------------

        if (verbose) printf("TESTING INPUT-RANGE INSERTION\n"
                            "=============================\n");

        if (verbose) printf("\n... with 'TestTypeAlloc' "
                            "and arbitrary input iterator.\n");
        TestDriver<TTA>::testCase19(CharList<TTA>());

        if (verbose) printf("\n... with 'TestTypeAlloc' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<TTA>::testCase19(CharArray<TTA>());


        if (verbose) printf("\n... with 'MediumTestTypeNoAlloc' "
                            "and arbitrary input iterator.\n");
        TestDriver<MedTT>::testCase19(CharList<MedTT>());

        if (verbose) printf("\n... with 'MediumTestTypeNoAlloc' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<MedTT>::testCase19(CharArray<MedTT>());


        if (verbose) printf("\n... with 'BitwiseMoveableTestTypeAlloc' "
                            "and arbitrary input iterator.\n");
        TestDriver<BMTTA>::testCase19(CharList<BMTTA>());

        if (verbose) printf("\n... with 'BitwiseMoveableTestTypeAlloc' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<BMTTA>::testCase19(CharArray<BMTTA>());


        if (verbose) printf("\n... with 'BitwiseCopyableTestTypeNoAlloc' "
                            "and arbitrary input iterator.\n");
        TestDriver<BCTT>::testCase19(CharList<BCTT>());

        if (verbose) printf("\n... with 'BitwiseCopyableTestTypeNoAlloc' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<BCTT>::testCase19(CharArray<BCTT>());

        typedef bsltf::StdAllocTestType<bsl::allocator<int> > AllocInt;
        StdBslmaTestDriver<AllocInt>::testCase19(CharArray<AllocInt>());

      } break;
      case 18: {
        // --------------------------------------------------------------------
        // TESTING VALUE INSERTION
        //
        // Testing:
        //   iterator insert(const_iterator pos, const T& value);
        //   iterator insert(const_iterator pos, size_type n, const T& value);
        // --------------------------------------------------------------------

        if (verbose) printf("TESTING VALUE INSERTION\n"
                            "=======================\n");

        RUN_EACH_TYPE(TestDriver,
                      testCase18,
                      char,
                      TTA,
                      SmlTT,
                      MedTT,
                      LrgTT,
                      BMTTA,
                      BCTT);

        typedef bsltf::StdAllocTestType<bsl::allocator<int> > AllocInt;
        StdBslmaTestDriver<AllocInt>::testCase18();

      } break;
      case 17: {
        // --------------------------------------------------------------------
        // TESTING PUSHING VALUES
        //
        // Testing:
        //   void push_front(const T& value);
        //   void push_back(const T& value);
        // --------------------------------------------------------------------

        if (verbose) printf("TESTING PUSHING VALUES\n"
                            "======================\n");

        RUN_EACH_TYPE(TestDriver,
                      testCase17,
                      char,
                      TTA,
                      SmlTT,
                      MedTT,
                      LrgTT,
                      BMTTA,
                      BCTT);

        typedef bsltf::StdAllocTestType<bsl::allocator<int> > AllocInt;
        StdBslmaTestDriver<AllocInt>::testCase17();
      } break;
      case 16: {
        // --------------------------------------------------------------------
        // TESTING ITERATORS
        //
        // Testing:
        //   iterator begin();
        //   iterator end();
        //   reverse_iterator rbegin();
        //   reverse_iterator rend();
        //   const_iterator begin() const;
        //   const_iterator cbegin() const;
        //   const_iterator end() const;
        //   const_iterator cend() const;
        //   const_reverse_iterator rbegin() const;
        //   const_reverse_iterator crbegin() const;
        //   const_reverse_iterator rend() const;
        //   const_reverse_iterator crend() const;
        // --------------------------------------------------------------------

        if (verbose) printf("TESTING ITERATORS\n"
                            "=================\n");

        RUN_EACH_TYPE(TestDriver,
                      testCase16,
                      char,
                      TTA);

        typedef bsltf::StdAllocTestType<bsl::allocator<int> > AllocInt;
        StdBslmaTestDriver<AllocInt>::testCase16();
      } break;
      case 15: {
        // --------------------------------------------------------------------
        // TESTING ELEMENT ACCESS
        //
        // Testing:
        //   reference operator[](size_type position);
        //   reference at(size_type position);
        //   reference front();
        //   reference back();
        //   const_reference front() const;
        //   const_reference back() const;
        // --------------------------------------------------------------------

        if (verbose) printf("TESTING ELEMENT ACCESS\n"
                            "======================\n");

        RUN_EACH_TYPE(TestDriver,
                      testCase15,
                      char,
                      TTA,
                      BMTTA,
                      BCTT);

        typedef bsltf::StdAllocTestType<bsl::allocator<int> > AllocInt;
        StdBslmaTestDriver<AllocInt>::testCase15();
      } break;
      case 14: {
        // --------------------------------------------------------------------
        // TESTING RESERVE AND CAPACITY
        //
        // Testing:
        //   void reserve(size_type n);
        //   void resize(size_type n);
        //   void resize(size_type n, const T& value);
        //   size_type max_size() const;
        //   size_type capacity() const;
        //   bool empty() const;
        // --------------------------------------------------------------------

        if (verbose) printf("TESTING RESERVE AND CAPACITY\n"
                            "============================\n");

        RUN_EACH_TYPE(TestDriver,
                      testCase14,
                      char,
                      TTA,
                      BMTTA,
                      BCTT);

        typedef bsltf::StdAllocTestType<bsl::allocator<int> > AllocInt;
        StdBslmaTestDriver<AllocInt>::testCase14();

      } break;
      case 13: {
        // --------------------------------------------------------------------
        // TESTING ASSIGN
        //
        // Testing:
        //   void assign(size_t n, const T& value);
        //   void assign(ITER first, ITER last);
        // --------------------------------------------------------------------

        if (verbose) printf("TESTING ASSIGNMENT\n"
                            "==================\n");

        RUN_EACH_TYPE(TestDriver,
                      testCase13,
                      char,
                      TTA,
                      BMTTA,
                      BCTT);

        typedef bsltf::StdAllocTestType<bsl::allocator<int> > AllocInt;
        StdBslmaTestDriver<AllocInt>::testCase13();

        if (verbose) printf("\nTesting Initial-Range Assignment"
                            "\n================================\n");

        if (verbose) printf("\n... with 'char' "
                            "and arbitrary input iterator.\n");
        TestDriver<char>::testCase13Range(CharList<char>());

        if (verbose) printf("\n... with 'char' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<char>::testCase13Range(CharArray<char>());

        if (verbose) printf("\n... with 'TestTypeAlloc' "
                            "and arbitrary input iterator.\n");
        TestDriver<TTA>::testCase13Range(CharList<TTA>());

        if (verbose) printf("\n... with 'TestTypeAlloc' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<TTA>::testCase13Range(CharArray<TTA>());

        if (verbose) printf("\n... with 'BitwiseMoveableTestTypeAlloc' "
                            "and arbitrary input iterator.\n");
        TestDriver<BMTTA>::testCase13Range(CharList<BMTTA>());

        if (verbose) printf("\n... with 'BitwiseMoveableTestTypeAlloc' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<BMTTA>::testCase13Range(CharArray<BMTTA>());

        if (verbose) printf("\n... with 'BitwiseCopyableTestTypeNoAlloc' "
                            "and arbitrary input iterator.\n");
        TestDriver<BCTT>::testCase13Range(CharList<BCTT>());

        if (verbose) printf("\n... with 'BitwiseCopyableTestTypeNoAlloc' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<BCTT>::testCase13Range(CharArray<BCTT>());

        StdBslmaTestDriver<AllocInt>::testCase13Range(CharArray<AllocInt>());

      } break;
      case 12: {
        // --------------------------------------------------------------------
        // TESTING RANGE AND INITIAL-LENGTH CONSTRUCTORS
        //
        // Testing:
        //   deque(size_type n, const A& a = A());
        //   deque(size_type n, const T& value, const A& a = A());
        //   deque(ITER first, ITER last, const A& a = A());
        // --------------------------------------------------------------------

        if (verbose) printf("TESTING RANGE AND INITIAL-LENGTH CONSTRUCTORS\n"
                            "=============================================\n");

        if (verbose) printf("\nTesting Initial-Length Constructor"
                            "\n==================================\n");

        RUN_EACH_TYPE(TestDriver,
                      testCase12,
                      char,
                      TTA,
                      BMTTA,
                      BCTT);

        typedef bsltf::StdAllocTestType<bsl::allocator<int> > AllocInt;
        StdBslmaTestDriver<AllocInt>::testCase12();

        if (verbose) printf("\nTesting Initial-Range Constructor"
                            "\n=================================\n");

        if (verbose) printf("\n... with 'char' "
                            "and arbitrary input iterator.\n");
        TestDriver<char>::testCase12Range(CharList<char>());

        if (verbose) printf("\n... with 'char' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<char>::testCase12Range(CharArray<char>());

        if (verbose) printf("\n... with 'TestTypeAlloc' "
                            "and arbitrary input iterator.\n");
        TestDriver<TTA>::testCase12Range(CharList<TTA>());

        if (verbose) printf("\n... with 'TestTypeAlloc' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<TTA>::testCase12Range(CharArray<TTA>());

        if (verbose) printf("\n... with 'BitwiseMoveableTestTypeAlloc' "
                            "and arbitrary input iterator.\n");
        TestDriver<BMTTA>::testCase12Range(CharList<BMTTA>());

        if (verbose) printf("\n... with 'BitwiseMoveableTestTypeAlloc' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<BMTTA>::testCase12Range(CharArray<BMTTA>());

        if (verbose) printf("\n... with 'BitwiseCopyableTestTypeNoAlloc' "
                            "and arbitrary input iterator.\n");
        TestDriver<BCTT>::testCase12Range(CharList<BCTT>());

        if (verbose) printf("\n... with 'BitwiseCopyableTestTypeNoAlloc' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<BCTT>::testCase12Range(CharArray<BCTT>());

        StdBslmaTestDriver<AllocInt>::testCase12Range(CharArray<AllocInt>());

      } break;
      case 11:  // falls through
      case 10:  // falls through
      case  9:  // falls through
      case  8:  // falls through
      case  7:  // falls through
      case  6:  // falls through
      case  5:  // falls through
      case  4:  // falls through
      case  3:  // falls through
      case  2:  // falls through
      case  1: {
        if (verbose)
            printf("\nTEST CASE %d IS HANDLED BY PRIMARY TEST DRIVER"
                   "\n==============================================\n",
                   test);
      } break;
      case -1: {
        // --------------------------------------------------------------------
        // PERFORMANCE TEST
        //
        // Concerns:
        //   Provide benchmark for subsequent improvements to 'bsl::deque'.
        //   The benchmark should measure the speed of various operations such
        //   as 'push_back', 'push_front', 'pop_back', 'pop_front', 'insert',
        //   etc.
        //
        // Plan:
        //   Using 'bsls_stopwatch', the run time of the various methods under
        //   test be tallied over various iterations.  These values should only
        //   be used as a comparison across various versions.  They are NOT
        //   meant to be used to compare which method runs faster since the
        //   loops they run in have various lengths.
        //
        // Testing:
        //   PERFORMANCE TEST
        // --------------------------------------------------------------------

        if (verbose) printf("\nPERFORMANCE TEST"
                            "\n================\n");

        if (verbose) printf("\n... with 'char' type.\n");
        TestDriver<char>::testCaseM1();

        if (verbose) printf("\n... with 'TestTypeAlloc'.\n");
        TestDriver<TTA>::testCaseM1();

        if (verbose) printf("\n... with 'SmallTestTypeNoAlloc'.\n");
        TestDriver<SmlTT>::testCaseM1();

        if (verbose) printf("\n... with 'MediumTestTypeNoAlloc'.\n");
        TestDriver<MedTT>::testCaseM1();

        if (verbose) printf("\n... with 'LargeTestTypeNoAlloc'.\n");
        TestDriver<LrgTT>::testCaseM1();

        if (verbose) printf("\n... with 'BitwiseMoveableTestTypeAlloc'.\n");
        TestDriver<BMTTA>::testCaseM1();

        if (verbose) printf("\n... with 'BitwiseCopyableTestTypeNoAlloc'.\n");
        TestDriver<BCTT>::testCaseM1();

        if (verbose) printf("\nPERFORMANCE TEST RANGE"
                            "\n======================\n");

        if (verbose) printf("\n... with 'TestTypeAlloc' "
                            "and arbitrary forward iterator.\n");
        TestDriver<TTA>::testCaseM1Range(CharList<TTA>());

        if (verbose) printf("\n... with 'TestTypeAlloc' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<TTA>::testCaseM1Range(CharArray<TTA>());

        if (verbose) printf("\n... with 'BitwiseMoveableTestTypeAlloc' "
                            "and arbitrary forward iterator.\n");
        TestDriver<BMTTA>::testCaseM1Range(CharList<BMTTA>());

        if (verbose) printf("\n... with 'BitwiseMoveableTestTypeAlloc' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<BMTTA>::testCaseM1Range(CharArray<BMTTA>());

        if (verbose) printf("\n... with 'BitwiseCopyableTestTypeNoAlloc' "
                            "and arbitrary forward iterator.\n");
        TestDriver<BCTT>::testCaseM1Range(CharList<BCTT>());

        if (verbose) printf("\n... with 'BitwiseCopyableTestTypeNoAlloc' "
                            "and arbitrary random-access iterator.\n");
        TestDriver<BCTT>::testCaseM1Range(CharArray<BCTT>());

      } break;
      default: {
        fprintf(stderr, "WARNING: CASE `%d' NOT FOUND.\n", test);
        testStatus = -1;
      }
    }

    ASSERTV(defaultAllocator.numBlocksInUse(),
            0 == defaultAllocator.numBlocksInUse());

    // CONCERN: In no case does memory come from the global allocator.
    ASSERTV(globalAllocator.numBlocksTotal(),
            0 == globalAllocator.numBlocksTotal());

    if (testStatus > 0) {
        fprintf(stderr, "Error, non-zero test status = %d.\n", testStatus);
    }

    return testStatus;
}

// ----------------------------------------------------------------------------
// Copyright 2013 Bloomberg Finance L.P.
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
