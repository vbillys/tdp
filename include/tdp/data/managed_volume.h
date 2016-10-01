/* Copyright (c) 2016, Julian Straub <jstraub@csail.mit.edu> Licensed
 * under the MIT license. See the license file LICENSE.
 */
#pragma once
#include <tdp/config.h>
#include <assert.h>
#include <tdp/data/allocator.h>
#ifdef CUDA_FOUND
#  include <tdp/data/allocator_gpu.h>
#endif
#include <tdp/data/volume.h>

namespace tdp {

template <class T, class Alloc>
class ManagedVolume : public Volume<T> {
  public:
   ManagedVolume(size_t w, size_t h, size_t d) 
     : Volume<T>(w,h,d,Alloc::construct(w*h*d))
   {}

  void Reinitialise(size_t w, size_t h, size_t d) {
    if (this->ptr_)  {
      Alloc::destroy(this->ptr_);
    }
    this->ptr_ = Alloc::construct(w*h*d);
    this->w_ = w;
    this->h_ = h;
    this->d_ = d;
    this->pitch_ = w*sizeof(T);
    this->pitchImg_ = h*w*sizeof(T);
  }

   ~ManagedVolume() {
     Alloc::destroy(this->ptr_);
   }
};

template <class T>
using ManagedHostVolume = ManagedVolume<T,CpuAllocator<T>>;

#ifdef CUDA_FOUND

template <class T>
using ManagedDeviceVolume = ManagedVolume<T,GpuAllocator<T>>;

template<class T>
void CopyVolume(Volume<T>& From, Volume<T>& To, cudaMemcpyKind cpType) { 
  assert(From.SizeBytes() == To.SizeBytes());
  cudaMemcpy(To.ptr_, From.ptr_, From.SizeBytes(), cpType);
}
#endif

template<typename T>
bool LoadVolume(ManagedHostVolume<T>& V, const std::string& path) {

  std::ifstream in;
  in.open(path, std::ios::in | std::ios::binary);
  if (!in.is_open())
    return false;

  size_t w,h,d;
  in.read((char *)&(w),sizeof(size_t));
  in.read((char *)&(h),sizeof(size_t));
  in.read((char *)&(d),sizeof(size_t));

  V.Reinitialize(w,h,d);

  for (size_t i=0; i < V.Vol(); ++i) {
    in.read((char *)&V[i],sizeof(T));
  }
  in.close();
  return true;
}


}
