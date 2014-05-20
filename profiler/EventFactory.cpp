#include <EventFactory.h>
#include <Profiler.h>
#include <ProcessState.h>
#include <Sample.h>

extern "C" {
#include <stdio.h>
#include <sys/queue.h>
#include <pmc.h>
#include <pmcstat.h>
#include <pmclog.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
}

extern void usage(void);

static bool debug = false;

// getting rid of compiler warning from profile.h
void
_mcount( uintfptr_t, uintfptr_t )
{
}

void ( *e )( uintfptr_t, uintfptr_t ) = _mcount;

void
EventFactory::createEvents(Profiler& profiler, uint32_t maxDepth)
{
    if ( debug )
    {
        printf( "datafile: %s\n", profiler.getDataFile().c_str() );
    }

    int fd = open( profiler.getDataFile().c_str(), O_RDONLY );

    if ( fd < 0 )
    {
        printf( "Could not open data file %s\n",
	    profiler.getDataFile().c_str());
	usage();
        return;
    }

    void* logCookie = pmclog_open( fd );

    if ( logCookie == 0 )
    {
        printf( "Could not open log file!\n" );
        close(fd);
        return;
    }

    pmclog_ev pmcEvent;

    while ( pmclog_read( logCookie, &pmcEvent ) == 0 )
    {
        assert( pmcEvent.pl_state == PMCLOG_OK );

        switch ( pmcEvent.pl_type )
        {
            case PMCLOG_TYPE_CLOSELOG:
                if ( debug )
                {
                    printf( "closelog\n" );
                }
                break;

            case PMCLOG_TYPE_DROPNOTIFY:
                if ( debug )
                {
                    printf( "drop\n" );
                }
                break;

            case PMCLOG_TYPE_INITIALIZE:
                if ( debug )
                {
                    printf( "initlog: 0x%x \"%s\"\n", pmcEvent.pl_u.pl_i.pl_version,
                        pmc_name_of_cputype( pmc_cputype( pmcEvent.pl_u.pl_i.pl_arch ) ) );
                }
                break;

            case PMCLOG_TYPE_MAPPINGCHANGE:
                /* this log type has been removed from pmc, so there's nothing to look at */
                break;

            case PMCLOG_TYPE_MAP_IN:
               if ( debug )
               {
                    printf( "mapping: insert %d %p \"%s\"\n",
                        pmcEvent.pl_u.pl_mi.pl_pid, (void *) pmcEvent.pl_u.pl_mi.pl_start,
                        pmcEvent.pl_u.pl_mi.pl_pathname );
                }
                profiler.processMapIn(pmcEvent.pl_u.pl_mi.pl_pid, pmcEvent.pl_u.pl_mi.pl_start,
                        pmcEvent.pl_u.pl_mi.pl_pathname);
                break;

            case PMCLOG_TYPE_MAP_OUT:
                if ( debug )
                {
                    printf( "mapping: delete %d %p %p \n",
                        pmcEvent.pl_u.pl_mo.pl_pid, (void *) pmcEvent.pl_u.pl_mo.pl_start,
                        (void *) pmcEvent.pl_u.pl_mo.pl_end  );
               }
               break;
            
            case PMCLOG_TYPE_PCSAMPLE:
                if ( debug )
                {
                    printf( "sample: 0x%x %d %p %s\n", pmcEvent.pl_u.pl_s.pl_pmcid,
                        pmcEvent.pl_u.pl_s.pl_pid, (void *) pmcEvent.pl_u.pl_s.pl_pc,
                        pmcEvent.pl_u.pl_s.pl_usermode ? "user" : "kernel" );
                }
                profiler.processEvent(Sample(pmcEvent.pl_u.pl_s));
                break;
            
            case PMCLOG_TYPE_CALLCHAIN:
                if ( debug )
                {
                    printf( "sample: 0x%x %d %p %s\n", pmcEvent.pl_u.pl_cc.pl_pid, pmcEvent.pl_u.pl_cc.pl_pid, (void*)pmcEvent.pl_u.pl_cc.pl_pc[0], PMC_CALLCHAIN_CPUFLAGS_TO_USERMODE(pmcEvent.pl_u.pl_cc.pl_cpuflags) ? "user" :"kernel" );
                }
                profiler.processEvent(Sample(pmcEvent.pl_u.pl_cc, maxDepth));
                break;

            case PMCLOG_TYPE_PMCALLOCATE:
                if ( debug )
                {
                    printf( "allocate: 0x%x \"%s\" 0x%x\n", pmcEvent.pl_u.pl_a.pl_pmcid,
                        pmcEvent.pl_u.pl_a.pl_evname, pmcEvent.pl_u.pl_a.pl_flags );
                }
                break;

            case PMCLOG_TYPE_PMCATTACH:
                printf( "attach: 0x%x %d \"%s\"\n", pmcEvent.pl_u.pl_t.pl_pmcid,
                    pmcEvent.pl_u.pl_t.pl_pid, pmcEvent.pl_u.pl_t.pl_pathname );
                break;

            case PMCLOG_TYPE_PMCDETACH:
                if ( debug )
                {
                    printf( "detach: 0x%x %d \n", pmcEvent.pl_u.pl_d.pl_pmcid,
                        pmcEvent.pl_u.pl_d.pl_pid );
                }
                break;

            case PMCLOG_TYPE_PROCCSW:
                if ( debug )
                {
                    printf( "cswval: 0x%x %d %jd\n", pmcEvent.pl_u.pl_c.pl_pmcid,
                        pmcEvent.pl_u.pl_c.pl_pid, pmcEvent.pl_u.pl_c.pl_value );
                }
                break;

            case PMCLOG_TYPE_PROCEXEC:
                if ( debug )
                {
                    printf( "exec: 0x%x %d %p \"%s\"\n", pmcEvent.pl_u.pl_x.pl_pmcid,
                        pmcEvent.pl_u.pl_x.pl_pid, (void *) pmcEvent.pl_u.pl_x.pl_entryaddr,
                        pmcEvent.pl_u.pl_x.pl_pathname );
                }
                ;
                profiler.processEvent( ProcessExec( pmcEvent.pl_u.pl_x.pl_pid,
                    std::string( pmcEvent.pl_u.pl_x.pl_pathname ) ) );
                break;

            case PMCLOG_TYPE_PROCEXIT:
                if ( debug )
                {
                    printf( "exitval: 0x%x %d %jd\n", pmcEvent.pl_u.pl_e.pl_pmcid,
                        pmcEvent.pl_u.pl_e.pl_pid, pmcEvent.pl_u.pl_e.pl_value );
                }
                break;

            case PMCLOG_TYPE_PROCFORK:
                if ( debug )
                {
                    printf( "fork %d %d\n", pmcEvent.pl_u.pl_f.pl_oldpid,
                        pmcEvent.pl_u.pl_f.pl_newpid );
                }
                break;

            case PMCLOG_TYPE_USERDATA:
                if ( debug )
                {
                    printf( "userdata 0x%x\n", pmcEvent.pl_u.pl_u.pl_userdata );
                }
                break;

            case PMCLOG_TYPE_SYSEXIT:
                if ( debug )
                {
                    printf( "exit %d\n", pmcEvent.pl_u.pl_se.pl_pid );
                }
                break;

            default:
                printf( "unknown pmc event type %d\n", pmcEvent.pl_type );

        }
    }

    pmclog_close( logCookie );
    close(fd);
}
