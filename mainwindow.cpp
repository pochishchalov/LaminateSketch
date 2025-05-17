#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QIcon>
#include <QFileDialog>
#include <QString>

void DrawableSketch::CreateSketch(ls::Iface &sketch, QRect window)
{
    const int pix_in_mm = MainWindow::PIX_IN_CM / 10;

    width_ = static_cast<int>(sketch.Width() * pix_in_mm);
    height_ = static_cast<int>(sketch.Height() * pix_in_mm);
    SetOrigin(window);

    for (const auto& layer : sketch.GetSketchLayers()){
        for (const auto& ply : layer){
            auto& layer = sketch_.emplace_back(DrawableLayer{});

            // Устанавливаем стиль линии у сегмента в зависимости от направления укладки
            switch (ply.ori) {
            case domain::ORI::ZERO:
                layer.style_ = Qt::PenStyle::SolidLine;
                break;
            case domain::ORI::PERP:
                layer.style_ = Qt::PenStyle::DashLine;
                break;
            default:
                layer.style_ = Qt::PenStyle::DashDotLine;
                break;
            }

            for (const auto& node : ply){
                layer.polyline_.emplace_back(QPointF{node.point.x * pix_in_mm + origin_.x(),
                                              (height_ - node.point.y * pix_in_mm) + origin_.y() - height_ });
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

    const QBrush brush(Qt::white);
    const qreal lines_width = 1.;

    for (const auto& layer : sketch_){
        painter->setPen(QPen{brush, lines_width, layer.style_}); // Для отрисовки каждого слоя со своими настройками
        painter->drawPolyline(layer.polyline_);
    }

    // Устанавливаем предыдущую "ручку" назад
    painter->setPen(old_pen);
}

void DrawableSketch::UpdateSketch(ls::Iface &sketch, QRect window)
{
    sketch_.clear();
    CreateSketch(sketch, window);
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

void DrawAxis(QPainter* painter, QPoint origin, QRect window, QRect sketch) {
    // Предварительные вычисления констант
    constexpr int PIX_IN_CM = MainWindow::PIX_IN_CM;
    constexpr int PANEL_SIZE = MainWindow::PANEL_SIZE;
    const int offset_y = PANEL_SIZE + PIX_IN_CM * 2;
    const int offset_x = PIX_IN_CM * 2;

    // Управление состоянием пера
    const auto old_pen = painter->pen();
    QPen axis_pen(Qt::white);
    axis_pen.setWidth(2);
    painter->setPen(axis_pen);

    // Вычисление координат
    const int x_pos = std::min(origin.x() - offset_x, offset_x);
    const int y_pos = (window.height() - origin.y() - offset_y < PIX_IN_CM)
                          ? origin.y() + PIX_IN_CM
                          : window.height() - offset_y;

    // Рисуем оси
    painter->drawLine(QPoint{x_pos, origin.y()}, QPoint{x_pos, origin.y() - sketch.height()});
    painter->drawLine(QPoint{origin.x(), y_pos}, QPoint{origin.x() + sketch.width(), y_pos});

    // Общие параметры для шкал
    const int strokes_step = PIX_IN_CM / 2;
    const int long_stroke = PIX_IN_CM / 2;
    const int short_stroke = long_stroke / 2;
    const int font_width = painter->fontInfo().pixelSize() / 2;

    // Лямбда для вычисления номеров и смещений
    auto get_number_and_offset = [&](int distance) -> std::pair<int, int> {
        const int number = distance / PIX_IN_CM;
        return {number, (number < 10) ? font_width / 2 : font_width};
    };

    // Унифицированная функция для рисования шкалы
    auto draw_scale = [&](bool vertical) {
        bool is_short = false;
        const int max_distance = vertical ? sketch.height() : sketch.width();
        const int base_pos = vertical ? origin.y() : origin.x();
        const int text_offset = vertical ? PIX_IN_CM : 0;

        for(int d = 0; d < max_distance; d += strokes_step) {
            const int pos = base_pos + (vertical ? -d : d);

            // Рисуем штрих
            if(vertical) {
                painter->drawLine(QPoint{x_pos - (is_short ? short_stroke : long_stroke), pos},
                                  QPoint{x_pos, pos});
            } else {
                painter->drawLine(QPoint{pos, y_pos + (is_short ? short_stroke : long_stroke)},
                                  QPoint{pos, y_pos});
            }

            // Рисуем текст для длинных штрихов
            if(!is_short) {
                const auto [number, offset] = get_number_and_offset(d);
                QPoint text_point;

                if(vertical) {
                    text_point = QPoint{x_pos - PIX_IN_CM, pos + offset};
                } else {
                    text_point = QPoint{pos - offset, y_pos + PIX_IN_CM};
                }

                painter->drawText(text_point, QString::number(number));
            }

            is_short = !is_short; // Переключение типа штриха
        }
    };

    // Рисуем обе шкалы
    draw_scale(true);  // Вертикальная
    draw_scale(false); // Горизонтальная

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
        bool is_correct_filling = sketch_.FillSketch(handler_.GetRawSketchFromData());

        if (is_correct_filling){
            drawable_sketch_.CreateSketch(sketch_, rect());
            ui->sb_offset->setEnabled(true);
            ui->sb_length->setEnabled(true);

            ui->lbl_message_text->setText("The file was successfully uploaded");
        }
        else {
            ui->lbl_message_text->setText("Error. Incorrect data in uploaded file");
        }

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

void MainWindow::on_sb_offset_valueChanged(double new_offset)
{
    offset_ = new_offset;
    sketch_.OptimizeSketch(offset_, length_);
    drawable_sketch_.UpdateSketch(sketch_, rect());
    update();
}


void MainWindow::on_sb_length_valueChanged(double new_length)
{
    length_ = new_length;
    sketch_.OptimizeSketch(offset_, length_);
    drawable_sketch_.UpdateSketch(sketch_, rect());
    update();
}

