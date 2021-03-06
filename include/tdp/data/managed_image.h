/* Copyright (c) 2016, Julian Straub <jstraub@csail.mit.edu> Licensed
 * under the MIT license. See the license file LICENSE.
 */
#pragma once
#include <assert.h>
#include <tdp/config.h>
#include <tdp/data/allocator.h>
#ifdef CUDA_FOUND
#  include <tdp/data/allocator_gpu.h>
#endif
#include <tdp/data/image.h>
#include <iostream>

namespace tdp {

template <class T, class Alloc>
class ManagedImage : public Image<T> {
 public:
  ManagedImage() : Image<T>(0,0,nullptr,Alloc::StorageType())
  {}
  ManagedImage(size_t w, size_t h=1) 
    : Image<T>(w,h,w*sizeof(T), Alloc::construct(w*h), Alloc::StorageType())
  {}
  ManagedImage(ManagedImage&& other)
  : Image<T>(other.w_, other.h_, other.pitch_, other.ptr_, other.storage_) {
    other.w_ = 0;
    other.h_ = 0;
    other.pitch_ = 0;
    other.ptr_ = nullptr;
  }

  /// Reinitialize the ManagedImage to a new size and discard all data.
  void Reinitialise(size_t w, size_t h=1) {
    if (this->w_ == w && this->h_ == h)
      return;
    if (this->ptr_)  {
      Alloc::destroy(this->ptr_);
    }
    this->ptr_ = Alloc::construct(w*h);
    this->w_ = w;
    this->h_ = h;
    this->pitch_ = w*sizeof(T);
  }

  /// Reshape this ManagedImage without changing its data.
  /// This method allocates a new chunk of data and then copies in the
  /// data from the old chunk of data before deallocating the old data
  /// block.
  void Reshape(size_t w, size_t h=1) {
    if (this->w_ == w && this->h_ == h)
      return;
    tdp::Image<T> tmp(*this);
    this->ptr_ = Alloc::construct(w*h);
    this->w_ = w;
    this->h_ = h;
    this->pitch_ = w*sizeof(T);
    if (tmp.ptr_)  {
      this->CopyFrom(tmp);
      Alloc::destroy(tmp.ptr_);
    }
  }

  void ResizeCopyFrom(const Image<T>& src) {
    Reinitialise(src.w_, src.h_);
    this->CopyFrom(src);
  }

  ~ManagedImage() {
    Alloc::destroy(this->ptr_);
  }
};

template <class T>
using ManagedHostImage = ManagedImage<T,CpuAllocator<T>>;

#ifdef CUDA_FOUND

template <class T>
using ManagedDeviceImage = ManagedImage<T,GpuAllocator<T>>;

template<class T>
void CopyImage(Image<T>& From, Image<T>& To, cudaMemcpyKind cpType) { 
  assert(From.SizeBytes() == To.SizeBytes());
  cudaMemcpy(To.ptr_, From.ptr_, From.SizeBytes(), cpType);
}
#endif

}
