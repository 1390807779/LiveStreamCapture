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
    // url = "https://stream-haikou-ct-124-225-94-171.edgesrv.com:443/live/228989rKqEA7TT5N_4000.flv?wsAuth=7f087e8fb3429c9809a1602c9c6be54c&token=web-h5-160135252-228989-c551876fc81c5479e7d30b63a36261b93cde0a0f21e1dae4&logo=0&expire=0&did=19d1a4d78266a674773996d600081601&pt=2&st=0&sid=353353398&vhost=play2&origin=tct&mix=0&isp=scdncthanhkidc";
    int ret = w.initPlayer(url);
    if (ret < 0)
    {
        a.quit();
    }
    w.play();
    

    return a.exec();
}
