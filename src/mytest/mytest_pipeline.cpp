#include "mytest/mytest_pipeline.hpp"


#include "guidance/files.hpp"
#include "guidance/guidance_processing.hpp"
#include "guidance/segregated_intersection_classification.hpp"
#include "guidance/turn_data_container.hpp"

#include "storage/io.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/range_table.hpp"
#include "util/timing_util.hpp"

#include "util/static_graph.hpp"
#include "util/static_rtree.hpp"

// Keep debug include to make sure the debug header is in sync with types.
#include "util/debug.hpp"


#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/iterator/function_input_iterator.hpp>
#include <boost/optional/optional.hpp>
#include <boost/scope_exit.hpp>

#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/osm/timestamp.hpp>
#include <osmium/thread/pool.hpp>
#include <osmium/visitor.hpp>

#include <tbb/pipeline.h>
#include <tbb/task_scheduler_init.h>

#include <cstdlib>

#include <algorithm>
#include <atomic>
#include <bitset>
#include <chrono>
#include <future>
#include <iostream>
#include <iterator>
#include <memory>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

using namespace std;
namespace osrm
{
namespace mytester
{
int Pipeline::run_pipeline()
{
    float a[] = {1,2,3,4,5,6,7,8,9,10};
    auto c = RootMeanSquare( &a[0],&a[10] );
    util::Log(logINFO) << "hahaha  " <<c ;
    return 0;
}

float Pipeline::RootMeanSquare( float* first, float* last )
{
    float sum=0;
    if( first > last )
        util::Log(logINFO) <<"error";
    tbb::parallel_pipeline( /*max_number_of_live_token=*/16,
        tbb::make_filter<void,float*>(
            tbb::filter::serial,
            [&](tbb::flow_control& fc)-> float*{
                if( first<last ) {
                    return first++;
                 } else {
                    fc.stop();
                    return NULL;
                }
            }
        ) &
        tbb::make_filter<float*,float>(
            tbb::filter::parallel,
            [](float* p){return (*p)*(*p);}
        ) &
        tbb::make_filter<float,void>(
            tbb::filter::serial,
            [&](float x) {sum+=x;}
        )
    );
    return sqrt(sum);
}

} // namespace mytest
} // namespace osrm
