
#include <tdp/data/image.h>
#include <tdp/preproc/blur.h>
#include <tdp/cuda/cuda.h>

namespace tdp {

template<typename T>
__global__
void KernelBlur5(
    const Image<T> Iin,
    Image<T> Iout,
    T sigma_in
    ) {
  const int idx = threadIdx.x + blockDim.x * blockIdx.x;
  const int idy = threadIdx.y + blockDim.y * blockIdx.y;
  if (idx < Iout.w_ && idy < Iout.h_) {
    const int D = 5; // size of the gaussian kernel
    
    int xMin = max(0,idx - D/2);
    int xMax = min(idx - D/2 + D, (int)Iin.w_);
    int yMin = max(0,idy - D/2);
    int yMax = min(idy - D/2 + D, (int)Iin.h_);

    T val0 = Iin(idx, idy);
    float ws[] = {0.375f, 0.25f, 0.0625f}; // weights of kernel

    float W = 0;
    float sum = 0;
    for (int y=yMin; y<yMax; ++y) {
      for (int x=xMin; x<xMax; ++x) {
        T val = Iin(x, y);
        if (fabs(val-val0) < 3*sigma_in) {
          float wx = ws[abs(x-idx)];
          float wy = ws[abs(y-idy)];
          sum += val*wx*wy;
          W += wx*wy;
        }
      }
    }
    Iout(idx,idy) = static_cast<T>(sum/W);
  }
}

void Blur5(
    const Image<float>& Iin,
    Image<float>& Iout,
    float sigma_in
    ) {
  //printf("%dx%d %dx%d\n",Iin.w_,Iin.h_,Iout.w_,Iout.h_);
  assert(Iin.w_ == Iout.w_*2);
  assert(Iin.h_ == Iout.h_*2);
  dim3 threads, blocks;
  ComputeKernelParamsForImage(blocks,threads,Iout,32,32);
  KernelBlur5<float><<<blocks,threads>>>(Iin,Iout,sigma_in);
  checkCudaErrors(cudaDeviceSynchronize());
}

}