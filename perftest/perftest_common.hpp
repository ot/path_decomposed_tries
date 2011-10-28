#pragma once

#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace detail {

    struct timer {
	timer(const std::string msg) 
	    : m_msg(msg)
	    , m_tick(boost::posix_time::microsec_clock::universal_time())
	    , m_done(false)
	{}
    
	bool done() { return m_done; } 
    
	void report(size_t n) {
	    double elapsed_usec = static_cast<double>((boost::posix_time::microsec_clock::universal_time() - m_tick).total_microseconds());
	    std::cerr << m_msg << " elapsed=" << elapsed_usec / 1000 << "ms";
	    if (n > 1) {
		std::cerr << " n=" << n << " single=" << elapsed_usec / n << "us";
	    }
	    std::cerr << std::endl;
	    m_done = true;
	}
    
	const std::string m_msg;
	boost::posix_time::ptime m_tick;
	bool m_done;
    };
}

#define TIMEIT(msg, n) for (detail::timer TIMEIT_timer(msg); !TIMEIT_timer.done(); TIMEIT_timer.report(n))
