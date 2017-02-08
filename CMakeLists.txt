cmake_minimum_required(VERSION 2.8.9)

if(POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)
endif()

project(SimpleITKFiltersModule)

if(NOT ITK_SOURCE_DIR)
  find_package(ITK REQUIRED)
  list(APPEND CMAKE_MODULE_PATH ${ITK_CMAKE_DIR})
  include(ITKModuleExternal)
else()
  itk_module_impl()
endif()
