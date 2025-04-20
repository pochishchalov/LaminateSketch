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

class DrawableSketch{
public:
    DrawableSketch()
        :width_(0.)
        ,height_(0.)
    {
    }

    void CreateSketch(sketch::Sketch& sketch, QPoint origin);
    void SetNewOrigin(QPoint new_origin);

    bool IsEmpty() { return sketch_.empty(); }

    void DrawSketch(QPainter* painter);

private:
    std::vector<QPolygonF> sketch_;
    qreal width_;
    qreal height_;
    QPointF origin_;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

    void paintEvent(QPaintEvent* event) override;

private slots:
    void on_btn_open_file_clicked();

    void on_btn_save_file_clicked();

private:
    Ui::MainWindow *ui;

    DxfHandler handler_;
    sketch::Sketch sketch_;
    DrawableSketch drawable_sketch_;

    QPoint origin_;
};
#endif // MAINWINDOW_H
