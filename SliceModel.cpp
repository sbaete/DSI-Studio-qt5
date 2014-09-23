// ---------------------------------------------------------------------------
#include <string>
#include "SliceModel.h"
#include "prog_interface_static_link.h"
#include "fib_data.hpp"
// ---------------------------------------------------------------------------
SliceModel::SliceModel(void):cur_dim(2)
{
    slice_visible[0] = false;
    slice_visible[1] = false;
    slice_visible[2] = false;
    texture_need_update[0] = true;
    texture_need_update[1] = true;
    texture_need_update[2] = true;
}

FibSliceModel::FibSliceModel(FibData* handle_):handle(handle_)
{
    // already setup the geometry and source image
    FibData& fib_data = *handle;
    geometry = fib_data.dim;
    voxel_size = fib_data.vs;
    source_images = image::make_image(geometry,fib_data.fib.fa[0]);
    center_point = geometry;
    center_point -= 1.0;
    center_point /= 2.0;
    slice_pos[0] = geometry.width() >> 1;
    slice_pos[1] = geometry.height() >> 1;
    slice_pos[2] = geometry.depth() >> 1;
    //loadImage(fib_data.fib.fa[0],false);
}
// ---------------------------------------------------------------------------
float FibSliceModel::get_value_range(void) const
{
    std::pair<float,float> value = image::min_max_value(source_images.begin(),source_images.end());
    return value.second-value.first;
}
// ---------------------------------------------------------------------------
void FibSliceModel::get_slice(image::color_image& show_image,float contrast,float offset) const
{
    handle->get_slice(view_name,overlay_name, cur_dim, slice_pos[cur_dim],show_image,contrast,offset);
}
// ---------------------------------------------------------------------------
void FibSliceModel::get_mosaic(image::color_image& show_image,
                               unsigned int mosaic_size,
                               float contrast,
                               float offset,
                               unsigned int skip) const
{
    unsigned slice_num = geometry[2] >> skip;
    show_image.clear();
    show_image.resize(image::geometry<2>(geometry[0]*mosaic_size,
                                          geometry[1]*(std::ceil((float)slice_num/(float)mosaic_size))));
    for(unsigned int z = 0;z < slice_num;++z)
    {
        image::color_image slice_image;
        handle->get_slice(view_name,overlay_name,2, z << skip,slice_image,contrast,offset);
        image::vector<2,int> pos(geometry[0]*(z%mosaic_size),
                                 geometry[1]*(z/mosaic_size));
        image::draw(slice_image,show_image,pos);
    }
}
// ---------------------------------------------------------------------------
void CustomSliceModel::init(void)
{
    geometry = source_images.geometry();
    slice_pos[0] = geometry.width() >> 1;
    slice_pos[1] = geometry.height() >> 1;
    slice_pos[2] = geometry.depth() >> 1;
    min_value = *std::min_element(source_images.begin(),source_images.end());
    max_value = *std::max_element(source_images.begin(),source_images.end());
    scale = max_value-min_value;
    if(scale != 0.0)
        scale = 255.0/scale;
}
// ---------------------------------------------------------------------------
float CustomSliceModel::get_value_range(void) const
{
    std::pair<float,float> value = image::min_max_value(source_images.begin(),source_images.end());
    return value.second-value.first;
}
// ---------------------------------------------------------------------------
void CustomSliceModel::get_slice(image::color_image& show_image,float contrast,float offset) const
{
    image::basic_image<float,2> buf;
    image::reslicing(source_images, buf, cur_dim, slice_pos[cur_dim]);
    show_image.resize(buf.geometry());
    buf += offset-min_value;
    if(contrast != 0.0)
        buf *= 255.99/contrast;
    image::upper_lower_threshold(buf,(float)0.0,(float)255.0);
    std::copy(buf.begin(),buf.end(),show_image.begin());
}
