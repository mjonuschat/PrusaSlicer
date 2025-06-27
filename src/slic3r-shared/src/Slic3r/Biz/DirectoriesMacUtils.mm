#import "Slic3r/Biz/DirectoriesMacUtils.hpp"

#import <Foundation/Foundation.h>

#include <string>

namespace Slic3r::Biz {

std::string get_platform_data_dir()
{
    NSURL* url = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory
                                                 inDomain:NSUserDomainMask
                                                 appropriateForURL:nil create:NO error:nil];

    return std::string([url.path UTF8String]);
}
}