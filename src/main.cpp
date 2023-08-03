#include "mainwindow.h"
#include <QApplication>
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    string url = "";
    if (argc > 1)
    {
        url = string(argv[1]);
        url = url.substr(url.find_first_of("//") + 2);
    }
    // url = "https://huos1a.douyucdn2.cn/live/228989rGvLJOImDq.flv?wsAuth=ad3b38e0016a86e439dde9e919cbb826&token=web-h5-160135252-228989-c551876fc81c5479c0dc75a83977a224ea4c26a2aff8329f&logo=0&expire=0&did=19d1a4d78266a674773996d600081601&pt=2&st=0&sid=358388902&origin=tct&mix=0&isp=";
    int ret = w.initPlayer(url);
    if (ret < 0)
    {
        a.quit();
    }
    w.play();
    

    return a.exec();
}
