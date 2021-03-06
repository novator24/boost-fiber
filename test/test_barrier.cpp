
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <string>

#include <boost/bind.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/all.hpp>

int value1 = 0;
int value2 = 0;

void fn1( boost::fibers::barrier & b)
{
    ++value1;
    boost::this_fiber::yield();

    b.wait();

    ++value1;
    boost::this_fiber::yield();
    ++value1;
    boost::this_fiber::yield();
    ++value1;
    boost::this_fiber::yield();
    ++value1;
}

void fn2( boost::fibers::barrier & b)
{
    ++value2;
    boost::this_fiber::yield();
    ++value2;
    boost::this_fiber::yield();
    ++value2;
    boost::this_fiber::yield();

    b.wait();

    ++value2;
    boost::this_fiber::yield();
    ++value2;
}

void test_barrier()
{
    boost::fibers::round_robin ds;
    boost::fibers::set_scheduling_algorithm( & ds);

    value1 = 0;
    value2 = 0;

    boost::fibers::barrier b( 2);
    boost::fibers::fiber s1(
            boost::bind(
                fn1, boost::ref( b) ) );
    BOOST_CHECK( s1);
    BOOST_CHECK_EQUAL( 1, value1);

    boost::fibers::fiber s2(
            boost::bind(
                fn2, boost::ref( b) ) );
    BOOST_CHECK( s2);
    BOOST_CHECK_EQUAL( 1, value2);

    s1.join();
    s2.join();

    BOOST_CHECK_EQUAL( 5, value1);
    BOOST_CHECK_EQUAL( 5, value2);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: barrier test suite");

    test->add( BOOST_TEST_CASE( & test_barrier) );

    return test;
}
