#include "mlpack.hpp"
#include <random>



using namespace arma;
using namespace mlpack;

#define FWD 10
#define LKB 10


int main()
{
    double x[100][2];

    double lower_bound = 0;
    double upper_bound = 1;
    std::uniform_real_distribution<double> unif(lower_bound,upper_bound);
    std::default_random_engine re;

    for (size_t i = 0; i < 100; i++) {
        x[i][0] = unif(re);
        x[i][1] = unif(re);
    }

    unsigned int ix = 19;
    std::vector<size_t> vec_labels;
    for (unsigned int j = ix - 9; j <= ix; j++)
    {
        vec_labels.push_back(rand() % 2);
    }
    arma::Row<size_t> labels(vec_labels);
    arma::mat dataset(&x[ix - 9 - FWD][0], 10, 2, false, true);

    const size_t numTrees = 100;
    const size_t numClasses = 2;
    const size_t minimumLeafSize = 1;

    RandomForest<GiniGain, RandomDimensionSelect> rf1, rf2;
    rf1 = RandomForest<GiniGain, RandomDimensionSelect>(
        dataset, labels, numClasses, numTrees, minimumLeafSize
    );

    dataset = dataset.t();
    rf2 = RandomForest<GiniGain, RandomDimensionSelect>(
        dataset, labels, numClasses, numTrees, minimumLeafSize
    );

    double s[2] = {-0.00141099, -0.00158099};
    
    size_t pred;
    arma::vec prob;
    arma::vec x_pred(s, 2, false, true);

    rf1.Classify(x_pred, pred, prob);
    double p_score = prob.at(1);
    std::cout << p_score << " | " << pred << std::endl;

    rf2.Classify(x_pred, pred, prob);
    p_score = prob.at(1);
    std::cout << p_score << " | " << pred << std::endl;

    
    
    // RandomForest<GiniGain, RandomDimensionSelect> rf;
    // mlpack::data::Load("mnq_10_bc0.bin", "model", rf);

    // double x[5] = {-0.00141099, -0.00158099, -0.0031739 , -0.00017024,  0.00077286};
    
    // size_t pred;
    // arma::vec prob;
    // arma::vec x_pred(x, 5, false, true);
    
    // rf.Classify(x_pred, pred, prob);
    
    // double p_score = prob.at(1);
    // std::cout << p_score << " | " << pred << std::endl;
    // std::cout << prob.n_rows << " | " <<  prob.n_cols << std::endl;

    return 0;
}