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

class Sketch {
public:
    explicit Sketch(ls::Iface& interface)
        : m_interface(interface)
    {
    }

    void create(QRect window);
    void setOrigin(QRect window);
    QPoint origin() const { return m_origin; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool isEmpty() const { return m_layers.empty(); }
    void draw(QPainter* painter) const;
    void update(QRect window, double offset, double length);

private:
    struct Layer {
        QPolygonF polyline;
        Qt::PenStyle penStyle = Qt::NoPen;
    };

    std::vector<Layer> m_layers;
    ls::Iface& m_interface;
    int m_width = 0;
    int m_height = 0;
    QPoint m_origin;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    constexpr static int PANEL_SIZE = 120;
    constexpr static int PIX_IN_CM = 30;

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void drawBackground(QPainter* painter, QRect window);
    void drawAxis(QPainter* painter, QPoint origin, QRect window, QRect sketch);
    void setStatusMessage(const QString& message);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void on_btn_open_file_clicked();
    void on_btn_save_file_clicked();
    void on_sb_offset_valueChanged(double offset);
    void on_sb_length_valueChanged(double length);

private:
    Ui::MainWindow *ui;

    dx::Handler m_dxHandler;
    ls::Iface m_interface;
    Sketch m_sketch;
    double m_offset = ls::Iface::DEFAULT_OFFSET;
    double m_length = ls::Iface::DEFAULT_SEG_LEN;
};

#endif // MAINWINDOW_H
