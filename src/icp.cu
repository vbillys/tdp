/* Copyright (c) 2016, Julian Straub <jstraub@csail.mit.edu> Licensed
 * under the MIT license. See the license file LICENSE.
 */

#include <assert.h>
#include <tdp/eigen/dense.h>
#include <tdp/cuda.h>
#include <tdp/nvidia/helper_cuda.h>
#include <tdp/image.h>
#include <tdp/managed_image.h>
#include <tdp/camera.h>
#include <tdp/reductions.cuh>

namespace tdp {

// R_mc: R_model_current
template<int BLK_SIZE>
__global__ void KernelICPStep(
    Image<Vector3fda> pc_m,
    Image<Vector3fda> n_m,
    Image<Vector3fda> pc_c,
    Image<Vector3fda> n_c,
    Matrix3fda R_mc, 
    Vector3fda t_mc, 
    const Camera<float> cam,
    float dotThr,
    float distThr,
    int N_PER_T,
    Image<float> out
    ) {
  const int tid = threadIdx.x;
  const int idx = threadIdx.x + blockDim.x * blockIdx.x;
  const int idS = idx*N_PER_T;
  const int N = pc_m.w_*pc_m.h_;
  const int idE = min(N,(idx+1)*N_PER_T);
  __shared__ Eigen::Matrix<float,29,1,Eigen::DontAlign> sum[BLK_SIZE];

  sum[tid] = Eigen::Matrix<float,29,1,Eigen::DontAlign>::Zero();

  for (int id=idS; id<idE; ++id) {
    const int idx = id%pc_c.w_;
    const int idy = id/pc_c.w_;
    Vector3fda pc_ci = pc_c(idx,idy);
    // project current point into model frame to get association
    if (idx < pc_c.w_ && idy < pc_c.h_ && IsValidData(pc_ci)) {
      Vector3fda pc_c_in_m = R_mc * pc_ci + t_mc;
      // project into model camera
      Vector2fda x_c_in_m = cam.Project(pc_c_in_m);
      int u = floor(x_c_in_m(0)+0.5f);
      int v = floor(x_c_in_m(1)+0.5f);
      if (0 <= u && u < pc_m.w_ && 0 <= v && v < pc_m.h_
          && IsValidData(pc_c_in_m)) {
        // found association -> check thresholds;
        Vector3fda n_c_in_m = R_mc * n_c(idx,idy);
        Vector3fda n_mi = n_m(idx,idy);
        Vector3fda pc_mi = pc_m(idx,idy);
        float dot  = n_mi.dot(n_c_in_m);
        float dist = n_mi.dot(pc_mi-pc_c_in_m);
        //if (tid < 10)
        //  printf("%d %d to %d %d; 3d: %f %f %f; %f >? %f\n",idx,idy,u,v,pc_c(idx,idy)(0),pc_c(idx,idy)(1),pc_c(idx,idy)(2),dot,dotThr);
        if (dot > dotThr && dist < distThr && IsValidData(pc_mi)) {
          // association is good -> accumulate
          // if we found a valid association accumulate the A and b for A x = b
          // where x \in se{3} as well as the residual error
          float ab[7];      
          Eigen::Map<Vector3fda> top(&(ab[0]));
          Eigen::Map<Vector3fda> bottom(&(ab[3]));
          top = (n_mi).cross(pc_c_in_m);
          //top = (R_mc * pc_ci).cross(n_mi);
          bottom = n_mi;
          ab[6] = dist;
          Eigen::Matrix<float,29,1,Eigen::DontAlign> upperTriangle;
          int k=0;
#pragma unroll
          for (int i=0; i<7; ++i) {
            for (int j=i; j<7; ++j) {
              upperTriangle(k++) = ab[i]*ab[j];
            }
          }
          assert(k==28);
          upperTriangle(28) = 1.; // to get number of data points
          sum[tid] += upperTriangle;
          //if (tid < 10)
          //  printf("%f %f %f %f %f\n",sum[tid](0),sum[tid](1),sum[tid](2),sum[tid](3),sum[tid](4));
        }
      }
    }
  }
  __syncthreads(); //sync the threads
#pragma unroll
  for(int s=(BLK_SIZE)/2; s>1; s>>=1) {
    if(tid < s) {
      sum[tid] += sum[tid+s];
    }
    __syncthreads();
  }
  if(tid < 29) {
    // sum the last two remaining matrixes directly into global memory
    atomicAdd(&out[tid], sum[0](tid)+sum[1](tid));
    //atomicAdd_<float>();
    //printf("%f %f %f \n",out[tid],sum[0](tid),sum[1](tid));
  }
}

void ICPStep (
    Image<Vector3fda> pc_m,
    Image<Vector3fda> n_m,
    Image<Vector3fda> pc_c,
    Image<Vector3fda> n_c,
    Matrix3fda& R_mc, 
    Vector3fda& t_mc, 
    const Camera<float>& cam,
    float dotThr,
    float distThr,
    Eigen::Matrix<float,6,6,Eigen::DontAlign>& ATA,
    Eigen::Matrix<float,6,1,Eigen::DontAlign>& ATb,
    float& error,
    float& count
    ) {
  size_t N = pc_m.w_*pc_m.h_;
  dim3 threads, blocks;
  ComputeKernelParamsForArray(blocks,threads,N/10,256);
  ManagedDeviceImage<float> out(29,1);
  cudaMemset(out.ptr_, 0, 29*sizeof(float));

  KernelICPStep<256><<<blocks,threads>>>(pc_m,n_m,pc_c,n_c,R_mc,t_mc,cam,dotThr,distThr,10,out);
  checkCudaErrors(cudaDeviceSynchronize());
  ManagedHostImage<float> sumAb(29,1);
  cudaMemcpy(sumAb.ptr_,out.ptr_,29*sizeof(float), cudaMemcpyDeviceToHost);

  //for (int i=0; i<29; ++i) std::cout << sumAb[i] << "\t";
  //std::cout << std::endl;
  ATA.fill(0.);
  ATb.fill(0.);
  int prevRowStart = 0;
  for (int i=0; i<6; ++i) {
    ATb(i) = sumAb[prevRowStart+7-i-1];
    for (int j=i; j<6; ++j) {
      ATA(i,j) = sumAb[prevRowStart+j-i];
      ATA(j,i) = sumAb[prevRowStart+j-i];
    }
    prevRowStart += 7-i;
  }
  count = sumAb[28];
  error = sumAb[27]/count;
  //std::cout << ATA << std::endl << ATb.transpose() << std::endl;
  //std::cout << "\terror&count " << error << " " << count << std::endl;
}

}
