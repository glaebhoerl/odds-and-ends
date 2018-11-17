#include <QApplication>
#include <QBasicTimer>
#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QMessageBox>
#include <QMultiMap>
#include <QProcess>
#include <QString>
#include <QTimerEvent>

struct App: QApplication
{
    static void reportError(QString error)
    {
        QMessageBox::critical(nullptr, "Error", error, QMessageBox::Close);
    }

    static void reportFatalError(QString error)
    {
        reportError(error);
        ::exit(1);
    }

    QBasicTimer m_timer;
    QDateTime   m_lastKnownDateTime;
    QString     m_inputFileName;

    App(int& argc, char** argv): QApplication(argc, argv)
    {
        if (arguments().count() != 2) {
            reportFatalError("Expected 1 argument (the input file)!");
        }
        m_inputFileName = arguments().last();
        m_lastKnownDateTime = QDateTime::currentDateTime();
        m_timer.start(60 * 1000, Qt::VeryCoarseTimer, this);
        setQuitOnLastWindowClosed(false);
    }

    void timerEvent(QTimerEvent* event) override
    {
        if (event->timerId() != m_timer.timerId()) {
            return;
        }
        event->accept();
        QDateTime previousDateTime = m_lastKnownDateTime,
                  currentDateTime  = QDateTime::currentDateTime();
        m_lastKnownDateTime = currentDateTime;
        if (previousDateTime.secsTo(currentDateTime) >= 60 || previousDateTime.time().minute() != currentDateTime.time().minute()) {
            if (previousDateTime.secsTo(currentDateTime) > 60 * 60) {
                // Don't process actions more than an hour old
                previousDateTime = currentDateTime.addSecs(-60 * 60);
            }
            processActions(m_inputFileName, previousDateTime.time(), currentDateTime.time());
        }
    }

    static void processActions(QString inputFileName, QTime startTime, QTime endTime)
    {
        QFile inputFile(inputFileName);
        if (!inputFile.open(QFile::ReadOnly)) {
            reportError("Failed to open file: " + inputFileName);
            return;
        }

        performActions(parseActions(inputFile.readAll()), startTime, endTime);
    }

    struct Action
    {
        enum Type { Command, Text };
        Type    type;
        QString content;
        Action(Type type, QString content): type(type), content(content) { }
    };

    typedef QMultiMap<QTime, Action> Actions;

    static Actions parseActions(QByteArray input)
    {
        Actions actions;
        foreach (QByteArray line, input.split('\n')) {
            line = line.simplified();
            if (line.isEmpty() || line.startsWith('#')) {
                continue;
            }

            if (line.indexOf(' ') != 5) {
                reportError("Invalid line in input: " + line);
                return Actions();
            }
            QByteArray timeString   = line.left(5),
                       actionString = line.mid(6);

            const QTime time = QTime::fromString(timeString, "HH:mm");
            if (!time.isValid()) {
                reportError("Invalid time in input line: " + line);
                return Actions();
            }

            Action::Type type;

            if (actionString.startsWith('"') && actionString.endsWith('"')) {
                type = Action::Text;
            } else if (actionString.startsWith('`') && actionString.endsWith('`')) {
                type = Action::Command;
            } else {
                reportError("Invalid action in input line: " + line);
                return Actions();
            }

            actionString = actionString.mid(1);
            actionString.chop(1);

            actions.insert(time, Action(type, actionString));
        }
        return actions;
    }

    static void performActions(Actions actions, QTime startTime, QTime endTime)
    {
        foreach (QTime time, actions.uniqueKeys()) {
            if (isBetween(time, startTime, endTime)) {
                foreach (Action action, actions.values(time)) {
                    performAction(time, action);
                }
            }
        }
    }

    static bool isBetween(QTime time, QTime startTime, QTime endTime)
    {
        if (startTime <= endTime) {
            return time >= startTime && time <= endTime;
        } else {
            return (time >= startTime && time <= QTime(0, 0, 0).addMSecs(-1)) || (time >= QTime(0, 0, 0) && time <= endTime);
        }
    }

    static void performAction(QTime time, Action action)
    {
        switch (action.type) {
            case Action::Command: {
                if (QProcess::execute("sh", QStringList() << "-c" << action.content) != 0) {
                    reportError("Command `" + action.content + "` failed!");
                }
                return;
            }

            case Action::Text: {
                QMessageBox* messageBox = new QMessageBox;
                messageBox->setAttribute(Qt::WA_DeleteOnClose);
                messageBox->setWindowTitle(time.toString("HH:mm"));
                messageBox->setText(action.content);
                messageBox->addButton(QMessageBox::Ok);
                messageBox->setStyleSheet("QLabel { font-size: 20pt; padding-top: 50px; padding-bottom: 50px; padding-left: 75px; padding-right: 75px; }");
                messageBox->open();
                return;
            }
        }
        reportFatalError("Unknown action type!");
    }
};

int main(int argc, char** argv)
{
    return App(argc, argv).exec();
}
