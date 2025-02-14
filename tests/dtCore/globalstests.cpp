/* -*-c++-*-
* allTests - This source file (.h & .cpp) - Using 'The MIT License'
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/
#include <prefix/dtgameprefix-src.h>
#include <cppunit/extensions/HelperMacros.h>
#include <dtCore/globals.h>

namespace dtTest
{
   /// unit tests for dtCore::Axis
   class GlobalsTests : public CPPUNIT_NS::TestFixture
   {
      CPPUNIT_TEST_SUITE( GlobalsTests );
      CPPUNIT_TEST( TestEnvironment );
      CPPUNIT_TEST_SUITE_END();

      public:
         void setUp()
         {}
         void tearDown()
         {}

         /// tests handling and order of handling of multiple listeners for state changes.
         void TestEnvironment()
         {
            std::string result = dtCore::GetEnvironment("nothing");
            CPPUNIT_ASSERT_EQUAL_MESSAGE("An environment variable that doesn't exist should yield a result of ./.  "
                  "This is for historic reasons.", std::string("./"), result);
            dtCore::SetEnvironment("silly", "goose");
            result = dtCore::GetEnvironment("silly");
            CPPUNIT_ASSERT_EQUAL_MESSAGE("The environment variable \"silly\" should have the value \"goose\".", std::string("goose"), result);
         }
         
      private:
   };
}

CPPUNIT_TEST_SUITE_REGISTRATION( dtTest::GlobalsTests );
