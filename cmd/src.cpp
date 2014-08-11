#include <QDir>
#include <iostream>
#include <iterator>
#include <string>
#include "image/image.hpp"
#include "boost/program_options.hpp"
#include "dicom/dwi_header.hpp"

namespace po = boost::program_options;
QStringList search_files(QString dir,QString filter);
bool load_all_files(QStringList file_list,boost::ptr_vector<DwiHeader>& dwi_files);
int src(int ac, char *av[])
{
    po::options_description rec_desc("dicom parsing options");
    rec_desc.add_options()
    ("help", "help message")
    ("action", po::value<std::string>(), "src:dicom parsing")
    ("source", po::value<std::string>(), "assign the directory for the dicom files")
    ("recursive", po::value<std::string>(), "search subdirectories")
    ("b_table", po::value<std::string>(), "assign the b-table")
    ("output", po::value<std::string>(), "assign the output filename")
    ;
    if(!ac)
    {
        std::cout << rec_desc << std::endl;
        return 1;
    }
    po::variables_map vm;
    po::store(po::command_line_parser(ac, av).options(rec_desc).run(), vm);
    po::notify(vm);


    std::string source = vm["source"].as<std::string>();
    std::string ext;
    if(source.size() > 4)
        ext = std::string(source.end()-4,source.end());

    boost::ptr_vector<DwiHeader> dwi_files;
    QStringList file_list;
    if(ext ==".nii" || ext == ".dcm" || ext == "dseq" || ext == "i.gz")
    {
        std::cout << "image=" << source.c_str() << std::endl;
        file_list << source.c_str();
    }
    else
    {
        std::cout << "load files in directory " << source.c_str() << std::endl;
        QDir directory = QString(source.c_str());
        if(vm.count("recursive"))
        {
            std::cout << "search recursively in the subdir" << std::endl;
            file_list = search_files(source.c_str(),"*.dcm");
        }
        else
        {
            file_list = directory.entryList(QStringList("*.dcm"),QDir::Files|QDir::NoSymLinks);
            for (unsigned int index = 0;index < file_list.size();++index)
                file_list[index] = QString(source.c_str()) + "/" + file_list[index];
        }
        std::cout << "A total of " << file_list.size() <<" files found in the directory" << std::endl;
    }

    if(file_list.empty())
    {
        std::cout << "No file found for creating src" << std::endl;
        return -1;
    }

    if(!load_all_files(file_list,dwi_files))
    {
        std::cout << "Invalid file format" << std::endl;
        return -1;
    }
    if(vm.count("b_table"))
    {
        std::string table_file_name = vm["b_table"].as<std::string>();
        std::ifstream in(table_file_name.c_str());
        if(!in)
        {
            std::cout << "Failed to open b-table" <<std::endl;
            return -1;
        }
        std::string line;
        std::vector<double> b_table;
        while(std::getline(in,line))
        {
            std::istringstream read_line(line);
            std::copy(std::istream_iterator<double>(read_line),
                      std::istream_iterator<double>(),
                      std::back_inserter(b_table));
        }
        if(b_table.size() != dwi_files.size()*4)
        {
            std::cout << "Mismatch between b-table and the loaded image" << std::endl;
            return -1;
        }
        for(unsigned int index = 0,b_index = 0;index < dwi_files.size();++index,b_index += 4)
        {
            dwi_files[index].set_bvalue(b_table[b_index]);
            dwi_files[index].set_bvec(b_table[b_index+1],b_table[b_index+2],b_table[b_index+3]);
        }
        std::cout << "B-table " << table_file_name << " loaded" << std::endl;
    }

    if(dwi_files.empty())
    {
        std::cout << "No file readed. Abort." << std::endl;
        return 1;
    }
    std::cout << "Output src " << vm["output"].as<std::string>().c_str() << std::endl;
    DwiHeader::output_src(vm["output"].as<std::string>().c_str(),dwi_files,0);
    return 0;
}
