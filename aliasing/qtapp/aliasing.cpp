#include <QApplication>
#include <QDesktopWidget>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>

struct Widget: QWidget
{
    int    m_pointSize;
    int    m_pixelSize;
    QImage m_image;

    Widget(): m_pointSize(10), m_pixelSize(600), m_image(QApplication::desktop()->size(), QImage::Format_Mono)
    {
        setMouseTracking(true);
        refresh();
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        setMouseTracking(!hasMouseTracking());
        mouseMoveEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        m_pointSize = event->y() + 1;
        m_pixelSize = event->x() + 1;
        refresh();
    }

    void refresh()
    {
        setWindowTitle(QString("%1pt%2px").arg(m_pointSize).arg(m_pixelSize));

        m_image.fill(0);
        const double scale = double(m_pointSize) / double(m_pixelSize);

        for (int i = 0; i < m_image.width(); i++) {
            for (int j = 0; j < m_image.height(); j++) {
                const double x = (i * 2 - m_image.width())  * scale;
                const double y = (j * 2 - m_image.height()) * scale;
                const double result = x*x + y*y;
                if (float(result) < result) {
                    m_image.setPixel(i, j, 1);
                }
            }
        }

        update();
    }

    void moveEvent(QMoveEvent*) override
    {
        update();
    }

    void resizeEvent(QResizeEvent*) override
    {
        update();
    }

    void paintEvent(QPaintEvent*) override
    {
        const QRect subRect = QRect(mapToGlobal(QPoint(0, 0)), mapToGlobal(QPoint(width() - 1, height() - 1)));
        QPainter(this).drawImage(rect(), m_image, subRect);
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        switch (event->key()) {
            case Qt::Key_Escape: {
                QApplication::instance()->quit();
                return;
            }
            case Qt::Key_Space:
            case Qt::Key_Return:
            case Qt::Key_Enter: {
                qDebug("Saved %s: %s", qPrintable(windowTitle() + ".png"), m_image.save(windowTitle() + ".png", "png") ? "SUCCESS" : "FAILURE");
                return;
            }
        }
    }
};

int main (int argc, char** argv)
{
    QApplication app(argc, argv);
    Widget label;
    label.resize(600, 600);
    label.move((app.desktop()->width() - label.width()) / 2, (app.desktop()->height() - label.height()) / 2);
    label.show();
    return app.exec();
}

