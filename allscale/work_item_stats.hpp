#ifndef ALLSCALE_WORK_ITEM_STATS_HPP
#define ALLSCALE_WORK_ITEM_STATS_HPP



namespace allscale
{
    struct work_item_stats 
    {
        double min, max, total;
        long long num_work_items;


#if HAVE_PAPI


#endif

        work_item_stats()
        {
	   min = 0; max = 0; 
 	   total = 0; num_work_items = 0;

        }


        double get_average() { return total/num_work_items; }
        double get_min() { return min; }
        double get_max() { return max; }
        double get_num_work_items() { return num_work_items; }

    };



}

#endif
