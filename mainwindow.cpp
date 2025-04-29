#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QIcon>
#include <QFileDialog>
#include <QString>

void DrawableSketch::CreateSketch(sketch::Sketch &sketch, QRect window)
{
    width_ = sketch.Width() * settings_.scale;
    height_ = sketch.Height() * settings_.scale;
    SetOrigin(window);

    for (const auto& layer : sketch.GetSketchLayers()){
        for (const auto& ply : layer){
            auto& layer = sketch_.emplace_back(DrawableLayer{});

            // Устанавливаем стиль линии у сегмента в зависимости от направления укладки
            switch (ply.orientation_) {
            case domain::ORIENTATION::ZERO:
                layer.style_ = Qt::PenStyle::SolidLine;
                break;
            case domain::ORIENTATION::PERPENDICULAR:
                layer.style_ = Qt::PenStyle::DashLine;
                break;
            default:
                layer.style_ = Qt::PenStyle::DashDotLine;
                break;
            }

            for (const auto& node : ply.nodes_){
                layer.polyline_.emplace_back(QPointF{node.point_.x * settings_.scale + origin_.x(),
                                              (height_ - node.point_.y * settings_.scale) + origin_.y() - height_ });
            }
        }
    }
}

void DrawableSketch::SetOrigin(QRect window){

    // Эскиз всегда находится в середине окна
    QPoint new_origin = {static_cast<int>((window.width() - GetWidth()) / 2),
                        static_cast<int>(window.height() - MainWindow::PANEL_SIZE
                                          - (window.height() - MainWindow::PANEL_SIZE
                                          - GetHeight()) / 2)};

    // Смещаем эскиз
    int dy = new_origin.y() - origin_.y();
    int dx = new_origin.x() - origin_.x();
    for (auto& layer : sketch_){
        for(auto& point : layer.polyline_){
            point.setY(point.y() + dy);
            point.setX(point.x() + dx);
        }
    }

    // Присваиваем новое начало эскиза
    origin_ = new_origin;
}

void DrawableSketch::DrawSketch(QPainter *painter){
    // Сохраняем предыдущую "ручку"
    const auto old_pen = painter->pen();

    const QBrush brush(settings_.color);

    for (const auto& layer : sketch_){
        painter->setPen(QPen{brush, settings_.lines_width, layer.style_}); // Для отрисовывки каждого слоя со своими настройками
        painter->drawPolyline(layer.polyline_);
    }

    // Устанавливаем предыдущую "ручку" назад
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

void DrawAxis(QPainter* painter, QPoint origin, QRect window, QRect sketch){
    const int offset_y = MainWindow::PANEL_SIZE + MainWindow::PIX_IN_CM * 2;
    const int offset_x = MainWindow::PIX_IN_CM * 2;

    const auto old_pen = painter->pen();

    QPen pen(Qt::white);
    pen.setWidth(2.);

    painter->setPen(pen);


    int x = (origin.x() - offset_x < offset_x)? origin.x() - offset_x
                                              : offset_x;
    int y = (window.height() - origin.y() - offset_y < MainWindow::PIX_IN_CM)
            ? origin.y() + MainWindow::PIX_IN_CM : window.height() - offset_y;

    // Рисуем ось Y
    painter->drawLine(QPoint{x, origin.y()},
                      QPoint{x, origin.y() - sketch.height()});

    // Рисуем ось X
    painter->drawLine(QPoint{origin.x(), y},
                      QPoint{origin.x() + sketch.width(), y});

    // Возвращает число санитиметровой шкалы и смещение для цифры
    auto number_and_offset = [](int d){
        int number = d / MainWindow::PIX_IN_CM;
        return std::make_pair(number, (number < 10) ? 3 : 7);
    };

    int strokes_step = MainWindow::PIX_IN_CM / 2;

    int long_stroke_length = MainWindow::PIX_IN_CM / 2;
    int short_stroke_length = long_stroke_length / 2;

    bool is_short_stroke = false;

    for(int dy = 0; dy < sketch.height(); dy += strokes_step){
        painter->drawLine(QPoint{(is_short_stroke)
                                ? x - short_stroke_length
                                : x - long_stroke_length,
                                origin.y() - dy},
                          QPoint{x, origin.y() - dy});
        if (is_short_stroke){
            is_short_stroke = false;
        }
        else {
            auto [number, offset] = number_and_offset(dy);
            painter->drawText(QPoint{x - MainWindow::PIX_IN_CM,
                                     origin.y() - dy + offset},
                              QString::number(number));
            is_short_stroke = true;
        }
    }

    is_short_stroke = false;

    for(int dx = 0; dx < sketch.width(); dx += strokes_step){
        painter->drawLine(QPoint{origin.x() + dx,
                                  (is_short_stroke)
                                  ? y + short_stroke_length
                                  : y + long_stroke_length,},
                          QPoint{origin.x() + dx, y});
        if (is_short_stroke){
            is_short_stroke = false;
        }
        else {
            auto [number, offset] = number_and_offset(dx);
            painter->drawText(QPoint{origin.x() + dx - offset,
                                     y + MainWindow::PIX_IN_CM},
                              QString::number(number));
            is_short_stroke = true;
        }
    }

    painter->setPen(old_pen);
}

void MainWindow::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    DrawBackground(&painter, rect());

    if (!drawable_sketch_.IsEmpty()){
        drawable_sketch_.SetOrigin(rect());
        DrawAxis(&painter, drawable_sketch_.GetOrigin(), rect(),
                 QRect{0, 0, drawable_sketch_.GetWidth(),
                  drawable_sketch_.GetHeight() }
                 );
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
        drawable_sketch_.CreateSketch(sketch_, rect());

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
