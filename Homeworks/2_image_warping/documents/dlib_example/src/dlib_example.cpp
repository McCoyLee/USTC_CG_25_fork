#include <dlib/dnn.h>
#include <iostream>
#include <vector>
#include <dlib/statistics.h>

using namespace std;
using namespace dlib;

int main()
{
    try {
        // 1. Network architecture
        using net_type = loss_mean_squared_multioutput<
                               fc<2,
                               relu<fc<100,
                               relu<fc<50,
                               input<matrix<double>>
                               >>>>>>;

        // 2. Generate synthetic data
        const int num_samples = 500;
        dlib::rand rnd;
        std::vector<matrix<double>> inputs;
        std::vector<matrix<float>> outputs;

        for (int i = 0; i < num_samples; ++i) {
            matrix<double> input(2,1);
            input(0,0) = rnd.get_random_double()*4 - 2;
            input(1,0) = rnd.get_random_double()*4 - 2;
            
            matrix<float> output(2,1);
            output(0,0) = sin(input(0)) + 0.5*cos(input(1)) + 0.01*rnd.get_random_gaussian();
            output(1,0) = 0.5*input(0)*input(1) + 0.2*rnd.get_random_gaussian();
            
            inputs.push_back(input);
            outputs.push_back(output);
        }

        // 3. Split dataset
        auto train_inputs = std::vector<matrix<double>>(inputs.begin(), inputs.begin()+400);
        auto train_outputs = std::vector<matrix<float>>(outputs.begin(), outputs.begin()+400);
        auto test_inputs = std::vector<matrix<double>>(inputs.begin()+400, inputs.end());
        auto test_outputs = std::vector<matrix<float>>(outputs.begin()+400, outputs.end());

        // 4. Data normalization
        vector_normalizer<matrix<double>> input_normalizer;
        input_normalizer.train(train_inputs);
        for (auto& x : train_inputs) x = input_normalizer(x);
        for (auto& x : test_inputs) x = input_normalizer(x);

        vector_normalizer<matrix<float>> output_normalizer;
        output_normalizer.train(train_outputs);
        auto output_means = output_normalizer.means();
        auto output_stds = output_normalizer.std_devs();
        for (auto& x : train_outputs) x = output_normalizer(x);
        for (auto& x : test_outputs) x = output_normalizer(x);

        // 5. Initialize network and trainer
        net_type net;
        dnn_trainer<net_type, adam> trainer(net);

        trainer.set_learning_rate(0.001);
        trainer.set_min_learning_rate(1e-6);
        trainer.set_mini_batch_size(128);
        trainer.set_iterations_without_progress_threshold(100);
        trainer.be_verbose();

        // 6. Training process
        cout << "Starting training..." << endl;
        trainer.train(train_inputs, train_outputs);

        // 7. Denormalization function
        auto denormalize = [&](matrix<float>& x) {
            for (long c = 0; c < x.nc(); ++c) {
                x(0,c) = x(0,c)*output_stds(c) + output_means(c);
            }
        };

        // 8. Training set evaluation
        double train_mse = 0;
        auto predictions = net(train_inputs);
        for (size_t i = 0; i < train_inputs.size(); ++i) {
            denormalize(predictions[i]);
            denormalize(train_outputs[i]);
            train_mse += length_squared(predictions[i] - train_outputs[i]);
        }
        train_mse /= train_inputs.size();

        // 9. Test set evaluation
        double test_mse = 0;
        auto test_predictions = net(test_inputs);
        for (size_t i = 0; i < test_inputs.size(); ++i) {
            denormalize(test_predictions[i]);
            denormalize(test_outputs[i]);
            test_mse += length_squared(test_predictions[i] - test_outputs[i]);
        }
        test_mse /= test_inputs.size();

        // 10. Display results
        cout << "\nFinal Results:" << endl
             << "Training MSE: " << train_mse << endl
             << "Test MSE:     " << test_mse << endl;

        // 11. Prediction samples
        cout << "\nSample Predictions:" << endl;
        for (size_t i = 0; i < 5; ++i) {
            auto in = test_inputs[i];
            auto true_out = test_outputs[i];
            auto pred_out = net(in);

            denormalize(true_out);
            denormalize(pred_out);

            cout << "Input: [" << in(0,0) << ", " << in(1,0) << "]\n"
                 << "True:  [" << true_out(0,0) << ", " << true_out(1,0) << "]\n"
                 << "Pred:  [" << pred_out(0,0) << ", " << pred_out(1,0) << "]\n"
                 << "Error: [" << pred_out(0,0)-true_out(0,0) 
                 << ", " << pred_out(1,0)-true_out(1,0) << "]\n\n";
        }
    }
    catch(std::exception& e) {
        cout << "Error: " << e.what() << endl;
    }
}