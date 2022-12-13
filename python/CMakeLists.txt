cmake_minimum_required(VERSION 2.8.8)

project(wiringX C)

set(PROJECT_VERSION 1.0)
set(PROJECT_NAME wiringX)

set(CMAKE_BUILD_TYPE Release)

find_package(Python COMPONENTS Interpreter Development)

set(CMAKE_SKIP_RULE_DEPENDENCY TRUE)

#Final compilation all platforms
#Removing debugging for final compilation
set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_EXE_LINKER_FLAGS " -Wl,-rpath=/usr/local/lib/,-rpath=/usr/lib/,-rpath=/lib/")
set(CMAKE_SHARED_LINKER_FLAGS " -Wl,-rpath=/usr/local/lib/,-rpath=/usr/lib/,-rpath=/lib/")
set(CMAKE_MODULE_LINKER_FLAGS " -Wl,-rpath=/usr/local/lib/,-rpath=/usr/lib/,-rpath=/lib/")

execute_process(COMMAND git describe --always
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	RESULT_VARIABLE git_result
	OUTPUT_VARIABLE git_ver)
	
# The printf is used to prevent bash naming errors e.g. libwiringx-107?-0963491.deb
execute_process(COMMAND git log --oneline
	COMMAND wc -l
	COMMAND xargs printf %d
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	RESULT_VARIABLE git_result
	OUTPUT_VARIABLE git_commits)	

STRING(REGEX REPLACE "\n" "" git_ver "${git_ver}")
add_definitions(-DHASH="${git_ver}")

include_directories(${PROJECT_SOURCE_DIR}/src/)

#py_ver: major.minor eg:python3.9
string(SUBSTRING ${Python_VERSION} 0 3 py_ver)
# message("--- py_ver is ${py_ver} ---")
#py_version: majorminor eg:39
string(JOIN "" py_version ${Python_VERSION_MAJOR} ${Python_VERSION_MINOR})
# message("py_version is ${py_version}")
set(desc_dir "/usr/local/lib/python${py_ver}/dist-packages/wiringX")

install(
	CODE "file(GLOB folder \"${CMAKE_BINARY_DIR}/lib.*\")"
	CODE "file(RENAME \${folder}/wiringX/gpio.cpython-${py_version}-aarch64-linux-gnu.so \${folder}/wiringX/gpio.so)"
	CODE "file(INSTALL \${folder}/wiringX/gpio.so DESTINATION ${desc_dir})"
	CODE "file(INSTALL \${folder}/wiringX/__init__.py DESTINATION ${desc_dir})"
)
install(FILES ${PROJECT_SOURCE_DIR}/PKG-INFO DESTINATION lib/python${py_ver}/dist-packages/ RENAME wiringX-1.0.egg-info)

execute_process(COMMAND python setup.py build
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	RESULT_VARIABLE python_results
	OUTPUT_VARIABLE python_output)
	
set(CPACK_GENERATOR "DEB RPM")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
set(CPACK_SOURCE_STRIP_FILES TRUE)
set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGE_CONTACT "CurlyMo <info@pilight.org>")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION}-${git_commits}-g${git_ver})
set(CPACK_PACKAGE_NAME "python-wiringX")
set(CPACK_PACKAGE_FILE_NAME python-wiringx-${git_commits}-g${git_ver})
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_PACKAGE_DESCRIPTION "Cross-platform GPIO Interface")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross-platform GPIO Interface")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${PROJECT_SOURCE_DIR}/res/deb/prerm;")

include(CPack)
