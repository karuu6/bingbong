#include <iostream>
#include "mlpack.hpp"

using namespace arma;
using namespace mlpack;

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "usage: " << argv[0] << " <data csv> <model dest>" << std::endl;
        return 1;
    }

    mat dataset;
    bool loaded = mlpack::data::Load(argv[1], dataset);
    if (!loaded)
        return -1;

    Row<size_t> labels;
    labels = conv_to<Row<size_t>>::from(dataset.row(dataset.n_rows - 1));
    dataset.shed_row(dataset.n_rows - 1);

    const size_t numTrees = 100;
    const size_t numClasses = 2;
    const size_t minimumLeafSize = 1;

    RandomForest<GiniGain, RandomDimensionSelect> rf;
    rf = RandomForest<GiniGain, RandomDimensionSelect>(
        dataset, labels, numClasses, numTrees, minimumLeafSize
    );

    mlpack::data::Save(argv[2], "model", rf, false);

    return 0;
}