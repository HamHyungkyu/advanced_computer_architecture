#include <string>

class ndp_config {
  public:
    ndp_config(std::string config_file);
  private:
    //SIMD config
    int num_simd_unit;
    int simd_width;
    
    int packet_size;
    int subtensor_size;
};