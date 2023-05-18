#include <k4a/k4a.h>
#include <k4a/k4a.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/opencv.hpp>
#include <thread>
#include <deque>
#include <string>
#include <vector>
#include <iostream>
#include <windows.h>
#include <mutex>
#include <condition_variable>
#include <opencv2/flann/matrix.h>
#include <ctime>
#include <k4a/k4a.h>
#include <k4a/k4a.hpp>
using namespace std;
using namespace cv;

Point transform_cord(vector<Point> camera_cord, Point target_cord, Point touch);


vector<Point> load_vector(String filename);


vector<int> load_vector_int(String filename);

void save_vector(const vector<int>& points, String filename);