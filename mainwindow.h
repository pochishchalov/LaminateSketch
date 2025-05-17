#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>

#include "dx_handler.h"
#include "ls_iface.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct DrawableLayer{
    QPolygonF polyline_;
    Qt::PenStyle style_ = Qt::PenStyle::NoPen;
};

class DrawableSketch{
public:
    DrawableSketch()
        :width_(0)
        ,height_(0)
    {
    }

    void CreateSketch(ls::Iface& sketch, QRect window);
    void SetOrigin(QRect window);
    QPoint GetOrigin() const { return origin_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    bool IsEmpty() { return sketch_.empty(); }
    void DrawSketch(QPainter* painter);
    void UpdateSketch(ls::Iface &sketch, QRect window);

private:
    std::vector<DrawableLayer> sketch_;
    int width_;
    int height_;
    QPoint origin_;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    const static int PANEL_SIZE = 120;
    const static int PIX_IN_CM = 30;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

    void paintEvent(QPaintEvent* event) override;

private slots:
    void on_btn_open_file_clicked();

    void on_btn_save_file_clicked();

    void on_sb_offset_valueChanged(double arg1);

    void on_sb_length_valueChanged(double arg1);

private:
    Ui::MainWindow *ui;

    dx::Handler handler_;
    ls::Iface sketch_;
    DrawableSketch drawable_sketch_;
    double offset_ = ls::Iface::DEFAULT_OFFSET;
    double length_ = ls::Iface::DEFAULT_SEG_LEN;
};
#endif // MAINWINDOW_H
