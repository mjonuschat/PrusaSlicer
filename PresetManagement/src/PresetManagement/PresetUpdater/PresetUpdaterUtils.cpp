#include "PresetUpdaterUtils.hpp"

#include "Version.hpp"
#include "../../Utils/Utils.hpp"
#include  "../../Utils/Format.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/system/error_code.hpp>
#include <boost/log/trivial.hpp>

namespace fs = boost::filesystem;

namespace PresetManagement {

void copy_file_fix(const fs::path &source, const fs::path &target)
{
	BOOST_LOG_TRIVIAL(debug) << Slic3r::GUI::format("PresetUpdater: Copying %1% -> %2%", source, target);
	std::string error_message;
	Slic3r::CopyFileResult cfr = Slic3r::copy_file(source.string(), target.string(), error_message, false);
	if (cfr != Slic3r::CopyFileResult::SUCCESS) {
		BOOST_LOG_TRIVIAL(error) << "Copying failed(" << cfr << "): " << error_message;
		throw Slic3r::CriticalException(Slic3r::GUI::format(
				"Copying of file %1% to %2% failed: %3%",
				source, target, error_message));
	}
	// Permissions should be copied from the source file by copy_file(). We are not sure about the source
	// permissions, let's rewrite them with 644.
	static constexpr const auto perms = fs::owner_read | fs::owner_write | fs::group_read | fs::others_read;
	fs::permissions(target, perms);
}

/// Load all idx from single directory
std::vector<Index> load_vendors_db(const fs::path& archive_path)
{
    std::vector<Index> index_db;
    std::string errors_cumulative;
    boost::system::error_code ec;
    if (!fs::exists(archive_path, ec) || ec) {
        throw Slic3r::RuntimeError(archive_path.string() + " does not exists. " + ec.message());
    }
    if (!fs::is_directory(archive_path, ec) || ec) {
        throw Slic3r::RuntimeError(archive_path.string() + " is not directory. " + ec.message());
    }
    ec.clear();
    for (auto &dir_entry : fs::directory_iterator(archive_path, ec)) {
        if (Slic3r::is_idx_file(dir_entry)) {
    	    Index idx;
            try {
        	    idx.load(dir_entry.path());
            } catch (const std::runtime_error &err) {
                errors_cumulative += err.what();
                errors_cumulative += "\n";
                continue;
    	    }
            if (std::find_if(index_db.begin(), index_db.end(), [idx](const Index& index) { return idx.vendor() == index.vendor();}) == index_db.end())
                index_db.emplace_back(std::move(idx));
        }
    }
    if (! errors_cumulative.empty())
        throw Slic3r::RuntimeError(errors_cumulative);
    return index_db;
}

std::vector<Index> load_vendors_db_filtered(const boost::filesystem::path& from_path, const std::vector<std::string>& filter)
{
    // We want only indicies of vendors from filter vector
    // Rest of the idx files are vendors of different archive
    std::vector<Index> index_db;
    std::string errors_cumulative;
    boost::system::error_code ec;
    if (!fs::exists(from_path, ec) || ec) {
        throw Slic3r::RuntimeError(from_path.string() + " does not exists. " + ec.message());
    }
    if (!fs::is_directory(from_path, ec) || ec) {
        throw Slic3r::RuntimeError(from_path.string() + " is not directory. " + ec.message());
    }

    fs::directory_iterator source_directory_iterator(from_path, ec);
    // Config::Index work with dir entries, so we iterate every time
    for (auto &dir_entry : source_directory_iterator) {
        if (Slic3r::is_idx_file(dir_entry)) {
            if (std::find(filter.begin(), filter.end(), dir_entry.path().filename().string()) == filter.end()) {
                // not an index from this archive
                continue;
            }
            Index idx;
            try {
        	    idx.load(dir_entry.path());
            } catch (const std::runtime_error &err) {
                errors_cumulative += err.what();
                errors_cumulative += "\n";
                continue;
            }
            if (std::find_if(index_db.begin(), index_db.end(), [idx](const Index& index) { return idx.vendor() == index.vendor();}) == index_db.end()) {
                index_db.emplace_back(std::move(idx));
            } else {
                assert(false);
            }
        }
    }
    //assert (index_db.size() == filter.size());
    if (! errors_cumulative.empty())
        throw Slic3r::RuntimeError(errors_cumulative);
    return index_db;
}

} // PresetManagement