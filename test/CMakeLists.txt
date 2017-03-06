itk_module_test()

#${itk-module} will be the name of this module and will not need to be
#changed when this module is renamed.

set(${itk-module}Tests
  itkObjectnessMeasureImageFilterTest.cxx
  itkHessianImageFilterTest.cxx
  itkSLICImageFilterTest.cxx
  itkSLICImageFilterTest2.cxx
)


set(BASELINE_ROOT ${ITK_DATA_ROOT}/Baseline)
set(TEMP ${ITK_TEST_OUTPUT_DIR})

CreateTestDriver(${itk-module}  "${${itk-module}-Test_LIBRARIES}" "${${itk-module}Tests}")

itk_add_test(NAME itkObjectnessMeasureImageFilterTest1
  COMMAND ${itk-module}TestDriver
    --compareIntensityTolerance .001
    --compare
      DATA{Baseline/ObjectnessMeasureImageFilterTest1.nii}
      ${TEMP}/ObjectnessMeasureImageFilterTestOutput1.nii
    itkObjectnessMeasureImageFilterTest
      DATA{Input/DSA.png}
      ${TEMP}/ObjectnessMeasureImageFilterTestOutput1.nii
      1 0
    )

itk_add_test(NAME itkObjectnessMeasureImageFilterTest2
  COMMAND ${itk-module}TestDriver
    --compareIntensityTolerance .001
    --compare
      DATA{Baseline/ObjectnessMeasureImageFilterTest2.nii}
      ${TEMP}/ObjectnessMeasureImageFilterTestOutput2.nii
    itkObjectnessMeasureImageFilterTest
      DATA{Input/DSA.png}
      ${TEMP}/ObjectnessMeasureImageFilterTestOutput2.nii
      0 0
    )

add_test(NAME itkHessianImageFilterTest
      COMMAND ${itk-module}TestDriver itkHessianImageFilterTest )

itk_add_test(NAME itkSLICImageFilterTest_1
  COMMAND ${itk-module}TestDriver
   --with-threads 1
   --compare  DATA{Baseline/itkSLICImageFilterTest1Baseline.nii}
              ${TEMP}/itkSLICImageFilterTest1Output.nii
   itkSLICImageFilterTest
   DATA{Input/VM1111Shrink-LAB.mha}
   ${TEMP}/itkSLICImageFilterTest1Output.nii 25 )

itk_add_test(NAME itkSLICImageFilterTest_2
  COMMAND ${itk-module}TestDriver
   --with-threads "$<$<CONFIG:Debug>:10>$<$<NOT:$<CONFIG:Debug>>:80>"
   --compare  DATA{Baseline/itkSLICImageFilterTest1Baseline.nii}
              ${TEMP}/itkSLICImageFilterTest2Output.nii
   itkSLICImageFilterTest
   DATA{Input/VM1111Shrink-LAB.mha}
   ${TEMP}/itkSLICImageFilterTest2Output.nii 25 )

itk_add_test(NAME itkSLICImageFilterTest_3
  COMMAND ${itk-module}TestDriver
    --compare  DATA{Baseline/itkSLICImageFilterTest3Baseline.nii}
              ${TEMP}/itkSLICImageFilterTest3Output.nii
   itkSLICImageFilterTest
   DATA{Input/cthead1.png}
   ${TEMP}/itkSLICImageFilterTest3Output.nii 20 )


itk_add_test(NAME itkSLICImageFilterTest2
  COMMAND ${itk-module}TestDriver
   itkSLICImageFilterTest2 )

#
# GTest based tests
#

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../CMake")

option(USE_SYSTEM_GTEST "Use system installed gtest" ${ITK_USE_SYSTEM_LIBRARIES})
mark_as_advanced(USE_SYSTEM_GTEST)
if(NOT USE_SYSTEM_GTEST)
  include(External_GTest)
  include(UseGTest)
  link_directories("${GTEST_LIBRARIES_DIR}")
else()
  find_package( GTest )
endif()

# we always include CMake's GTest to get the gtest_add_tests method

set(${itk-module}GTests
  itkSliceImageFilterTest.cxx
  itkFunctorsTest.cxx
)

add_executable(${itk-module}GTestDriver ${${itk-module}GTests})

if(NOT USE_SYSTEM_GTEST)
  add_dependencies(${itk-module}GTestDriver GTest)
endif()
add_dependencies(${itk-module}-all ${itk-module}GTestDriver)

target_link_libraries(${itk-module}GTestDriver
  ${${itk-module}-Test_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  )

target_include_directories(${itk-module}GTestDriver PRIVATE
  ${GTEST_INCLUDE_DIRS})

gtest_add_tests(${itk-module}GTestDriver "" ${${itk-module}GTests})
