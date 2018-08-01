#ifndef ALLSCALE_PROFILE_HPP
#define ALLSCALE_PROFILE_HPP


#include <chrono>
#include <math.h>
#include <vector>
#include <string>

#ifdef HAVE_PAPI
#include <string.h>
#endif


namespace allscale
{
    struct profile
    {
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;
        std::chrono::steady_clock::time_point result_ready;
#ifdef HAVE_PAPI
        long long papi_counters_start[4];
        long long papi_counters_stop[4];
        std::uint32_t num_counters;
#endif

        std::string work_item_id_;
        std::string parent_work_item_id_;
        std::string work_item_name_;

        // Data to compute mean and stdev for all children
        // see http://www.johndcook.com/standard_deviation.html
        // or Donald Knuth's Art of Computer Programming, Vol 2, page 232, 3rd edition
//        unsigned int m_n;
//        double m_oldM, m_newM, m_oldS, m_newS;


        profile(std::string id, std::string name, std::string parent_id):start(std::chrono::steady_clock::now())
        {

           work_item_id_ = id;
           work_item_name_ = name;
           parent_work_item_id_ = parent_id;

//           m_n = 0;
//           m_oldM = m_newM = m_oldS = m_newS = 0.0;
#ifdef HAVE_PAPI
           memset(papi_counters_start, 0, sizeof(long long) * 4);
           memset(papi_counters_stop, 0, sizeof(long long) * 4);
#endif
        }


        std::string get_wid() { return work_item_id_; }
        std::string get_parent_wid() { return parent_work_item_id_; }
        std::string get_wname() { return work_item_name_; }

/*
	void push(double x)
	{
	   m_n++;

	   // See Knuth TAOCP vol 2, 3rd edition, page 232
	   if (m_n == 1)
	   {
		m_oldM = x;
		m_oldS = 0.0;
	   }
	   else
	   {
		m_newM = m_oldM + (x - m_oldM)/m_n;
		m_newS = m_oldS + (x - m_oldM)*(x - m_newM);

		// set up for next iteration
		m_oldM = m_newM;
		m_oldS = m_newS;
	   }
	}

	double Mean() const
	{
	   return (m_n > 0) ? m_newM : 0.0;
	}

	double Variance() const
	{
	   return ( (m_n > 1) ? m_newS/(m_n - 1) : 0.0 );
	}

	double StandardDeviation() const
	{
	   return sqrt( Variance() );
	}
*/

        // Other performance metrics
        double get_exclusive_time() {

           std::chrono::duration<double> time_elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
           return time_elapsed.count();

        }


        uint64_t get_exclusive_time_ns() {

           uint64_t time_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
           return time_elapsed;

        }

        double get_inclusive_time() {

           std::chrono::duration<double> time_elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(result_ready - start);
           return time_elapsed.count();

        }

#ifdef HAVE_PAPI
        long long *get_counters() {

	   long long *counter_values = (long long *)malloc(sizeof(long long)*4);
           for(int i = 0; i < 4; i++)
		counter_values[i] = papi_counters_stop[i] - papi_counters_start[i];

           return counter_values;
        }
#endif
    };



}

#endif
