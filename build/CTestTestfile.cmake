# CMake generated Testfile for 
# Source directory: C:/Users/Carlos/source/repos/Contabilidad en C
# Build directory: C:/Users/Carlos/source/repos/Contabilidad en C/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[formula_tests]=] "C:/Users/Carlos/source/repos/Contabilidad en C/build/Debug/formula_tests.exe")
  set_tests_properties([=[formula_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Carlos/source/repos/Contabilidad en C/CMakeLists.txt;81;add_test;C:/Users/Carlos/source/repos/Contabilidad en C/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[formula_tests]=] "C:/Users/Carlos/source/repos/Contabilidad en C/build/Release/formula_tests.exe")
  set_tests_properties([=[formula_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Carlos/source/repos/Contabilidad en C/CMakeLists.txt;81;add_test;C:/Users/Carlos/source/repos/Contabilidad en C/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[formula_tests]=] "C:/Users/Carlos/source/repos/Contabilidad en C/build/MinSizeRel/formula_tests.exe")
  set_tests_properties([=[formula_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Carlos/source/repos/Contabilidad en C/CMakeLists.txt;81;add_test;C:/Users/Carlos/source/repos/Contabilidad en C/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[formula_tests]=] "C:/Users/Carlos/source/repos/Contabilidad en C/build/RelWithDebInfo/formula_tests.exe")
  set_tests_properties([=[formula_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Carlos/source/repos/Contabilidad en C/CMakeLists.txt;81;add_test;C:/Users/Carlos/source/repos/Contabilidad en C/CMakeLists.txt;0;")
else()
  add_test([=[formula_tests]=] NOT_AVAILABLE)
endif()
