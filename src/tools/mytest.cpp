
#include "osrm/exception.hpp"
#include "osrm/mytest.hpp"
#include "osrm/mytest_config.hpp"
#include "util/log.hpp"
#include "util/version.hpp"

#include <tbb/task_scheduler_init.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <cstdlib>
#include <exception>
#include <new>

#include "util/meminfo.hpp"

using namespace osrm;

enum class return_code : unsigned
{
    ok,
    fail,
    exit
};


int main(int argc, char *argv[]) try
{
    util::LogPolicy::GetInstance().Unmute();
    mytester::MytestConfig extractor_config;
    std::string verbosity;


    util::Log(logINFO) << "argc " << argc << "argv" << argv[0];
    util::LogPolicy::GetInstance().SetLevel(verbosity);

    util::DumpSTXXLStats();
    util::DumpMemoryStats();



    return EXIT_SUCCESS;
}
catch (const osrm::RuntimeError &e)
{
    util::DumpSTXXLStats();
    util::DumpMemoryStats();
    util::Log(logERROR) << e.what();
    return e.GetCode();
}
catch (const std::system_error &e)
{
    util::DumpSTXXLStats();
    util::DumpMemoryStats();
    util::Log(logERROR) << e.what();
    return e.code().value();
}
catch (const std::bad_alloc &e)
{
    util::DumpSTXXLStats();
    util::DumpMemoryStats();
    util::Log(logERROR) << "[exception] " << e.what();
    util::Log(logERROR) << "Please provide more memory or consider using a larger swapfile";
    return EXIT_FAILURE;
}
#ifdef _WIN32
catch (const std::exception &e)
{
    util::Log(logERROR) << "[exception] " << e.what() << std::endl;
    return EXIT_FAILURE;
}
#endif
