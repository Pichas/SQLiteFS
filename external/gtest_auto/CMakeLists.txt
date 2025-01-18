option(BENCHMARK_ENABLE_TESTING "Enable testing of the benchmark library." ON)
option(BENCHMARK_ENABLE_EXCEPTIONS "Enable the use of exceptions in the benchmark library." ON)
option(BENCHMARK_ENABLE_LTO "Enable link time optimisation of the benchmark library." OFF)
option(BENCHMARK_USE_LIBCXX "Build and test using libc++ as the standard library." OFF)
option(BENCHMARK_ENABLE_WERROR "Build Release candidates with -Werror." OFF)
option(BENCHMARK_FORCE_WERROR "Build Release candidates with -Werror regardless of compiler issues." OFF)
option(BENCHMARK_ENABLE_INSTALL "Enable installation of benchmark. (Projects embedding benchmark may want to turn this OFF.)" OFF)
option(BENCHMARK_ENABLE_DOXYGEN "Build documentation with Doxygen." OFF)
option(BENCHMARK_INSTALL_DOCS "Enable installation of documentation." OFF)
option(BENCHMARK_DOWNLOAD_DEPENDENCIES "Allow the downloading and in-tree building of unmet dependencies" ON)
option(BENCHMARK_ENABLE_GTEST_TESTS "Enable building the unit tests which depend on gtest" ON)
option(BENCHMARK_USE_BUNDLED_GTEST "Use bundled GoogleTest. If disabled, the find_package(GTest) will be used." ON)
option(BENCHMARK_ENABLE_LIBPFM "Enable performance counters provided by libpfm" OFF)


if (SQLITEFS_BUILD_TESTS)
    AddExternalPackage(
        benchmark
            v1.9.0
            https://github.com/google/benchmark.git
            NO
            fix_cmake_file.cmake.patch
    )
endif()
