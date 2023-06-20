#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <memory>
#include <iostream>
#include <atomic>
#include <functional>
#include <future>

class PlayWindow;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    int initPlayer(std::string url);
    void play();

private:
    Ui::MainWindow *ui;
    std::shared_ptr<QTimer> imageTimer;
    std::shared_ptr<PlayWindow> playWindow;
    int videoUpdateTime;
    std::atomic<bool> isRecord;
    std::atomic<bool> isMute;
    std::string saveDir;

    void record();
    void mute();
    std::string setTip(std::string error);
    void setSaveDir();
};

#endif // MAINWINDOW_H
