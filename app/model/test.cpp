#include "mlpack.hpp"

using namespace arma;
using namespace mlpack;

int main()
{
    RandomForest<GiniGain, RandomDimensionSelect> rf;
    mlpack::data::Load("mnq_10_bc0.bin", "model", rf);

    double x[5] = {-0.00141099, -0.00158099, -0.0031739 , -0.00017024,  0.00077286};
    
    size_t pred;
    arma::vec prob;
    arma::vec x_pred(x, 5, false, true);
    
    rf.Classify(x_pred, pred, prob);
    
    double p_score = prob.at(1);
    std::cout << p_score << " | " << pred << std::endl;
    std::cout << prob.n_rows << " | " <<  prob.n_cols << std::endl;

    return 0;
}