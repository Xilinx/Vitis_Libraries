#include "svd_top.hpp"
void svd_top(double dataA_reduced[4][4],
             double sigma[4][4],
             double dataU_reduced[4][4],
             double dataV_reduced[4][4],
             int diagSize1) {
    xf::fintech::svd<double, 4>(dataA_reduced, sigma, dataU_reduced, dataV_reduced);
}
