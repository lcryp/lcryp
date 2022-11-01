#include "canvas.h"
#include <qapplication.h>
#include <qvbox.h>
#include <qpushbutton.h>
class Window : public QMainWindow
{
public:
    Window()
    {
        setCaption("QCanvas test");
        QVBox* vb = new QVBox(this);
        setCentralWidget(vb);
        Canvas* c = new Canvas(vb);
        QPushButton* b = new QPushButton("Change color", vb);
        connect(b, SIGNAL(clicked()), c, SLOT(change_color()));
    }
};
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Window *w = new Window();
    app.setMainWidget(w);
    w->show();
    return app.exec();
}
