#ifndef GLWIDGET_H
#define GLWIDGET_H
#include <QTimer>
#define NOMINMAX
#include <memory>
#include "QtOpenGL/QGLWidget"
#include "SliceModel.h"
#include "boost/ptr_container/ptr_vector.hpp"
#include "tracking/region/RegionModel.h"
#include "libs/coreg/linear.hpp"
#include "tracking/tracking_window.h"
class RenderingTableWidget;
 class GLWidget : public QGLWidget
 {
     Q_OBJECT

 public:
     GLWidget(bool samplebuffer,
              tracking_window& cur_tracking_window_,
              RenderingTableWidget* renderWidget_,
              QWidget *parent = 0);
     ~GLWidget();
 public:// editing
     unsigned char editing_option;
     float mat[16];
     image::vector<3,float> pos,dir1,dir2;
     bool angular_selection;
     void get_pos(void);
     void set_view(unsigned char view_option);
     void scale_by(float scale);
     void move_by(int x,int y);
 private:
     bool object_selected,slice_selected;
     image::vector<3,float> accumulated_dis;
     float slice_distance;
     unsigned char moving_at_slice_index;
     float slice_dx,slice_dy;
     void slice_location(unsigned char dim,std::vector<image::vector<3,float> >& points);
     float get_slice_projection_point(unsigned char dim,
                                      const image::vector<3,float>& pos,const image::vector<3,float>& dir,
                                      float& dx,float& dy);
     void get_view_dir(QPoint p,image::vector<3,float>& dir);
     void select_slice(void);
 private:
     int selected_index;
     float object_distance;
     void select_object(void);
 public:// other slices
     std::auto_ptr<QTimer> timer;
     boost::ptr_vector<CustomSliceModel> other_slices;
     boost::ptr_vector<LinearMapping<image::const_pointer_image<float,3> > > mi3s;
     std::vector<std::vector<float> > transform;

     boost::ptr_vector<image::basic_image<float,3> > roi_image;
     std::vector<float*> roi_image_buf;

     unsigned int current_visible_slide;
     bool addSlices(QStringList filenames);
     void delete_slice(int index);
     const image::geometry<3>& getCurrentGeo(void) const
     {return other_slices[current_visible_slide-1].geometry;}
     void get_current_slice_transformation(image::geometry<3>& geo,
                                           image::vector<3,float>& vs,
                                           std::vector<float>& tr);
     bool get_mouse_pos(QMouseEvent *mouseEvent,image::vector<3,float>& position);
 private://surface
     std::auto_ptr<RegionModel> surface;

 private://odf
     std::vector<image::vector<3,float> >odf_points;
     std::vector<float>odf_colors;
     unsigned char odf_dim;
     unsigned char odf_slide_pos;
     void add_odf(image::pixel_index<3> pos);

 public slots:
     void makeTracts(void);
     void addSurface(void);
     void catchScreen(void);
     void catchScreen2(void);
     void saveCamera(void);
     void loadCamera(void);
     void saveRotationSeries(void);
     void saveLeftRight3DImage(void);
     void save3ViewImage(void);
     void saveMapping(void);
     void loadMapping(void);
     void copyToClipboard(void);
     void check_reg(void);
 signals:
     void edited(void);
     void region_edited(void);
 protected:
     void initializeGL();
     void paintGL();
     void resizeGL(int width, int height);
     void mousePressEvent(QMouseEvent *event);
     void mouseReleaseEvent(QMouseEvent *event);
     void mouseMoveEvent(QMouseEvent *event);
     void mouseDoubleClickEvent(QMouseEvent *event);
     void wheelEvent ( QWheelEvent * event );
 private:
     tracking_window& cur_tracking_window;
     RenderingTableWidget* renderWidget;
     int width,height;
     float max_fa;

 private:
     int get_param(const char* name);
     float get_param_float(const char* name);
 private:
     float tract_alpha;
     unsigned char scale_voxel;
     unsigned char tract_alpha_style;
     unsigned char tract_style;
     unsigned char tract_color_style;
     float tube_diameter;
     unsigned char tract_color_contrast;
     unsigned char tract_tube_detail;
     unsigned char end_point_shift;
     unsigned char odf_position;
     unsigned char odf_skip;
     float odf_scale;
     float slice_contrast;
     float slice_offset;
     unsigned char slice_index;
 public:
     GLuint tracts,slice_texture[3];
     QPoint lastPos;
     float transformation_matrix[16];
     float current_scale;
     bool set_view_flip;

 };

#endif // GLWIDGET_H
