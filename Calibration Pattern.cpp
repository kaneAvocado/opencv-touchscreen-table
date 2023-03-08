#include <Windows.h>
#include <opencv2/opencv.hpp>

#pragma comment(lib, "User32.lib")

using namespace cv; 
using namespace std;

// Эта функция вычисляет расстояние между точками сетки на основе соотношения сторон экрана и соотношения сторон сетки
int calculateDotSpacing(int screen_width, int screen_height, int grid_cols, int grid_rows) {
    // Вычислить соотношение сторон
    const float aspect_ratio = (float)screen_width / (float)screen_height;
    const float grid_aspect_ratio = (float)grid_cols / (float)grid_rows;
    int dot_spacing;
    if (aspect_ratio > grid_aspect_ratio) {
        // Экран шире, чем сетка, поэтому используйте высоту экрана в качестве основы для расстояния между точками
        dot_spacing = screen_height / (grid_rows + 1);
    }
    else {
        // Экран выше, чем сетка, поэтому используйте ширину экрана в качестве основы для интервалов между точками
        dot_spacing = screen_width / (grid_cols + 1);
    }
    return dot_spacing;
}

// Эта функция рисует сетку из черных точек на изображении
void drawGrid(Mat& img, int dot_radius, int dot_spacing, int grid_cols, int grid_rows) {
    for (int i = 0; i < grid_rows; i++) {
        for (int j = 0; j < grid_cols; j++) {
            circle(img, Point((j + 1) * dot_spacing, (i + 1) * dot_spacing), dot_radius, Scalar(0, 0, 0), FILLED);
        }
    }
}

// Эта функция создает полноэкранное окно и отображает белое изображение с сеткой черных точек
void displayCalibrationWindow(Mat& img) {
    // Получаем размеры экрана
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);

    // Создайте полноэкранное окно
    namedWindow("Calibration", WINDOW_NORMAL);
    setWindowProperty("Calibration", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);

    // Вычисление расстояния между точками сетки
    const int grid_cols = 30;
    const int grid_rows = 20;
    const int dot_radius = 5;
    int dot_spacing = calculateDotSpacing(screen_width, screen_height, grid_cols, grid_rows);

    // Нарисуйте сетку из черных точек на изображении
    drawGrid(img, dot_radius, dot_spacing, grid_cols, grid_rows);

    // Выведите изображение на экран и дождитесь нажатия клавиши
    imshow("Calibration", img);
    waitKey(0);
}

int main() {
    // Создаем белое изображение
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    Mat img(Size(screen_width, screen_height), CV_8UC3, Scalar(255, 255, 255));

    // Отображение окна калибровки
    displayCalibrationWindow(img);

    return 0;
}
