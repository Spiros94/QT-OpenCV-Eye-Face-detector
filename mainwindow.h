#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QMessageBox>
#include <QPlainTextEdit>

#include <QThread>
#include <QDebug>

#include "detector.h"

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public slots:
    void write_to_log(QString log_writer);
    void frame_stats(int compute_time, int frame_width, int frame_height, int faces_found, int eyes_found);
    void ProccessFrame();
    void thread_finish_on_prob();
    void DispImgFromThread(cv::Mat img);
    void continue_computing(bool EqualizeHist, int resizeFactor, bool coloredOutput);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void closeEvent(QCloseEvent * event);

signals:
    void startThread(bool EqualizeHist, int resizeFactor, bool coloredOutput);
    void closeThread();
    void close_camera();

private slots:
    void on_actionExit_triggered();

    void on_actionStart_Detecting_triggered();

    void on_actionAbout_triggered();

    void on_actionStop_Detection_triggered();

    void on_equalizeHist_clicked(bool checked);

    void on_comboBox_currentIndexChanged(int index);

    void on_console_lines_textChanged(const QString &arg1);

    void on_checkBox_clicked(bool checked);

    void on_checkBox_2_clicked(bool checked);

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;

    void StatusBar(const QString &) const;
    void MessageOut(const QString& title, const QString& text, QMessageBox::Icon icon = QMessageBox::Information,  const QString& status_in = QString('-1'), const QString& status_out = QString(""), Qt::TextFormat text_format = Qt::RichText);
    void ConsoleAppend(QString text);

    QImage MatToQImage(cv::Mat &);    // Conversion function of mat obj from opencv to QImage for QT display
    void init_thread();

    void stop_thread_frames();  // Disconnect feedback loop
    void restart_thread_framing(ulong sleep_time); // Disconnect and re-connect thread feedback with msleep in the middle

    bool shouldClose = false;   // variable is set true only from menubar -> exit
    bool thread_exist = false;

    QThread* thread;
    detector* detect;

    bool show_output_in_external_frame = false;
    int frames_resized_to_fit = 3; // Check three times because some frames may come after disconnect as it's async communication with thread

    int frame_compute_time = 0; // How much time took for a frame to be computed

    int resize_factor = 1;
    bool UseEqualizeHist = false;
    bool coloredOutput = false;

};

#endif // MAINWINDOW_H
