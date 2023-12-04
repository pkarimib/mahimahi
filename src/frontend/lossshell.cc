/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>
#include <iostream>

#include <getopt.h>

#include "loss_queue.hh"
#include "util.hh"
#include "ezio.hh"
#include "packetshell.cc"

using namespace std;

void usage( const string & program_name )
{
    throw runtime_error(
        "Usage: " + program_name +
        " IID|bursty|GE uplink|downlink BAD_LOSS_RATE PROB_BAD_TO_GOOD PROB_GOOD_TO_BAD LOG_FILE GOOD_LOSS_RATE [COMMAND...]"
        );
}

int main( int argc, char *argv[] )
{
    try {
        const bool passthrough_until_signal = getenv( "MAHIMAHI_PASSTHROUGH_UNTIL_SIGNAL" );

        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 4 ) {
            usage( argv[ 0 ] );
        }

        const string loss_type = argv[1];
        int expected_args = 4;

        /* parse first rate (BAD_LOSS_RATE) as loss rate (either IID or in the bursty lossy state or good-state loss rate) */
        const double loss_rate = myatof( argv[ 3 ] );
        if ( (0 <= loss_rate) and (loss_rate <= 1) ) {
            /* do nothing */
        } else {
            cerr << "Error: IID/bursty/GE loss rate must be between 0 and 1." << endl;
            usage( argv[ 0 ] );
        }

        /* parse second probability as probability of leaving loss state
         * if it is bursty */
        double leave_bad_prob = 0;
        double leave_good_prob = 0;
        double good_loss_rate = 0;
        string logfile;

        if ( loss_type == "IID" ) {
            expected_args = 4;
        } else if (loss_type == "bursty") {
            expected_args = 7;
            if ( argc < expected_args ) {
                usage( argv[ 0 ] );
            }

            leave_bad_prob = myatof( argv[ 4 ] );
            if ( (0 <= leave_bad_prob) and (leave_bad_prob <= 1) ) {
                /* do nothing */
            } else {
                cerr << "Error: transition prob must be between 0 and 1." << endl;
                usage( argv[ 0 ] );
            }

            leave_good_prob = myatof( argv[ 5 ] );
            if ( (0 <= leave_good_prob) and (leave_good_prob <= 1) ) {
                /* do nothing */
            } else {
                cerr << "Error: transition prob must be between 0 and 1." << endl;
                usage( argv[ 0 ] );
            }

            logfile = argv[ 6 ];
        } else {
            /* parse forth rate as loss rate in the good state */
            expected_args = 8;
            if ( argc < expected_args ) {
                usage( argv[ 0 ] );
            }

            leave_bad_prob = myatof( argv[ 4 ] );
            if ( (0 <= leave_bad_prob) and (leave_bad_prob <= 1) ) {
                /* do nothing */
            } else {
                cerr << "Error: transition prob must be between 0 and 1." << endl;
                usage( argv[ 0 ] );
            }

            leave_good_prob = myatof( argv[ 5 ] );
            if ( (0 <= leave_good_prob) and (leave_good_prob <= 1) ) {
                /* do nothing */
            } else {
                cerr << "Error: transition prob must be between 0 and 1." << endl;
                usage( argv[ 0 ] );
            }

            logfile = argv[ 6 ];

            good_loss_rate = myatof( argv[ 7 ] );
            if ( (0 <= good_loss_rate) and (good_loss_rate <= 1) ) {
                /* do nothing */
            } else {
                cerr << "Error: loss prob must be between 0 and 1." << endl;
                usage( argv[ 0 ] );
            }

        }

        /* assign losses to the right direction of the link */
        string uplink_log, downlink_log;
        double uplink_loss = 0, downlink_loss = 0;
        double uplink_good_loss = 0, downlink_good_loss = 0;
        double uplink_leave_bad_prob = 0, downlink_leave_bad_prob = 0;
        double uplink_leave_good_prob = 0, downlink_leave_good_prob = 0;

        const string link = argv[ 2 ];
        if ( link == "uplink" ) {
            uplink_loss = loss_rate;
            uplink_leave_bad_prob = leave_bad_prob;
            uplink_leave_good_prob = leave_good_prob;
            uplink_log = logfile;
            uplink_good_loss = good_loss_rate;
        } else if ( link == "downlink" ) {
            downlink_loss = loss_rate;
            downlink_leave_bad_prob = leave_bad_prob;
            downlink_leave_good_prob = leave_good_prob;
            downlink_log = logfile;
            downlink_good_loss = good_loss_rate;
        } else {
            usage( argv[ 0 ] );
        }

        /* parse additional command specific options */
        vector<string> command;

        if ( argc == expected_args ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = expected_args; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        /* prefix for the shell - doesn't show up in zsh */

        string shell_prefix = "[loss ";
        if ( link == "uplink" ) {
            shell_prefix += "up=";
        } else {
            shell_prefix += "down=";
        }
        shell_prefix += argv[ 3 ];

        /* start IID or bursty loss packet shell */
        if ( loss_type == "IID" ) {
            shell_prefix += "] ";
            PacketShell<IIDLoss> loss_app( "loss", user_environment, passthrough_until_signal);

            loss_app.start_uplink( shell_prefix,
                                   command,
                                   uplink_loss );
            loss_app.start_downlink( downlink_loss );
            return loss_app.wait_for_exit();

        } else if ( loss_type == "bursty") {
            shell_prefix += " ";
            shell_prefix += argv[4];
            shell_prefix += " ";
            shell_prefix += argv[5];
            shell_prefix += " ";
            shell_prefix += argv[6];
            shell_prefix += "] ";

            PacketShell<BurstyLoss> loss_app( "loss", user_environment, passthrough_until_signal);

            loss_app.start_uplink( shell_prefix,
                                   command,
                                   uplink_loss,
                                   uplink_leave_bad_prob,
                                   uplink_leave_good_prob,
                                   uplink_log );
            loss_app.start_downlink( downlink_loss,
                                     downlink_leave_bad_prob,
                                     downlink_leave_good_prob,
                                     downlink_log );
            return loss_app.wait_for_exit();

        } else if ( loss_type == "GE") {
            shell_prefix += " ";
            shell_prefix += argv[4];
            shell_prefix += " ";
            shell_prefix += argv[5];
            shell_prefix += " ";
            shell_prefix += argv[6];
            shell_prefix += " ";
            shell_prefix += argv[7];
            shell_prefix += "] ";

            PacketShell<GELoss> loss_app( "loss", user_environment, passthrough_until_signal);

            loss_app.start_uplink( shell_prefix,
                                   command,
                                   uplink_loss,
                                   uplink_leave_bad_prob,
                                   uplink_leave_good_prob,
                                   uplink_log,
                                   uplink_good_loss );
            loss_app.start_downlink( downlink_loss,
                                     downlink_leave_bad_prob,
                                     downlink_leave_good_prob,
                                     downlink_log,
                                     downlink_good_loss );
            return loss_app.wait_for_exit();

        }

        else {
            usage( argv[ 0 ] );
        }
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}
