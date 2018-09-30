#include <QApplication>
#include <QDesktopWidget>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>

struct Widget: QWidget
{
    QImage m_image;
    int    m_pointSize;

    Widget(): m_pointSize(10)
    {
        setMouseTracking(true);
    }

    void resizeEvent(QResizeEvent*) override
    {
        m_image = QImage(width(), height(), QImage::Format_Mono);
        refresh();
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        m_pointSize = qAbs(event->x() - width() / 2) + qAbs(event->y() - height() / 2);
        refresh();
    }

    void refresh()
    {
        setWindowTitle(QString("%1_%2x%3").arg(m_pointSize).arg(width()).arg(height()));
        m_image.fill(0);
        const double pixelSize = qMax(width(), height());
        const double scale = m_pointSize / pixelSize;
        for (int i = 0; i < width(); i++) {
            for (int j = 0; j < height(); j++) {
                const double x = (i * 2 - width())  * scale;
                const double y = (j * 2 - height()) * scale;
                const double result = x*x + y*y;
                if (float(result) < result) {
                    m_image.setPixel(i, j, 1);
                }
            }
        }
        update();
    }

    void paintEvent(QPaintEvent*) override
    {
        QPainter(this).drawImage(rect(), m_image);
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
    label.resize(500, 500);
    label.move((app.desktop()->width() - label.width()) / 2, (app.desktop()->height() - label.height()) / 2);
    label.show();
    return app.exec();
}

