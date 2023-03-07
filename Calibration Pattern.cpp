#include <opencv2/opencv.hpp>
using namespace cv;
using namespace std;

int main() {
// Создаем окно на весь экран
namedWindow("Calibration", WINDOW_NORMAL);
setWindowProperty("Calibration", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
// Создаем белое изображение
Mat img(Size(getWindowImageRect("Calibration").width, getWindowImageRect("Calibration").height), CV_8UC3, Scalar(255, 255, 255));

// Рисуем сетку черных точек на изображении
const int grid_rows = 8;
const int grid_cols = 12;
const int dot_radius = 10;
const int dot_spacing = 80;
for (int i = 0; i < grid_rows; i++) {
    for (int j = 0; j < grid_cols; j++) {
        circle(img, Point((j + 1) * dot_spacing, (i + 1) * dot_spacing), dot_radius, Scalar(0, 0, 0), FILLED);
    }
}

// Отображаем изображение и ждем нажатия клавиши
imshow("Calibration", img);
waitKey(0);

return 0;
