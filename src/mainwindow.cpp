#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <PlayWindow.h>
#include <chrono>
#include <filesystem>
#include <QFileDialog>

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    videoUpdateTime(10),
    playWindow(nullptr),
    imageTimer(nullptr),
    isRecord(true),
    isMute(true)
{
    ui->setupUi(this);
    this->setFixedSize(this->width(), this->height());
    imageTimer = make_shared<QTimer>(this);
    // ui->tipLabel->setText("tip test");
    connect(ui->captureButton, &QPushButton::clicked, this, &MainWindow::record);
    connect(ui->muteButton, &QPushButton::clicked, this, &MainWindow::mute);
    connect(ui->dirButton, &QPushButton::clicked, this, &MainWindow::setSaveDir);
}

MainWindow::~MainWindow()
{
    delete ui;
}

int MainWindow::initPlayer(std::string url)
{
    saveDir = filesystem::current_path().generic_string();
    ui->tipLabel->setText(QString::fromStdString(saveDir));
    playWindow = make_shared<PlayWindow>(ui->frame->width(), ui->frame->height());
    playWindow->tcallback = bind(&MainWindow::setTip, this, placeholders::_1);
    int ret = playWindow->initWindowFrom((void *)(ui->frame->winId()));
    if (ret < 0)
    {
        return ret;
    }
    ret = playWindow->initMedia(url);
    if (ret < 0)
    {
        return ret;
    }
    playWindow->startDecode();
    return 0;
}

void MainWindow::play()
{
    playWindow->updateAudioWithSDL();
    imageTimer->setInterval(videoUpdateTime);
    imageTimer->connect(imageTimer.get(), &QTimer::timeout, 
                        [timer = imageTimer, player = playWindow, time = &videoUpdateTime ]()
                        { player->updateVideo(time);
                        timer->setInterval(*time);});
    imageTimer->start();
}

void MainWindow::record()
{
    if (isRecord)
    {
        playWindow->startEncode(saveDir);
        ui->captureButton->setText("stop record");
    }
    else
    {
        playWindow->cancelEncode();
        ui->captureButton->setText("record");
    }
    isRecord = !isRecord;
}

void MainWindow::mute()
{
    if (isMute)
    {
        ui->muteButton->setText("vocal");
    }
    else
    {
        ui->muteButton->setText("mute");
    }
    playWindow->changeMuteOrVocal();
    isMute = !isMute;
}

std::string MainWindow::setTip(std::string error)
{
    ui->tipLabel->setText(QString::fromStdString(error));
    return std::string();
}

void MainWindow::setSaveDir()
{
    QFileDialog *dialog = new QFileDialog(this);
    dialog->setFileMode(QFileDialog::FileMode::Directory);
    dialog->setOption(QFileDialog::DontUseNativeDialog);
    dialog->exec();
    QList<QUrl> lists = dialog->selectedUrls();
    for (auto &&url : lists)
    {
        saveDir = url.toString().toStdString();
    }
    saveDir = saveDir.substr(saveDir.find("///") + 3);
    // QString selectDir = QFileDialog::getExistingDirectory(this, "select record save dir", QString::fromStdString(saveDir), QFileDialog::ShowDirsOnly);
    // saveDir = selectDir.toStdString();
    ui->tipLabel->setText(QString::fromStdString(saveDir));
    delete dialog;
}
