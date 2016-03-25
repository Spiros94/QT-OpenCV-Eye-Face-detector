#ifndef DETECTOR_H
#define DETECTOR_H

#include <QObject>
#include <QDebug>
#include <QImage>
#include <QTime>
#include <QPlainTextEdit>

// path from .pro file opencv installation
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"



class detector : public QObject
{
    Q_OBJECT
public:
    explicit detector(int camera_id = 0, QObject *parent = 0);
    ~detector();

signals:
    void thread_finish_on_prob();
    void write_to_log(QString log_writer);
    void frame_stats(int compute_time, int frame_width, int frame_height, int faces_found, int eyes_found);
    void frame_finished(bool EqualizeHist, int resizeFactor, bool coloredOutput);
    void rtnMat(cv::Mat img1);
    void finished();


public slots:
    void compute(bool EqualizeHist, int resizeFactor, bool coloredOutput); // The main thread
    void stop_camera();
    void close()
    {
        emit this->finished();
    }

private:

    cv::CascadeClassifier face_casc;    // Classifier objects
    cv::CascadeClassifier face_eye_casc;
    std::string face_casc_file = "haarcascade_frontalface_alt.xml";
    std::string face_eye_casc_file = "haarcascade_eye_tree_eyeglasses.xml";

    cv::Mat* matoriginal_o; // Mat greped from camera

    cv::Mat* matoriginal;

    cv::Mat* mateye;
    cv::Mat* matgray;

    QTime time;
    cv::VideoCapture camera;

    signed short int proccess_mean_time_output_every = 30;
    int proccess_mean_time = 0;

    int camera_id;


    bool init_vars();
    bool init_procedure = false;
};

#endif // DETECTOR_H
