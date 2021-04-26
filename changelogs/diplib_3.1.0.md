---
layout: post
title: "Changes DIPlib 3.1.0"
---

## Changes to DIPlib

### New functionality

- Added `dip::MarginalPercentile()` histogram-based statistics function.

- Added `dip::SplitRegions()`, a function to transform labeled images.

- Added `dip::FlushToZero()`, a function to remove denormal values from an image.

- Added `dip::maximum_gauss_truncation()`, returning the maximum truncation that is useful for Gaussian functions.

- `dip::NeighborList` allows accessing neighbors by index, through the new member functions `Coordinates`,
  `Distance` and `IsInImage`.

- Added `dip::Image::SwapBytesInSample()` to convert from little endian to big endian representation.

### Changed functionality

- Replaced the use of `std::map` and `std::set` in various functions to `tsl::robin_map` and
  `tsl::robin_set`, which are significantly faster. This affects `dip::Measurement` and functions
  using it (including `dip::MeasurementTool::Measure()`), `dip::GetImageChainCodes()`, `dip::GetObjectLabels()`,
  `dip::Relabel()`, `dip::ChordLength()` and `dip::PairCorrelation()`.

- Added `dip::Measurement::SetObjectIDs()`, improved speed of `dip::Measurement::AddObjectIDs()` and
  `dip::Measurement::operator+()`.

- `dip::Label` is slightly more efficient for 3D and higher-dimensional images; added tests.

- Sped up `dip::MomentAccumulator`.

- `dip::OptimalFourierTransformSize()` has a new option to return a smaller or equal size, rather than
  a larger or equal size, so we can crop an image for efficient FFT instead of padding.

- All the variants of the Gaussian filter (`dip::Gauss()` et al.) now limit the truncation value to avoid
  unnecessarily large filter kernels. `dip::DrawBandlimitedPoint()`, `dip::DrawBandlimitedBall()`,
  `dip::DrawBandlimitedBox()`, `dip::GaussianEdgeClip()` and `dip::GaussianLineClip()` also limit the
  truncation in the same way.

- `dip::ColorSpaceConverter` has a `SetWhitePoint` member function. `dip::ColorSpaceManager::SetWhitePoint`
  calls the `SetWhitePoint` member function for all registered converters. This allows custom converters
  to use the white point as well.

- `dip::ColorSpaceManager::XYZ` is deprecated, use `dip::XYZ` instead.

### Bug fixes

- `dip::DrawPolygon2D()` produced wrong results for filled polygons when vertices were very close together
  (distances smaller than a pixel).

- `dip::ColorSpaceManager` didn't register the ICH and ISH color spaces.




## Changes to DIPimage

### New functionality

- Added `dip_image/ftz`, a function to remove denormal values from an image.

### Changed functionality

### Bug fixes

- `readim` and `writeim`, when using MATLAB's functionality, would work in linear RGB space, instead of sRGB
  like the DIPlib functions do.

- `readics` tried to read the file as a TIFF file instead of an ICS file.




## Changes to PyDIP

### New functionality

- Added `dip.FlushToZero()`, `dip.SplitRegions()`.

### Changed functionality

### Bug fixes




## Changes to DIPviewer

### New functionality

### Changed functionality

### Bug fixes




## Changes to DIPjavaio

### New functionality

### Changed functionality

### Bug fixes

- Signed integer images could not be read.