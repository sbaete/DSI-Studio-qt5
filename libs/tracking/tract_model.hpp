#ifndef TRACT_MODEL_HPP
#define TRACT_MODEL_HPP
#include <vector>
#include <iosfwd>
#include "image/image.hpp"

class ODFModel;
class fiber_orientations;
struct connectivity_info{
    unsigned int count;
    std::vector<unsigned int> length;
    connectivity_info(void):count(0){}
    void add(const std::vector<float>& track)
    {
        ++count;
        length.push_back(track.size());
    }
public:

};

class TractModel{
public:
    std::string report;
private:
        ODFModel* handle;
        image::geometry<3> geometry;
        image::vector<3> vs;
        std::auto_ptr<fiber_orientations> fib;
private:
        std::vector<std::vector<float> > tract_data;
        std::vector<std::vector<float> > deleted_tract_data;
        std::vector<unsigned int> tract_color;
        std::vector<unsigned int> deleted_tract_color;
        std::vector<unsigned int> deleted_count;
        std::vector<std::pair<unsigned int,unsigned int> > redo_size;
        // offset, size
private:
        // for loading multiple clusters
        std::vector<unsigned int> tract_cluster;
public:
        static bool save_all(const char* file_name,const std::vector<TractModel*>& all);
        const std::vector<unsigned int>& get_cluster_info(void) const{return tract_cluster;}
        std::vector<unsigned int>& get_cluster_info(void) {return tract_cluster;}
private:
        void select(float select_angle,
                    const image::vector<3,float>& from_dir,const image::vector<3,float>& to_dir,
                    const image::vector<3,float>& from_pos,std::vector<unsigned int>& selected);
        // selection
        void delete_tracts(const std::vector<unsigned int>& tracts_to_delete);
        void select_tracts(const std::vector<unsigned int>& tracts_to_select);
public:
        TractModel(ODFModel* handle_);
        const TractModel& operator=(const TractModel& rhs)
        {
            geometry = rhs.geometry;
            vs = rhs.vs;
            handle = rhs.handle;
            tract_data = rhs.tract_data;
            tract_color = rhs.tract_color;
            report = rhs.report;
            return *this;
        }
        const fiber_orientations& get_fib(void) const{return *fib.get();}
        fiber_orientations& get_fib(void){return *fib.get();}
        void add(const TractModel& rhs);
        bool load_from_file(const char* file_name,bool append = false);

        bool save_fa_to_file(const char* file_name);
        bool save_tracts_to_file(const char* file_name);
        bool save_transformed_tracts_to_file(const char* file_name,const float* transform,bool end_point);
        bool save_data_to_file(const char* file_name,const std::string& index_name);
        void save_end_points(const char* file_name) const;

        bool load_tracts_color_from_file(const char* file_name);
        bool save_tracts_color_to_file(const char* file_name);


        void release_tracts(std::vector<std::vector<float> >& released_tracks);
        void add_tracts(std::vector<std::vector<float> >& new_tracks);
        void cull(float select_angle,
                  const image::vector<3,float>& from_dir,
                  const image::vector<3,float>& to_dir,
                  const image::vector<3,float>& from_pos,
                  bool delete_track);
        void cut(float select_angle,const image::vector<3,float>& from_dir,const image::vector<3,float>& to_dir,
                  const image::vector<3,float>& from_pos);
        void paint(float select_angle,const image::vector<3,float>& from_dir,const image::vector<3,float>& to_dir,
                  const image::vector<3,float>& from_pos,
                  unsigned int color);
        void set_color(unsigned int color){std::fill(tract_color.begin(),tract_color.end(),color);}
        void set_tract_color(unsigned int index,unsigned int color){tract_color[index] = color;}
        void cut_by_mask(const char* file_name);
        void undo(void);
        void redo(void);
        void trim(void);


        void get_end_points(std::vector<image::vector<3,short> >& points);
        void get_tract_points(std::vector<image::vector<3,short> >& points);

        unsigned int get_deleted_track_count(void) const{return deleted_tract_data.size();}
        unsigned int get_visible_track_count(void) const{return tract_data.size();}
        
        const std::vector<float>& get_tract(unsigned int index) const{return tract_data[index];}
        const std::vector<std::vector<float> >& get_tracts(void) const{return tract_data;}
        unsigned int get_tract_color(unsigned int index) const{return tract_color[index];}
        unsigned int get_tract_length(unsigned int index) const{return tract_data[index].size();}
        void get_density_map(image::basic_image<unsigned int,3>& mapping,
             const std::vector<float>& transformation,bool endpoint);
        void get_density_map(image::basic_image<image::rgb_color,3>& mapping,
             const std::vector<float>& transformation,bool endpoint);
        void save_tdi(const char* file_name,bool sub_voxel,bool endpoint);

        void get_quantitative_data(std::vector<float>& data);
        void get_quantitative_info(std::string& result);
        void get_report(unsigned int profile_dir,float band_width,const std::string& index_name,
                        std::vector<float>& values,
                        std::vector<float>& data_profile);

public:


        void get_tract_data(unsigned int fiber_index,
                            unsigned int index_num,
                            std::vector<float>& data);
        void get_tracts_data(
                const std::string& index_name,
                std::vector<std::vector<float> >& data);

        void get_tract_fa(unsigned int fiber_index,std::vector<float>& data);
        void get_tracts_fa(std::vector<std::vector<float> >& data);
        double get_spin_volume(void);
public:

        void get_connectivity_matrix(const std::vector<std::vector<image::vector<3,short> > >& regions,
                                     std::vector<std::vector<connectivity_info> >& matrix,
                                     bool use_end_only) const;

};




class atlas;
class ConnectivityMatrix{
public:
    std::vector<std::vector<connectivity_info> > matrix;
    std::vector<unsigned int> connectivity_count;
    std::vector<float> tract_median_length;
    std::vector<float> tract_mean_length;
public:
    typedef std::map<float,std::pair<std::vector<image::vector<3,short> >,std::string> > region_table_type;
    std::vector<std::vector<image::vector<3,short> > > regions;
    std::vector<std::string> region_name;
    void set_atlas(const atlas& data,const image::basic_image<image::vector<3,float>,3 >& mni_position);
    void set_regions(const region_table_type& region_table);
public:
    void save_to_image(image::color_image& cm,bool log,bool norm);
    void save_to_file(const char* file_name);
    void calculate(const TractModel& tract_model,bool use_end_only);
};




#endif//TRACT_MODEL_HPP
