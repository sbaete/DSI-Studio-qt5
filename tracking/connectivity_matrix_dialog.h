#ifndef CONNECTIVITY_MATRIX_DIALOG_H
#define CONNECTIVITY_MATRIX_DIALOG_H
#include <QDialog>
#include <QGraphicsScene>
#include "image/image.hpp"
#include "libs/tracking/tract_model.hpp"

class TractModel;
namespace Ui {
class connectivity_matrix_dialog;
}

class tracking_window;

class connectivity_matrix_dialog : public QDialog
{
    Q_OBJECT
    image::color_image cm;
    QImage view_image;
    QGraphicsScene scene;
public:
    ConnectivityMatrix data;
public:
    tracking_window* cur_tracking_window;
    explicit connectivity_matrix_dialog(tracking_window *parent);
    ~connectivity_matrix_dialog();

    void mouse_move(QMouseEvent *mouseEvent);
    bool is_graphic_view(QObject *) const;
private slots:

    void on_recalculate_clicked();

    void on_zoom_valueChanged(double arg1);

    void on_log_toggled(bool checked);

    void on_save_as_clicked();

    void on_norm_toggled(bool checked);

private:
    Ui::connectivity_matrix_dialog *ui;
};

#endif // CONNECTIVITY_MATRIX_DIALOG_H
