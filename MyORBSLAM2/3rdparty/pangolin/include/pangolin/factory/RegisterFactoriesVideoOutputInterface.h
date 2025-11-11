// CMake generated file. Do Not Edit.

#pragma once

namespace pangolin {

  // Forward declarations
  bool RegisterImagesVideoOutputFactory();
  bool RegisterPangoVideoOutputFactory();


  inline bool RegisterFactoriesVideoOutputInterface() {
    bool success = true;
    success &= RegisterImagesVideoOutputFactory();
    success &= RegisterPangoVideoOutputFactory();
    return success;
  }


} // pangolin
