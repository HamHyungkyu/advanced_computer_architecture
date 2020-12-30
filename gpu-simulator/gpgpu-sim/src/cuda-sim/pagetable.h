#ifndef pagetable_h_included
#define pagetable_h_included


class pagetable_element {
  public:


  private: 
  unsigned addr;
  size_t size;
  bool is_read_only;
  bool is_dirty;

}


#endif