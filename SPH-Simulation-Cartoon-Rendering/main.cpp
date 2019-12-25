//Compute shader particle demo from
// https://github.com/cirosantilli/OpenGL_ParticleSystem/tree/ubuntu-linux-16-04


#include <iostream>
#include <stdexcept>

#include "ConfigLoader.hpp"
#include "ParticleSystem.hpp"


int main(){
  
  try{
    Config config;
    config.loadConfig();
    
    ParticleSystem ps(config);
    
    std::cout << "Initializing Particle System" <<std::endl;
    ps.initialize();
    std::cout << "Executing Particle System" <<std::endl;
    ps.run();
  }
  catch(std::exception& e){
    std::cout << e.what() <<std::endl;
    std::cin.get();
    return -1;
  }
  
  return 0;
}