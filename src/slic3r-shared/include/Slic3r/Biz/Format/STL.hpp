#pragma once

#include <string>

namespace Slic3r::Domain {
    class TriangleMesh;
}

namespace Slic3r::Biz {
	
    bool load_stl(const std::string& path, Domain::TriangleMesh& mesh);
    bool store_stl(const std::string& path, const Domain::TriangleMesh& mesh, bool binary, const std::string& label = "");

}
