# -------------------------------------------------
# Project created by QtCreator 2011-01-20T20:02:59
# -------------------------------------------------
QT += core \
    gui \
    opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport
#CONFIG += console
TARGET = dsi_studio
TEMPLATE = app

win32* {
#change to the directory that contains boost library
INCLUDEPATH += C:/frank/myprog/include
}

win32-g++ {

LIBS += -L. -lboost_thread-mgw45-mt-1_45.dll \
     -L. -lboost_program_options-mgw45-mt-1_45.dll
}

linux* {
QMAKE_CXXFLAGS += -fpermissive
LIBS += -lboost_thread \
        -lboost_program_options \
        -lGLU \
        -lz \
        -lboost_system
}

mac{

#change to the directory that contains boost library
INCLUDEPATH += /Users/frank/boost/include
LIBS += -L/Users/frank/boost/lib -lboost_system \
        -L/Users/frank/boost/lib -lboost_thread \
        -L/Users/frank/boost/lib -lboost_program_options
}


# you may need to change the include directory of boost library
INCLUDEPATH += libs \
    libs/dsi \
    libs/tracking \
    libs/mapping
HEADERS += mainwindow.h \
    dicom/dicom_parser.h \
    dicom/dwi_header.hpp \
    libs/dsi/tessellated_icosahedron.hpp \
    libs/dsi/space_mapping.hpp \
    libs/dsi/sh_process.hpp \
    libs/dsi/sample_model.hpp \
    libs/dsi/racian_noise.hpp \
    libs/dsi/qbi_process.hpp \
    libs/dsi/odf_process.hpp \
    libs/dsi/odf_deconvolusion.hpp \
    libs/dsi/odf_decomposition.hpp \
    libs/dsi/mix_gaussian_model.hpp \
    libs/dsi/layout.hpp \
    libs/dsi/image_model.hpp \
    libs/dsi/gqi_process.hpp \
    libs/dsi/gqi_mni_reconstruction.hpp \
    libs/dsi/dti_process.hpp \
    libs/dsi/dsi_process.hpp \
    libs/dsi/basic_voxel.hpp \
    libs/dsi_interface_static_link.h \
    SliceModel.h \
    tracking/tracking_window.h \
    reconstruction/reconstruction_window.h \
    tracking/slice_view_scene.h \
    opengl/glwidget.h \
    libs/tracking/tracking_method.hpp \
    libs/tracking/roi.hpp \
    libs/tracking/interpolation_process.hpp \
    libs/tracking/fib_data.hpp \
    libs/tracking/basic_process.hpp \
    libs/tracking/tract_cluster.hpp \
    tracking/region/regiontablewidget.h \
    tracking/region/Regions.h \
    tracking/region/RegionModel.h \
    libs/tracking/tract_model.hpp \
    tracking/tract/tracttablewidget.h \
    opengl/renderingtablewidget.h \
    qcolorcombobox.h \
    libs/coreg/linear.hpp \
    libs/tracking/tracking_thread.hpp \
    libs/prog_interface_static_link.h \
    simulation.h \
    reconstruction/vbcdialog.h \
    libs/mapping/atlas.hpp \
    libs/mapping/fa_template.hpp \
    plot/qcustomplot.h \
    view_image.h \
    libs/vbc/vbc_database.h \
    libs/gzip_interface.hpp \
    libs/dsi/racian_noise.hpp \
    libs/dsi/mix_gaussian_model.hpp \
    libs/dsi/layout.hpp \
    manual_alignment.h \
    tracking/vbc_dialog.hpp \
    tracking/tract_report.hpp \
    tracking/color_bar_dialog.hpp \
    tracking/connectivity_matrix_dialog.h \
    tracking/atlasdialog.h \
    dicom/motion_dialog.hpp \
    ui_mainwindow.h \
    ui_dicom_parser.h \
    ui_view_image.h \
    ui_vbcdialog.h \
    ui_vbc_dialog.h \
    ui_tract_report.h \
    ui_tracking_window.h \
    ui_simulation.h \
    ui_reconstruction_window.h \
    ui_motion_dialog.h \
    ui_manual_alignment.h \
    ui_connectivity_matrix_dialog.h \
    ui_color_bar_dialog.h \
    ui_atlasdialog.h
FORMS += mainwindow.ui \
    tracking/tracking_window.ui \
    reconstruction/reconstruction_window.ui \
    dicom/dicom_parser.ui \
    simulation.ui \
    reconstruction/vbcdialog.ui \
    view_image.ui \
    manual_alignment.ui \
    tracking/vbc_dialog.ui \
    tracking/tract_report.ui \
    tracking/color_bar_dialog.ui \
    tracking/connectivity_matrix_dialog.ui \
    tracking/atlasdialog.ui \
    dicom/motion_dialog.ui
RESOURCES += \
    icons.qrc
SOURCES += main.cpp \
    mainwindow.cpp \
    dicom/dicom_parser.cpp \
    dicom/dwi_header.cpp \
    libs/utility/prog_interface.cpp \
    libs/dsi/sample_model.cpp \
    libs/dsi/dsi_interface_imp.cpp \
    libs/tracking/interpolation_process.cpp \
    libs/tracking/tract_cluster.cpp \
    SliceModel.cpp \
    tracking/tracking_window.cpp \
    reconstruction/reconstruction_window.cpp \
    tracking/slice_view_scene.cpp \
    opengl/glwidget.cpp \
    tracking/region/regiontablewidget.cpp \
    tracking/region/Regions.cpp \
    tracking/region/RegionModel.cpp \
    libs/tracking/tract_model.cpp \
    tracking/tract/tracttablewidget.cpp \
    opengl/renderingtablewidget.cpp \
    qcolorcombobox.cpp \
    cmd/trk.cpp \
    cmd/rec.cpp \
    simulation.cpp \
    gzlib/zutil.c \
    gzlib/uncompr.c \
    gzlib/trees.c \
    gzlib/inftrees.c \
    gzlib/inflate.c \
    gzlib/inffast.c \
    gzlib/infback.c \
    gzlib/gzwrite.c \
    gzlib/gzread.c \
    gzlib/gzlib.c \
    gzlib/gzclose.c \
    gzlib/deflate.c \
    gzlib/crc32.c \
    gzlib/compress.c \
    gzlib/adler32.c \
    reconstruction/vbcdialog.cpp \
    cmd/src.cpp \
    libs/mapping/atlas.cpp \
    libs/mapping/fa_template.cpp \
    plot/qcustomplot.cpp \
    cmd/ana.cpp \
    view_image.cpp \
    libs/vbc/vbc_database.cpp \
    manual_alignment.cpp \
    tracking/vbc_dialog.cpp \
    tracking/tract_report.cpp \
    tracking/color_bar_dialog.cpp \
    cmd/exp.cpp \
    tracking/connectivity_matrix_dialog.cpp \
    libs/dsi/tessellated_icosahedron.cpp \
    cmd/atl.cpp \
    tracking/atlasdialog.cpp \
    dicom/motion_dialog.cpp

OTHER_FILES += \
    options.txt \
    update_mac.sh \
    install_linux_red_hat.sh \
    dsi_studio.pro.user \
    dsi_studio.pro.user.b14b646.2.6pre1 \
    readme.txt \
    object_script.dsi_studio.Release \
    object_script.dsi_studio.Debug \
    dsi_studio.pro.user.2.5pre1 \
    dsi_studio.pro.user.2.1pre1 \
    INSTALL_LINUX.txt \
    COPYRIGHT \
    Makefile.Release \
    Makefile.Debug \
    Makefile
