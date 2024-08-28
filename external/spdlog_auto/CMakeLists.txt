if(TARGET spdlog::spdlog)
    return()
endif()

message(STATUS "external: downloading target 'spdlog::spdlog'")

include(FetchContent)
FetchContent_Declare(
  spdlog_git
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.14.1
)

FetchContent_MakeAvailable(spdlog_git)
