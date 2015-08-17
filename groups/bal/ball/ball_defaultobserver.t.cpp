// ball_defaultobserver.t.cpp                                         -*-C++-*-

#include <ball_defaultobserver.h>

#include <ball_context.h>                      // for testing only
#include <ball_record.h>                       // for testing only
#include <ball_recordattributes.h>             // for testing only

#include <bdlt_datetime.h>                     // for testing only
#include <bdlt_epochutil.h>                    // for testing only

#include <bsls_platform.h>                     // for testing only

#include <bslma_testallocator.h>               // for testing only
#include <bslma_testallocatorexception.h>      // for testing only

#include <bsl_cstdlib.h>     // atoi()
#include <bsl_cstring.h>     // strlen(), memset(), memcpy(), memcmp()

#include <bsl_new.h>         // placement 'new' syntax
#include <bsl_iostream.h>

// Note: on Windows -> WinGDI.h:#define PASSTHROUGH 19
#if defined(BSLS_PLATFORM_CMP_MSVC) && defined(PASSTHROUGH)
#undef PASSTHROUGH
#endif

using namespace BloombergLP;
using namespace bsl;  // automatically added by script

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
//                              Overview
//                              --------
// TBD - fill
//
// Note that places where test drivers in this family are likely to require
// adjustment are indicated by the tag: "ADJ".
//-----------------------------------------------------------------------------
// [  ] etc.
//-----------------------------------------------------------------------------
// [ 1] BREATHING TEST
// [ 2] USAGE EXAMPLE

//=============================================================================
//                      STANDARD BDE ASSERT TEST MACRO
//-----------------------------------------------------------------------------
static int testStatus = 0;

void aSsErT(int c, const char *s, int i)
{
    if (c) {
        cout << "Error " << __FILE__ << "(" << i << "): " << s
             << "    (failed)" << endl;
        if (0 <= testStatus && testStatus <= 100) ++testStatus;
    }
}

#define ASSERT(X) { aSsErT(!(X), #X, __LINE__); }

//=============================================================================
//                  STANDARD BDE LOOP-ASSERT TEST MACROS
//-----------------------------------------------------------------------------
#define LOOP_ASSERT(I,X) { \
   if (!(X)) { cout << #I << ": " << I << "\n"; aSsErT(1, #X, __LINE__); }}

#define LOOP2_ASSERT(I,J,X) { \
   if (!(X)) { cout << #I << ": " << I << "\t" << #J << ": " \
              << J << "\n"; aSsErT(1, #X, __LINE__); } }

#define LOOP3_ASSERT(I,J,K,X) { \
   if (!(X)) { cout << #I << ": " << I << "\t" << #J << ": " << J << "\t" \
              << #K << ": " << K << "\n"; aSsErT(1, #X, __LINE__); } }

#define LOOP4_ASSERT(I,J,K,L,X) { \
   if (!(X)) { cout << #I << ": " << I << "\t" << #J << ": " << J << "\t" << \
       #K << ": " << K << "\t" << #L << ": " << L << "\n"; \
       aSsErT(1, #X, __LINE__); } }

#define LOOP5_ASSERT(I,J,K,L,M,X) { \
   if (!(X)) { cout << #I << ": " << I << "\t" << #J << ": " << J << "\t" << \
       #K << ": " << K << "\t" << #L << ": " << L << "\t" << \
       #M << ": " << M << "\n"; \
       aSsErT(1, #X, __LINE__); } }

#define LOOP6_ASSERT(I,J,K,L,M,N,X) { \
   if (!(X)) { cout << #I << ": " << I << "\t" << #J << ": " << J << "\t" << \
       #K << ": " << K << "\t" << #L << ": " << L << "\t" << \
       #M << ": " << M << "\t" << #N << ": " << N << "\n"; \
       aSsErT(1, #X, __LINE__); } }

//=============================================================================
//                  SEMI-STANDARD TEST OUTPUT MACROS
//-----------------------------------------------------------------------------
#define P(X) cout << #X " = " << (X) << endl; // Print identifier and value.
#define Q(X) cout << "<| " #X " |>" << endl;  // Quote identifier literally.
#define P_(X) cout << #X " = " << (X) << ", "<< flush; // P(X) without '\n'
#define L_ __LINE__                           // current Line number
#define T_()  cout << "\t" << flush;          // Print tab w/o newline

//=============================================================================
//                  GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
//-----------------------------------------------------------------------------

typedef ball::DefaultObserver Obj;

//=============================================================================
//                  GLOBAL HELPER FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

//=============================================================================
//              GENERATOR FUNCTIONS 'g' AND 'gg' FOR TESTING
//-----------------------------------------------------------------------------

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int test = argc > 1 ? atoi(argv[1]) : 0;
    int verbose = argc > 2;
//    int veryVerbose = argc > 3; // not used
    int veryVeryVerbose = argc > 4;

    cout << "TEST " << __FILE__ << " CASE " << test << endl;;

    bslma::TestAllocator testAllocator(veryVeryVerbose);

    switch (test) { case 0:  // Zero is always the leading case.
      case 2: {
        // --------------------------------------------------------------------
        // TESTING USAGE EXAMPLE
        //
        // Concerns:
        //   The usage example provided in the component header file must
        //   compile, link, and run on all platforms as shown.
        //
        // Plan:
        //   Incorporate usage example from header into driver, remove leading
        //   comment characters, and replace 'assert' with 'ASSERT'.
        //
        // Testing:
        //   USAGE EXAMPLE
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "Testing Usage Example" << endl
                                  << "=====================" << endl;

        {
            ball::DefaultObserver defaultObserver(&bsl::cout);

            // Do *not* do this.  It significantly increases the level #
            // of this component.
            // ball::LoggerManagerConfiguration lmc;
            // ball::LoggerManager loggerManager(&defaultObserver, lmc);
        }
      } break;
      case 1: {
        // --------------------------------------------------------------------
        // BREATHING TEST
        //
        // Concerns:
        //   We want to exercise basic functionality.
        //
        //
        //
        // Plan:
        //   TBD - fill
        //   Note: The ad hoc tests provided here are repeated for the
        //   deprecated c'tor taking an 'ostream&' and for the c'tor taking
        //   the proper 'ostream*'.  The use of 'cout' in production testing
        //   is not actually permitted.
        //
        // Testing:
        //   This "test" *exercises* basic functionality, but *tests* nothing.

        // --------------------------------------------------------------------

        if (verbose) cout << endl << "BREATHING TEST" << endl
                                  << "==============" << endl;

        if (verbose)
            cout << "Publish a single message (deprecated c'tor)." << endl;
        {
            Obj X(bsl::cout);

            ball::RecordAttributes fixed;
            ball::UserFields  userValues;

            bdlt::Datetime now = bdlt::EpochUtil::convertFromTimeT(time(0));
            fixed.setTimestamp(now);
            fixed.setProcessID(100);
            fixed.setThreadID(0);
            X.publish(ball::Record(fixed, userValues),
                    ball::Context(ball::Transmission::e_PASSTHROUGH, 0, 1));
        }

        if (verbose) cout << "Publish a single message." << endl;
        {
            Obj X(&bsl::cout);

            ball::RecordAttributes fixed;
            ball::UserFields  userValues;

            bdlt::Datetime now = bdlt::EpochUtil::convertFromTimeT(time(0));
            fixed.setTimestamp(now);
            fixed.setProcessID(100);
            fixed.setThreadID(0);
            X.publish(ball::Record(fixed, userValues),
                      ball::Context(ball::Transmission::e_PASSTHROUGH,
                                               0,
                                               1));
        }

        if (verbose) cout << "Publish a sequence of three messages "
                          << "(deprecated c'tor)." << endl;
        {
            Obj X(bsl::cout);

            ball::RecordAttributes fixed;
            ball::UserFields  userValues;

            const int NUM_MESSAGES = 3;
            for (int n = 0; n < NUM_MESSAGES; ++n) {
                bdlt::Datetime now =
                                    bdlt::EpochUtil::convertFromTimeT(time(0));
                fixed.setTimestamp(now);
                fixed.setProcessID(201 + n);
                fixed.setThreadID(31 + n);
                X.publish(ball::Record(fixed, userValues),
                          ball::Context(ball::Transmission::e_TRIGGER,
                                       n,
                                       NUM_MESSAGES));
            }
        }

        if (verbose) cout << "Publish a sequence of three messages." << endl;
        {
            Obj X(&bsl::cout);

            ball::RecordAttributes fixed;
            ball::UserFields  userValues;

            const int NUM_MESSAGES = 3;
            for (int n = 0; n < NUM_MESSAGES; ++n) {
                bdlt::Datetime now =
                                    bdlt::EpochUtil::convertFromTimeT(time(0));
                fixed.setTimestamp(now);
                fixed.setProcessID(201 + n);
                fixed.setThreadID(31 + n);
                X.publish(ball::Record(fixed, userValues),
                          ball::Context(ball::Transmission::e_TRIGGER,
                                       n,
                                       NUM_MESSAGES));
            }
        }
      } break;
      default: {
        cerr << "WARNING: CASE `" << test << "' NOT FOUND." << endl;
        testStatus = -1;
      }
    }

    if (testStatus > 0) {
        cerr << "Error, non-zero test status = " << testStatus << "." << endl;
    }
    return testStatus;
}

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
