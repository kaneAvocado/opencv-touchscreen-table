#include "Header.h"


// https://goo-gl.me/NLl94 - учебник на русском, дальше будет указание страниц где описана нужная информация
// https://docs.opencv.org/4.x/index.html - оригинальная документация на английском



Mat computeOpticalFlow(Mat previous_frame, Mat current_frame) {
    // вычисляем параметры оптического потока
    // задаем параметры
    // подробнее: https://docs.opencv.org/4.x/dc/d6b/group__video__track.html#ga5d10ebbd59fe09c5f650289ec0ece5af
    // учебник - 498с.
    double pyrScale = 0.5;
    int levels = 1;
    int winsize = 13;
    int iterations = 2;
    int polyN = 7;
    double polySigma = 1.5;
    int flags = 0;
    Mat flow;
    // Вычисляем оптический поток методом Фарнебека
    calcOpticalFlowFarneback(previous_frame, current_frame, flow, pyrScale, levels, winsize, iterations, polyN, polySigma, flags);
    // переводим картинку из серых тонов, в пространство ргб
    cvtColor(previous_frame, previous_frame, COLOR_GRAY2RGB);
    // делим поток на оси х и у
    Mat flow_xy[2], mag, ang;
    split(flow, flow_xy);
    Mat flow_magnitude, flow_angle;
    // переводим координаты в полярные
    cv::cartToPolar(flow_xy[0], flow_xy[1], flow_magnitude, flow_angle);

    // задаем параметры фильтра мусора
    double threshold = 5.0;
    cv::Mat flow_magnitude_thresholded;
    // https://docs.opencv.org/4.x/d7/d1b/group__imgproc__misc.html#gae8a4a146d1ca78c626a53577199e9c57
    cv::threshold(flow_magnitude, flow_magnitude_thresholded, threshold, 1.0, cv::THRESH_BINARY);

    // переводим в формат CV_8UC1
    cv::Mat flow_magnitude_thresholded_8u;
    flow_magnitude_thresholded.convertTo(flow_magnitude_thresholded_8u, CV_8UC1, 255.0);

    // Преобразуем фильтры в карту цветов
    // https://docs.opencv.org/4.x/d3/d50/group__imgproc__colormap.html#gadf478a5e5ff49d8aa24e726ea6f65d15
    cv::Mat flow_magnitude_colormap;
    cv::applyColorMap(flow_magnitude_thresholded_8u, flow_magnitude_colormap, cv::COLORMAP_JET);
    //imshow("Example4", flow_magnitude_colormap);
    return flow_magnitude_colormap;
}



Point get_point_in_cam(Mat frame, vector<int> &hsv_set) {
    // получаем точку касания путем усреднения всех областей попадающих в диапазон фильтра
    Mat res, dist;
    Point ans = Point(0, 0);
    frame.copyTo(res);
    // очень важно подобрать цветовые пороги, чтобы выделять только точку касания
    // переводим из пространства цветов брг в hsv 
    inRange(res, Scalar(hsv_set[0], hsv_set[1], hsv_set[2]), Scalar(hsv_set[3], hsv_set[4], hsv_set[5]), dist);
    // отрисовываем полученное изображение
    //imshow("Example5", dist);
    // считаем моменты 
    // https://docs.opencv.org/4.x/d8/d23/classcv_1_1Moments.html
    // учебник - 366с.
    auto mom = moments(dist, 1);
    auto dm01 = mom.m01;
    auto dm10 = mom.m10;
    auto dArea = mom.m00;
    // размер искомой области больше 2-х пикселей
    if (dArea > 2) {
        ans.x = dm10 / dArea;
        ans.y = dm01 / dArea;
    }
    return ans;
}
void procesing_frame(vector<int> hsv_set, Mat previous_frame, Mat current_frame, Point & output, mutex & control) {
    
        // вычисляем оптический поток, он в флоу сохранится
        Mat result = computeOpticalFlow(previous_frame, current_frame);
        // получаем координату точки касания
        auto ans = get_point_in_cam(result, hsv_set);        
        unique_lock<mutex> ul(control);
        output = ans;
        ul.unlock();
}

Point split_frame(vector<int>& hsv_set, Mat& previous_frame, Mat& current_frame, Point & previous, int numRows, int numCols) {
    int height = previous_frame.rows;
    int width = previous_frame.cols;
    int subImgHeight = (double)height / numRows;
    int subImgWidth = (double)width / numCols;
    vector<vector<Point>> ans(numRows, vector<Point>(numCols));
    mutex control;
    vector<thread> t;
    // делим картинку на numRows*numCols изображений и обрабатываем каждое в отдельном потоке
    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < numCols; j++) {
            int x = j * subImgWidth;
            int y = i * subImgHeight;

            auto roi = Rect(x, y, subImgWidth, subImgHeight);
            try {
                Mat subImage_prev = previous_frame(roi).clone();
                Mat subImage_current = current_frame(roi).clone();
                t.push_back(thread(procesing_frame, hsv_set, subImage_prev, subImage_current, ref(ans[i][j]), ref(control)));
            }
            catch (cv::Exception& e) {
                cerr << "Reason: " << e.msg << endl;
                exit(1);
            }
        }
    }
    for (auto& i : t) {
        i.join();
    }
    // выбираем самую близкую точку к предыдущей
    Point answer = ans[0][0];
    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < numCols; j++) {
            int x = j * subImgWidth;
            int y = i * subImgHeight;
            if (ans[i][j] != Point(0, 0)) {
                ans[i][j].x += x;
                ans[i][j].y += y;
                answer = ans[i][j];
            }
        }
    }
    auto temp2 = answer - previous;
    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < numCols; j++) {
            if (ans[i][j] == Point(0, 0))continue;
            auto temp1 = ans[i][j] - previous;
            if (temp1.x * temp1.x + temp1.y + temp1.y < temp2.x * temp2.x + temp2.y + temp2.y) {
                answer = ans[i][j];
                temp2 = answer - previous;
            }
        }
    }
    return answer;
}


// типа главный
int main(int argc, char** argv) {
    // создаем окошки для вывода картинки с камеры и результатов работы
    auto const MASK_WINDOW = "Settings";
    auto const filename_hsv = "hsv_settings.txt";
    // подбираем имепрически :)
    // ну из того соображения что ширина и высота рои соответсвенно должна делиться на эти числа без остатка я
    int numRows = 8, numCols = 2;

    VideoCapture cap;
    // если что можно подсунуть видео через параметр запуска
    if (argc <= 2) {
        cap.open(0);
    }
    else {
        cap.open(argv[2]);
    }
    Mat frame;
    // загружаем координаты угловых точек, которые определяем через 2 программу
    auto cords = load_vector("settings.txt");
    Point previous = cords[0], next = cords[1];
    // обрабатываем кадры с камеры
    // читаем первый кадр т.к. он будет пустым
    for(int i = 0; i < 10; i++)
        cap >> frame;
    cap >> frame;
    Mat gray_frame, current_frame;
    // переводим из бгр в отенки серого
    cvtColor(frame, gray_frame, COLOR_BGR2GRAY);

    // Задаем рабочую область
    int x1 = min(cords[0].x, cords[1].x);
    int y1 = min(cords[0].y, cords[1].y);
    int x2 = max(cords[0].x, cords[1].x);
    int y2 = max(cords[0].y, cords[1].y);
    Rect roi(x1, y1, x2 - x1, y2 - y1);

    // обрезаем серый фрейм и присваеваем ссылку в предыдущий кадр
    Mat previous_frame = gray_frame(roi).clone();

    // параметры фильтров для hsv
    vector<int> hsv_set = { 0,0,90,255,255,220 };
    auto temp = load_vector_int(filename_hsv);
    if (temp.size() == 6) hsv_set = temp;

    /*
    // меняем параметры в реальном времени
    if (argc >= 2 && argv[1][0] == 'O' && argv[1][1] == 'N') {
        cout << "DEBUG MODE ON\n";
        namedWindow("Example3", cv::WINDOW_AUTOSIZE);
        namedWindow("Example4", cv::WINDOW_AUTOSIZE);
        namedWindow("Example5", cv::WINDOW_AUTOSIZE);
        cv::namedWindow(MASK_WINDOW, WINDOW_AUTOSIZE);
        createTrackbar("h min", MASK_WINDOW, &hsv_set[0], 255);
        createTrackbar("h max", MASK_WINDOW, &hsv_set[3], 255);
        createTrackbar("s min", MASK_WINDOW, &hsv_set[1], 255);
        createTrackbar("s max", MASK_WINDOW, &hsv_set[4], 255);
        createTrackbar("v min", MASK_WINDOW, &hsv_set[2], 255);
        createTrackbar("v max", MASK_WINDOW, &hsv_set[5], 255);
    }
    */
    while (true) {
        auto start = std::chrono::high_resolution_clock::now();
        cap >> frame;
        if (frame.empty()) break; // фильм кончился
        cvtColor(frame, current_frame, COLOR_BGR2GRAY);
        current_frame = current_frame(roi).clone();
        if (!current_frame.isContinuous()) {
            cout << current_frame.type() << " - type\n";
            current_frame = current_frame.reshape(1, current_frame.rows * current_frame.cols).clone();
        }
        if (!previous_frame.isContinuous()) {
            cout << current_frame.type() << " - type\n";
            previous_frame = previous_frame.reshape(1, previous_frame.rows * previous_frame.cols).clone();
        }
        next = split_frame(hsv_set, previous_frame, current_frame, previous, numRows, numCols);
        if (next == Point(0, 0)) next = previous;
        // рисуем круги граничных точек
        auto result = std::chrono::high_resolution_clock::now();
        double fps = 1.0 / ((double)(result - start).count() / 1e9);
        if (argc >= 2 && argv[1][0] == 'O' && argv[1][1] == 'N') {
            putText(frame, to_string(fps), cords[0], FONT_HERSHEY_PLAIN, 3, (100, 255, 0), 3, LINE_AA);
            circle(frame, cords[0], 10, Scalar(255, 0, 255), 2, 3);
            circle(frame, cords[1], 10, Scalar(255, 0, 255), 2, 3);
            // рисуем точку касания
            circle(frame, next + cords[0], 10, Scalar(0, 0, 255), 2, 3);
            imshow("Example3", frame);
        }

        // нажимаем мышкой
        //если прошло смещение больше чем на 5 пикселей
        if (abs(next.x - previous.x) >= 5 || abs(next.y - previous.y) >= 5) {
            Point screen = transform_cord(cords, Point(1920, 1080), next);
            SetCursorPos(screen.x, screen.y);
            // раскомментировать после калибровки
            //mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, screen.x, screen.y, 0, 0); // нажали
            //mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, screen.x, screen.y, 0, 0); //отпустили
        }
        previous = next;
        current_frame.copyTo(previous_frame);
        // нажата любая кнопка или эксейп(точно не помню) -> выход
        if (waitKey(33) >= 0) {
            break;
        }
    }
    save_vector(hsv_set, filename_hsv);
    return 0;
}