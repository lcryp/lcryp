#include "ui_hello_world_widget.h"
#include <qapplication.h>
#include <qwidget.h>
#include <qpushbutton.h>
int main(int argc, char **argv) {
    QApplication a(argc, argv);
    QWidget w;
    Ui::HelloWorldWidget wm;
    wm.setupUi(&w);
    QObject::connect(wm.OkButton, SIGNAL(clicked()), &w, SLOT(close()));
    w.show();
    return a.exec();
}
