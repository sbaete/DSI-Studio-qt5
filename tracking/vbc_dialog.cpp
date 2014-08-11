#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include "vbc_dialog.hpp"
#include "ui_vbc_dialog.h"
#include "ui_tracking_window.h"
#include "tracking_window.h"
#include "tract/tracttablewidget.h"

vbc_dialog::vbc_dialog(QWidget *parent,vbc_database* vbc_ptr,QString work_dir_) :
    QDialog(parent),vbc(vbc_ptr),work_dir(work_dir_),
    ui(new Ui::vbc_dialog)
{
    ui->setupUi(this);
    ui->vbc_view->setScene(&vbc_scene);
    ui->subject_list->setColumnCount(3);
    ui->subject_list->setColumnWidth(0,300);
    ui->subject_list->setColumnWidth(1,50);
    ui->subject_list->setColumnWidth(2,50);
    ui->subject_list->setHorizontalHeaderLabels(
                QStringList() << "name" << "value" << "R2");
    ui->subject_list->setRowCount(vbc->subject_count());
    for(unsigned int index = 0;index < vbc->subject_count();++index)
    {
        ui->subject_list->setItem(index,0, new QTableWidgetItem(QString(vbc->subject_name(index).c_str())));
        ui->subject_list->setItem(index,1, new QTableWidgetItem(QString::number(0)));
        ui->subject_list->setItem(index,2, new QTableWidgetItem(QString::number(vbc->subject_R2(index))));
    }
    ui->AxiSlider->setMaximum(vbc->handle->fib_data.dim[2]-1);
    ui->AxiSlider->setMinimum(0);
    ui->AxiSlider->setValue(vbc->handle->fib_data.dim[2] >> 1);

    // dist report
    connect(ui->line_width,SIGNAL(valueChanged(int)),this,SLOT(show_report()));
    connect(ui->span_to,SIGNAL(valueChanged(int)),this,SLOT(show_report()));
    connect(ui->show_null_greater,SIGNAL(toggled(bool)),this,SLOT(show_report()));
    connect(ui->show_null_lesser,SIGNAL(toggled(bool)),this,SLOT(show_report()));
    connect(ui->show_greater,SIGNAL(toggled(bool)),this,SLOT(show_report()));
    connect(ui->show_lesser,SIGNAL(toggled(bool)),this,SLOT(show_report()));

    connect(ui->line_width_2,SIGNAL(valueChanged(int)),this,SLOT(show_fdr_report()));
    connect(ui->span_to_2,SIGNAL(valueChanged(int)),this,SLOT(show_fdr_report()));
    connect(ui->show_greater_2,SIGNAL(toggled(bool)),this,SLOT(show_fdr_report()));
    connect(ui->show_lesser_2,SIGNAL(toggled(bool)),this,SLOT(show_fdr_report()));


    connect(ui->AxiSlider,SIGNAL(valueChanged(int)),this,SLOT(on_subject_list_itemSelectionChanged()));
    connect(ui->zoom,SIGNAL(valueChanged(double)),this,SLOT(on_subject_list_itemSelectionChanged()));

    ui->subject_list->selectRow(0);
    qApp->installEventFilter(this);

}

vbc_dialog::~vbc_dialog()
{
    qApp->removeEventFilter(this);

    delete ui;
}

bool vbc_dialog::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() != QEvent::MouseMove || obj->parent() != ui->vbc_view)
        return false;
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    QPointF point = ui->vbc_view->mapToScene(mouseEvent->pos().x(),mouseEvent->pos().y());
    image::vector<3,float> pos;
    pos[0] =  ((float)point.x()) / ui->zoom->value() - 0.5;
    pos[1] =  ((float)point.y()) / ui->zoom->value() - 0.5;
    pos[2] = ui->AxiSlider->value();
    if(!vbc->handle->fib_data.dim.is_valid(pos))
        return true;
    ui->coordinate->setText(QString("(%1,%2,%3)").arg(pos[0]).arg(pos[1]).arg(pos[2]));

    // show data
    std::vector<float> vbc_data;
    vbc->get_data_at(
            image::pixel_index<3>(std::floor(pos[0] + 0.5), std::floor(pos[1] + 0.5), std::floor(pos[2] + 0.5),
                                  vbc->handle->fib_data.dim).index(),0,vbc_data);
    if(!vbc_data.empty())
    {
        for(unsigned int index = 0;index < vbc->subject_count();++index)
            ui->subject_list->item(index,1)->setText(QString::number(vbc_data[index]));

        vbc_data.erase(std::remove(vbc_data.begin(),vbc_data.end(),0.0),vbc_data.end());
        if(!vbc_data.empty())
        {
            float max_y = *std::max_element(vbc_data.begin(),vbc_data.end());
            std::vector<unsigned int> hist;
            image::histogram(vbc_data,hist,0,max_y,20);
            QVector<double> x(hist.size()+1),y(hist.size()+1);
            unsigned int max_hist = 0;
            for(unsigned int j = 0;j < hist.size();++j)
            {
                x[j] = max_y*(float)j/(float)hist.size();
                y[j] = hist[j];
                max_hist = std::max<unsigned int>(max_hist,hist[j]);
            }
            x.back() = max_y*(hist.size()+1)/hist.size();
            y.back() = 0;
            ui->vbc_report->clearGraphs();
            ui->vbc_report->addGraph();
            QPen pen;
            pen.setColor(QColor(20,20,100,200));
            ui->vbc_report->graph(0)->setLineStyle(QCPGraph::lsLine);
            ui->vbc_report->graph(0)->setPen(pen);
            ui->vbc_report->graph(0)->setData(x, y);

            ui->vbc_report->xAxis->setRange(0,x.back());
            ui->vbc_report->yAxis->setRange(0,max_hist);
            ui->vbc_report->replot();
            }
    }
    return true;
}

void vbc_dialog::show_fdr_report()
{
    ui->fdr_dist->clearGraphs();
    if(fdr.size() != 2)
        return;
    std::vector<std::vector<float> > vbc_data;
    char legends[4][60] = {"greater","lesser"};
    std::vector<const char*> legend;

    if(ui->show_greater_2->isChecked())
    {
        vbc_data.push_back(fdr[0]);
        legend.push_back(legends[0]);
    }
    if(ui->show_lesser_2->isChecked())
    {
        vbc_data.push_back(fdr[1]);
        legend.push_back(legends[1]);
    }


    QPen pen;
    QColor color[4];
    color[0] = QColor(20,20,100,200);
    color[1] = QColor(100,20,20,200);
    for(unsigned int i = 0; i < vbc_data.size(); ++i)
    {
        QVector<double> x(vbc_data[i].size());
        QVector<double> y(vbc_data[i].size());
        for(unsigned int j = 0;j < vbc_data[i].size();++j)
        {
            x[j] = (float)j;
            y[j] = vbc_data[i][j];
        }
        ui->fdr_dist->addGraph();
        pen.setColor(color[i]);
        pen.setWidth(ui->line_width_2->value());
        ui->fdr_dist->graph()->setLineStyle(QCPGraph::lsLine);
        ui->fdr_dist->graph()->setPen(pen);
        ui->fdr_dist->graph()->setData(x, y);
        ui->fdr_dist->graph()->setName(QString(legend[i]));
    }
    ui->fdr_dist->xAxis->setLabel("mm");
    ui->fdr_dist->yAxis->setLabel("FDR");
    ui->fdr_dist->xAxis->setRange(2,ui->span_to_2->value());
    ui->fdr_dist->yAxis->setRange(0,1.0);
    ui->fdr_dist->legend->setVisible(true);
    QFont legendFont = font();  // start out with MainWindow's font..
    legendFont.setPointSize(9); // and make a bit smaller for legend
    ui->fdr_dist->legend->setFont(legendFont);
    ui->fdr_dist->legend->setPositionStyle(QCPLegend::psRight);
    ui->fdr_dist->legend->setBrush(QBrush(QColor(255,255,255,230)));
    ui->fdr_dist->replot();

}

void vbc_dialog::show_report()
{
    ui->null_dist->clearGraphs();
    if(dist.empty())
        return;
    std::vector<std::vector<unsigned int> > vbc_data;
    char legends[4][60] = {"null greater","null lesser","greater","lesser"};
    std::vector<const char*> legend;

    if(ui->show_null_greater->isChecked())
    {
        vbc_data.push_back(dist[0]);
        legend.push_back(legends[0]);
    }
    if(ui->show_null_lesser->isChecked())
    {
        vbc_data.push_back(dist[1]);
        legend.push_back(legends[1]);
    }
    if(ui->show_greater->isChecked())
    {
        vbc_data.push_back(dist[2]);
        legend.push_back(legends[2]);
    }
    if(ui->show_lesser->isChecked())
    {
        vbc_data.push_back(dist[3]);
        legend.push_back(legends[3]);
    }

    // normalize
    float max_y1 = *std::max_element(dist[0].begin(),dist[0].end());
    float max_y2 = *std::max_element(dist[1].begin(),dist[1].end());
    float max_y3 = *std::max_element(dist[2].begin(),dist[2].end());
    float max_y4 = *std::max_element(dist[3].begin(),dist[3].end());


    if(vbc_data.empty())
        return;

    unsigned int x_size = 0;
    for(unsigned int i = 0;i < vbc_data.size();++i)
        x_size = std::max<unsigned int>(x_size,vbc_data[i].size());
    if(x_size == 0)
        return;
    QVector<double> x(x_size);
    std::vector<QVector<double> > y(vbc_data.size());
    for(unsigned int i = 0;i < vbc_data.size();++i)
        y[i].resize(x_size);
    for(unsigned int j = 0;j < x_size;++j)
    {
        x[j] = (float)j;
        for(unsigned int i = 0; i < vbc_data.size(); ++i)
            if(j < vbc_data[i].size())
                y[i][j] = vbc_data[i][j];
    }
    QPen pen;
    QColor color[4];
    color[0] = QColor(20,20,100,200);
    color[1] = QColor(100,20,20,200);
    color[2] = QColor(20,100,20,200);
    color[3] = QColor(20,100,100,200);
    for(unsigned int i = 0; i < vbc_data.size(); ++i)
    {
        ui->null_dist->addGraph();
        pen.setColor(color[i]);
        pen.setWidth(ui->line_width->value());
        ui->null_dist->graph()->setLineStyle(QCPGraph::lsLine);
        ui->null_dist->graph()->setPen(pen);
        ui->null_dist->graph()->setData(x, y[i]);
        ui->null_dist->graph()->setName(QString(legend[i]));
    }

    ui->null_dist->xAxis->setLabel("mm");
    ui->null_dist->yAxis->setLabel("count");
    ui->null_dist->xAxis->setRange(2,ui->span_to->value());
    ui->null_dist->yAxis->setRange(0,std::max<float>(std::max<float>(max_y1,max_y2),std::max<float>(max_y3,max_y4))*1.1);
    ui->null_dist->legend->setVisible(true);
    QFont legendFont = font();  // start out with MainWindow's font..
    legendFont.setPointSize(9); // and make a bit smaller for legend
    ui->null_dist->legend->setFont(legendFont);
    ui->null_dist->legend->setPositionStyle(QCPLegend::psRight);
    ui->null_dist->legend->setBrush(QBrush(QColor(255,255,255,230)));
    ui->null_dist->replot();
}

void vbc_dialog::show_dis_table(void)
{
    if(dist.size() != 4)
        return;
    ui->dist_table->setColumnCount(9);
    ui->dist_table->setColumnWidth(0,50);
    ui->dist_table->setColumnWidth(1,150);
    ui->dist_table->setColumnWidth(2,150);
    ui->dist_table->setColumnWidth(3,150);
    ui->dist_table->setColumnWidth(4,150);
    ui->dist_table->setColumnWidth(5,150);
    ui->dist_table->setColumnWidth(6,150);
    ui->dist_table->setColumnWidth(7,150);
    ui->dist_table->setColumnWidth(8,150);
    ui->dist_table->setHorizontalHeaderLabels(
                QStringList() << "length (mm)" << "null greater pdf" << "null greater cdf" <<
                                            "null lesser pdf" << "null lesser cdf" <<
                                            "greater pdf" << "greater cdf" <<
                                            "lesser pdf" << "lesser cdf");

    ui->dist_table->setRowCount(100);
    std::vector<float> sum(4);
    for(unsigned int index = 0;index < 100;++index)
    {
        ui->dist_table->setItem(index,0, new QTableWidgetItem(QString::number(index + 1)));
        ui->dist_table->setItem(index,1, new QTableWidgetItem(QString::number(dist[0][index+1])));
        ui->dist_table->setItem(index,2, new QTableWidgetItem(QString::number(sum[0] += dist[0][index+1])));
        ui->dist_table->setItem(index,3, new QTableWidgetItem(QString::number(dist[1][index+1])));
        ui->dist_table->setItem(index,4, new QTableWidgetItem(QString::number(sum[1] += dist[1][index+1])));
        ui->dist_table->setItem(index,5, new QTableWidgetItem(QString::number(dist[2][index+1])));
        ui->dist_table->setItem(index,6, new QTableWidgetItem(QString::number(sum[2] += dist[2][index+1])));
        ui->dist_table->setItem(index,7, new QTableWidgetItem(QString::number(dist[3][index+1])));
        ui->dist_table->setItem(index,8, new QTableWidgetItem(QString::number(sum[3] += dist[3][index+1])));
    }
    ui->dist_table->selectRow(0);
}

void vbc_dialog::show_fdr_table(void)
{
    if(fdr.size() != 2)
        return;
    ui->fdr_table->setColumnCount(3);
    ui->fdr_table->setColumnWidth(0,50);
    ui->fdr_table->setColumnWidth(1,150);
    ui->fdr_table->setColumnWidth(2,150);
    ui->fdr_table->setHorizontalHeaderLabels(
                QStringList() << "length (mm)" << "FDR greater" << "FDR lesser");

    ui->fdr_table->setRowCount(100);
    for(unsigned int index = 0;index < 100;++index)
    {
        ui->fdr_table->setItem(index,0, new QTableWidgetItem(QString::number(index + 1)));
        ui->fdr_table->setItem(index,1,
                               new QTableWidgetItem(index + 1 < fdr[0].size() ? QString::number(fdr[0][index+1]):QString()));
        ui->fdr_table->setItem(index,2,
                               new QTableWidgetItem(index + 1 < fdr[1].size() ? QString::number(fdr[1][index+1]):QString()));
    }
    ui->fdr_table->selectRow(0);
}

void vbc_dialog::on_subject_list_itemSelectionChanged()
{
    image::basic_image<float,2> slice;
    vbc->get_subject_slice(ui->subject_list->currentRow(),ui->AxiSlider->value(),slice);
    image::normalize(slice);
    image::color_image color_slice(slice.geometry());
    std::copy(slice.begin(),slice.end(),color_slice.begin());
    QImage qimage((unsigned char*)&*color_slice.begin(),color_slice.width(),color_slice.height(),QImage::Format_RGB32);
    vbc_slice_image = qimage.scaled(color_slice.width()*ui->zoom->value(),color_slice.height()*ui->zoom->value());
    vbc_scene.clear();
    vbc_scene.setSceneRect(0, 0, vbc_slice_image.width(),vbc_slice_image.height());
    vbc_scene.setItemIndexMethod(QGraphicsScene::NoIndex);
    vbc_scene.addRect(0, 0, vbc_slice_image.width(),vbc_slice_image.height(),QPen(),vbc_slice_image);
    vbc_slice_pos = ui->AxiSlider->value();
}

void vbc_dialog::on_save_vbc_dist_clicked()
{
    QString filename = QFileDialog::getSaveFileName(
                this,
                "Save report as",
                work_dir + "/dist_report.txt",
                "Report file (*.txt);;All files (*)");
    if(filename.isEmpty())
        return;
    ui->null_dist->saveTxt(filename);
}

void vbc_dialog::on_save_fdr_dist_clicked()
{
    QString filename = QFileDialog::getSaveFileName(
                this,
                "Save report as",
                work_dir + "/fdr_report.txt",
                "Report file (*.txt);;All files (*)");
    if(filename.isEmpty())
        return;
    ui->fdr_dist->saveTxt(filename);
}



void vbc_dialog::on_open_files_clicked()
{
    filename = QFileDialog::getOpenFileNames(
                                this,
                "Select subject fib file for analysis",
                work_dir,"Fib files (*.fib.gz);;All files (*)" );
    if (filename.isEmpty())
        return;
    ui->show_file_name->setText(QString("Subject files:")+QFileInfo(filename[0]).fileName()+"...etc.");
    ui->file_name_widget->show();
}

void vbc_dialog::on_FDR_analysis_clicked()
{
    dist.clear();
    dist.resize(4);

    std::vector<std::string> individal_file_names;
    for(unsigned int index = 0;index < filename.size();++index)
        individal_file_names.push_back(filename[index].toLocal8Bit().begin());

    if(!vbc->calculate_individual_distribution(1.0-ui->percentile_rank->value(),
                                         0, //output_tracks ? ui->length_threshold->value():0,
                                         individal_file_names,dist[2],dist[3]))
    {
        QMessageBox::information(this,"error",vbc->error_msg.c_str(),0);
        return;
    }

    ui->result_label1->setText(QString::number(100.0*vbc->total_greater/vbc->total) +
                               "% orientations in study group > " +
                               QString::number(ui->percentile_rank->value()) + " rank.");
    ui->result_label2->setText(QString::number(100.0*vbc->total_lesser/vbc->total) +
                               "% orientations in study group < " +
                               QString::number(ui->percentile_rank->value()) + " rank.");

    individal_file_names.clear();
    vbc->calculate_individual_distribution(1.0-ui->percentile_rank->value(),0,individal_file_names,dist[0],dist[1]);

    ui->result_label3->setText(QString::number(100.0*vbc->total_greater/vbc->total) +
                               "% orientations in control group > " +
                               QString::number(ui->percentile_rank->value()) + " rank.");
    ui->result_label4->setText(QString::number(100.0*vbc->total_lesser/vbc->total) +
                               "% orientations in control group < " +
                               QString::number(ui->percentile_rank->value()) + " rank.");

    calculate_FDR();
    /*
    if(ui->Trend->isChecked())
    {
        std::ifstream in(filename[0].toLocal8Bit().begin());
        std::vector<float> data;
        std::copy(std::istream_iterator<float>(in),
                  std::istream_iterator<float>(),std::back_inserter(data));

        if(data.size() != vbc->subject_count())
        {
            QMessageBox::information(this,"error","The number of data does not mactch the subject count",0);
            return;
        }
        vbc->trend_analysis(data,cur_subject_fib);
        vbc->calculate_subject_distribution(1.0-ui->percentile_rank->value(),cur_subject_fib,dist[2],dist[3]);
        cur_subject_fib.add_greater_lesser_mapping_for_tracking(vbc->handle.get());
        vbc->calculate_null_trend_distribution(vbc->get_trend_std(data),1.0-ui->percentile_rank->value(),dist[0],dist[1]);
    }

    if(ui->Group->isChecked())
    {


        std::ifstream in(filename[0].toLocal8Bit().begin());
        std::vector<int> data;
        std::copy(std::istream_iterator<int>(in),
                  std::istream_iterator<int>(),std::back_inserter(data));

        if(data.size() != vbc->subject_count())
        {
            QMessageBox::information(this,"error","The number of data does not mactch the subject count",0);
            return;
        }
        vbc->group_analysis(data,cur_subject_fib);
        vbc->calculate_subject_distribution(1.0-ui->percentile_rank->value(),cur_subject_fib,dist[2],dist[3]);
        cur_subject_fib.add_greater_lesser_mapping_for_tracking(vbc->handle.get());
        vbc->calculate_null_group_distribution(data,1.0-ui->percentile_rank->value(),dist[0],dist[1]);
    }
    */


}

void vbc_dialog::calculate_FDR(void)
{
    {
        fdr.clear();
        fdr.resize(2);
        fdr[0].resize(dist[0].size());
        fdr[1].resize(dist[0].size());
        float sum1 = std::accumulate(dist[0].begin(),dist[0].end(),0.0);
        float sum2 = std::accumulate(dist[1].begin(),dist[1].end(),0.0);
        float sum3 = std::accumulate(dist[2].begin(),dist[2].end(),0.0);
        float sum4 = std::accumulate(dist[3].begin(),dist[3].end(),0.0);

        // if the null distribution get more findings due to scan parameter differences
        if(sum1 + sum2 > (sum3 + sum4) * 2.0)
        {
            // then normalize it!
            image::multiply_constant(dist[0].begin(),dist[0].end(),sum1/sum1);
            image::multiply_constant(dist[1].begin(),dist[1].end(),sum1/sum2);
            image::multiply_constant(dist[2].begin(),dist[2].end(),sum1/sum3);
            image::multiply_constant(dist[3].begin(),dist[3].end(),sum1/sum4);
        }

        std::vector<double> sum(4);
        for(int index = dist[0].size()-1;index >= 0;--index)
        {
            for(unsigned int j = 0;j < 4;++j)
                sum[j] += dist[j][index];
            if(sum[2] > 0.0)
                fdr[0][index] = std::min(1.0,sum[0]/sum[2]);
            else
                fdr[0][index] = 1.0;
            if(sum[3] > 0.0)
                fdr[1][index] = std::min(1.0,sum[1]/sum[3]);
            else
                fdr[1][index] = 1.0;

        }
        show_report();
        show_dis_table();
        show_fdr_report();
        show_fdr_table();
        ui->FDR_widget->show();
    }
}

void vbc_dialog::on_buttonBox_accepted()
{
    close();
}

void vbc_dialog::on_tabWidget_currentChanged(int index)
{
    if(index == 3)
        ui->FDR_widget->hide();
    else
        ui->FDR_widget->show();
}

void vbc_dialog::on_open_mr_files_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
                this,
                "Open demographics",
                work_dir,
                "Text file (*.txt);;All files (*)");
    if(filename.isEmpty())
        return;

    std::ifstream in(filename.toLocal8Bit().begin());
    std::string line;
    std::vector<std::string> titles;
    // read title
    {
        std::getline(in,line);
        std::istringstream str(line);
        std::copy(std::istream_iterator<std::string>(str),
                  std::istream_iterator<std::string>(),std::back_inserter(titles));
        mr.feature_count = titles.size()+1; // additional one for intercept
    }
    mr.X.clear();
    mr.subject_index.clear();
    for(unsigned int index = 0;index < vbc->subject_count() && std::getline(in,line);++index)
    {
        std::istringstream str(line);
        std::vector<std::string> values;
        std::copy(std::istream_iterator<std::string>(str),
                  std::istream_iterator<std::string>(),std::back_inserter(values));
        if(values.size() != titles.size())
        {
            QMessageBox::information(this,"Error",QString("Cannot parse:") + line.c_str(),0);
            return;
        }
        std::vector<double> x(titles.size());
        bool ok = true;
        for(unsigned int j = 0;j < x.size();++j)
        {
            x[j] = QString(values[j].c_str()).toDouble(&ok);
            if(!ok)
                break;
        }
        if(!ok)
            continue;
        mr.subject_index.push_back(index);
        mr.X.push_back(1); // for the intercep
        for(unsigned int j = 0;j < x.size();++j)
            mr.X.push_back(x[j]);
    }


    QStringList t;
    t << "Subject ID";
    for(unsigned int index = 0;index < titles.size();++index)
        t << titles[index].c_str();
    ui->foi->clear();
    ui->foi->addItems(t);
    ui->foi->removeItem(0);
    ui->foi->setCurrentIndex(ui->foi->count()-1);
    ui->subject_demo->clear();
    ui->subject_demo->setColumnCount(titles.size()+1);
    ui->subject_demo->setHorizontalHeaderLabels(t);
    ui->subject_demo->setRowCount(vbc->subject_count());
    for(unsigned int row = 0,subject_id = 0,index = 0;row < ui->subject_demo->rowCount();++row)
    {
        ui->subject_demo->setItem(row,0,new QTableWidgetItem(QString(vbc->subject_name(row).c_str())));
        if(subject_id < mr.subject_index.size() && mr.subject_index[subject_id] == row)
        {
            ++index;// skip intercep
            for(unsigned int col = 1;col < ui->subject_demo->columnCount();++col,++index)
                ui->subject_demo->setItem(row,col,new QTableWidgetItem(QString::number(mr.X[index])));
            ++subject_id;
        }
        else
            for(unsigned int col = 1;col < ui->subject_demo->columnCount();++col)
                ui->subject_demo->setItem(row,col,new QTableWidgetItem("-"));

    }
    mr.type = 1; // multiple regression
    if(!mr.pre_process())
    {
        QMessageBox::information(this,"Error","Dependant features found in the demographics",0);
        ui->run_mr_analysis->setEnabled(false);
        return;
    }
    ui->run_mr_analysis->setEnabled(true);
    ui->view_mr_result->setEnabled(true);
}

void vbc_dialog::on_run_mr_analysis_clicked()
{
    dist.clear();
    dist.resize(4);
    mr.study_feature = ui->foi->currentIndex()+1;
    begin_prog("calculating");
    vbc->permutation_count = ui->mr_permutation->value();
    vbc->t_threshold = ui->t_threshold->value();
    vbc->calculate_length_distribution(mr,dist[0],dist[1],dist[2],dist[3],ui->multithread->value());
    calculate_FDR();
}

void vbc_dialog::on_view_mr_result_clicked()
{
    begin_prog("loading");
    mr.study_feature = ui->foi->currentIndex()+1;
    vbc->calculate_spm(mr,cur_subject_fib,mr.subject_index);
    cur_subject_fib.add_greater_lesser_mapping_for_tracking(vbc->handle.get());
    tracking_window* new_mdi = new tracking_window(this,vbc->handle.get(),false);
    new_mdi->setAttribute(Qt::WA_DeleteOnClose);
    new_mdi->absolute_path = work_dir;
    new_mdi->setWindowTitle(QString("Connectometry mapping on ") + ui->foi->currentText());
    new_mdi->showNormal();
    check_prog(0,0);
}
