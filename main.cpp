#include "server_interface.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Server_Interface w;
    w.show();
    return a.exec();
}
