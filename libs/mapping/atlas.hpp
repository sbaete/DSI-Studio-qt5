#ifndef ATLAS_HPP
#define ATLAS_HPP
#include "image/image.hpp"
#include <vector>
#include <string>
class atlas{
private:
    image::basic_image<short,3> I;
    std::vector<short> label_num;
    std::vector<std::string> labels;
    std::vector<float> transform;

private:// for talairach only
    std::vector<std::vector<unsigned int> > index2label;
    std::vector<std::vector<unsigned int> > label2index;

public:
    std::string name;
public:
    bool load_from_file(const char* file_name);
    const std::vector<std::string>& get_list(void) const{return labels;}
    const std::vector<short>& get_num(void) const{return label_num;}
    short get_label_at(const image::vector<3,float>& mni_space)  const;
    std::string get_label_name_at(const image::vector<3,float>& mni_space) const;
    bool is_labeled_as(const image::vector<3,float>& mni_space,short label) const;
    bool label_matched(short image_label,short region_label) const;
public:
    void calculate_order(std::vector<float>& order) const;
};

#endif // ATLAS_HPP
