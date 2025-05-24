#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QIcon>
#include <QFileDialog>
#include <QString>

void Sketch::create(QRect window)
{
    const int pixPerMm = MainWindow::PixInCm / 10;

    m_width = static_cast<int>(m_interface.width() * pixPerMm);
    m_height = static_cast<int>(m_interface.height() * pixPerMm);
    setOrigin(window);

    for (const auto& layer : m_interface.sketchLayers()) {
        for (const auto& ply : layer) {
            auto& newLayer = m_layers.emplace_back(Layer{});

            switch (ply.orientation) {
            case domain::Orientation::Zero:
                newLayer.penStyle = Qt::SolidLine;
                break;
            case domain::Orientation::Perpendicular:
                newLayer.penStyle = Qt::DashLine;
                break;
            default:
                newLayer.penStyle = Qt::DashDotLine;
                break;
            }

            for (const auto& node : ply) {
                newLayer.polyline << QPointF{
                    node.point.x * pixPerMm + m_origin.x(),
                    (m_height - node.point.y * pixPerMm) + m_origin.y() - m_height
                };
            }
        }
    }
}

void Sketch::setOrigin(QRect window)
{
    const QPoint newOrigin{
        (window.width() - m_width) / 2,
        window.height() - MainWindow::PanelSize - (window.height() - MainWindow::PanelSize - m_height) / 2
    };

    const int dy = newOrigin.y() - m_origin.y();
    const int dx = newOrigin.x() - m_origin.x();

    for (auto& layer : m_layers) {
        for (auto& point : layer.polyline) {
            point += QPointF(dx, dy);
        }
    }

    m_origin = newOrigin;
}

void Sketch::draw(QPainter* painter) const
{
    const QBrush brush(Qt::white);
    const QPen oldPen = painter->pen();
    constexpr qreal lineWidth = 1.0;

    for (const auto& layer : m_layers) {
        painter->setPen(QPen{brush, lineWidth, layer.penStyle});
        painter->drawPolyline(layer.polyline);
    }

    painter->setPen(oldPen);
}

void Sketch::update(QRect window, double offset, double length)
{
    m_interface.optimizeSketch(offset, length);
    m_layers.clear();
    create(window);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_sketch(m_interface)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    setMinimumSize(800, 600);

    const auto installEventFilter = [this](QWidget* widget) {
        widget->setMouseTracking(true);
        widget->installEventFilter(this);
    };

    installEventFilter(ui->btn_open_file);
    installEventFilter(ui->btn_save_file);
    installEventFilter(ui->cb_is_binary);
    installEventFilter(ui->cb_format_selection);
    installEventFilter(ui->sb_length);
    installEventFilter(ui->sb_offset);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    drawBackground(&painter, rect());

    if (!m_sketch.isEmpty()) {
        m_sketch.setOrigin(rect());
        drawAxis(&painter, m_sketch.origin(), rect(),
                 QRect{0, 0, m_sketch.width(), m_sketch.height()});
        m_sketch.draw(&painter);
    }
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    auto setMessage = [this](const QString& text) {
        ui->lbl_message_text->setText(text);
        return true;
    };

    const auto type = event->type();

    if (obj == ui->btn_open_file) {
        if (type == QEvent::Enter) return setMessage(tr("Open DXF/DWG file"));
        if (type == QEvent::Leave) return setMessage("");
    }
    else if (obj == ui->btn_save_file) {
        if (type == QEvent::Enter) return setMessage(tr("Save to DXF file"));
        if (type == QEvent::Leave) return setMessage("");
    }
    else if (obj == ui->cb_format_selection) {
        if (type == QEvent::Enter) return setMessage(tr("Select file format"));
        if (type == QEvent::Leave) return setMessage("");
    }
    else if (obj == ui->cb_is_binary) {
        if (type == QEvent::Enter) return setMessage(tr("Use binary format"));
        if (type == QEvent::Leave) return setMessage("");
    }
    else if (obj == ui->sb_length) {
        if (type == QEvent::Enter) return setMessage(tr("Max segment length"));
        if (type == QEvent::Leave) return setMessage("");
    }
    else if (obj == ui->sb_offset) {
        if (type == QEvent::Enter) return setMessage(tr("Layer distance"));
        if (type == QEvent::Leave) return setMessage("");
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::drawBackground(QPainter* painter, QRect window)
{
    const QPixmap background("://backgrounds/my_background.png");
    const QRect source(0, background.height() - window.height(),
                       window.width(), window.height());
    painter->drawPixmap(window, background, source);
}

void MainWindow::drawAxis(QPainter* painter, QPoint origin, QRect window, QRect sketch)
{
    constexpr int pixPerCm = MainWindow::PixInCm;
    constexpr int panelSize = MainWindow::PanelSize;

    const QPen oldPen = painter->pen();
    QPen axisPen(Qt::white);
    axisPen.setWidth(2);
    painter->setPen(axisPen);

    const int xAxisPos = qMin(origin.x() - pixPerCm*2, pixPerCm*2);
    const int yAxisPos = (window.height() - origin.y() - panelSize - pixPerCm*2 < pixPerCm)
                             ? origin.y() + pixPerCm
                             : window.height() - panelSize - pixPerCm*2;

    // Draw main axis lines
    painter->drawLine(xAxisPos, origin.y(), xAxisPos, origin.y() - sketch.height());
    painter->drawLine(origin.x(), yAxisPos, origin.x() + sketch.width(), yAxisPos);

    // Scale drawing logic
    auto drawScale = [&](bool vertical) {
        const int maxLength = vertical ? sketch.height() : sketch.width();
        const int basePos = vertical ? origin.y() : origin.x();
        const int step = pixPerCm / 2;
        bool isShort = false;

        for (int d = 0; d < maxLength; d += step) {
            const int pos = basePos + (vertical ? -d : d);
            const int strokeLength = isShort ? pixPerCm/4 : pixPerCm/2;

            if (vertical) {
                painter->drawLine(xAxisPos - strokeLength, pos, xAxisPos, pos);
            } else {
                painter->drawLine(pos, yAxisPos + strokeLength, pos, yAxisPos);
            }

            if (!isShort) {
                const int number = d / pixPerCm;
                QPoint textPos = vertical
                                     ? QPoint(xAxisPos - pixPerCm*1.5, pos + pixPerCm/4)
                                     : QPoint(pos - pixPerCm/4, yAxisPos + pixPerCm*1.5);

                painter->drawText(textPos, QString::number(number));
            }

            isShort = !isShort;
        }
    };

    drawScale(true);  // Vertical scale
    drawScale(false); // Horizontal scale

    painter->setPen(oldPen);
}

void MainWindow::on_btn_open_file_clicked()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                          QDir::currentPath(), tr("DXF/DWG Files (*.dxf *.dwg)"));

    if (fileName.isEmpty()) return;

    if (m_dxHandler.importFile(fileName.toStdString())) {
        if (m_interface.fillSketch(m_dxHandler.getRawSketch())) {
            m_sketch.create(rect());
            ui->sb_offset->setEnabled(true);
            ui->sb_length->setEnabled(true);
            setStatusMessage(tr("File loaded successfully"));
        } else {
            setStatusMessage(tr("Invalid file content"));
        }
    } else {
        setStatusMessage(tr("File loading failed"));
    }
}

void MainWindow::on_btn_save_file_clicked()
{
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                          QDir::currentPath(), tr("DXF Files (*.dxf)"));

    if (fileName.isEmpty()) return;

    const DRW::Version version = [this]{
        switch (ui->cb_format_selection->currentIndex()) {
        case 0: return DRW::AC1027;
        case 1: return DRW::AC1024;
        case 2: return DRW::AC1021;
        case 3: return DRW::AC1018;
        case 4: return DRW::AC1015;
        default: return DRW::UNKNOWNV;
        }
    }();

    m_dxHandler.putRawSketch(m_interface.rawSketch());
    const bool success = m_dxHandler.exportFile(
        fileName.toStdString(),
        version,
        ui->cb_is_binary->isChecked()
        );

    setStatusMessage(success ? tr("File saved successfully") : tr("Save failed"));
}

void MainWindow::on_sb_offset_valueChanged(double offset)
{
    m_offset = offset;
    m_sketch.update(rect(), m_offset, m_length);
    update();
}

void MainWindow::on_sb_length_valueChanged(double length)
{
    m_length = length;
    m_sketch.update(rect(), m_offset, m_length);
    update();
}

void MainWindow::setStatusMessage(const QString& message)
{
    ui->lbl_message_text->setText(message);
}
