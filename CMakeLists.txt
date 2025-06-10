project(tcparticle)

cmake_minimum_required(VERSION 3.5)

find_package(Catch2 3 REQUIRED)

add_executable(tcparticle 
    tcparticle.cpp 
    mocks.cpp
)
set_property(TARGET tcparticle PROPERTY CXX_STANDARD 20)
target_link_libraries(tcparticle PRIVATE Catch2::Catch2WithMain)
