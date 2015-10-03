#include <QApplication>
#include "markovchaingui.h"

using namespace std;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MarkovChainGUI w;
    w.show();

    return a.exec();
}
