#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qRegisterMetaType< cv::Mat >("cv::Mat"); // Register type for signals and slots

    ui->actionStop_Detection->setEnabled(false);    // Disable Stop Detection menu button
    ui->console_out->setReadOnly(true);             // Set log window as read only
    ui->console_out->clear();                       // Clear log window
    ui->console_out->setMaximumBlockCount(500);     // Set maximum lines

    ui->console_lines->setValidator( new QIntValidator(0, 1000, this) );
    ui->camera_id->setValidator(new QIntValidator(0, 10, this));

    this->ConsoleAppend("Program Started..");
    this->StatusBar("Ready to Detect..");
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::frame_stats(int compute_time, int frame_width, int frame_height, int faces_found, int eyes_found)
{
    QString compute_time_l = QString::number(compute_time);
    QString frame_width_l = QString::number(frame_width);
    QString frame_height_l = QString::number(frame_height);
    QString faces_found_l = QString::number(faces_found);
    QString eyes_found_l = QString::number(eyes_found);

    this->frame_compute_time = compute_time;

    this->ConsoleAppend("Frame proccess time: " + compute_time_l + " ms || Faces: " + faces_found_l + " and " + eyes_found_l + " eyes found || Camera Frame Size: " + frame_width_l + "x" + frame_height_l);
    this->StatusBar("Detecting..");

    if(!this->frames_resized_to_fit == 0 && !this->show_output_in_external_frame) // Resize main Window to fit webcam picture
    {
        if(frame_height <= 1080)
        {
            this->frames_resized_to_fit--;
            ui->lblProc->resize(frame_width, frame_height);
            this->resize(this->size().width(), frame_height + ui->lblProc->y() + 50);
        }
        else
        {
            this->resize(this->size().width(), (frame_height/2) + ui->lblProc->y() + 100); // If you have a ultra HD camera, resize manualy mate...
        }
    }
}

void MainWindow::ProccessFrame()
{
    ui->actionStart_Detecting->setEnabled(false);   // Disable start button from menu

    if(!thread_exist)
    {
        this->init_thread();
    }
    this->frames_resized_to_fit = 3;
    emit this->startThread(this->UseEqualizeHist, this->resize_factor, this->coloredOutput);
}

void MainWindow::continue_computing(bool EqualizeHist, int resizeFactor, bool coloredOutput)
{
    emit this->startThread(EqualizeHist, resizeFactor, coloredOutput);
}


void MainWindow::thread_finish_on_prob()
{
    ui->actionStart_Detecting->setEnabled(true);
    ui->actionStop_Detection->setEnabled(false);
    emit this->closeThread();   // Emit the finish thread
    this->thread_exist = false;
}


void MainWindow::DispImgFromThread(cv::Mat img)
{
    if(this->show_output_in_external_frame)
    {
        cv::namedWindow("Output Image");
        cv::imshow("Output Image", img);
    }
    else
    {
        ui->lblProc->setPixmap(QPixmap::fromImage(this->MatToQImage(img))); // Output it to the program
    }
}


void MainWindow::on_actionStart_Detecting_triggered() // MenuBar item for start detecting faces
{
    this->StatusBar("Starting Threading..");
    this->ProccessFrame();
    ui->actionStop_Detection->setEnabled(true);
}


void MainWindow::closeEvent(QCloseEvent * event)    //
{
    if(!this->shouldClose)
    {
        this->MessageOut("Exiting Properly", "Use from the top menu bar File->Exit\nto exit properly");
        event->ignore();
    }
    else
    {
        QApplication::quit();
    }
}


void MainWindow::stop_thread_frames()
{
    if(this->thread_exist)
    {
        disconnect(this->detect, SIGNAL(frame_finished(bool,int,bool)), this, SLOT(continue_computing(bool,int,bool)));
    }
}

void MainWindow::restart_thread_framing(ulong sleep_time)
{
    if(this->thread_exist)
    {
       this->stop_thread_frames(); // Restart compute feedback loop with new params
       this->thread->msleep(sleep_time); // Give it a little time
       connect(this->detect, SIGNAL(frame_finished(bool,int,bool)), this, SLOT(continue_computing(bool,int,bool))); // Reconnect compute
       this->ProccessFrame();   // Start again feedback loop
    }
}

void MainWindow::on_actionStop_Detection_triggered()
{
    this->write_to_log("Stopping detection");
    this->StatusBar("Detection Stopped");

    this->stop_thread_frames();

    emit this->close_camera(); // Emit the signal for the connection above
    emit this->closeThread();   // Emit the finish thread

    this->thread_exist = false; // Thread existance flag to false

    ui->actionStop_Detection->setEnabled(false);    // Disable stop button
    ui->actionStart_Detecting->setEnabled(true);    // Enable start button
}

void MainWindow::on_actionExit_triggered()  // Close only from the menubar exit selection
{
    if(ui->actionStop_Detection->isEnabled())
    {
        this->MessageOut("Deteciton Running", "Detection is running. Stop Detection first before exiting", QMessageBox::Critical);
        return;
    }
    this->shouldClose = true;
    this->close();
}


QImage MainWindow::MatToQImage(cv::Mat& mat)
{
    if(mat.channels() == 1) {                           // if 1 channel (grayscale or black and white) image
        return QImage((uchar*)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Indexed8);     // return QImage
    } else if(mat.channels() == 3) {                    // if 3 channel color image
        cv::cvtColor(mat, mat, CV_BGR2RGB);             // flip colors
        return QImage((uchar*)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);       // return QImage
    } else {
        this->write_to_log("cv::Mat image channel was not 1 or 3. Something gone wrong..");
    }
    return QImage();        // return a blank QImage if the above did not work
}


void MainWindow::init_thread()
{
    this->thread = new QThread;
    this->detect = new detector(ui->camera_id->text().toInt());

    this->detect->moveToThread(this->thread);

    connect(this,         SIGNAL(startThread(bool , int, bool)),        this->detect,   SLOT(compute(bool , int, bool)));    // Call compute immediatly after thread start
    connect(this,         SIGNAL(closeThread()),                        this->detect,   SLOT(close()));  // Trigger finish emittion from main thread
    connect(this,         SIGNAL(close_camera()),                       this->detect,   SLOT(stop_camera()));   // Close - Release the camera
    connect(this->detect, SIGNAL(write_to_log(QString)),                this,           SLOT(write_to_log(QString)));
    connect(this->detect, SIGNAL(frame_stats(int, int, int, int, int)), this,           SLOT(frame_stats(int, int, int, int, int)));
    connect(this->detect, SIGNAL(thread_finish_on_prob()),              this,           SLOT(thread_finish_on_prob())); // Finish thread when something gone wrong in init
    connect(this->detect, SIGNAL(frame_finished(bool,int,bool)),        this,           SLOT(continue_computing(bool,int,bool)));
    connect(this->detect, SIGNAL(rtnMat(cv::Mat)),                      this,           SLOT(DispImgFromThread(cv::Mat))); // Return Mat image from thread to display in form

    connect(this->detect, SIGNAL(finished()), this->thread, SLOT(quit()));      // Kill thread procedure
    connect(this->detect, SIGNAL(finished()), this->detect, SLOT(deleteLater()));
    connect(this->thread, SIGNAL(finished()), this->thread, SLOT(deleteLater()));

    this->thread->start();
    this->thread_exist = true;
}



void MainWindow::on_equalizeHist_clicked(bool checked)  // Equalize histogram checkbox
{
    this->UseEqualizeHist = checked;
    this->restart_thread_framing(this->frame_compute_time * 2);
}


void MainWindow::on_comboBox_currentIndexChanged(int index)     // Resize image factor
{
    this->resize_factor = index +1; // index for checkbox is 0 so +1
    this->frames_resized_to_fit = 3;
    this->restart_thread_framing(this->frame_compute_time * 2);
}


void MainWindow::on_checkBox_clicked(bool checked)     // Show output image in external or internal frame
{
    this->show_output_in_external_frame = checked;
}


void MainWindow::on_checkBox_2_clicked(bool checked)    // When coloredOutput checkbox checked
{
    this->coloredOutput = checked;
    this->restart_thread_framing(this->frame_compute_time * 2);
}


void MainWindow::on_pushButton_clicked()    // On clear log pushbon pussed clear log
{
    ui->console_out->clear();
}

void MainWindow::on_console_lines_textChanged(const QString &arg1)  // Setting the maximum console lines visible
{
    ui->console_out->setMaximumBlockCount( arg1.toInt() );
}


void MainWindow::on_actionAbout_triggered()
{
    this->StatusBar("About");
    QMessageBox about(this);
    about.setIcon(QMessageBox::Question);
    about.setWindowTitle("About this program");
    about.setTextFormat(Qt::RichText);
    about.setText("This program is made by Mitropoylos Spiros with Qt 5.4.2<br /> Mail me at: <a href='mailto:cont@eyrhka.gr'>cont@eyrhka.gr</a> <br /> My site: <a href='http://www.eyrhka.gr'>www.eyrhka.gr</a>");
    about.exec();
    this->StatusBar("");
}

void MainWindow::write_to_log(QString log_writer)
{
    this->ConsoleAppend(log_writer);
}

void MainWindow::ConsoleAppend(QString text)
{
    ui->console_out->appendHtml(text.append("\n"));
}


void MainWindow::StatusBar(const QString& message = "") const
{
    ui->statusBar->showMessage(message);
}

// this->MessageOut("Title", "Text Inside", [QMessageBox::Icon], ["statusbar text with in the function"], ["statusbar text with exiting the function"], [Qt::TextFormat]);
void MainWindow::MessageOut(const QString& title, const QString& text, QMessageBox::Icon icon,  const QString& status_in, const QString& status_out, Qt::TextFormat text_format)
{
    if(status_in != QString('-1')) this->StatusBar(status_in);
    QMessageBox message(this);
    message.setIcon(icon);
    message.setWindowTitle(title);
    message.setTextFormat(text_format);
    message.setText(text);
    message.exec();
    if(status_out != QString('-1')) this->StatusBar(status_out);
}


