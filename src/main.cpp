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
    // url = "https://huos1a.douyucdn2.cn/live/228989ruFCfzWp8e_4000.flv?wsAuth=59fd86f7d0f4d7df6cdab0a5718ecae9&token=web-h5-160135252-228989-c551876fc81c5479c1d3c41d42896babcbadf39512db2570&logo=0&expire=0&did=19d1a4d78266a674773996d600081601&pt=2&st=0&sid=358832858&origin=tct&mix=0&isp=";
    int ret = w.initPlayer(url);
    if (ret < 0)
    {
        a.quit();
    }
    w.play();
    

    return a.exec();
}
