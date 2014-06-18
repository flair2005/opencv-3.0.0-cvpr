#include <aruco/aruco.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

void Autobalance(cv::Mat* im);
void DetectArucoMarkers(cv::Mat* im, std::vector<aruco::Marker>* markers);

struct camera {
  cv::Matx33d K;
  cv::Matx<double, 5, 1> distorsion;
  cv::Size size;
};

aruco::MarkerDetector aruco_detector;
aruco::BoardConfiguration aruco_board_config;
std::vector<int> aruco_marker_map;
std::vector<cv::Point3f> aruco_board_pts;

std::vector<std::vector<int> > num_detected_markers;
std::vector<std::vector<std::vector<cv::Point2f> > > marker_pnts_;
std::vector<std::vector<std::vector<cv::Point3f> > > marker_pnts_map_;
std::vector<std::vector<cv::Mat> > rvecs_;
std::vector<std::vector<cv::Mat> > tvecs_;

bool AUTOBALANCE = true;
bool DRAWMARKERS = true;

int main(int argc, char *argv[]) {
  
  if (argc < 2) { 
    std::cerr << "Usage: ./fisheye_calibration <first device #> "
              << "<OPTIONAL: # cameras>\n";
    return -1;
  }
  
  int num_cameras = 1;
  if (argc == 3) {
    num_cameras = atoi(argv[2]);
  }

  std::vector<int> video_num(num_cameras); 
  std::vector<cv::VideoCapture> video_stream(num_cameras);
  std::string window_name = "Video stream";
  video_num[0] = atoi(argv[1]);

  for (int i = 0; i < num_cameras; ++i) {
    video_num[i] = video_num[0] + i;
    video_stream[i].open(CV_CAP_FIREWIRE + video_num[i]);  
    if (!video_stream[i].isOpened()) {
      std::cerr << "Cannot open firewire video stream: " 
                << video_num[i] << std::endl;
      return -1;
    } else {
      std::cout << "Video stream /dev/fw" << video_num[i] << " opened.\n";
    }
  }
  
  aruco_marker_map.resize(1024, -1); // in place of hashtable
  aruco_board_config.readFromFile("../data/aruco20x10_meters.yml");
  for (int i = 0; i < aruco_board_config.size(); ++i) {
    aruco_marker_map[aruco_board_config[i].id] = i;
  }

  std::vector<camera> cameras(num_cameras);
  cv::namedWindow(window_name, CV_WINDOW_AUTOSIZE); 
  std::vector<cv::Mat> frame(num_cameras);
  cv::Mat frames;
  //  cv::Mat frame_edit;

  for (int i = 0; i < num_cameras; ++i) {
    video_stream[i].set(CV_CAP_PROP_RECTIFICATION, 1);
    video_stream[i].read(frame[i]);  
    cameras[i].size = frame[i].size();
    cameras[i].K = cv::Matx33d::eye();
    cameras[i].distorsion = cv::Matx<double, 5, 1>::zeros();
    if (i == 0) 
      frames = frame[i];
    else 
      cv::hconcat(frames, frame[i], frames);
  }

  std::vector<aruco::Marker> aruco_markers_detected; ////       
  cv::imshow(window_name, frames);
  int keypress = cv::waitKey(30);
  
  while(video_stream[0].read(frame[0])) {
    frames = frame[0];

    DetectArucoMarkers(&(frame[0]), &aruco_markers_detected);

    for (int i = 1; i < num_cameras; ++i) {
      video_stream[i].read(frame[i]);  
      DetectArucoMarkers(&(frame[i]), &aruco_markers_detected);
      cv::hconcat(frames, frame[i], frames);
    }   
    cv::imshow(window_name, frames);
    keypress = cv::waitKey(30);
    if (keypress == 27) break;
  }

  for (int i = 0; i < num_cameras; ++i)
    video_stream[i].release();

  return 0;
}

void DetectArucoMarkers(cv::Mat* im, std::vector<aruco::Marker>* markers) {
  cv::Mat im_copy;
  cv::cvtColor(*im, im_copy, CV_BGR2GRAY);
  if (AUTOBALANCE) Autobalance(&im_copy);
  markers->clear();
  aruco_detector.detect(im_copy, *markers);
  if (DRAWMARKERS) {
    for (int i = 0; i < markers->size(); ++i) {
      (*markers)[i].draw(*im, cv::Scalar(0, 0, 255), 2);
    }
  }
}
                                                                 
void Autobalance(cv::Mat* im) {
  if (!(im == NULL)) {
    double min_px, max_px;
    cv::minMaxLoc(*im, &min_px, &max_px);
    double d = 255.0f/(max_px - min_px);
    int n = im->rows * im->cols;
    for (int i = 0; i < n; ++i) {
      im->data[i] = static_cast<unsigned char>((static_cast<double>(im->data[i]) - min_px) * d);
    }
  }
}
