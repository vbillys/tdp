#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include <string>
#include <cstdlib>
#include <random>

#include <pangolin/pangolin.h>
#include <pangolin/video/video_record_repeat.h>
#include <pangolin/gl/gltexturecache.h>
#include <pangolin/gl/glpixformat.h>
#include <pangolin/handler/handler_image.h>
#include <pangolin/utils/file_utils.h>
#include <pangolin/utils/timer.h>
#include <pangolin/gl/gl.h>
#include <pangolin/gl/glsl.h>
#include <pangolin/gl/glvbo.h>
#include <pangolin/gl/gldraw.h>
#include <pangolin/image/image_io.h>

#include <tdp/eigen/dense.h>
#include <Eigen/Eigenvalues>
#include <Eigen/Sparse>
#include <Eigen/Core>

#include <tdp/preproc/depth.h>
#include <tdp/preproc/pc.h>
#include <tdp/camera/camera.h>
#ifdef CUDA_FOUND
#include <tdp/preproc/normals.h>
#endif

#include <tdp/io/tinyply.h>
#include <tdp/gl/shaders.h>
#include <tdp/gl/gl_draw.h>

#include <tdp/gui/gui.hpp>
#include <tdp/gui/quickView.h>

#include <tdp/nn/ann.h>
#include <tdp/manifold/S.h>
#include <tdp/manifold/SE3.h>
#include <tdp/data/managed_image.h>

#include <tdp/utils/status.h>
#include <tdp/utils/timer.hpp>
#include <tdp/eigen/std_vector.h>

#include <tdp/laplace_beltrami/laplace_beltrami.h>

/************Declarations***************************************
 ***************************************************************/
void Test_printImage();
void Test_randomSeed();
void Test_addGaussianNoise();
void Test_f_landmark();
void Test_projections();
void Test_samePc_sameSamples(
        int nSamples, 
        int nEv,
        int nTrain,
        // std::string option = std::string("rbf"), 
        std::string& option,
        bool showDecomposition = false);
void Test_samePc_diffSamples(
        int nSamples, 
        // std::string option = std::string("rbf"), 
        std::string& option,
        bool showDecomposition = false);

/************end delcarations************************************/
int main(){
//    Test_printImage();
    //Test_f_landmark();
    //Test_randomSeed();
  //Test_addGaussianNoise();


  std::cout << "Test Correspondences---" << std::endl;
  std::string option("rbf");
  int nSamples = 1500;
  int nEv = 30;
  int nTrain;
  bool showDecomposition = false;
  for (int i=0; i< 33; ++i){
      nTrain = i*nEv;
      std::cout << "\n================================="<< std::endl;
      std::cout <<"nPoints: " << nSamples <<", nEv: " << nEv << ", nPW corresp.: "
                << nTrain << std::endl;
      Test_samePc_sameSamples(nSamples,nEv, nTrain,option,showDecomposition);
      std::cout << "=================================\n"<< std::endl;

    }

  //todo: time to incorporate this with the fmap main.cpp so that
  //    we don't recalcualte repeated things (such as Laplacian)
  //run the experiement with the pipe to a log file



  // Test_samePc_diffSamples(option);
}



/************Test mplementations***********************************
 ******************************************************************/
void Test_randomSeed(){
  tdp::ManagedHostImage<tdp::Vector3fda> pc1, pc2, pc3, pc4;
  tdp::GetSphericalPc(pc1, 10);
  tdp::GetSphericalPc(pc2, 10);
  tdp::GetSphericalPc(pc3, 10);
  pc4.ResizeCopyFrom(pc3);

  std::cout << "PC1---" << std::endl;
  tdp::printImage(pc1, 0, pc1.Area() -1);
  std::cout << "PC2---" << std::endl;
  tdp::printImage(pc2, 0, pc2.Area() -1);
  std::cout << "PC3---" << std::endl;
  tdp::printImage(pc3, 0, pc3.Area() -1);
  std::cout << "PC4---" << std::endl;
  tdp::printImage(pc4, 0, pc4.Area() -1);
}

void Test_printImage(){
    tdp::ManagedHostImage<tdp::Vector3fda> pc = tdp::GetSimplePc();
    std::cout << "---tdp::ing image---" << std::endl;
    tdp::printImage(pc,0,pc.Area());

    pc.Reinitialise(10);
    for (int i =0; i< 10; ++i){
        pc[i] = tdp::Vector3fda(i,i,i);
    }
    std::cout << "---Printing image---" << std::endl;
    tdp::printImage(pc,0,pc.Area());
}

void Test_addGaussianNoise(){
   tdp::ManagedHostImage<tdp::Vector3fda> pc_s, pc_t; 
   tdp::GetSimplePc(pc_s);
   tdp::addGaussianNoise(pc_s, 0.1f, pc_t);

   std::cout << "PC_S ----" << std::endl;
   tdp::printImage(pc_s, 0, pc_s.Area());
   std::cout << "PC_T ---" << std::endl;
   tdp::printImage(pc_t, 0, pc_t.Area());
}

void Test_f_landmark(){
    std::string opt;
    float alpha = 0.1;
    tdp::ManagedHostImage<tdp::Vector3fda> pc = tdp::GetSimplePc();
    Eigen::VectorXf f_w;

    for (int p_idx = 0; p_idx < pc.Area(); p_idx++){
        opt = "rbf";
        tdp::f_landmark(pc, p_idx, alpha, opt, f_w);
        std::cout << "opt: " << opt << std::endl;
        std::cout << "fw: " << f_w.transpose() << std::endl;

        opt = "ind";
        tdp::f_landmark(pc, p_idx, alpha, opt, f_w);
        std::cout << "opt: " << opt << std::endl;
        std::cout << "fw: " << f_w.transpose() << std::endl;
        std::cout << "\n\n" << std::endl;
    }
}

void Test_projections(){
    // Test for projectToLocal and projectToWorld
}

// void Test_samePc_withGNoise
void Test_samePc_sameSamples(int nSamples, int nEv, int nPW, std::string& option, bool showDecomposition){
  /* nSamples: number of points on the surface
   * nTrain: number of  correpondence pairs given to approximate functional mapping
   *  - must be at least numEv, and at most nSamples
   * option: "rbf" or "ind".  function to construct a function on the surface based on a given point
   * showDecompotion: true to see (eigenfunctions and) eigenvalues of the Laplacian of both surfaces
   */

//todo: std::option
    tdp::ManagedHostImage<tdp::Vector3fda> pc_s;
    tdp::ManagedHostImage<tdp::Vector3fda> pc_t;

    // Get random points from a sphere
    tdp::GetSimplePc(pc_s, nSamples);
    //tdp::GetSphericalPc(pc_s, nSamples);
    pc_t.ResizeCopyFrom(pc_s);

    //tdp::print("PC_S---");
    //tdp::printImage(pc_s, 0, pc_s.Area());
    //tdp::print("PC_T---");
    //tdp::printImage(pc_t, 0, pc_t.Area());

    /****parameters*****/
    /*******************/
    //int nEv = std::min(30, (int)(pc_s.Area()/3));//pc_s.Area()-2;
    //get ALL eigenvectors of L
    int knn = pc_s.Area(); // use all points as neighbors
    float eps = 1e-6;
    float alpha = 0.01;
    float alpha2 = 0.1;

    //int nPW = nTrain;//nEv;//number of pointwise correspondences
    int nHKS = 0; //number of heat kernel signature correspondences
    int nCst = nPW + nHKS;//pc_s.Area();
    
    //int nTest = pc_s.Area() - nPW;
    int nShow = std::min(10, nEv);
    /*******************/

    // build kd tree
    tdp::ANN ann_s, ann_t;
    ann_s.ComputeKDtree(pc_s);
    ann_t.ComputeKDtree(pc_t);

    // construct laplacian matrices
    Eigen::SparseMatrix<float> L_s(pc_s.Area(), pc_s.Area()),
                               L_t(pc_t.Area(), pc_t.Area());
    Eigen::MatrixXf S_wl(L_s.rows(),(int)nEv),//cols are evectors
                    T_wl(L_t.rows(),(int)nEv),
                    S_desc_w, T_desc_w,
                    S_desc_l, T_desc_l;
    Eigen::VectorXf S_evals((int)nEv), T_evals((int)nEv);


    L_s = tdp::getLaplacian(pc_s, ann_s, knn, eps, alpha);
    L_t = tdp::getLaplacian(pc_t, ann_t, knn, eps, alpha);
    tdp::decomposeLaplacian(L_s, nEv, S_evals, S_wl); //todo: check if size initialization is included
    tdp::decomposeLaplacian(L_t, nEv, T_evals, T_wl);

    if (showDecomposition){
        // std::cout << "Basis ---" << std::endl;
        // std::cout << "n of evec: " << nEv << std::endl;
        // std::cout << S_wl << std::endl;
        std::cout << "-----------------" << std::endl;
        std::cout << T_wl << std::endl;
        std::cout << "Evals ---" << std::endl;
        std::cout << "\tS: " << S_evals.transpose() << std::endl;
        std::cout << "\tT: " << T_evals.transpose() << std::endl;
    }

    //--Construct function pairs
    Eigen::VectorXf f_w(pc_s.Area()), g_w(pc_t.Area()),
                    f_l((int)nEv), g_l((int)nEv);
    Eigen::MatrixXf F((int)nCst, (int)nEv), G((int)nCst, (int)nEv);
    Eigen::MatrixXf C((int)nEv, (int)nEv);

    // --construct F(data matrix) and G based on the correspondences

    for (int i=0; i< (int)nPW; ++i){
        // if (option == "rbf"){
        //     tdp::f_rbf(pc_s, pc_s[i], alpha2, f_w); //todo: check if I can use this same alpha?
        //     tdp::f_rbf(pc_t, pc_t[i], alpha2, g_w);
        // } else {
        //     tdp::f_indicator(pc_s, i, f_w); //todo: check if I can use this same alpha?
        //     tdp::f_indicator(pc_t, i, g_w);
        // }
        tdp::f_landmark(pc_s, i, alpha2, option, f_w);
        tdp::f_landmark(pc_t, i, alpha2, option, g_w);

        f_l = (S_wl.transpose()*S_wl).fullPivLu().solve(S_wl.transpose()*f_w);
        g_l = (T_wl.transpose()*T_wl).fullPivLu().solve(T_wl.transpose()*g_w);
        //f_l = tdp::projectToLocal(S_wl, f_w);
        //g_l = tdp::projectToLocal(T_wl, g_w);

        F.row(i) = f_l;
        G.row(i) = g_l;
    }


    if (nHKS >0){
        //-----Add  heat kernel signatures as constraints
        std::cout << "CALCULATEING HKS ---" <<std::endl;
        S_desc_w = tdp::getHKS(S_wl,S_evals,nHKS);
        T_desc_w = tdp::getHKS(T_wl,T_evals,nHKS);
        S_desc_l = (S_wl.transpose()*S_wl).fullPivLu().solve(S_wl.transpose()*S_desc_w);
        T_desc_l = (T_wl.transpose()*T_wl).fullPivLu().solve(T_wl.transpose()*T_desc_w);
        //S_desc_l = tdp::projectToLocal(S_wl, S_desc_w); //columne is a feature
        //T_desc_l = tdp::projectToLocal(T_wl, T_desc_w);
        
        assert(S_desc_l.cols() == nHKS);
        for (int i=0; i<nHKS; ++i){
          F.row(nPW+i) = S_desc_l.col(i);
          G.row(nPW+i) = T_desc_l.col(i);
        }
        std::cout << "S,T descriptors at time 0--------" << std::endl;
        std::cout << S_desc_l.col(0) << std::endl;//heat kernel at timestap i//todo: check at which point for S and T manifolds
        std::cout << T_desc_l.col(0) << std::endl;//heat kernel at timestap i
    }
    //----Add operator constratins
    //

    // solve least-square
    C = (F.transpose()*F).fullPivLu().solve(F.transpose()*G);
    //std::cout << "F: \n" << F.rows() << F.cols() << std::endl;
    //std::cout << "\nG: \n" << G.rows() << G.cols() << std::endl;

    std::cout << "-----------\n"
              << "C(10x10) \n" 
              << C.block(0,0,nShow,nShow) 
              << std::endl;

    std::cout << "----------\n"
              << "Diagnoals\n"
              << C.diagonal().transpose() 
              << std::endl;

    // Test
    assert(nPW < pc_s.Area());
    int nTest = (int)pc_s.Area()-nPW;
    float error = 0;
    Eigen::VectorXf true_w, true_l, guess_w;
    for (int i=nPW; i< (int)pc_s.Area(); ++i ){
        tdp::f_landmark(pc_s, i, alpha2, option, true_w);
        true_l = (S_wl.transpose()*S_wl).fullPivLu().solve(S_wl.transpose()*true_w);
        guess_w = S_wl * (C*true_l);
        // tdp::Vector3fda true_l = tdp::projectToLocal(S_wl, true_w);
        // tdp::Vector3fda guess_w = tdp::projectToWorld(S_wl, C*true_l);
        error += (true_w - guess_w).squaredNorm(); //todo: 
    }
    error = std::sqrt(error/nTest); //rms
    std::cout << "Surface dim: " << pc_s.Area() << std::endl;
    std::cout << "N test points: " << nTest << std::endl;
    std::cout << "rms: " << error << std::endl;

}

void Test_samePc_diffSamples(int nSamples, std::string& option, bool showDecomposition){
//todo: std::option
    //todo: think about how to get the correspondances - sort?
    //    : change seed, get the two different spheres 

    tdp::ManagedHostImage<tdp::Vector3fda> pc_s = tdp::GetSimplePc();
    //tdp::ManagedHostImage<tdp::Vector3fda> pc_s;
    //    tdp::GetSphericalPc(pc_s, nSamples);

    tdp::ManagedHostImage<tdp::Vector3fda> pc_t;
    addGaussianNoise(pc_s, 0.1f, pc_t);

    int nEv = std::min(30, (int)(pc_s.Area()/2));//pc_s.Area()-2; //get ALL eigenvectors of L
    int knn = pc_s.Area(); // use all points as neighbors
    float eps = 1e-6;
    float alpha = 0.01; // param for calculating Laplacian of a surface
    float alpha2 = 0.1; // param for radial basis function construction

    int nPW = nEv;//nber of pointwise correspondences
    int nHKS = 0; //nber of heat kernel signature correspondences
    int nCst = nPW + nHKS;//pc_s.Area();
    
    //int nTest = pc_s.Area() - nPW;

    // build kd tree
    tdp::ANN ann_s, ann_t;
    ann_s.ComputeKDtree(pc_s);
    ann_t.ComputeKDtree(pc_t);

    // construct laplacian matrices
    Eigen::SparseMatrix<float> L_s(pc_s.Area(), pc_s.Area()),
                               L_t(pc_t.Area(), pc_t.Area());
    Eigen::MatrixXf S_wl(L_s.rows(),(int)nEv),//cols are evectors
                    T_wl(L_t.rows(),(int)nEv),
                    S_desc_w, T_desc_w,
                    S_desc_l, T_desc_l;
    Eigen::VectorXf S_evals((int)nEv), T_evals((int)nEv);


    L_s = tdp::getLaplacian(pc_s, ann_s, knn, eps, alpha);
    L_t = tdp::getLaplacian(pc_t, ann_t, knn, eps, alpha);
    tdp::decomposeLaplacian(L_s, nEv, S_evals, S_wl); //todo: check if size initialization is included
    tdp::decomposeLaplacian(L_t, nEv, T_evals, T_wl);

    std::cout << "Basis ---" << std::endl;
    std::cout << "n of evec: " << nEv << std::endl;
    std::cout << S_wl << std::endl;
    std::cout << "-----------------" << std::endl;
    std::cout << T_wl << std::endl;
    std::cout << "Evals ---" << std::endl;
    std::cout << S_evals.transpose() << std::endl;
    std::cout << T_evals.transpose() << std::endl;


    //--Construct function pairs
    Eigen::VectorXf f_w(pc_s.Area()), g_w(pc_t.Area()),
                    f_l((int)nEv), g_l((int)nEv);
    Eigen::MatrixXf F((int)nCst, (int)nEv), G((int)nCst, (int)nEv);
    Eigen::MatrixXf C((int)nEv, (int)nEv);

    // --construct F(data matrix) and G based on the correspondences

    for (int i=0; i< (int)nPW; ++i){
        tdp::f_landmark(pc_s, i, alpha2, option, f_w);
        tdp::f_landmark(pc_t, i, alpha2, option, g_w);

        f_l = (S_wl.transpose()*S_wl).fullPivLu().solve(S_wl.transpose()*f_w);
        g_l = (T_wl.transpose()*T_wl).fullPivLu().solve(T_wl.transpose()*g_w);
        //f_l = tdp::projectToLocal(S_wl, f_w);
        //g_l = tdp::projectToLocal(T_wl, g_w);

        F.row(i) = f_l;
        G.row(i) = g_l;
    }


    if (nHKS >0){
        //-----Add  heat kernel signatures as constraints
        std::cout << "CALCULATEING HKS ---" <<std::endl;
        S_desc_w = tdp::getHKS(S_wl,S_evals,nHKS);
        T_desc_w = tdp::getHKS(T_wl,T_evals,nHKS);
        S_desc_l = (S_wl.transpose()*S_wl).fullPivLu().solve(S_wl.transpose()*S_desc_w);
        T_desc_l = (T_wl.transpose()*T_wl).fullPivLu().solve(T_wl.transpose()*T_desc_w);
        //S_desc_l = tdp::projectToLocal(S_wl, S_desc_w); //columne is a feature
        //T_desc_l = tdp::projectToLocal(T_wl, T_desc_w);
        
        assert(S_desc_l.cols() == nHKS);
        for (int i=0; i<nHKS; ++i){
          F.row(nPW+i) = S_desc_l.col(i);
          G.row(nPW+i) = T_desc_l.col(i);
        }
        std::cout << "S,T descriptors at time 0--------" << std::endl;
        std::cout << S_desc_l.col(0) << std::endl;//heat kernel at timestap i//todo: check at which point for S and T manifolds
        std::cout << T_desc_l.col(0) << std::endl;//heat kernel at timestap i
    }
    //----Add operator constratins
    //

    // solve least-square
    C = (F.transpose()*F).fullPivLu().solve(F.transpose()*G);
    //std::cout << "F: \n" << F.rows() << F.cols() << std::endl;
    //std::cout << "\nG: \n" << G.rows() << G.cols() << std::endl;
    std::cout << "\nC---------\n" << C << /*C.rows() << C.cols() <<*/ std::endl;

    // Test
    assert(nPW < pc_s.Area());
    int nTest = (int)pc_s.Area()-nPW;
    float error = 0;
    Eigen::VectorXf true_w, true_l, guess_w;
    for (int i=nPW; i< (int)pc_s.Area(); ++i ){
        tdp::f_landmark(pc_s, i, alpha2, option, true_w);
        true_l = (S_wl.transpose()*S_wl).fullPivLu().solve(S_wl.transpose()*true_w);
        guess_w = S_wl * (C*true_l);
        // tdp::Vector3fda true_l = tdp::projectToLocal(S_wl, true_w);
        // tdp::Vector3fda guess_w = tdp::projectToWorld(S_wl, C*true_l);
        error += (true_w - guess_w).squaredNorm();
    }
    error = std::sqrt(error/nTest);
    std::cout << "vector length: " << pc_s.Area() << std::endl;
    std::cout << "nber of test points: " << nTest << std::endl;
    std::cout << "error: " << error << std::endl;

}

