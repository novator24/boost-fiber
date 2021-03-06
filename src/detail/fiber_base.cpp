
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/detail/fiber_base.hpp"

#include <boost/exception_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/locks.hpp>

#include "boost/fiber/detail/scheduler.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

void
fiber_base::set_terminated_() BOOST_NOEXCEPT
{
    state_t previous = TERMINATED;
    std::swap( state_, previous);
    BOOST_ASSERT( RUNNING == previous);
}

void
fiber_base::trampoline_( coro::coroutine< void >::push_type & c)
{
    BOOST_ASSERT( c);
    BOOST_ASSERT( ! is_terminated() );

    callee_ = & c;
    set_running();
    suspend();

    try
    {
        BOOST_ASSERT( is_running() );
        run();
        BOOST_ASSERT( is_running() );
    }
    catch ( coro::detail::forced_unwind const&)
    {
        set_terminated_();
        release();
        throw;
    }
    catch (...)
    { except_ = current_exception(); }

    set_terminated_();
    release();
    suspend();

    BOOST_ASSERT_MSG( false, "fiber already terminated");
}

fiber_base::~fiber_base()
{
    BOOST_ASSERT( is_terminated() );
    BOOST_ASSERT( waiting_.empty() );
}

void
fiber_base::resume()
{
    BOOST_ASSERT( caller_);
    BOOST_ASSERT( is_running() ); // set by the scheduler-algorithm

    caller_();
    if ( has_exception() ) rethrow();
}

void
fiber_base::suspend()
{
    BOOST_ASSERT( callee_);
    BOOST_ASSERT( * callee_);

    ( * callee_)();

    BOOST_ASSERT( is_running() ); // set by the scheduler-algorithm
}

void
fiber_base::release()
{
    BOOST_ASSERT( is_terminated() );

    std::vector< ptr_t > waiting;

    // get all waiting fibers
    waiting.swap( waiting_);

    // notify all waiting fibers
    BOOST_FOREACH( fiber_base::ptr_t p, waiting)
    { p->set_ready(); }

    // release all fiber-specific-pointers
    BOOST_FOREACH( fss_data_t::value_type & data, fss_data_)
    { data.second.do_cleanup(); }
}

bool
fiber_base::join( ptr_t const& p)
{
    // protect against concurrent access to waiting_
    if ( is_terminated() ) return false;
    waiting_.push_back( p);
    return true;
}

void
fiber_base::interruption_blocked( bool blck) BOOST_NOEXCEPT
{
    if ( blck)
        flags_ |= flag_interruption_blocked;
    else
        flags_ &= ~flag_interruption_blocked;
}

void
fiber_base::request_interruption( bool req) BOOST_NOEXCEPT
{
    if ( req)
        flags_ |= flag_interruption_requested;
    else
        flags_ &= ~flag_interruption_requested;
}

void
fiber_base::set_ready() BOOST_NOEXCEPT
{
    state_t previous = READY;
    std::swap( state_, previous);
    BOOST_ASSERT( WAITING == previous || RUNNING == previous || READY == previous);
}

void
fiber_base::set_running() BOOST_NOEXCEPT
{
    state_t previous = RUNNING;
    std::swap( state_, previous);
    BOOST_ASSERT( READY == previous);
}

void
fiber_base::set_waiting() BOOST_NOEXCEPT
{
    state_t previous = WAITING;
    std::swap( state_, previous);
    BOOST_ASSERT( RUNNING == previous);
}

void *
fiber_base::get_fss_data( void const* vp) const
{
    uintptr_t key( reinterpret_cast< uintptr_t >( vp) );
    fss_data_t::const_iterator i( fss_data_.find( key) );

    return fss_data_.end() != i ? i->second.vp : 0;
}

void
fiber_base::set_fss_data(
    void const* vp,
    fss_cleanup_function::ptr_t const& cleanup_fn,
    void * data, bool cleanup_existing)
{
    BOOST_ASSERT( cleanup_fn);

    uintptr_t key( reinterpret_cast< uintptr_t >( vp) );
    fss_data_t::iterator i( fss_data_.find( key) );

    if ( fss_data_.end() != i)
    {
        if( cleanup_existing) i->second.do_cleanup();
        if ( data)
            fss_data_.insert(
                    i,
                    std::make_pair(
                        key,
                        fss_data( data, cleanup_fn) ) );
        else fss_data_.erase( i);
    }
    else
        fss_data_.insert(
            std::make_pair(
                key,
                fss_data( data, cleanup_fn) ) );
}

void
fiber_base::rethrow() const
{
    BOOST_ASSERT( has_exception() );

    rethrow_exception( except_);
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
