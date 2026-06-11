#include "inst.hpp"

#include <QApplication>
#include <QFile>
#include "windows/MainWindow.hpp"

void registerAllFunctions();

int main(int argc, char *argv[])
{
    // QSettings (used here and in the Updater) needs stable org/app
    // metadata; otherwise its on-disk location is platform-default and
    // varies between binaries. Set before QApplication is constructed.
    QCoreApplication::setOrganizationName("DISSCO");
    QCoreApplication::setOrganizationDomain("dissco.illinois.edu");
    QCoreApplication::setApplicationName("LASSIE");

    QApplication a(argc, argv);
    registerAllFunctions();
    Inst *m = Inst::instance();
    MainWindow *w = new MainWindow(m);
    w->show();
    return a.exec();
}
