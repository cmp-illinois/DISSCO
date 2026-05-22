#include "inst.hpp"

#include <QApplication>
#include <QFile>
#include "windows/MainWindow.hpp"

void registerAllFunctions();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    registerAllFunctions();
    Inst *m = Inst::instance();
    MainWindow *w = new MainWindow(m);
    w->show();
    return a.exec();
}
