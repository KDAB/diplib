/*
 * PyDIP 3.0, Python bindings for DIPlib 3.0
 *
 * (c)2017, Flagship Biosciences, Inc., written by Cris Luengo.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pydip.h"
#include "diplib/generic_iterators.h"

namespace {

dip::Image BufferToImage( py::buffer& buf ) {
   py::buffer_info info = buf.request();
   //std::cout << "--Constructing dip::Image from Python buffer.\n";
   //std::cout << "   info.ptr = " << info.ptr << std::endl;
   //std::cout << "   info.format = " << info.format << std::endl;
   //std::cout << "   info.ndims = " << info.shape.size() << std::endl;
   //std::cout << "   info.size = " << info.size << std::endl;
   //std::cout << "   info.itemsize = " << info.itemsize << std::endl;
   //std::cout << "   info.shape[0] = " << info.shape[0] << std::endl;
   //std::cout << "   info.shape[1] = " << info.shape[1] << std::endl;
   //std::cout << "   info.strides[0] = " << info.strides[0] << std::endl;
   //std::cout << "   info.strides[1] = " << info.strides[1] << std::endl;
   // Data type
   dip::DataType datatype;
   switch( info.format[ 0 ] ) {
      case py::format_descriptor< bool >::c:
         datatype = dip::DT_BIN;
         break;
      case py::format_descriptor< dip::uint8 >::c:
         datatype = dip::DT_UINT8;
         break;
      case py::format_descriptor< dip::uint16 >::c:
         datatype = dip::DT_UINT16;
         break;
      case py::format_descriptor< dip::uint32 >::c:
         datatype = dip::DT_UINT32;
         break;
      case py::format_descriptor< dip::sint8 >::c:
         datatype = dip::DT_SINT8;
         break;
      case py::format_descriptor< dip::sint16 >::c:
         datatype = dip::DT_SINT16;
         break;
      case py::format_descriptor< dip::sint32 >::c:
         datatype = dip::DT_SINT32;
         break;
      case py::format_descriptor< dip::sfloat >::c:
         datatype = dip::DT_SFLOAT;
         break;
      case py::format_descriptor< dip::dfloat >::c:
         datatype = dip::DT_DFLOAT;
         break;
      case py::format_descriptor< dip::scomplex >::c:
         datatype = dip::DT_SCOMPLEX;
         break;
      case py::format_descriptor< dip::dcomplex >::c:
         datatype = dip::DT_DCOMPLEX;
         break;
      default:
         DIP_THROW( "Image data is not numeric" );
   }
   // Sizes
   dip::uint ndim = static_cast< dip::uint >( info.ndim );
   DIP_ASSERT( ndim == info.shape.size());
   dip::UnsignedArray sizes( ndim, 1 );
   for( dip::uint ii = 0; ii < ndim; ++ii ) {
      sizes[ ii ] = static_cast< dip::uint >( info.shape[ ii ] );
   }
   // Strides
   dip::IntegerArray strides( ndim, 1 );
   for( dip::uint ii = 0; ii < ndim; ++ii ) {
      dip::sint s = info.strides[ ii ] / static_cast< dip::sint >( info.itemsize );
      DIP_THROW_IF( s * static_cast< dip::sint >( info.itemsize ) != info.strides[ ii ],
                    "Cannot create image out of an array where strides are not in whole pixels" );
      strides[ ii ] = s;
   }
   // The containing Python object. We increase its reference count, and create a unique_ptr that decreases
   // its reference count again.
   PyObject* pyObject = buf.ptr();
   //std::cout << "   *** Incrementing ref count for pyObject " << pyObject << std::endl;
   Py_XINCREF( pyObject );
   dip::DataSegment dataSegment{ pyObject, []( void* obj ) {
      //std::cout << "   *** Decrementing ref count for pyObject " << obj << std::endl;
      Py_XDECREF( static_cast< PyObject* >( obj ));
   }};
   // Create an image with all of this.
   dip::Image out( dataSegment, info.ptr, datatype, sizes, strides, {}, 1 );
   // If the last dimension has <10 pixels, let's assume it's a tensor dimension:
   if( sizes.back() < 10 ) {
      out.SpatialToTensor();
   }
   return out;
}

py::buffer_info ImageToBuffer( dip::Image const& image ) {
   dip::String format;
   switch( image.DataType()) {
      case dip::DT_BIN:
         format = py::format_descriptor< bool >::format();
         break;
      case dip::DT_UINT8:
         format = py::format_descriptor< dip::uint8 >::format();
         break;
      case dip::DT_UINT16:
         format = py::format_descriptor< dip::uint16 >::format();
         break;
      case dip::DT_UINT32:
         format = py::format_descriptor< dip::uint32 >::format();
         break;
      case dip::DT_SINT8:
         format = py::format_descriptor< dip::sint8 >::format();
         break;
      case dip::DT_SINT16:
         format = py::format_descriptor< dip::sint16 >::format();
         break;
      case dip::DT_SINT32:
         format = py::format_descriptor< dip::sint32 >::format();
         break;
      case dip::DT_SFLOAT:
         format = py::format_descriptor< dip::sfloat >::format();
         break;
      case dip::DT_DFLOAT:
         format = py::format_descriptor< dip::dfloat >::format();
         break;
      case dip::DT_SCOMPLEX:
         format = py::format_descriptor< dip::scomplex >::format();
         break;
      case dip::DT_DCOMPLEX:
         format = py::format_descriptor< dip::dcomplex >::format();
         break;
      default:
         DIP_THROW( "Image of unknown type" ); // should never happen
   }
   dip::sint itemsize = static_cast< dip::sint >( image.DataType().SizeOf() );
   dip::IntegerArray strides = image.Strides();
   for( dip::sint& s : strides ) {
      s *= itemsize;
   }
   dip::UnsignedArray sizes = image.Sizes();
   if( !image.IsScalar() ) {
      sizes.push_back( image.TensorElements() );
      strides.push_back( image.TensorStride() );
   }
   py::buffer_info info{ image.Origin(), itemsize, format, static_cast< py::ssize_t >( sizes.size() ), sizes, strides };
   //std::cout << "--Constructed Python buffer for dip::Image object.\n";
   //std::cout << "   info.ptr = " << info.ptr << std::endl;
   //std::cout << "   info.format = " << info.format << std::endl;
   //std::cout << "   info.ndims = " << info.ndims << std::endl;
   //std::cout << "   info.size = " << info.size << std::endl;
   //std::cout << "   info.itemsize = " << info.itemsize << std::endl;
   //std::cout << "   info.shape[0] = " << info.shape[0] << std::endl;
   //std::cout << "   info.shape[1] = " << info.shape[1] << std::endl;
   //std::cout << "   info.strides[0] = " << info.strides[0] << std::endl;
   //std::cout << "   info.strides[1] = " << info.strides[1] << std::endl;
   return info;
}

} // namespace

void init_image( py::module& m ) {
   auto img = py::class_< dip::Image >( m, "Image", py::buffer_protocol() );
   // Constructor
   img.def( py::init< dip::UnsignedArray const&, dip::uint, dip::DataType >(),
         "sizes"_a, "tensorElems"_a = 1, "dt"_a = dip::DT_SFLOAT );
   // Constructor for raw (unforged) image, to be used e.g. when no mask input argument is needed
   img.def( "__init__", []( dip::Image& self ) { new( &self ) dip::Image(); } );
   // Constructor that takes a Python raw buffer
   img.def( "__init__", []( dip::Image& self, py::buffer& buf ) { new( &self ) dip::Image(); self = BufferToImage( buf ); } );
   py::implicitly_convertible< py::buffer, dip::Image >();
   // Export a Python raw buffer
   img.def_buffer( []( dip::Image& self ) -> py::buffer_info { return ImageToBuffer( self ); } );
   // Basic properties
   img.def( "__repr__", []( dip::Image const& a ) { std::ostringstream os; os << a; return os.str(); } );
   img.def( "__len__", []( dip::Image const& a ) { return a.NumberOfPixels(); } );
   img.def( "IsEmpty", []( dip::Image const& a ) { return !a.IsForged(); } );
   img.def( "Dimensionality", &dip::Image::Dimensionality );
   img.def( "Sizes", &dip::Image::Sizes );
   img.def( "Size", &dip::Image::Size, "dim"_a );
   img.def( "NumberOfPixels", &dip::Image::NumberOfPixels );
   img.def( "NumberOfSamples", &dip::Image::NumberOfSamples );
   img.def( "Strides", &dip::Image::Strides );
   img.def( "Stride", &dip::Image::Stride, "dim"_a );
   img.def( "TensorStride", &dip::Image::TensorStride );
   img.def( "HasContiguousData", &dip::Image::HasContiguousData );
   img.def( "HasNormalStrides", &dip::Image::HasNormalStrides );
   img.def( "IsSingletonExpanded", &dip::Image::IsSingletonExpanded );
   img.def( "HasSimpleStride", &dip::Image::HasSimpleStride );
   //.def( "GetSimpleStrideAndOrigin", &dip::Image::GetSimpleStrideAndOrigin, "stride"_a, "origin"_a ) // both output parameters...
   img.def( "HasSameDimensionOrder", &dip::Image::HasSameDimensionOrder, "other"_a );
   img.def( "TensorSizes", &dip::Image::TensorSizes );
   img.def( "TensorElements", &dip::Image::TensorElements );
   img.def( "TensorColumns", &dip::Image::TensorColumns );
   img.def( "TensorRows", &dip::Image::TensorRows );
   img.def( "TensorShape", &dip::Image::TensorShape );
   img.def( "Tensor", &dip::Image::Tensor );
   img.def( "IsScalar", &dip::Image::IsScalar );
   img.def( "IsVector", &dip::Image::IsVector );
   img.def( "IsSquare", &dip::Image::IsSquare );
   img.def( "DataType", &dip::Image::DataType );
   img.def( "ColorSpace", &dip::Image::ColorSpace );
   img.def( "IsColor", &dip::Image::IsColor );
   img.def( "SetColorSpace", &dip::Image::SetColorSpace, "colorSpace"_a );
   img.def( "ResetColorSpace", &dip::Image::ResetColorSpace );
   img.def( "PixelSize", py::overload_cast<>( &dip::Image::PixelSize ));
   img.def( "PixelSize", py::overload_cast<>( &dip::Image::PixelSize, py::const_ ));
   img.def( "PixelSize", py::overload_cast< dip::uint >( &dip::Image::PixelSize, py::const_ ), "dim"_a );
   img.def( "SetPixelSize", &dip::Image::SetPixelSize, "pixelSize"_a );
   img.def( "HasPixelSize", &dip::Image::HasPixelSize );
   img.def( "IsIsotropic", &dip::Image::IsIsotropic );
   img.def( "PixelsToPhysical", &dip::Image::PixelsToPhysical, "array"_a );
   img.def( "PhysicalToPixels", &dip::Image::PhysicalToPixels, "array"_a );
   // About the data segment
   img.def( "IsShared", &dip::Image::IsShared );
   img.def( "ShareCount", &dip::Image::ShareCount );
   img.def( "SharesData", &dip::Image::SharesData, "other"_a );
   img.def( "Aliases", &dip::Image::Aliases, "other"_a );
   img.def( "IsIdenticalView", &dip::Image::IsIdenticalView, "other"_a );
   img.def( "IsOverlappingView", py::overload_cast< dip::Image const& >( &dip::Image::IsOverlappingView, py::const_ ), "other"_a );
   img.def( "Protect", &dip::Image::Protect, "set"_a = true );
   img.def( "IsProtected", &dip::Image::IsProtected );
   // Modify image without copying pixel data
   img.def( "PermuteDimensions", &dip::Image::PermuteDimensions, "order"_a, py::return_value_policy::reference_internal );
   img.def( "SwapDimensions", &dip::Image::SwapDimensions, "dim1"_a, "dim2"_a, py::return_value_policy::reference_internal );
   img.def( "Flatten", &dip::Image::Flatten, py::return_value_policy::reference_internal );
   img.def( "Squeeze", py::overload_cast<>( &dip::Image::Squeeze ), py::return_value_policy::reference_internal );
   img.def( "AddSingleton", &dip::Image::AddSingleton, "dim"_a, py::return_value_policy::reference_internal );
   img.def( "ExpandDimensionality", &dip::Image::ExpandDimensionality, "dim"_a, py::return_value_policy::reference_internal );
   img.def( "ExpandSingletonDimension", &dip::Image::ExpandSingletonDimension, "dim"_a, "newSize"_a, py::return_value_policy::reference_internal );
   img.def( "ExpandSingletonDimensions", &dip::Image::ExpandSingletonDimensions, "newSizes"_a, py::return_value_policy::reference_internal );
   img.def( "UnexpandSingletonDimensions", &dip::Image::UnexpandSingletonDimensions, py::return_value_policy::reference_internal );
   img.def( "IsSingletonExpansionPossible", &dip::Image::IsSingletonExpansionPossible, "newSizes"_a );
   img.def( "ExpandSingletonTensor", &dip::Image::ExpandSingletonTensor, "size"_a, py::return_value_policy::reference_internal );
   img.def( "Mirror", &dip::Image::Mirror, "process"_a, py::return_value_policy::reference_internal );
   img.def( "Rotation90", py::overload_cast< dip::sint, dip::uint, dip::uint >( &dip::Image::Rotation90 ), "n"_a, "dimension1"_a, "dimension2"_a, py::return_value_policy::reference_internal );
   img.def( "Rotation90", py::overload_cast< dip::sint, dip::uint >( &dip::Image::Rotation90 ), "n"_a, "axis"_a, py::return_value_policy::reference_internal );
   img.def( "Rotation90", py::overload_cast< dip::sint >( &dip::Image::Rotation90 ), "n"_a, py::return_value_policy::reference_internal );
   img.def( "StandardizeStrides", &dip::Image::StandardizeStrides, py::return_value_policy::reference_internal );
   img.def( "ReshapeTensor", py::overload_cast< dip::uint, dip::uint >( &dip::Image::ReshapeTensor ), "rows"_a, "cols"_a, py::return_value_policy::reference_internal );
   img.def( "ReshapeTensorAsVector", &dip::Image::ReshapeTensorAsVector, py::return_value_policy::reference_internal );
   img.def( "ReshapeTensorAsDiagonal", &dip::Image::ReshapeTensorAsDiagonal, py::return_value_policy::reference_internal );
   img.def( "Transpose", &dip::Image::Transpose, py::return_value_policy::reference_internal );
   img.def( "TensorToSpatial", py::overload_cast<>( &dip::Image::TensorToSpatial ), py::return_value_policy::reference_internal );
   img.def( "TensorToSpatial", py::overload_cast< dip::uint >( &dip::Image::TensorToSpatial ), "dim"_a, py::return_value_policy::reference_internal  );
   img.def( "SpatialToTensor", py::overload_cast<>( &dip::Image::SpatialToTensor ), py::return_value_policy::reference_internal );
   img.def( "SpatialToTensor", py::overload_cast< dip::uint >( &dip::Image::SpatialToTensor ), "dim"_a, py::return_value_policy::reference_internal );
   img.def( "SpatialToTensor", py::overload_cast< dip::uint, dip::uint >( &dip::Image::SpatialToTensor ), "rows"_a, "cols"_a, py::return_value_policy::reference_internal );
   img.def( "SpatialToTensor", py::overload_cast< dip::uint, dip::uint, dip::uint >( &dip::Image::SpatialToTensor ), "dim"_a, "rows"_a, "cols"_a, py::return_value_policy::reference_internal );
   img.def( "SplitComplex", py::overload_cast<>( &dip::Image::SplitComplex ), py::return_value_policy::reference_internal );
   img.def( "SplitComplex", py::overload_cast< dip::uint >( &dip::Image::SplitComplex ), "dim"_a, py::return_value_policy::reference_internal );
   img.def( "MergeComplex", py::overload_cast<>( &dip::Image::MergeComplex ), py::return_value_policy::reference_internal );
   img.def( "MergeComplex", py::overload_cast< dip::uint >( &dip::Image::MergeComplex ), "dim"_a, py::return_value_policy::reference_internal );
   img.def( "SplitComplexToTensor", &dip::Image::SplitComplexToTensor, py::return_value_policy::reference_internal );
   img.def( "MergeTensorToComplex", &dip::Image::MergeTensorToComplex, py::return_value_policy::reference_internal );
   // Create a new image with a view of another image (the At() method is below, under indexing)
   img.def( "Diagonal", &dip::Image::Diagonal );
   img.def( "TensorRow", &dip::Image::TensorRow, "index"_a );
   img.def( "TensorColumn", &dip::Image::TensorColumn, "index"_a );
   img.def( "Crop", py::overload_cast< dip::UnsignedArray const&, dip::String const& >( &dip::Image::Crop, py::const_ ), "sizes"_a, "cropLocation"_a = "center" );
   img.def( "Real", &dip::Image::Real );
   img.def( "Imaginary", &dip::Image::Imaginary );
   img.def( "QuickCopy", &dip::Image::QuickCopy );
   // Copy or write data (the CopyAt() method is below, under indexing)
   img.def( "Pad", py::overload_cast< dip::UnsignedArray const&, dip::String const& >( &dip::Image::Pad, py::const_ ), "sizes"_a, "cropLocation"_a = "center" );
   img.def( "Copy", &dip::Image::Copy, "src"_a );
   img.def( "Convert", &dip::Image::Convert, "dataType"_a );
   img.def( "ExpandTensor", &dip::Image::ExpandTensor );
   img.def( "Fill", py::overload_cast< dip::Image::Sample const& >( &dip::Image::Fill ), "sample"_a );
   img.def( "Fill", py::overload_cast< dip::Image::Pixel const& >( &dip::Image::Fill ), "pixel"_a );
   // TODO: Indexing does not match that in C++, and we don't have tensor indexing here at all!
   // Indexing into single pixel using coordinates
   // TODO: The following functions should make make a Python object of the right type
   img.def( "At", []( dip::Image const& image, dip::uint index ) { return image.At( index ); }, "index"_a );
   img.def( "__getitem__", []( dip::Image const& image, dip::uint index ) { return image.At( index ); } );
   img.def( "At", []( dip::Image const& image, dip::uint x_index, dip::uint y_index ) { return image.At( x_index, y_index ); }, "x_index"_a, "y_index"_a );
   // (The function cannot be called?)
   //img.def( "__getitem__", []( dip::Image const& image, dip::uint x_index, dip::uint y_index ) { return image.At( x_index, y_index ); } );
   img.def( "At", []( dip::Image const& image, dip::uint x_index, dip::uint y_index, dip::uint z_index ) { return image.At( x_index, y_index, z_index ); }, "x_index"_a, "y_index"_a, "z_index"_a );
   // (The function cannot be called?)
   //img.def( "__getitem__", []( dip::Image const& image, dip::uint x_index, dip::uint y_index, dip::uint z_index ) { return image.At( x_index, y_index, z_index ); } );
   img.def( "At", []( dip::Image const& image, dip::UnsignedArray const& coords ) { return image.At( coords ); }, "coords"_a );
   // (The function below gets called instead of the mask version)
   //img.def( "__getitem__", []( dip::Image const& image, dip::UnsignedArray const& coords ) { return image.At( coords ); } );
   // Indexing into slice for 1D image
   img.def( "At", []( dip::Image const& image, dip::Range x_range ) { return image.At( x_range ); }, "x_range"_a );
   img.def( "__getitem__", []( dip::Image const& image, dip::Range const& range ) { return image.At( range ); } );
   // Indexing into slice for 2D image
   img.def( "At", []( dip::Image const& image, dip::Range x_range, dip::Range y_range ) { return image.At( x_range, y_range ); }, "x_range"_a, "y_range"_a );
   // Indexing into slice for 3D image
   img.def( "At", []( dip::Image const& image, dip::Range x_range, dip::Range y_range, dip::Range z_range ) { return image.At( x_range, y_range, z_range ); }, "x_range"_a, "y_range"_a, "z_range"_a );
   // Indexing into slice for nD image
   img.def( "At", []( dip::Image const& image, dip::RangeArray ranges ) { return image.At( ranges ); }, "ranges"_a );
   img.def( "__getitem__", []( dip::Image const& image, dip::RangeArray const& rangeArray ) { return image.At( rangeArray ); } );
   // Indexing using a mask image
   img.def( "CopyAt", py::overload_cast< dip::Image const& >( &dip::Image::CopyAt, py::const_ ), "mask"_a );
   img.def( "__getitem__", []( dip::Image const& image, dip::Image const& mask ) { return image.CopyAt( mask ); } );
   // Indexing using linear indices (can probably not be called because of implicit cast to dip::Image)
   img.def( "CopyAt", py::overload_cast< dip::UnsignedArray const& >( &dip::Image::CopyAt, py::const_ ), "indices"_a );
   // Indexing using a coordinate array (can probably not be called because of implicit cast to dip::Image)
   img.def( "CopyAt", py::overload_cast< dip::CoordinateArray const& >( &dip::Image::CopyAt, py::const_ ), "coordinates"_a );
   // Assignment into single pixel using linear index
   img.def( "__setitem__", []( dip::Image& image, dip::uint index, dip::Image::Sample const& v ) { image.At( index ) = v; } );
   img.def( "__setitem__", []( dip::Image& image, dip::uint index, dip::Image::Pixel const& v ) { image.At( index ) = v; } );
   // Assignment into slice for 1D image
   img.def( "__setitem__", []( dip::Image& image, dip::Range const& range, dip::Image::Sample const& value ) { image.At( range ).Fill( value ); } );
   img.def( "__setitem__", []( dip::Image& image, dip::Range const& range, dip::Image::Pixel const& value ) { image.At( range ).Fill( value ); } );
   img.def( "__setitem__", []( dip::Image& image, dip::Range const& range, dip::Image const& value ) { image.At( range ).Copy( value ); } );
   // Assignment into slice for nD image
   img.def( "__setitem__", []( dip::Image& image, dip::RangeArray const& rangeArray, dip::Image::Sample const& value ) { image.At( rangeArray ).Fill( value ); } );
   img.def( "__setitem__", []( dip::Image& image, dip::RangeArray const& rangeArray, dip::Image::Pixel const& value ) { image.At( rangeArray ).Fill( value ); } );
   img.def( "__setitem__", []( dip::Image& image, dip::RangeArray const& rangeArray, dip::Image const& value ) { image.At( rangeArray ).Copy( value ); } );
   // Assignment using a mask image
   img.def( "CopyAt", []( dip::Image& image, dip::Image const& source, dip::Image const& mask ) { image.CopyAt( source, mask, dip::Option::ThrowException::DO_THROW ); }, "source"_a, "mask"_a );
   img.def( "__setitem__", []( dip::Image& image, dip::Image const& mask, dip::Image const& value ) { image.CopyAt( value, mask, dip::Option::ThrowException::DO_THROW ); } );
   img.def( "FillAt", py::overload_cast< dip::Image::Sample const&, dip::Image const& >( &dip::Image::FillAt ), "sample"_a, "mask"_a );
   img.def( "__setitem__", []( dip::Image& image, dip::Image const& mask, dip::Image::Sample const& value ) { image.FillAt( value, mask ); } );
   img.def( "FillAt", py::overload_cast< dip::Image::Pixel const&, dip::Image const& >( &dip::Image::FillAt ), "pixel"_a, "mask"_a );
   img.def( "__setitem__", []( dip::Image& image, dip::Image const& mask, dip::Image::Pixel const& value ) { image.FillAt( value, mask ); } );
   // Assignment using linear indices (can probably not be called because of implicit cast to dip::Image)
   img.def( "CopyAt", py::overload_cast< dip::Image const&, dip::UnsignedArray const& >( &dip::Image::CopyAt ), "source"_a, "indices"_a );
   img.def( "FillAt", py::overload_cast< dip::Image::Sample const&, dip::UnsignedArray const& >( &dip::Image::FillAt ), "sample"_a, "indices"_a );
   img.def( "FillAt", py::overload_cast< dip::Image::Pixel const&, dip::UnsignedArray const& >( &dip::Image::FillAt ), "pixel"_a, "indices"_a );
   // Assignment using a coordinate array (can probably not be called because of implicit cast to dip::Image)
   img.def( "CopyAt", py::overload_cast< dip::Image const&, dip::CoordinateArray const& >( &dip::Image::CopyAt ), "source"_a, "coordinates"_a );
   img.def( "FillAt", py::overload_cast< dip::Image::Sample const&, dip::CoordinateArray const& >( &dip::Image::FillAt ), "sample"_a, "coordinates"_a );
   img.def( "FillAt", py::overload_cast< dip::Image::Pixel const&, dip::CoordinateArray const& >( &dip::Image::FillAt ), "pixel"_a, "coordinates"_a );
   // Operators
   img.def( py::self += py::self );
   img.def( py::self += float() );
   img.def( py::self + py::self );
   img.def( py::self + float() );
   img.def( py::self -= py::self );
   img.def( py::self -= float() );
   img.def( py::self - py::self );
   img.def( py::self - float() );
   img.def( py::self *= py::self );
   img.def( py::self *= float() );
   img.def( py::self * py::self );
   img.def( py::self * float() );
   img.def( py::self /= py::self );
   img.def( py::self /= float() );
   img.def( py::self / py::self );
   img.def( py::self / float() );
   img.def( py::self %= py::self );
   img.def( py::self %= float() );
   img.def( py::self % py::self );
   img.def( py::self % float() );
   img.def( "__pow__", []( dip::Image const& a, dip::Image const& b ) { return dip::Power( a, b ); }, py::is_operator() );
   img.def( "__pow__", []( dip::Image const& a, float b ) { return dip::Power( a, b ); }, py::is_operator() );
   img.def( "__ipow__", []( dip::Image& a, dip::Image const& b ) { dip::Power( a, b, a ); return a; }, py::is_operator() );
   img.def( "__ipow__", []( dip::Image& a, float b ) { dip::Power( a, b, a ); return a; }, py::is_operator() );
   img.def( py::self == py::self );
   img.def( py::self == float() );
   img.def( py::self != py::self );
   img.def( py::self != float() );
   img.def( py::self > py::self );
   img.def( py::self > float() );
   img.def( py::self >= py::self );
   img.def( py::self >= float() );
   img.def( py::self < py::self );
   img.def( py::self < float() );
   img.def( py::self <= py::self );
   img.def( py::self <= float() );
   img.def( py::self & py::self );
   img.def( py::self & int() );
   img.def( py::self | py::self );
   img.def( py::self | int() );
   img.def( py::self ^ py::self );
   img.def( py::self ^ int() );
   img.def( !py::self );
   img.def( -py::self );
   img.def( ~py::self );
   // Iterators
   // TODO: This does not work!
   //img.def( "__iter__", []( dip::Image const& image ) {
   //         return py::make_iterator< py::return_value_policy::reference_internal,
   //                                   dip::GenericImageIterator<>,
   //                                   dip::GenericImageIterator<>,
   //                                   dip::Image::Pixel >( dip::GenericImageIterator<>( image ),
   //                                                        dip::GenericImageIterator<>() );
   //}, py::keep_alive< 0, 1 >() ); // Essential: keep object alive while iterator exists
}
