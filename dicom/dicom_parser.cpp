#include <qmessagebox.h>
#include <QProgressDialog>
#include <QFileDialog>
#include <QSettings>
#include "dicom_parser.h"
#include "ui_dicom_parser.h"
#include "image/image.hpp"
#include "mainwindow.h"
#include "prog_interface_static_link.h"
#include "libs/gzip_interface.hpp"
#include "motion_dialog.hpp"

struct compare_qstring{
    bool operator()(const QString& lhs,const QString& rhs)
    {
        if(lhs.length() != rhs.length())
            return lhs.length() < rhs.length();
        return lhs < rhs;
    }
};

void get_report_from_dicom(const image::io::dicom& header,std::string& report_);

QString get_src_name(QString file_name)
{
    image::io::dicom header;
    if (header.load_from_file(file_name.toLocal8Bit().begin()))
    {
        std::string Person;
        header.get_patient(Person);
        return QFileInfo(file_name).absolutePath() + "/" + &*Person.begin() + ".src.gz";

    }
    else
        return file_name+".src.gz";
}



dicom_parser::dicom_parser(QStringList file_list,QWidget *parent) :
        QDialog(parent),
        ui(new Ui::dicom_parser)
{
    ui->setupUi(this);
    cur_path = QFileInfo(file_list[0]).absolutePath();
    std::sort(file_list.begin(),file_list.end(),compare_qstring());
    load_files(file_list);

    if (!dwi_files.empty())
    {
        ui->SrcName->setText(get_src_name(file_list[0]));
        image::io::dicom header;
        if (header.load_from_file(file_list[0].toLocal8Bit().begin()))
        {
            slice_orientation.resize(9);
            header.get_image_orientation(slice_orientation.begin());
        }
    }
}
void dicom_parser::set_name(QString name)
{
    ui->SrcName->setText(name);
}

dicom_parser::~dicom_parser()
{
    delete ui;
}

bool load_dicom_multi_frame(const char* file_name,boost::ptr_vector<DwiHeader>& dwi_files)
{
    image::io::dicom dicom_header;// multiple frame image
    if(!dicom_header.load_from_file(file_name))
        return false;
    {
        // Philips multiframe
        unsigned int slice_num = dicom_header.get_int(0x2001,0x102D);
        if(!slice_num)
            slice_num = 1;
        image::basic_image<unsigned short,3> buf_image;
        dicom_header >> buf_image;
        unsigned short num_gradient = buf_image.depth()/slice_num;
        unsigned int plane_size = buf_image.width()*buf_image.height();
        for(unsigned int index = 0;check_prog(index,num_gradient);++index)
        {
            std::auto_ptr<DwiHeader> new_file(new DwiHeader);
            if(index == 0)
                get_report_from_dicom(dicom_header,new_file->report);
            new_file->image.resize(image::geometry<3>(buf_image.width(),buf_image.height(),slice_num));

            for(unsigned int j = 0;j < slice_num;++j)
            std::copy(buf_image.begin()+(j*num_gradient + index)*plane_size,
                      buf_image.begin()+(j*num_gradient + index+1)*plane_size,
                      new_file->image.begin()+j*plane_size);
            // Philips use neurology convention. Now changes it to radiology
            image::flip_x(new_file->image);
            new_file->file_name = file_name;
            std::ostringstream out;
            out << index;
            new_file->file_name += out.str();
            dicom_header.get_voxel_size(new_file->voxel_size);
            dwi_files.push_back(new_file.release());
        }
    }
    return true;
}


void load_bvec(const char* file_name,std::vector<double>& b_table)
{
    std::ifstream in(file_name);
    if(!in)
        return;
    std::string line;
    unsigned int total_line = 0;
    while(std::getline(in,line))
    {
        std::istringstream read_line(line);
        std::copy(std::istream_iterator<double>(read_line),
                  std::istream_iterator<double>(),
                  std::back_inserter(b_table));
        ++total_line;
    }
    if(total_line == 3)
        image::matrix::transpose(b_table.begin(),image::dyndim(3,b_table.size()/3));
}
void load_bval(const char* file_name,std::vector<double>& bval)
{
    std::ifstream in(file_name);
    if(!in)
        return;
    std::copy(std::istream_iterator<double>(in),
              std::istream_iterator<double>(),
              std::back_inserter(bval));
}

bool load_4d_nii(const char* file_name,boost::ptr_vector<DwiHeader>& dwi_files)
{
    gz_nifti analyze_header;
    if(!analyze_header.load_from_file(file_name))
        return false;
    std::cout << "loading 4d nifti" << std::endl;


    std::vector<double> bvals,bvecs;
    {
        QString bval_name,bvec_name;
        bval_name = QFileInfo(file_name).absolutePath() + "/bvals";
        if(!QFileInfo(bval_name).exists())
            bval_name = QFileInfo(file_name).absolutePath() + "/bvals.txt";

        bvec_name = QFileInfo(file_name).absolutePath() + "/bvecs";
        if(!QFileInfo(bvec_name).exists())
            bvec_name = QFileInfo(file_name).absolutePath() + "/bvecs.txt";


        if(QFileInfo(bval_name).exists() && QFileInfo(bvec_name).exists())
        {
            load_bval(bval_name.toLocal8Bit().begin(),bvals);
            load_bvec(bvec_name.toLocal8Bit().begin(),bvecs);
            if(analyze_header.dim(4) != bvals.size() ||
               bvals.size()*3 != bvecs.size())
            {
                bvals.clear();
                bvecs.clear();
            }
            else
                std::cout << "bvals and bvecs loaded" << std::endl;
        }

    }

    image::basic_image<float,4> grad_dev;
    image::basic_image<unsigned char,3> mask;
    if(QFileInfo(QFileInfo(file_name).absolutePath() + "/grad_dev.nii.gz").exists())
    {
        gz_nifti grad_header;
        if(grad_header.load_from_file(QString(QFileInfo(file_name).absolutePath() + "/grad_dev.nii.gz").toLocal8Bit().begin()))
        {
            grad_header.toLPS(grad_dev);
            std::cout << "grad_dev used" << std::endl;
        }
    }
    if(QFileInfo(QFileInfo(file_name).absolutePath() + "/nodif_brain_mask.nii.gz").exists())
    {
        gz_nifti mask_header;
        if(mask_header.load_from_file(QString(QFileInfo(file_name).absolutePath() + "/nodif_brain_mask.nii.gz").toLocal8Bit().begin()))
        {
            mask_header.toLPS(mask);
            std::cout << "mask used" << std::endl;
        }
    }
    {
        float vs[4];
        analyze_header.get_voxel_size(vs);
        for(unsigned int index = 0;check_prog(index,analyze_header.dim(4));++index)
        {
            std::auto_ptr<DwiHeader> new_file(new DwiHeader);
            if(!analyze_header.toLPS(new_file->image,false))
                break;
            image::lower_threshold(new_file->image,0);
            new_file->file_name = file_name;
            std::ostringstream out;
            out << index;
            new_file->file_name += out.str();
            std::copy(vs,vs+3,new_file->voxel_size);
            if(!bvals.empty())
            {
                new_file->bvalue = bvals[index];
                new_file->bvec[0] = bvecs[index*3];
                new_file->bvec[1] = bvecs[index*3+1];
                new_file->bvec[2] = bvecs[index*3+2];
                new_file->bvec.normalize();
                if(new_file->bvalue < 10)
                {
                    new_file->bvalue = 0;
                    new_file->bvec = image::vector<3>(0,0,0);
                }
            }
            if(index == 0 && !grad_dev.empty())
                new_file->grad_dev.swap(grad_dev);
            if(index == 0 && !mask.empty())
                new_file->mask.swap(mask);
            dwi_files.push_back(new_file.release());
        }
    }

    return true;
}

bool load_4d_2dseq(const char* file_name,boost::ptr_vector<DwiHeader>& dwi_files)
{
    image::io::bruker_2dseq bruker_header;
    if(!bruker_header.load_from_file(file_name))
        return false;
    image::basic_image<float,4> buf_image;
    float vs[4]={0};
    image::io::bruker_info method_file;
    {
        bruker_header >> buf_image;
        bruker_header.get_voxel_size(vs);
        QString method_name = QFileInfo(QFileInfo(QFileInfo(file_name).
                absolutePath()).absolutePath()).absolutePath();
        method_name += "/method";
        if(method_file.load_from_file(method_name.toLocal8Bit().begin()))
        {
            std::vector<float> bvalues;
            std::istringstream bvalue(method_file["PVM_DwEffBval"]);
            std::copy(std::istream_iterator<float>(bvalue),
                      std::istream_iterator<float>(),
                      std::back_inserter(bvalues));
            if(buf_image.geometry()[3] == 1 && !bvalues.empty())
            {
                short dim[4];
                std::copy(buf_image.geometry().begin(),
                          buf_image.geometry().begin()+4,dim);
                dim[2] /= bvalues.size();
                vs[2] *= bvalues.size();
                dim[3] = bvalues.size();
                buf_image.resize(image::geometry<4>(dim));
            }
            if(vs[2] == 0.0)
            {
                std::istringstream slice_thick(method_file["PVM_SliceThick"]);
                slice_thick >> vs[2];
                if(vs[2] == 0.0)
                        vs[2] = vs[0];
            }
            if(bvalues.empty())
            {
                QMessageBox::information(0,"error",QString("Cannot find bvalue in method file at ") + method_name,0);
                return false;
            }
        }
        else
        {
            QMessageBox::information(0,"error",QString("Cannot find method file at ") + method_name,0);
            return false;
        }
    }
    if(dwi_files.size() && dwi_files.back().image.geometry() !=
            image::geometry<3>(buf_image.width(),buf_image.height(),buf_image.depth()))
        return false;
    image::lower_threshold(buf_image,0.0);
    image::normalize(buf_image,32767.0);
    std::istringstream bvalue(method_file["PVM_DwEffBval"]);
    std::istringstream bvec(method_file["PVM_DwGradVec"]);
    for (unsigned int index = 0;index < buf_image.geometry()[3];++index)
    {
        std::auto_ptr<DwiHeader> new_file(new DwiHeader);
        if(index == 0)
            new_file->report = "The diffusion images were acquired on a Bruker scanner.";
        new_file->image.resize(image::geometry<3>(buf_image.width(),buf_image.height(),buf_image.depth()));

        std::copy(buf_image.begin()+index*new_file->image.size(),
              buf_image.begin()+(index+1)*new_file->image.size(),
              new_file->image.begin());
        new_file->file_name = file_name;
        std::ostringstream out;
        out << index;
        new_file->file_name += out.str();
        std::copy(vs,vs+3,new_file->voxel_size);
        dwi_files.push_back(new_file.release());
        bvalue >> dwi_files.back().bvalue;
        bvec >> dwi_files.back().bvec[0]
             >> dwi_files.back().bvec[1]
             >> dwi_files.back().bvec[2];
        dwi_files[index].bvec.normalize();
    }
    return true;
}

bool load_multiple_slice_dicom(QStringList file_list,boost::ptr_vector<DwiHeader>& dwi_files)
{
    image::io::dicom dicom_header;// multiple frame image
    image::geometry<3> geo;
    if(!dicom_header.load_from_file(file_list[0].toLocal8Bit().begin()))
        return false;
    dicom_header.get_image_dimension(geo);
    // philips or GE single slice images
    if(geo[2] != 1)
        return false;

    image::io::dicom dicom_header2;
    if(!dicom_header2.load_from_file(file_list[1].toLocal8Bit().begin()))
        return false;
    float s1 = dicom_header.get_slice_location();
    if(s1 == dicom_header2.get_slice_location()) // iterater b-value first
    {
        unsigned int b_num = 2;
        for (;b_num < file_list.size();++b_num)
        {
            if(!dicom_header2.load_from_file(file_list[b_num].toLocal8Bit().begin()))
                return false;
            if(dicom_header2.get_slice_location() != s1)
                break;
        }
        geo[2] = file_list.size()/b_num;

        for (unsigned int index = 0,b_index = 0,slice_index = 0;
                    check_prog(index,file_list.size());++index)
        {
            if(!dicom_header2.load_from_file(file_list[index].toLocal8Bit().begin()))
                return false;
            if(slice_index == 0)
            {
                dwi_files.push_back(new DwiHeader);
                dwi_files.back().open(file_list[index].toLocal8Bit().begin());
                dwi_files.back().image.resize(geo);
                dwi_files.back().file_name = file_list[index].toLocal8Bit().begin();
                dicom_header.get_voxel_size(dwi_files.back().voxel_size);
            }
            dicom_header2.save_to_buffer(
                    dwi_files[b_index].image.begin() + slice_index*geo.plane_size(),geo.plane_size());
            ++b_index;
            if(b_index >= b_num)
            {
                b_index = 0;
                ++slice_index;
            }
        }
    }
    else
    // iterater slice first
    {
        unsigned int slice_num = 2;
        for (;slice_num < file_list.size();++slice_num)
        {
            if(!dicom_header2.load_from_file(file_list[slice_num].toLocal8Bit().begin()))
                return false;
            if(dicom_header2.get_slice_location() == s1)
                break;
        }
        geo[2] = slice_num;
        for (unsigned int index = 0,b_index = 0,slice_index = 0;
                    check_prog(index,file_list.size());++index)
        {
            if(!dicom_header2.load_from_file(file_list[index].toLocal8Bit().begin()))
                return false;
            if(slice_index == 0)
            {
                dwi_files.push_back(new DwiHeader);
                dwi_files.back().open(file_list[index].toLocal8Bit().begin());
                dwi_files.back().file_name = file_list[index].toLocal8Bit().begin();
                dwi_files.back().image.resize(geo);
                dicom_header.get_voxel_size(dwi_files.back().voxel_size);
            }
            dicom_header2.save_to_buffer(
                    dwi_files[b_index].image.begin() + slice_index*geo.plane_size(),geo.plane_size());
            ++slice_index;
            if(slice_index >= slice_num)
            {
                slice_index = 0;
                ++b_index;
            }
        }
    }
    return true;
}
bool load_4d_fdf(QStringList file_list,boost::ptr_vector<DwiHeader>& dwi_files)
{
    for (unsigned int index = 0;check_prog(index,file_list.size());++index)
    {
        std::map<std::string,std::string> value_list;
        {
            std::ifstream in(file_list[index].toLocal8Bit().begin());
            std::string line;
            while(std::getline(in,line))
            {
                std::string::size_type pos = 0;
                if(line.empty() || line[0] == '#' || (pos = line.find('=')) == std::string::npos)
                    continue;
                std::istringstream read_line(line);
                std::string s1,s2,value;
                read_line >> s1 >> s2;
                value = line.substr(pos+2,line.length()-pos-3);
                std::replace(value.begin(),value.end(),',',' ');
                std::replace(value.begin(),value.end(),'"',' ');
                std::replace(value.begin(),value.end(),'{',' ');
                std::replace(value.begin(),value.end(),'}',' ');
                value_list[s2] = value;
                if(s2 == "checksum")
                    break;
            }
        }
        if(value_list["*storage"] != " float ")
            return false;
        // allocate all space
        if(index == 0)
        {
            float dwi_num,width,height,depth,fov1,fov2,fov3;
            if(!(std::istringstream(value_list["array_dim"]) >> dwi_num) ||
               !(std::istringstream(value_list["matrix[]"]) >> width >> height ) ||
               !(std::istringstream(value_list["slices"]) >> depth) ||
               !(std::istringstream(value_list["roi[]"]) >> fov1 >> fov2 >> fov3))
                return false;
            for(unsigned int i = 0;i < dwi_num;++i)
            {
                dwi_files.push_back(new DwiHeader);
                dwi_files.back().image.resize(image::geometry<3>(width,height,depth));
                dwi_files.back().voxel_size[0] = fov1*10.0/width;
                dwi_files.back().voxel_size[1] = fov2*10.0/height;
                dwi_files.back().voxel_size[2] = fov3*100.0/depth;
                dwi_files.back().file_name = value_list["*studyid"];
                //dwi_files
            }
            if(dwi_files.empty())
                return false;
            dwi_files.back().report = " The diffusion images were acquired on a Varian scanner.";
        }
        // get DWI and slice location
        int dwi_id,slice_id;
        {
            float v1,v2;
            if(!(std::istringstream(value_list["array_index"]) >> v1) ||
               !(std::istringstream(value_list["slice_no"]) >> v2))
                return false;
            dwi_id = v1 - 1.0;
            slice_id = v2 - 1.0;
            if(dwi_id < 0 || dwi_id >= dwi_files.size() || slice_id < 0 || slice_id >= dwi_files.front().image.depth())
                return 0;
        }
        // get b_value
        if(slice_id == 0)
        {
            if(!(std::istringstream(value_list["dro"]) >> dwi_files[dwi_id].bvec[0]) ||
               !(std::istringstream(value_list["dpe"]) >> dwi_files[dwi_id].bvec[1]) ||
               !(std::istringstream(value_list["dsl"]) >> dwi_files[dwi_id].bvec[2]) ||
               !(std::istringstream(value_list["bvalue"]) >> dwi_files[dwi_id].bvalue))
                return false;
            dwi_files[dwi_id].bvec.normalize();
        }
        std::vector<float> buf(dwi_files[dwi_id].image.width()*dwi_files[dwi_id].image.height());
        std::ifstream in(file_list[index].toLocal8Bit().begin(),std::ifstream::binary);
        in.seekg(-(int)buf.size()*4,std::ios_base::end);
        if(!in.read((char*)&*buf.begin(),buf.size()*4))
            return false;
        std::copy(buf.begin(),buf.end(),dwi_files[dwi_id].image.begin() + slice_id*dwi_files[dwi_id].image.plane_size());
    }
    return true;
}

bool load_3d_series(QStringList file_list,boost::ptr_vector<DwiHeader>& dwi_files)
{
    for (unsigned int index = 0;check_prog(index,file_list.size());++index)
    {
        std::auto_ptr<DwiHeader> new_file(new DwiHeader);
        if (!new_file->open(file_list[index].toLocal8Bit().begin()))
            continue;
        new_file->file_name = file_list[index].toLocal8Bit().begin();
        dwi_files.push_back(new_file.release());
    }
    return !dwi_files.empty();
}

bool load_all_files(QStringList file_list,boost::ptr_vector<DwiHeader>& dwi_files)
{
    if(QFileInfo(file_list[0]).baseName() == "2dseq")
    {
        for(unsigned int index = 0;index < file_list.size();++index)
            load_4d_2dseq(file_list[index].toLocal8Bit().begin(),dwi_files);
        return !dwi_files.empty();
    }
    if(QFileInfo(file_list[0]).suffix() == "fdf")
    {
        return load_4d_fdf(file_list,dwi_files);
    }

    if(file_list.size() == 1 && QFileInfo(file_list[0]).isDir()) // single folder with DICOM files
    {
        QDir cur_dir = file_list[0];
        QStringList dicom_file_list = cur_dir.entryList(QStringList("*.dcm"),QDir::Files|QDir::NoSymLinks);
        if(dicom_file_list.empty())
            return false;
        for (unsigned int index = 0;index < dicom_file_list.size();++index)
            dicom_file_list[index] = file_list[0] + "/" + dicom_file_list[index];
        return load_all_files(dicom_file_list,dwi_files);
    }

    //Combine 2dseq folders
    if(file_list.size() > 1 && QFileInfo(file_list[0]).isDir() &&
            QFileInfo(file_list[0]+"/pdata/1/2dseq").exists())
    {
        for(unsigned int index = 0;index < file_list.size();++index)
            load_all_files(QStringList() << (file_list[index]+"/pdata/1/2dseq"),dwi_files);
        return !dwi_files.empty();
    }


    begin_prog("loading");
    if (file_list.size() == 1) // 4d image
    {
        if(!load_dicom_multi_frame(file_list[0].toLocal8Bit().begin(),dwi_files) &&
           !load_4d_nii(file_list[0].toLocal8Bit().begin(),dwi_files))
            return false;
         return !dwi_files.empty();
    }

    if(load_multiple_slice_dicom(file_list,dwi_files))
        return !dwi_files.empty();

    return load_3d_series(file_list,dwi_files);
}

void dicom_parser::load_files(QStringList file_list)
{
    if(!load_all_files(file_list,dwi_files))
    {
        QMessageBox::information(this,"Error","Invalid file format",0);
        close();
        return;
    }
    if(prog_aborted())
    {
        close();
        return;
    }
    unsigned int last_index = ui->tableWidget->rowCount();
    unsigned int add_count = dwi_files.size()-last_index;
    if(add_count)
    {
        ui->tableWidget->setRowCount(dwi_files.size());
        double max_b = 0;
        for(unsigned int index = last_index;index < dwi_files.size();++index)
        {
            if(dwi_files[index].get_bvalue() < 100)
                dwi_files[index].set_bvalue(0);
            ui->tableWidget->setItem(index, 0, new QTableWidgetItem(QFileInfo(dwi_files[index].file_name.data()).fileName()));
            ui->tableWidget->setItem(index, 1, new QTableWidgetItem(QString::number(dwi_files[index].get_bvalue())));
            ui->tableWidget->setItem(index, 2, new QTableWidgetItem(QString::number(dwi_files[index].get_bvec()[0])));
            ui->tableWidget->setItem(index, 3, new QTableWidgetItem(QString::number(dwi_files[index].get_bvec()[1])));
            ui->tableWidget->setItem(index, 4, new QTableWidgetItem(QString::number(dwi_files[index].get_bvec()[2])));
            max_b = std::max(max_b,(double)dwi_files[index].get_bvalue());
        }
        if(max_b == 0.0)
            QMessageBox::information(this,"DSI Studio","Cannot find b-table from the header. You may need to load an external b-table",0);
    }
}

void dicom_parser::on_buttonBox_accepted()
{
    if (dwi_files.empty())
        return;

    // save b table info to dwi header
    for (unsigned int index = 0;index < dwi_files.size();++index)
    {
        if(QString::number(dwi_files[index].get_bvalue()) != ui->tableWidget->item(index,1)->text())
            dwi_files[index].set_bvalue(ui->tableWidget->item(index,1)->text().toFloat());
        if(QString::number(dwi_files[index].get_bvec()[0]) != ui->tableWidget->item(index,2)->text() ||
           QString::number(dwi_files[index].get_bvec()[1]) != ui->tableWidget->item(index,3)->text() ||
           QString::number(dwi_files[index].get_bvec()[2]) != ui->tableWidget->item(index,4)->text())
            dwi_files[index].set_bvec(
                    ui->tableWidget->item(index,2)->text().toFloat(),
                    ui->tableWidget->item(index,3)->text().toFloat(),
                    ui->tableWidget->item(index,4)->text().toFloat());
    }

    DwiHeader::output_src(ui->SrcName->text().toLocal8Bit().begin(),
                          dwi_files,
                          ui->upsampling->currentIndex());

    dwi_files.clear();
    if(QFileInfo(ui->SrcName->text()).suffix() != "gz")
        ((MainWindow*)parent())->addSrc(ui->SrcName->text()+".gz");
    else
        ((MainWindow*)parent())->addSrc(ui->SrcName->text());
    QMessageBox::information(this,"DSI Studio","SRC file created",0);
    close();
}

void dicom_parser::on_toolButton_clicked()
{
    // flip x
    for (unsigned int index = 0;index < ui->tableWidget->rowCount();++index)
        ui->tableWidget->item(index,2)->setText(QString::number(-(ui->tableWidget->item(index,2)->text().toDouble())));
}

void dicom_parser::on_toolButton_3_clicked()
{
    // flip y
    for (unsigned int index = 0;index < ui->tableWidget->rowCount();++index)
        ui->tableWidget->item(index,3)->setText(QString::number(-(ui->tableWidget->item(index,3)->text().toDouble())));
}

void dicom_parser::on_toolButton_4_clicked()
{
    // flip z
    for (unsigned int index = 0;index < ui->tableWidget->rowCount();++index)
        ui->tableWidget->item(index,4)->setText(QString::number(-(ui->tableWidget->item(index,4)->text().toDouble())));

}

void dicom_parser::on_toolButton_5_clicked()
{
    // switch bx by
    for (unsigned int index = 0;index < ui->tableWidget->rowCount();++index)
    {
        QString temp = ui->tableWidget->item(index,3)->text();
        ui->tableWidget->item(index,3)->setText(ui->tableWidget->item(index,2)->text());
        ui->tableWidget->item(index,2)->setText(temp);
    }
}

void dicom_parser::on_toolButton_6_clicked()
{
    // switch bx bz
    for (unsigned int index = 0;index < ui->tableWidget->rowCount();++index)
    {
        QString temp = ui->tableWidget->item(index,4)->text();
        ui->tableWidget->item(index,4)->setText(ui->tableWidget->item(index,2)->text());
        ui->tableWidget->item(index,2)->setText(temp);
    }
}

void dicom_parser::on_toolButton_7_clicked()
{
    // switch by bz
    for (unsigned int index = 0;index < ui->tableWidget->rowCount();++index)
    {
        QString temp = ui->tableWidget->item(index,4)->text();
        ui->tableWidget->item(index,4)->setText(ui->tableWidget->item(index,3)->text());
        ui->tableWidget->item(index,3)->setText(temp);
    }
}

void dicom_parser::on_toolButton_2_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
            this,
            "Open b-table",
            QFileInfo(ui->SrcName->text()).absolutePath(),
            "Text files (*.txt);;All files (*)" );

    std::ifstream in(filename.toLocal8Bit().begin());
    if(!in)
        return;
    std::string line;
    std::vector<double> b_table;
    while(std::getline(in,line))
    {
        std::istringstream read_line(line);
        std::copy(std::istream_iterator<double>(read_line),
                  std::istream_iterator<double>(),
                  std::back_inserter(b_table));
    }
    for (unsigned int index = 0,b_index = 0;index < ui->tableWidget->rowCount();++index)
    {
        for(unsigned int j = 0;j < 4 && b_index < b_table.size();++j,++b_index)
            ui->tableWidget->item(index,j+1)->setText(QString::number(b_table[b_index]));
    }
}


void dicom_parser::on_load_bval_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
            this,
            "Open bval",
            QFileInfo(ui->SrcName->text()).absolutePath(),
            "All files (*)" );
    if(filename.isEmpty())
        return;
    std::vector<double> bval;
    load_bval(filename.toLocal8Bit().begin(),bval);
    if(bval.empty())
        return;
    for (unsigned int index = 0;index < ui->tableWidget->rowCount();++index)
        ui->tableWidget->item(index,1)->setText(QString::number(bval[index]));
}



void dicom_parser::on_load_bvec_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
            this,
            "Open bvec",
            QFileInfo(ui->SrcName->text()).absolutePath(),
            "All files (*)" );
    if(filename.isEmpty())
        return;
    std::vector<double> b_table;
    load_bvec(filename.toLocal8Bit().begin(),b_table);
    if(b_table.empty())
        return;
    for (unsigned int index = 0,b_index = 0;index < ui->tableWidget->rowCount();++index)
    {
        for(unsigned int j = 0;j < 3 && b_index < b_table.size();++j,++b_index)
            ui->tableWidget->item(index,j+2)->setText(QString::number(b_table[b_index]));
    }
}


void dicom_parser::on_toolButton_8_clicked()
{
    QString filename = QFileDialog::getSaveFileName(
            this,
            "Save b-table",
            QFileInfo(ui->SrcName->text()).absolutePath() + "/b_table.txt",
            "Text files (*.txt);;All files (*)");

    std::ofstream btable(filename.toLocal8Bit().begin());
    if(!btable)
        return;
    for (unsigned int index = 0;index < ui->tableWidget->rowCount();++index)
    {
        for(unsigned int j = 0;j < 4;++j)
            btable << ui->tableWidget->item(index,j+1)->text().toDouble() << "\t";
        btable<< std::endl;
    }
}

void dicom_parser::on_pushButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(
            this,"Save file",
            ui->SrcName->text(),
            "Src files (*.src.gz *.src);;All files (*)" );
    if(filename.isEmpty())
        return;
#ifdef __APPLE__
// fix the Qt double extension bug here
if(QFileInfo(filename).completeSuffix().contains(".src.gz"))
    filename = QFileInfo(filename).absolutePath() + "/" + QFileInfo(filename).baseName() + ".src.gz";
#endif
    ui->SrcName->setText(filename);
}

void dicom_parser::on_loadImage_clicked()
{
    QStringList filenames = QFileDialog::getOpenFileNames(
            this,"Open Images files",cur_path,
            "Images (*.dcm *.hdr *.nii *.nii.gz 2dseq);;All files (*)" );
    if( filenames.isEmpty() )
        return;
    load_files(filenames);
}

void dicom_parser::on_upperDir_clicked()
{
    QDir path(QFileInfo(ui->SrcName->text()).absolutePath());
    path.cdUp();
    ui->SrcName->setText(path.absolutePath() + "/" +
                         QFileInfo(ui->SrcName->text()).fileName());
}

void dicom_parser::update_b_table(void)
{
    for (unsigned int index = 0;index < ui->tableWidget->rowCount();++index)
    {
        ui->tableWidget->item(index,2)->setText(QString::number(dwi_files[index].bvec[0]));
        ui->tableWidget->item(index,3)->setText(QString::number(dwi_files[index].bvec[1]));
        ui->tableWidget->item(index,4)->setText(QString::number(dwi_files[index].bvec[2]));
    }
}

void dicom_parser::on_apply_slice_orientation_clicked()
{
    if(slice_orientation.empty())
        return;
    for (unsigned int index = 0;index < ui->tableWidget->rowCount();++index)
    {
        image::vector<3,float> bvec,cbvec;
        bvec[0] = ui->tableWidget->item(index,2)->text().toDouble();
        bvec[1] = ui->tableWidget->item(index,3)->text().toDouble();
        bvec[2] = ui->tableWidget->item(index,4)->text().toDouble();
        image::vector_rotation(bvec.begin(),cbvec.begin(),slice_orientation.begin(),image::vdim<3>());
        ui->tableWidget->item(index,2)->setText(QString::number(-cbvec[0]));
        ui->tableWidget->item(index,3)->setText(QString::number(-cbvec[1]));
        ui->tableWidget->item(index,4)->setText(QString::number(-cbvec[2]));
    }
}


void dicom_parser::on_motion_correction_clicked()
{
    unsigned int b0_count = 0;
    for(unsigned int index = 0;index < dwi_files.size();++index)
        if(dwi_files[index].get_bvalue() < 100)
            ++b0_count;
    if(b0_count <= 1)
    {
        QMessageBox::information(this,"Error","No extra b0 image found for motion detection.");
        return;
    }

    motion_dialog* md = new motion_dialog(this,dwi_files);
    md->setAttribute(Qt::WA_DeleteOnClose);
    md->show();

}
