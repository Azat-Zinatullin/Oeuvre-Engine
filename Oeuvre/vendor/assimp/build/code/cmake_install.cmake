# Install script for directory: E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Assimp")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "libassimp5.4.3-dev" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/build/lib/Debug/assimp-vc143-mtd.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/build/lib/Release/assimp-vc143-mt.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/build/lib/MinSizeRel/assimp-vc143-mt.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/build/lib/RelWithDebInfo/assimp-vc143-mt.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "libassimp5.4.3" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/build/bin/Debug/assimp-vc143-mtd.dll")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/build/bin/Release/assimp-vc143-mt.dll")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/build/bin/MinSizeRel/assimp-vc143-mt.dll")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/build/bin/RelWithDebInfo/assimp-vc143-mt.dll")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "assimp-dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/assimp" TYPE FILE FILES
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/anim.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/aabb.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/ai_assert.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/camera.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/color4.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/color4.inl"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/build/code/../include/assimp/config.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/ColladaMetaData.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/commonMetaData.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/defs.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/cfileio.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/light.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/material.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/material.inl"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/matrix3x3.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/matrix3x3.inl"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/matrix4x4.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/matrix4x4.inl"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/mesh.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/ObjMaterial.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/pbrmaterial.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/GltfMaterial.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/postprocess.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/quaternion.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/quaternion.inl"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/scene.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/metadata.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/texture.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/types.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/vector2.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/vector2.inl"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/vector3.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/vector3.inl"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/version.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/cimport.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/AssertHandler.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/importerdesc.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/Importer.hpp"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/DefaultLogger.hpp"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/ProgressHandler.hpp"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/IOStream.hpp"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/IOSystem.hpp"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/Logger.hpp"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/LogStream.hpp"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/NullLogger.hpp"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/cexport.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/Exporter.hpp"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/DefaultIOStream.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/DefaultIOSystem.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/ZipArchiveIOSystem.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/SceneCombiner.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/fast_atof.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/qnan.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/BaseImporter.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/Hash.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/MemoryIOWrapper.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/ParsingUtils.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/StreamReader.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/StreamWriter.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/StringComparison.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/StringUtils.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/SGSpatialSort.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/GenericProperty.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/SpatialSort.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/SkeletonMeshBuilder.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/SmallVector.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/SmoothingGroups.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/SmoothingGroups.inl"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/StandardShapes.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/RemoveComments.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/Subdivision.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/Vertex.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/LineSplitter.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/TinyFormatter.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/Profiler.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/LogAux.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/Bitmap.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/XMLTools.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/IOStreamBuffer.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/CreateAnimMesh.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/XmlParser.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/BlobIOSystem.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/MathFunctions.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/Exceptional.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/ByteSwapper.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/Base64.hpp"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "assimp-dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/assimp/Compiler" TYPE FILE FILES
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/Compiler/pushpack1.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/Compiler/poppack1.h"
    "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/code/../include/assimp/Compiler/pstdint.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/build/bin/Debug/assimp-vc143-mtd.pdb")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/build/bin/Release/assimp-vc143-mt.pdb")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/build/bin/MinSizeRel/assimp-vc143-mt.pdb")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "E:/Development/Projects/C_C++/Oeuvre Engine/Oeuvre/vendor/assimp/build/bin/RelWithDebInfo/assimp-vc143-mt.pdb")
  endif()
endif()

