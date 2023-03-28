#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include "string"
#include <vector>
#include "windows.h"


using namespace cv;
using namespace std;

// https://goo-gl.me/NLl94 - учебник на русском, дальше будет указание страниц где описана нужная информация
// https://docs.opencv.org/4.x/index.html - оригинальная документация на английском

vector<Point> calibrate(Mat src, String winName) {
    // считываем координаты 4 кругов на экране
    Mat image;
    cvtColor(src, image, COLOR_BGR2GRAY);
    vector<Vec3f> circles;
    //https://docs.opencv.org/4.x/dd/d1a/group__imgproc__feature.html#ga47849c3be0d0406ad3ca45db65a25d2d
    // 306c.
    HoughCircles(image, circles, HOUGH_GRADIENT, 2, image.cols / 10, 150, 50,
        1, 100);
    // наноси круги на кадр
    for (size_t i = 0; i < circles.size(); i++) {
        circle(src, Point(cvRound(circles[i][0]), cvRound(circles[i][1])),
            cvRound(circles[i][2]), Scalar(0, 0, 255), 2, 3);
    }
    imshow(winName, src);
    vector<Point> ans(4);
    // возвращаем только в случае если 4 круга
    if (circles.size() == 4) {
        for (int i = 0; i < 4; i++) {
            ans[i] = Point(cvRound(circles[i][0]), cvRound(circles[i][1]));
        }
        return ans;
    }
    else
        return {};
}


Point get_min_point(vector<Point> a) {
    // минимальные координаты по ху
    Point ans = a[0];
    for (int i = 1; i < a.size(); i++) {
        if (a[i].x <= ans.x) {
            ans.x = a[i].x;
        }
        if (a[i].y <= ans.y) {
            ans.y = a[i].y;
        }
    }
    return ans;
}

Point get_maxi_point(vector<Point> a) {
    // максимальные координаты по ху
    Point ans = a[0];
    for (int i = 1; i < a.size(); i++) {
        if (a[i].x >= ans.x) {
            ans.x = a[i].x;
        }
        if (a[i].y >= ans.y) {
            ans.y = a[i].y;
        }
    }
    return ans;
}

vector<Point> get_average(vector<vector<Point>>& frame_circles_cord, int cnt) {
    // усредняем координаты крайних точек
    vector<Point> array(2);
    for (int i = 0; i < 2; i++) {
        array[i] = Point(0, 0);
    }
    for (auto& fr : frame_circles_cord) {
        array[0] += get_min_point(fr);
        array[1] += get_maxi_point(fr);
    }
    for (int i = 0; i < 2; i++) {
        array[i] /= cnt;
    }
    return array;
}

void save_vector(const vector<Point>& points, String filename) {
    // сохраняем вектор
    // подробнее -->> мейн програм
    FileStorage fs(filename, FileStorage::WRITE);
    fs << "points" << points;
    fs.release();
}


int main()
{
    //// задаем параметры такие как названия окон, число кадров для усреднения, путь куда сохраняем файл
    auto const MASK_WINDOW = "Settings";
    auto const winName = "Input Video";
    auto const fps = 30;
    auto const button1 = "Show Circles";
    auto const button2 = "Update Callibration";
    // изменить если проекты называются по другому, и по другому расположены относительно друг друга
    // лучше вынести в параметры командной строки
    auto const filename = "..\\settings.txt";
    cv::namedWindow(MASK_WINDOW, WINDOW_AUTOSIZE);
    vector<Point> cords = {};
    int circles = 0, save = 0;

    // кнопка создания кругов
    createTrackbar(button1, MASK_WINDOW, &circles, 1);


    cv::VideoCapture videoCapture(0);
    
    vector<vector<Point>> array;
    bool first = 1;
    while (true) {
        // читаем картинку
        cv::Mat inputVideo;
        videoCapture.read(inputVideo);
        cv::imshow(winName, inputVideo);
        if (circles) {
            // определяем условные точки
            auto temp = calibrate(inputVideo, winName);
            if (temp.size() == 4) {
                array.push_back(temp);
            }
            if (array.size() == fps) {
                circles = 0;
                setTrackbarPos(button1, MASK_WINDOW, 0);
                if(first)
                createTrackbar(button2, MASK_WINDOW, &save, 1);
                first = 0;
            }
        }
        // сохраняем
        if (save) {
            cords = get_average(array, fps);
            Point previous = cords[0], next = cords[1];
            save = 0;
            setTrackbarPos(button2, MASK_WINDOW, 0);
            array.clear();
            save_vector(cords, filename);
            waitKey(0);
        }
        if (cords.size() == 2) {
            circle(inputVideo, cords[0], 10, Scalar(255, 0, 255), 2, 3);
            circle(inputVideo, cords[1], 10, Scalar(255, 0, 255), 2, 3);
            cv::imshow(winName, inputVideo);
        }
        if (cv::waitKey(30) == 27) break;
    }
}