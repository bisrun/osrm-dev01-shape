#include "osrm/mytest.hpp"
#include "mytest/mytest.hpp"
#include "mytest/mytest_config.hpp"
#include "mytest/mytest_pipeline.hpp"


namespace osrm
{

// Pimpl-like facade

void mytest(const mytester::MytestConfig &config)
{
    mytester::Mytest(config).run();
}

} // ns osrm
