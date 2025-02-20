add_cmake_project(
    CGAL
    URL      https://github.com/CGAL/cgal/archive/refs/tags/v5.6.1.zip
    URL_HASH SHA256=a968cc77b9a2c6cbe5b1680ceee8d8cd8c5369aedb9daced9e5c90b4442dc574
)

include(GNUInstallDirs)

set(DEP_CGAL_DEPENDS Boost GMP MPFR)
