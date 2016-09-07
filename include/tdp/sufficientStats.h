
#include <tdp/eigen/dense.h>
#include <tdp/image.h>

namespace tdp {

Eigen::Vector4fda SufficientStats1stOrder(const Image<Eigen::Vector3f>& I);
Eigen::Matrix<float,4,Eigen::Dynamic, Eigen::DontAlign> SufficientStats1stOrder(
    const Image<Eigen::Vector3f>& I, const Image<uint16_t> z, uint16_t K);

}
