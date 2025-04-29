#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>

#include "handler.h"
#include "sketch.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct DrawableLayer{
    QPolygonF polyline_;
    Qt::PenStyle style_ = Qt::PenStyle::NoPen;
};

struct DrawableSketchSettings{
    qreal lines_width = 1.;
    qreal scale = 3.;
    QColor color = Qt::white;
};

class DrawableSketch{
public:
    DrawableSketch()
        :width_(0.)
        ,height_(0.)
    {
    }

    void CreateSketch(sketch::Sketch& sketch, QRect window);
    void SetOrigin(QRect window);
    QPoint GetOrigin() const { return origin_; }
    qreal GetWidth() const { return width_; }
    qreal GetHeight() const { return height_; }
    bool IsEmpty() { return sketch_.empty(); }
    void DrawSketch(QPainter* painter);

private:
    std::vector<DrawableLayer> sketch_;
    DrawableSketchSettings settings_;
    qreal width_;
    qreal height_;
    QPoint origin_;
};

struct WindowSettings {
    qreal axis_width = 2.;
    qreal scale = 3.;
    QColor color = Qt::white;
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

private:
    Ui::MainWindow *ui;

    dxf::DxfHandler handler_;
    sketch::Sketch sketch_;
    DrawableSketch drawable_sketch_;
    WindowSettings settings_;
};
#endif // MAINWINDOW_H
