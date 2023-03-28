#include "Header.h"

Point transform_cord(vector<Point> camera_cord, Point target_cord, Point touch) {
    // ��������� ���������� ����� �� ������� ������ � ������� ������
    // scr - ���������� ������
    int scr_x = abs(camera_cord[0].x - camera_cord[1].x), scr_y = abs(camera_cord[0].y - camera_cord[1].y);
    // ����� ���������� ������� �� �������: ������_���������� / ����������_������ * ����������_��������_������
    // �� � �������� �.�. ������� ��������� ������ �����������
    Point new_touch = Point((float)touch.x / scr_x * target_cord.x, target_cord.y - (float)touch.y / scr_y * target_cord.y);
    return new_touch;
}



vector<Point> load_vector(String filename) {
    // ��������� ������ �������� �� �����
    // ���������� ��� ����� ������� opencv
    // ���� ���������: https://docs.opencv.org/4.x/da/d56/classcv_1_1FileStorage.html
    // 180� - �������
    FileStorage fs(filename, FileStorage::READ);
    vector<Point> ans;
    fs["points"] >> ans;
    return ans;
}

vector<int> load_vector_int(String filename) {
    // ��������� ������ �������� �� �����
    // ���������� ��� ����� ������� opencv
    // ���� ���������: https://docs.opencv.org/4.x/da/d56/classcv_1_1FileStorage.html
    // 180� - �������
    FileStorage fs(filename, FileStorage::READ);
    vector<int> ans;
    fs["points"] >> ans;
    return ans;
}

void save_vector(const vector<int>& points, String filename) {
    // ��������� ������
    FileStorage fs(filename, FileStorage::WRITE);
    fs << "points" << points;
    fs.release();
}