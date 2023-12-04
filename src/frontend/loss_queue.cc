/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>
#include <stdexcept>

#include "loss_queue.hh"
#include "timestamp.hh"

using namespace std;

LossQueue::LossQueue()
    : prng_( random_device()() )
{}

void LossQueue::read_packet( const string & contents )
{
    /* now shows the relative time to the beginning of the experiment*/
    const uint64_t now = timestamp();
    if ( not drop_packet(now, contents ) ) {
        packet_queue_.emplace( contents );
    }
}

void LossQueue::write_packets( FileDescriptor & fd )
{
    while ( not packet_queue_.empty() ) {
        fd.write( packet_queue_.front() );
        packet_queue_.pop();
    }
}

unsigned int LossQueue::wait_time( void )
{
    return packet_queue_.empty() ? numeric_limits<uint16_t>::max() : 0;
}

bool IIDLoss::drop_packet(const uint64_t time, const string & packet __attribute((unused)) )
{
    return drop_dist_( prng_ );
}


bool BurstyLoss::drop_packet(const uint64_t time, const string & packet __attribute((unused)) )
{
    if ( in_loss_state_ ) {
        in_loss_state_ = not ( leave_loss_dist_( prng_ ) );
    } else {
        in_loss_state_ = leave_no_loss_dist_( prng_ );
    }

    /* log state */
    if ( log_ ) {
        *log_ << time << " : " << in_loss_state_ << endl;
    }

    if ( in_loss_state_ ) {
        return drop_dist_( prng_ );
    } else {
        return false;
    }
}

bool GELoss::drop_packet( const uint64_t time, const string & packet __attribute((unused)) )
{
    if ( in_bad_state_ ) {
        in_bad_state_ = not ( leave_bad_dist_( prng_ ) );
    } else {
        in_bad_state_ = leave_good_dist_( prng_ );
    }
    /* log state */
    if ( log_ ) {
        *log_ << time << " : " << in_bad_state_ << endl;
    }

    if ( in_bad_state_ ) {
        return drop_bad_dist_( prng_ );
    } else {
        return drop_good_dist_( prng_ );
    }
}

static const double MS_PER_SECOND = 1000.0;

StochasticSwitchingLink::StochasticSwitchingLink( const double mean_on_time, const double mean_off_time )
    : link_is_on_( false ),
      on_process_( 1.0 / (MS_PER_SECOND * mean_off_time) ),
      off_process_( 1.0 / (MS_PER_SECOND * mean_on_time) ),
      next_switch_time_( timestamp() )
{}

uint64_t bound( const double x )
{
    if ( x > (1 << 30) ) {
        return 1 << 30;
    }

    return x;
}

unsigned int StochasticSwitchingLink::wait_time( void )
{
    const uint64_t now = timestamp();

    while ( next_switch_time_ <= now ) {
        /* switch */
        link_is_on_ = !link_is_on_;
        /* worried about integer overflow when mean time = 0 */
        next_switch_time_ += bound( (link_is_on_ ? off_process_ : on_process_)( prng_ ) );
    }

    if ( LossQueue::wait_time() == 0 ) {
        return 0;
    }

    if ( next_switch_time_ - now > numeric_limits<uint16_t>::max() ) {
        return numeric_limits<uint16_t>::max();
    }

    return next_switch_time_ - now;
}

bool StochasticSwitchingLink::drop_packet(const uint64_t time, const string & packet __attribute((unused)) )
{
    return !link_is_on_;
}

PeriodicSwitchingLink::PeriodicSwitchingLink( const double on_time, const double off_time )
    : link_is_on_( false ),
      on_time_( bound( MS_PER_SECOND * on_time ) ),
      off_time_( bound( MS_PER_SECOND * off_time ) ),
      next_switch_time_( timestamp() )
{
  if ( on_time_ == 0 and off_time_ == 0 ) {
      throw runtime_error( "on_time and off_time cannot both be zero" );
  }
}

unsigned int PeriodicSwitchingLink::wait_time( void )
{
    const uint64_t now = timestamp();

    while ( next_switch_time_ <= now ) {
        /* switch */
        link_is_on_ = !link_is_on_;
        next_switch_time_ += link_is_on_ ? on_time_ : off_time_;
    }

    if ( LossQueue::wait_time() == 0 ) {
        return 0;
    }

    if ( next_switch_time_ - now > numeric_limits<uint16_t>::max() ) {
        return numeric_limits<uint16_t>::max();
    }

    return next_switch_time_ - now;
}

bool PeriodicSwitchingLink::drop_packet(const uint64_t time, const string & packet __attribute((unused)) )
{
    return !link_is_on_;
}
