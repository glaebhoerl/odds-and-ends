#include <unistd.h>

#include <QApplication>
#include <QBasicTimer>
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QString>
#include <QTimerEvent>
#include <QVector>

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

    struct Action
    {
        enum Type { Command, Text };
        Type    type;
        QString content;
        QTime   time;
        Action(Type type, QString content, QTime time): type(type), content(content), time(time) { }
    };

    struct Reminder
    {
        Action action;
        QTime  newTime;
        Reminder(Action action, QTime newTime): action(action), newTime(newTime) { }
    };

    struct RegisterReminder
    {
        virtual void operator ()(Reminder) = 0;
    };

    struct Reminders: QList<Reminder>, RegisterReminder
    {
        void operator ()(Reminder reminder) override
        {
            append(reminder);
        }
    };

    QBasicTimer m_timer;
    QDateTime   m_lastKnownDateTime;
    QString     m_inputFileName;
    Reminders   m_reminders;

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
            processActions(m_inputFileName, previousDateTime.time(), currentDateTime.time(), m_reminders);
        }

        hotReloadCheck(previousDateTime, m_inputFileName);
    }

    static void hotReloadCheck(QDateTime previousDateTime, QString inputFileName)
    {
        const QFileInfo selfInfo(arguments().first());
        if (selfInfo.exists() && selfInfo.isExecutable() && selfInfo.lastModified() >= previousDateTime) {
            QByteArray selfName = arguments().first().toUtf8();
            execv(selfName.data(), (QVector<char*>() << selfName.data() << inputFileName.toUtf8().data() << nullptr).constData());
            reportError("Failed to hot reload: " + QString(strerror(errno)));
        }
    }

    static void processActions(QString inputFileName, QTime startTime, QTime endTime, Reminders& reminders)
    {
        QFile inputFile(inputFileName);
        if (!inputFile.open(QFile::ReadOnly)) {
            reportError("Failed to open file: " + inputFileName);
            return;
        }

        performActions(parseActions(inputFile.readAll()), startTime, endTime, reminders);
    }

    struct Actions
    {
        QList<Action> actions;
        QList<uint>   remindMinutes;
    };

    static Actions parseActions(QByteArray input)
    {
        Actions actions;
        foreach (QByteArray line, input.split('\n')) {
            line = line.simplified();
            if (line.isEmpty() || line.startsWith('#')) {
                continue;
            }

            if (line.startsWith("remind ")) {
                foreach (QString remindString, line.split(' ').mid(1)) {
                    actions.remindMinutes.append(remindString.toUInt());
                    if (actions.remindMinutes.last() == 0) {
                        reportError("Invalid remind value in line: " + line);
                        return Actions();
                    }
                }
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

            actions.actions.append(Action(type, actionString, time));
        }
        return actions;
    }

    static void performActions(Actions actions, QTime startTime, QTime endTime, Reminders& reminders)
    {
        for (int i = reminders.count() - 1; i >= 0; i--) {
            if (isBetween(reminders.at(i).newTime, startTime, endTime)) {
                performAction(reminders.takeAt(i).action, actions.remindMinutes, reminders);
            }
        }

        foreach (Action action, actions.actions) {
            if (isBetween(action.time, startTime, endTime)) {
                performAction(action, actions.remindMinutes, reminders);
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

    static void performAction(Action action, QList<uint> remindMinutes, RegisterReminder& registerReminder)
    {
        switch (action.type) {
            case Action::Command: {
                if (QProcess::execute("sh", QStringList() << "-c" << action.content) != 0) {
                    reportError("Command `" + action.content + "` failed!");
                }
                return;
            }

            case Action::Text: {
                struct MessageBox: QMessageBox
                {
                    Action                       action;
                    QMap<QAbstractButton*, uint> remindButtons;
                    RegisterReminder&            registerReminder;

                    MessageBox(Action action, QList<uint> remindMinutes, RegisterReminder& registerReminder): action(action), registerReminder(registerReminder)
                    {
                        Q_ASSERT(action.type == Action::Text);
                        setAttribute(Qt::WA_DeleteOnClose);
                        setWindowFlag(Qt::WindowStaysOnTopHint);
                        setStyleSheet("QLabel { font-size: 20pt; padding-top: 50px; padding-bottom: 50px; padding-left: 75px; padding-right: 75px; }");
                        setWindowTitle(action.time.toString("HH:mm"));
                        setText(action.content);

                        foreach (uint remindMinute, remindMinutes) {
                            remindButtons.insert(addButton(QString("Gimme %1").arg(remindMinute), QMessageBox::NoRole), remindMinute);
                        }
                        addButton("Done!", QMessageBox::YesRole);
                    }

                    ~MessageBox()
                    {
                        foreach (uint remindMinute, remindButtons.values(clickedButton())) {
                            registerReminder(Reminder(action, QDateTime::currentDateTime().addSecs(60 * remindMinute).time()));
                        }
                    }
                };

                (new MessageBox(action, remindMinutes, registerReminder))->open();
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
