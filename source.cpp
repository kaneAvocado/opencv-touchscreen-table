#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <vector>
#include <iostream>
#include "windows.h"
using namespace std;
using namespace cv;

vector<Point> calibrate(Mat src) {
    // считываем координаты 4 кругов на экране
    Mat image;
    cvtColor(src, image, COLOR_BGR2GRAY);
    vector<Vec3f> circles;
    HoughCircles(image, circles, HOUGH_GRADIENT, 2, image.cols / 10, 150, 50,
        1, 100);
    //cout << circles.size() << " ";
    for (size_t i = 0; i < circles.size(); i++) {
        circle(src, Point(cvRound(circles[i][0]), cvRound(circles[i][1])),
            cvRound(circles[i][2]), Scalar(0, 0, 255), 2, 3);
    }
    imshow("Example3", src);
    vector<Point> ans(4);
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

vector<Point> get_average(vector<vector<Point>> frame_circles_cord, int cnt) {
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

Point transform_cord(vector<Point> camera_cord, Point target_cord, Point touch) {
    // переводим координаты точки из системы камеры в систему экрана
    int scr_x = abs(camera_cord[0].x - camera_cord[1].x), scr_y = abs(camera_cord[0].y - camera_cord[1].y);
    Point new_touch = Point(touch.x / target_cord.x * scr_x, touch.y / target_cord.y * scr_y);
    return touch;
}

Point get_point_in_cam(Mat frame) {
    Mat res, dist;
    Point ans = Point(0, 0);
    cvtColor(frame, res, COLOR_HSV2BGR);
    // очень важно подобрать цветовые пороги, чтобы выделять только точку касания
    inRange(res, Scalar(20, 20, 20), Scalar(60, 150, 150), dist);
    // черная магия(моменты, оптический потох, вся вот эта херня....)
    auto mom = moments(dist, 1);
    auto dm01 = mom.m01;
    auto dm10 = mom.m10;
    auto dArea = mom.m00;
    if (dArea > 20) {
        ans.x = dm10 / dArea;
        ans.y = dm01 / dArea;
    }
    return ans;
}

// типа главный
int main(int argc, char** argv) {
    // создаем окошко для вывода картинки с камеры
    namedWindow("Example3", cv::WINDOW_AUTOSIZE);
    VideoCapture cap;
    // если что можно подсунуть видео через параметр запуска
    if (argc == 1) {
        cap.open(0);
    }
    else {
        cap.open(argv[1]);
    }
    Mat frame;
    // сколько кадров берем для усреднения границы
    int cnt = 30;
    // эээээ записываем координаты специальных точек
    vector<vector<Point>> frame_circles_cord;
    for (int i = 0; i < cnt;) {
        cap >> frame;
        if (frame.empty()) break; // фильм кончился
        vector<Point> array;
        array = calibrate(frame);
        if (array.size() == 4) {
            frame_circles_cord.push_back(array);
            i++;
        }

    }
    // считаем средние двух крайних точек
    auto cords = get_average(frame_circles_cord, cnt);
    Point previous = cords[0], next = cords[1];
    // обрабатываем кадры с камеры
    while (true) {
        cap >> frame;
        if (frame.empty()) break; // фильм кончился
        // получаем координаты определенной точки на экране
        next = get_point_in_cam(frame);
        circle(frame, cords[0], 10, Scalar(255, 0, 255), 2, 3);
        circle(frame, cords[1], 10, Scalar(255, 0, 255), 2, 3);
        circle(frame, next, 10, Scalar(0, 0, 255), 2, 3);
        imshow("Example3", frame);
        // нажимаем мышкой
        // раскомментировать после калибровки
        /*
        if (abs(next.x - previous.x) >= 50 || abs(next.y - previous.y) >= 50) {
            Point screen = transform_cord(cords, Point(1920, 1080), next);
            SetCursorPos(screen.x, screen.y);
            mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, screen.x, screen.y, 0, 0); // нажали
            mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, screen.x, screen.y, 0, 0); //отпустили
        }
        */
        previous = next;
        // нажата любая кнопка или эксейп(точно не помню) -> выход
        if (waitKey(33) >= 0) break;
    }
    return 0;
}