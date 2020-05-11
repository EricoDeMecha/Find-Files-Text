#include <QApplication>
#include "findfiles.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    FindFiles window;
    window.show();
    return app.exec();
}
