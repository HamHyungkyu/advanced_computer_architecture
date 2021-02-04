class simd_controller {
  public:
    void init();
    void cycle();
  
  private:
    int clk;
    float write_high_watermark;
    float write_low_watermark;
};
