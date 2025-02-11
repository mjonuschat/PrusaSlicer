#pragma once

#include <string_view>
#include <boost/filesystem.hpp>

namespace Tests {
    inline boost::filesystem::path get_datadir() {
        namespace fs = boost::filesystem;
        return fs::absolute(fs::canonical(fs::path{TEST_DATA_DIR}));
    }
}
