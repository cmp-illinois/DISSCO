#include "inst.hpp"

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QString>
#include <QStringList>
#include <QTimer>

#include "win/file_association.hpp"
#include "windows/MainWindow.hpp"

namespace {

class LassieApplication : public QApplication
{
public:
    LassieApplication(int &argc, char **argv) : QApplication(argc, argv)
    {
        for (const QString &arg : arguments().mid(1)) {
            if (arg.endsWith(QStringLiteral(".dissco"), Qt::CaseInsensitive)
                && QFileInfo::exists(arg)) {
                pendingFile_ = arg;
                break;
            }
        }
    }

    void setMainWindow(MainWindow *w) { mainWindow_ = w; }

    QString takePendingFile()
    {
        const QString p = pendingFile_;
        pendingFile_.clear();
        return p;
    }

    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::FileOpen) {
            const QString file = static_cast<QFileOpenEvent*>(e)->file();
            if (mainWindow_) {
                QTimer::singleShot(0, mainWindow_,
                                   [w = mainWindow_, file]() { w->openFile(file); });
            } else {
                pendingFile_ = file;
            }
            return true;
        }
        return QApplication::event(e);
    }

private:
    QString pendingFile_;
    MainWindow *mainWindow_ = nullptr;
};

} // namespace

int main(int argc, char *argv[])
{
    LassieApplication a(argc, argv);
    // No-op except on Windows release builds (LASSIE_CLAIM_DISSCO compile-time
    // gate). Self-registers .dissco -> this exe under HKCU on first launch.
    dissco::win::registerDisscoAssociation();
    Inst *m = Inst::instance();
    MainWindow *w = new MainWindow(m);
    a.setMainWindow(w);
    w->show();

    const QString pending = a.takePendingFile();
    if (!pending.isEmpty()) {
        QTimer::singleShot(0, w, [w, pending]() { w->openFile(pending); });
    }

    return a.exec();
}
