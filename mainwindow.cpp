#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QIcon>
#include <QFileDialog>
#include <QString>

inline QPoint GetOrigin(int height){
    return QPoint{60, height - 150};
}

void DrawableSketch::CreateSketch(sketch::Sketch &sketch, QPoint origin)
{
    width_ = sketch.Width() * 3;
    height_ = sketch.Height() * 3;
    origin_ = {origin.x() + 30., origin.y() - 30.};

    for (const auto& layer : sketch.GetSketchLayers()){
        for (const auto& ply : layer){
            auto& layer = sketch_.emplace_back(DrawableLayer{});

            layer.orientation_ = ply.orientation_;

            for (const auto& node : ply.nodes_){
                //QPointF point {node.point_.x + origin_.x(), height_ - node.point_.y + origin_.y()};
                layer.polyline_.emplace_back(QPointF{node.point_.x * 3 + origin_.x(),
                                              (height_ - node.point_.y * 3) + origin_.y() - height_ });
            }
        }
    }
}

void DrawableSketch::SetNewOrigin(QPoint new_origin){
    int dy = new_origin.y() - origin_.y();

    for (auto& layer : sketch_){
        for(auto& point : layer.polyline_){
            point.setY(point.y() + dy);
        }
    }
    origin_ = new_origin;
}

void DrawableSketch::DrawSketch(QPainter *painter){

    const auto old_pen = painter->pen();

    QPen zero_pen(Qt::white);
    zero_pen.setStyle(Qt::PenStyle::SolidLine);
    zero_pen.setWidth(1);

    QPen perp_pen(Qt::white);
    perp_pen.setStyle(Qt::PenStyle::DashLine);
    perp_pen.setWidth(1);

    QPen other_pen(Qt::white);
    other_pen.setStyle(Qt::PenStyle::DashDotLine);
    other_pen.setWidth(1);

    for (const auto& layer : sketch_){

        switch (layer.orientation_) {
        case domain::ORIENTATION::ZERO:
            painter->setPen(zero_pen);
            break;
        case domain::ORIENTATION::PERPENDICULAR:
            painter->setPen(perp_pen);
            break;
        default:
            painter->setPen(other_pen);
            break;
        }

        painter->drawPolyline(layer.polyline_);
    }

    painter->setPen(old_pen);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Устанавливаем главную иконку приложения
    setWindowIcon(QIcon(":/icons/app_icon.png"));

    // Устанавливаем минимальное разрешение окна
    setMinimumSize(640, 480); // Минимум 640x480

    // Включаем отслеживание событий мыши для кнопок и др.
    // и устанавливаем фильтр событий

    ui->btn_open_file->setMouseTracking(true);
    ui->btn_open_file->installEventFilter(this);

    ui->btn_save_file->setMouseTracking(true);
    ui->btn_save_file->installEventFilter(this);

    ui->cb_is_binary->setMouseTracking(true);
    ui->cb_is_binary->installEventFilter(this);

    ui->cb_format_selection->setMouseTracking(true);
    ui->cb_format_selection->installEventFilter(this);

    origin_ = GetOrigin(height());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void DrawBackground(QPainter* painter, QRect window){

    QPixmap background("://backgrounds/my_background.png");

    // Смещение для обрезки по оси Y
    int offset = background.height() - window.height();

    // Область изображения для отрисовки (с учетом смещения)
    QRect drawing_area(
        0.,
        offset,
        window.width(),
        window.height()
        );

    // Отрисовка обрезанной части изображения
    painter->drawPixmap(window, background, drawing_area);
}

void DrawAxis(QPainter* painter, QPoint origin, int width){
    const int offset_y = 30;
    const int offset_x = 60;

    // Рисуем ось Y
    painter->drawLine(QPoint{origin.x(), offset_y}, origin);

    // Рисуем ось X
    painter->drawLine(origin, QPoint{width - offset_x, origin.y()});

    int step = 15;
    bool is_short_line = true;
    for(int y = origin.y() - step, distance = -5; y >= offset_y; y -= step, distance += 5){
        painter->drawLine(QPoint{(is_short_line) ? 52 : 45, y},
                          QPoint{origin.x(), y});
        if (is_short_line){
            is_short_line = false;
        }
        else {
            painter->drawText(QPoint{25, y + 5}, QString::number(distance));
            is_short_line = true;
        }
    }

    const int right_border = width - offset_x;
    is_short_line = true;
    for(int x = offset_x + step, distance = -5; x < right_border; x += step, distance += 5){
        painter->drawLine(QPoint{x, (is_short_line) ? origin.y() + 8
                                                    : origin.y() + 15},
                          QPoint{x, origin.y()});
        if (is_short_line){
            is_short_line = false;
        }
        else {
            painter->drawText(QPoint{x - 10, origin.y() + 30}, QString::number(distance));
            is_short_line = true;
        }
    }
}

void MainWindow::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    DrawBackground(&painter, rect());

    QPen pen(Qt::white);
    pen.setWidth(2);

    painter.setPen(pen);

    origin_ = GetOrigin(height());

    DrawAxis(&painter, origin_, width());

    pen.setWidth(1);
    painter.setPen(pen);

    if (!drawable_sketch_.IsEmpty()){
        drawable_sketch_.SetNewOrigin({origin_.x() + 30, origin_.y() - 30});
        drawable_sketch_.DrawSketch(&painter);
    }

}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    // Обрабатываем события

    if (obj == ui->btn_open_file) {
        if (event->type() == QEvent::Enter) {
            ui->lbl_message_text->setText("Click and select a dxf or dwg file to upload");
            return true;
        } else if (event->type() == QEvent::Leave) {
            ui->lbl_message_text->clear();
            return true;
        }
    }
    else if (obj == ui->btn_save_file) {
        if (event->type() == QEvent::Enter) {
            ui->lbl_message_text->setText("Click and enter a file name or select a dxf file to save");
            return true;
        } else if (event->type() == QEvent::Leave) {
            ui->lbl_message_text->clear();
            return true;
        }
    }
    else if (obj == ui->cb_format_selection) {
        if (event->type() == QEvent::Enter) {
            ui->lbl_message_text->setText("Select the file format to save");
            return true;
        } else if (event->type() == QEvent::Leave) {
            ui->lbl_message_text->clear();
            return true;
        }
    }
    else if (obj == ui->cb_is_binary) {
        if (event->type() == QEvent::Enter) {
            ui->lbl_message_text->setText("To save the file in binary format, check the box");
            return true;
        } else if (event->type() == QEvent::Leave) {
            ui->lbl_message_text->clear();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::on_btn_open_file_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
                        this,
                        QString("Открыть файл"),
                        QDir::currentPath(),
                        "*.dxf;*.dwg"
    );

    if (fileName.isEmpty()){
        return;
    }

    // Импортируем файл в handler
    bool success = handler_.ImportFile(fileName.toStdString());

    if (success){
        // Наполняем эскиз
        sketch_.FillSketch(handler_.GetRawSketchFromData());
        drawable_sketch_.CreateSketch(sketch_, origin_);

        ui->lbl_message_text->setText("The file was successfully uploaded");
    }
    else{
        ui->lbl_message_text->setText("Error. Failed to upload file");
    }
}


void MainWindow::on_btn_save_file_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    QString("Сохранить файл"),
                                                    QDir::currentPath(),
                                                    "*.dxf"
    );

    if (fileName.isEmpty()){
        return;
    }

    DRW::Version export_version;

    switch (ui->cb_format_selection->currentIndex()) {
    case 0:
        export_version = DRW::Version::AC1027;
        break;
    case 1:
        export_version = DRW::Version::AC1024;
        break;
    case 2:
        export_version = DRW::Version::AC1021;
        break;
    case 3:
        export_version = DRW::Version::AC1018;
        break;
    case 4:
        export_version = DRW::Version::AC1015;
        break;
    default:
        export_version = DRW::Version::UNKNOWNV;
        break;
    }

    bool is_binary = ui->cb_is_binary->isChecked();

    // Получаем эскиз
    handler_.PutRawSketchToData(sketch_.GetRawSketch());

    // Экспортируем файл из handler'а
    bool success = handler_.ExportFile(fileName.toStdString(), export_version, is_binary);

    if (success){
        ui->lbl_message_text->setText("The file was successfully saved");
    }
    else{
        ui->lbl_message_text->setText("Error. The file was not saved");
    }
}
