//
//  Experiments.hpp
//  Controller
//
//  Created by FireWolf on 11/30/20.
//

#ifndef Experiments_hpp
#define Experiments_hpp

#include <functional>
#include <chrono>
#include <vector>
#include <numeric>
#include <thread>

/// Measures the execution time of a function call
struct ExecutionTimeMeasurer
{
    /// Represents the experiment result
    struct Result
    {
        /// A vector that stores the execution time in nanoseconds of each trial
        std::vector<uint64_t> durations;

        ///
        /// Create a new experiment result
        ///
        /// @param size The number of trails
        ///
        explicit Result(size_t size)
        {
            this->durations.reserve(size);
        }

        /// Get the minimum execution time
        [[nodiscard]] uint64_t min() const
        {
            return *std::min_element(this->durations.begin(), this->durations.end());
        }

        /// Get the maximum execution time
        [[nodiscard]] uint64_t max() const
        {
            return *std::max_element(this->durations.begin(), this->durations.end());
        }

        /// Get the average execution time
        [[nodiscard]] double mean() const
        {
            double sum = std::accumulate(this->durations.begin(), this->durations.end(), 0.0);

            return sum / static_cast<double>(this->durations.size());
        }

        /// Get the standard deviation of the execution time
        [[nodiscard]] double sd() const
        {
            double avg = this->mean();

            std::vector<double> differences(this->durations.size());

            std::transform(this->durations.begin(), this->durations.end(), differences.begin(), [avg](double x) { return x - avg; });

            double sq_sum = std::inner_product(differences.begin(), differences.end(), differences.begin(), 0.0);

            return std::sqrt(sq_sum / static_cast<double>(this->durations.size()));
        }

        /// Get the medium execution time
        [[nodiscard]] uint64_t medium() const
        {
            auto tmp = this->durations;

            std::sort(tmp.begin(), tmp.end());

            return tmp[this->durations.size() / 2];
        }
    };

    ///
    /// Measure the execution time of a function call
    ///
    /// @param trials Specify the number of trials to invoke the given function
    /// @param delay Specify the amount of time to wait until the next function invocation
    /// @param func A callable function
    /// @param args Zero or more arguments passed to the function
    /// @return The experiment result.
    ///
    template <typename Representation, typename Period, typename Func, typename... Args>
    Result operator()(size_t trials, const std::chrono::duration<Representation, Period>& delay, Func&& func, Args&&... args)
    {
        // Allocate the storage to record the execution time of each trial
        Result result(trials);

        // Run the experiment
        for (size_t trial = 0; trial < trials; trial += 1)
        {
            // Measure the execution time
            auto start = std::chrono::high_resolution_clock::now();

            std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);

            auto end = std::chrono::high_resolution_clock::now();

            // Store the execution time
            result.durations.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());

            // Wait for a while until the next experiment
            std::this_thread::sleep_for(delay);
        }

        // All done
        return result;
    }
};

/// Measures the execution time of a function call and returns the result
struct ExecutionTimeMeasurerWithResult
{
    ///
    /// Measure the execution time of a function call
    ///
    /// @param trials Specify the number of trials to invoke the given function
    /// @param func A callable function
    /// @param args Zero or more arguments passed to the function
    /// @return The medium execution time along with the invocation result.
    ///
    template <typename Func, typename... Args>
    std::pair<uint64_t, std::invoke_result_t<Func, Args...>> operator()(size_t trials, Func&& func, Args&&... args)
    {
        return std::make_pair(ExecutionTimeMeasurer{}(trials, std::forward<Func>(func), std::forward<Args>(args)...).medium(),
                              std::invoke(std::forward<Func>(func), std::forward<Args>(args)...));
    }
};

#endif /* Experiments_hpp */
