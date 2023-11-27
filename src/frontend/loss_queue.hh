/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef LOSS_QUEUE_HH
#define LOSS_QUEUE_HH

#include <queue>
#include <cstdint>
#include <string>
#include <random>
#include <iostream>

#include "file_descriptor.hh"

class LossQueue
{
private:
    std::queue<std::string> packet_queue_ {};

    virtual bool drop_packet( const std::string & packet ) = 0;

protected:
    std::default_random_engine prng_;

public:
    LossQueue();
    virtual ~LossQueue() {}

    void read_packet( const std::string & contents );

    void write_packets( FileDescriptor & fd );

    unsigned int wait_time( void );

    bool pending_output( void ) const { return not packet_queue_.empty(); }

    static bool finished( void ) { return false; }
};

class IIDLoss : public LossQueue
{
private:
    std::bernoulli_distribution drop_dist_;

    bool drop_packet( const std::string & packet ) override;

public:
    IIDLoss( const double loss_rate ) : drop_dist_( loss_rate ) {}
};

class BurstyLoss : public LossQueue
{
private:
    bool in_loss_state_;

    std::bernoulli_distribution leave_loss_dist_;
    std::bernoulli_distribution leave_no_loss_dist_;
    std::bernoulli_distribution drop_dist_;

    bool drop_packet( const std::string & packet ) override;

public:
    BurstyLoss( const double loss_rate, const double prob_leave_loss, const double prob_leave_no_loss) :
        in_loss_state_( false ),
        leave_loss_dist_( prob_leave_loss ),
        leave_no_loss_dist_( prob_leave_no_loss ),
        drop_dist_( loss_rate ) {
            std::cerr << "bursty loss link P(leave loss) " << prob_leave_loss
                << " P(leave no loss) " << prob_leave_no_loss
                << " loss rate " << loss_rate << std::endl; }
};

class GELoss : public LossQueue
{
private:
    bool in_bad_state_;

    std::bernoulli_distribution leave_good_dist_;
    std::bernoulli_distribution leave_bad_dist_;
    std::bernoulli_distribution drop_good_dist_;
    std::bernoulli_distribution drop_bad_dist_;

    bool drop_packet( const std::string & packet ) override;

public:
    GELoss( const double bad_loss_rate, const double prob_leave_bad,
            const double prob_leave_good, const double good_loss_rate) :
        in_bad_state_( false ),
        leave_good_dist_( prob_leave_good ),
        leave_bad_dist_( prob_leave_bad ),
        drop_good_dist_( good_loss_rate ),
        drop_bad_dist_( bad_loss_rate ) {
            std::cerr << "GE loss link P(leave good) " << prob_leave_good
                << " P(leave bad) " << prob_leave_bad
                << " good loss rate " << good_loss_rate
                << " bad loss rate " << bad_loss_rate << std::endl; }
};

class StochasticSwitchingLink : public LossQueue
{
private:
    bool link_is_on_;
    std::exponential_distribution<> on_process_;
    std::exponential_distribution<> off_process_;

    uint64_t next_switch_time_;

    bool drop_packet( const std::string & packet ) override;

public:
    StochasticSwitchingLink( const double mean_on_time_, const double mean_off_time );

    unsigned int wait_time( void );
};

class PeriodicSwitchingLink : public LossQueue
{
private:
    bool link_is_on_;
    uint64_t on_time_, off_time_, next_switch_time_;

    bool drop_packet( const std::string & packet ) override;

public:
    PeriodicSwitchingLink( const double on_time, const double off_time );

    unsigned int wait_time( void );
};

#endif /* LOSS_QUEUE_HH */
