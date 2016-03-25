#include "detector.h"

detector::detector(int camera_id,QObject *parent) : QObject(parent)
{
    this->camera_id = camera_id;
}

detector::~detector()
{

}

void detector::compute(bool EqualizeHist, int resizeFactor, bool coloredOutput)
{
    if(!this->init_procedure)
    {
        if(!this->init_vars())  // If init of thread variables fails
        {
            emit this->thread_finish_on_prob(); // Finish thread prior to init
            return;
        }
    }

    this->time.start(); // frame proccess time
    std::vector<cv::Rect_<int> > faces;    // Store face areas
    std::vector<cv::Rect_<int> > eyes;     // Store eyes areas

    bool blnFrameReadSuccessfully = this->camera.read((*this->matoriginal_o));   // grab camera frame

    while(!blnFrameReadSuccessfully || (*this->matoriginal_o).empty())    // If the frame is lost or damaged
    {
       emit this->write_to_log("[!] Unable to grep a frame compute() -- trying again");
       blnFrameReadSuccessfully = this->camera.read(*this->matoriginal_o);
    }

    cv::resize((*this->matoriginal_o), (*this->matoriginal), cv::Size( (((int)this->camera.get(3))/resizeFactor), (((int)this->camera.get(4))/resizeFactor)) );


    cv::cvtColor(*this->matoriginal, *this->matgray, CV_BGR2GRAY);    // Copy mat to new grayscale mat
    if(EqualizeHist) cv::equalizeHist(*this->matgray, *this->matgray);     // Equalizes the histogram of a grayscale image


    this->face_casc.detectMultiScale(*this->matgray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, cv::Size(30,30));

    for(size_t i = 0; i < faces.size(); i++)
    {

       int face_text_pos_x = std::max(faces[i].tl().x, 0);
       int face_text_pos_y = std::max(faces[i].tl().y, 0);

        if(coloredOutput)
        {
            cv::putText(*this->matoriginal, std::to_string(i+1), cv::Point(face_text_pos_x + 10, face_text_pos_y + 20), cv::FONT_HERSHEY_TRIPLEX, 1.0, CV_RGB(255,255,0), 2.0);
            cv::rectangle(*this->matoriginal, faces[i], cv::Scalar(255, 0, 255));
        }
        else
        {
            cv::putText(*this->matgray, std::to_string(i+1), cv::Point(face_text_pos_x + 10, face_text_pos_y + 20), cv::FONT_HERSHEY_TRIPLEX, 1.0, CV_RGB(0,0,0), 2.0);
            cv::rectangle(*this->matgray, faces[i], cv::Scalar(255, 0, 255));
        }

        (*this->mateye) = (*this->matgray)(faces[i]);   // Copy face range to the mateye obj
        this->face_eye_casc.detectMultiScale(*this->mateye, eyes, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, cv::Size(30, 30)); // Detect eyes in the current face


        for(size_t j = 0; j < eyes.size(); j++)
        {
            if(eyes.size() > 2) break; // False positive for each face unless it's a monster
            cv::Point eye_center( faces[i].x + eyes[j].x + eyes[j].width/2, faces[i].y + eyes[j].y + eyes[j].height/2 );    // Find the eye center for centering the circle
            int radius = cvRound( (eyes[j].width + eyes[j].height)*0.25 );  // Round the number to int
            if(coloredOutput)
            {
                cv::circle(*this->matoriginal, eye_center, radius, cv::Scalar( 255, 0, 0 ), 3, 8, 0 );
            }
            else
            {
                cv::circle(*this->matgray, eye_center, radius, cv::Scalar( 255, 0, 0 ), 3, 8, 0 );
            }
        }
    }

    if(coloredOutput)
    {
        emit this->rtnMat(*this->matoriginal); // Return img to main thread
        this->matoriginal->release();
        this->matgray->release();
        this->matoriginal_o->release();
        this->mateye->release();
    }
    else
    {
        emit this->rtnMat(*this->matgray); // Return img to main thread
        this->matoriginal->release();
        this->matgray->release();
        this->matoriginal_o->release();
        this->mateye->release();
    }

    int time_passed = time.elapsed();

    if(this->proccess_mean_time_output_every != 0)
    {
        --this->proccess_mean_time_output_every;
        this->proccess_mean_time += time_passed;
    }
    else    // Output average proccess time every 30 frames and zero the counters
    {
        this->proccess_mean_time_output_every = 30; // Restart the counter
        emit this->write_to_log("[+] Average compute time for 30 frames: " + QString::number(this->proccess_mean_time/this->proccess_mean_time_output_every) + "ms");
        this->proccess_mean_time = 0;
    }

    emit this->frame_stats(time_passed, ((int)this->camera.get(3)/resizeFactor), ((int)this->camera.get(4)/resizeFactor), faces.size(), eyes.size());
    emit this->frame_finished( EqualizeHist, resizeFactor, coloredOutput);
}

void detector::stop_camera()
{
    if(this->camera.isOpened())
    {
        this->camera.release();
        emit this->write_to_log("[*] Camera Released");
    }
}

bool detector::init_vars()
{
    emit this->write_to_log("[+] Initializing detector");

    QString face, eye;

    this->matoriginal_o = new cv::Mat;
    this->matoriginal= new cv::Mat;
    this->mateye= new cv::Mat;
    this->matgray= new cv::Mat;


    // Check if both cascade files exist
    if(this->face_casc.load(this->face_casc_file))
    {
        QTextStream(&face) << QString::fromStdString(this->face_casc_file) << " found";
    }
    else
    {
        emit this->write_to_log("[!!] " + QString::fromStdString(face_casc_file) + " - Face detection xml not found - Stopping");
        return false;
    }

    if(this->face_eye_casc.load(this->face_eye_casc_file))
    {
        QTextStream(&eye) << QString::fromStdString(this->face_eye_casc_file) << " found";
    }
    else
    {
        emit this->write_to_log("[!!] " + QString::fromStdString(face_eye_casc_file) + " - Eye detection xml not found - Stopping");
        return false;
    }

    emit this->write_to_log(face);
    emit this->write_to_log(eye);

    this->camera.open(this->camera_id);    // Use default camera


    if(!this->camera.isOpened())
    {
        for(int i = 0; i<30; i++)   // Read 30 times if camera is opened
        {
            if(this->camera.isOpened()) break;    // If ok continue
        }
        if(!this->camera.isOpened()) // Finish thread if can't be opened
        {
            emit this->finished();
            emit this->write_to_log("[!!] Camera cant open properly camera with id: " + QString::number(this->camera_id));
            return false;
        }
    }

    emit this->write_to_log("[+] Camera with id = " + QString::number(this->camera_id) + " opened");
    this->init_procedure = true; // Init done
    return true;
}

