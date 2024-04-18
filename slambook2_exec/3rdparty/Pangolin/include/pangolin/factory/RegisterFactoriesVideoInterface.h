// CMake generated file. Do Not Edit.

#pragma once

namespace pangolin {

  // Forward declarations
  bool RegisterTestVideoFactory();
  bool RegisterImagesVideoFactory();
  bool RegisterSplitVideoFactory();
  bool RegisterTruncateVideoFactory();
  bool RegisterPangoVideoFactory();
  bool RegisterDebayerVideoFactory();
  bool RegisterShiftVideoFactory();
  bool RegisterTransformVideoFactory();
  bool RegisterUnpackVideoFactory();
  bool RegisterPackVideoFactory();
  bool RegisterJoinVideoFactory();
  bool RegisterMergeVideoFactory();
  bool RegisterJsonVideoFactory();
  bool RegisterMjpegVideoFactory();
  bool RegisterThreadVideoFactory();
  bool RegisterFirewireVideoFactory();
  bool RegisterDeinterlaceVideoFactory();
  bool RegisterV4lVideoFactory();


  inline bool RegisterFactoriesVideoInterface() {
    bool success = true;
    success &= RegisterTestVideoFactory();
    success &= RegisterImagesVideoFactory();
    success &= RegisterSplitVideoFactory();
    success &= RegisterTruncateVideoFactory();
    success &= RegisterPangoVideoFactory();
    success &= RegisterDebayerVideoFactory();
    success &= RegisterShiftVideoFactory();
    success &= RegisterTransformVideoFactory();
    success &= RegisterUnpackVideoFactory();
    success &= RegisterPackVideoFactory();
    success &= RegisterJoinVideoFactory();
    success &= RegisterMergeVideoFactory();
    success &= RegisterJsonVideoFactory();
    success &= RegisterMjpegVideoFactory();
    success &= RegisterThreadVideoFactory();
    success &= RegisterFirewireVideoFactory();
    success &= RegisterDeinterlaceVideoFactory();
    success &= RegisterV4lVideoFactory();
    return success;
  }


} // pangolin
