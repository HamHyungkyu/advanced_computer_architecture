#include <string>

class ndp_config {
  public:
    ndp_config(std::string config_file);
  private:
    int packet_size;
    int subtensor_size;
};