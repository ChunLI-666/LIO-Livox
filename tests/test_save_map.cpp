#include <iostream>
#include <memory>
#include "Estimator/Estimator.h"

int main() {
    std::cout << "Testing map save functionality..." << std::endl;
    
    // Create estimator instance
    auto estimator = std::make_shared<Estimator>(0.2, 0.4);
    
    // Test saving empty map
    std::string output_dir = "/home/charles/project/LIO-Livox/mapping_results";
    estimator->saveMapToPCD(output_dir);
    
    std::cout << "Test completed!" << std::endl;
    return 0;
}
