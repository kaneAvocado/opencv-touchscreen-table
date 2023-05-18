#include "Header.h"


// https://goo-gl.me/NLl94 - учебник на русском, дальше будет указание страниц где описана нужная информация
// https://docs.opencv.org/4.x/index.html - оригинальная документация на английском


k4a_image_t upscaleIR16Image(const k4a_image_t& ir16Image)
{
    // Get the dimensions of the input IR16 image
    int width = k4a_image_get_width_pixels(ir16Image);
    int height = k4a_image_get_height_pixels(ir16Image);

    // Create a new image with the target resolution (720p) and custom16 format
    k4a_image_t upscaledImage = nullptr;
    k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM16, 1280, 720, 1280 * static_cast<int>(sizeof(uint16_t)), &upscaledImage);

    // Get the buffer containing the IR16 image data
    uint16_t* ir16Data = reinterpret_cast<uint16_t*>(k4a_image_get_buffer(ir16Image));
    uint16_t* upscaledData = reinterpret_cast<uint16_t*>(k4a_image_get_buffer(upscaledImage));

    // Copy and upscale the IR16 image
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint16_t ir16Value = ir16Data[y * width + x];
            upscaledData[y * 1280 + x] = ir16Value;
        }
    }

    return upscaledImage;
}

k4a_image_t convertIR16ToCustom16(const k4a_image_t& ir16Image)
{
    // Get the dimensions of the input IR16 image
    int width = k4a_image_get_width_pixels(ir16Image);
    int height = k4a_image_get_height_pixels(ir16Image);

    // Create a new image with the same resolution and custom16 format
    k4a_image_t custom16Image = nullptr;
    k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM16, width, height, width * static_cast<int>(sizeof(uint16_t)), &custom16Image);

    // Get the buffer containing the IR16 image data
    uint16_t* ir16Data = reinterpret_cast<uint16_t*>(k4a_image_get_buffer(ir16Image));
    uint16_t* custom16Data = reinterpret_cast<uint16_t*>(k4a_image_get_buffer(custom16Image));

    // Copy the data from IR16 image to custom16 image
    std::memcpy(custom16Data, ir16Data, width * height * sizeof(uint16_t));

    return custom16Image;
}


k4a_image_t IR_to_color(k4a_transformation_t& transformation, k4a_image_t& depth_image, k4a_image_t& IR_image_raw, k4a_image_t& color_image)
{
    // Get attributes of the color image
    uint32_t color_height = k4a_image_get_height_pixels(color_image);
    uint32_t color_width = k4a_image_get_width_pixels(color_image);
    uint32_t color_stride = k4a_image_get_stride_bytes(color_image);

    // Upscale the IR image to match the color image resolution
    k4a_image_t IR_image = convertIR16ToCustom16(IR_image_raw);

    // Create blank image containers for transformed images
    k4a_image_t transformed_IR_image = nullptr;
    k4a_image_t transformed_depth_image = nullptr;
    if (k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM16, color_width, color_height, color_stride, &transformed_IR_image) != K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to create blank IR image (IR_to_color)\n");
    }
    if (k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16, color_width, color_height, color_stride, &transformed_depth_image) != K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to create blank depth image (IR_to_color)\n");
    }

    // Apply the transformation
    if (k4a_transformation_depth_image_to_color_camera_custom(transformation, depth_image, IR_image, transformed_depth_image,
        transformed_IR_image, K4A_TRANSFORMATION_INTERPOLATION_TYPE_NEAREST, 0) != K4A_RESULT_SUCCEEDED)
    {
        printf("IR/depth transformation failed\n");
    }
    cout << "Вроде успех\n";
    // Release the intermediate IR image
    k4a_image_release(IR_image);
    //k4a_image_release(transformed_depth_image);
    return transformed_IR_image;
}


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
    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < numCols; j++) {
            int x = j * subImgWidth;
            int y = i * subImgHeight;

            auto roi = Rect(x, y, subImgWidth, subImgHeight);
            // Extract the current sub-image
            //cout << i << " " << j<<endl;
            //cout << x << " " << y << " " << previous_frame.isContinuous() << " " << endl;
            //cout << roi.width << " " << roi.height << endl;
            try {
                Mat subImage_prev = previous_frame(roi).clone();
                Mat subImage_current = current_frame(roi).clone();
                //cout << subImage_prev.cols << " " << subImage_prev.rows << endl;
                t.push_back(thread(procesing_frame, hsv_set, subImage_prev, subImage_current, ref(ans[i][j]), ref(control)));
            }
            catch (cv::Exception& e) {
                cerr << "Reason: " << e.msg << endl;
                // nothing more we can do
                exit(1);
            }

            // Create a thread to process the sub-image

            // Detach the thread so that it runs independently
        }
    }
    for (auto& i : t) {
        //unique_lock<mutex> ul(control);
        i.join();
        //ul.unlock();
    }
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

    k4a_device_t device;
    k4a_device_open(K4A_DEVICE_DEFAULT, &device);

    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
    config.color_resolution = K4A_COLOR_RESOLUTION_720P;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    config.synchronized_images_only = true;
    config.depth_delay_off_color_usec = 0;
    config.subordinate_delay_off_master_usec = 0;
    config.wired_sync_mode = K4A_WIRED_SYNC_MODE_STANDALONE;
    


    k4a_device_start_cameras(device, &config);

    // Configure IR camera
    k4a_calibration_t calibration;
    k4a_device_get_calibration(device, config.depth_mode, K4A_COLOR_RESOLUTION_720P, &calibration);

    k4a_calibration_camera_t camera = calibration.color_camera_calibration;
    k4a_calibration_camera_t ir_camera = calibration.depth_camera_calibration;

    


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
    
    k4a_color_control_mode_t color_control_mode;
    int32_t color_control_value;
    k4a_result_t result = k4a_device_get_color_control(device, K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, &color_control_mode, &color_control_value);
    if (result == K4A_RESULT_SUCCEEDED)
    {
        // adjust exposure settings
        int32_t new_exposure_value = color_control_value + 1100; // increase by 500 units
        result = k4a_device_set_color_control(device, K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, K4A_COLOR_CONTROL_MODE_MANUAL, new_exposure_value);
    }
    Mat frame = cv::Mat::zeros(ir_camera.resolution_height, ir_camera.resolution_width, CV_16U);
    k4a_image_t ir_image;
    // загружаем координаты угловых точек, которые определяем через 2 программу
    auto cords = load_vector("settings.txt");
    Point previous = cords[0], next = cords[1];
    // обрабатываем кадры с камеры
    // читаем первый кадр т.к. он будет пустым
    k4a_capture_t capture;
    k4a_capture_create(&capture);
    for (int i = 0; i < 10; i++) {
        k4a_wait_result_t result = k4a_device_get_capture(device, &capture, K4A_WAIT_INFINITE);
        if (result == K4A_WAIT_RESULT_SUCCEEDED) {
            ir_image = k4a_capture_get_ir_image(capture);
            frame.data = reinterpret_cast<uchar*>(k4a_image_get_buffer(ir_image));
            // Process the frame here...
            k4a_capture_release(capture);
        }
        if (result != K4A_WAIT_RESULT_SUCCEEDED) {
            printf("Failed to get capture: \n");
        }
    }
    //frame.convertTo(frame, CV_8U, 255.0 / 65535.0);
    Mat gray_frame, current_frame;
    cv::cvtColor(frame, frame, cv::COLOR_GRAY2BGR);
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

    
    int cnt_frame = 0;
    Point mega_frame = next;
    auto trans = k4a_transformation_create(&calibration);
    while (true) {
        auto start = std::chrono::high_resolution_clock::now();
        //cap >> frame;
        frame = cv::Mat::zeros(ir_camera.resolution_height, ir_camera.resolution_width, CV_16U);
        k4a_wait_result_t resulted = k4a_device_get_capture(device, &capture, K4A_WAIT_INFINITE);
        if (resulted == K4A_WAIT_RESULT_SUCCEEDED) {
            ir_image = k4a_capture_get_ir_image(capture);
            frame.data = reinterpret_cast<uchar*>(k4a_image_get_buffer(ir_image));
            // Process the frame here...
            //frame.convertTo(frame, CV_8U, 255.0 / 65535.0);
            cv::cvtColor(frame, frame, cv::COLOR_GRAY2BGR);
            // переводим из бгр в отенки серого
            cvtColor(frame, current_frame, COLOR_BGR2GRAY);
            if (frame.empty()) break; // фильм кончился
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
                putText(frame, to_string(fps), cords[0], FONT_HERSHEY_PLAIN, 3, (255, 255, 255), 3, LINE_AA);
                circle(frame, cords[0], 10, Scalar(255, 0, 255), 2, 3);
                circle(frame, cords[1], 10, Scalar(255, 0, 255), 2, 3);
            }

            // нажимаем мышкой
            //если прошло смещение больше чем на 5 пикселей
            if (cnt_frame == 0 && next != previous) {
                // рисуем точку касания
                mega_frame = next;
                Point screen = transform_cord(cords, Point(1920, 1080), next);
                SetCursorPos(screen.x, screen.y);
                //mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, screen.x, screen.y, 0, 0); // нажали
                //mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, screen.x, screen.y, 0, 0); //отпустили
                cnt_frame++;
            }
            else if (cnt_frame != 0) {
                cnt_frame++;
                if (cnt_frame == 10)
                    cnt_frame = 0;
            }
            circle(frame, mega_frame + cords[0], 10, Scalar(0, 0, 255), 2, 3);
            imshow("Example3", frame);
            previous = next;
            current_frame.copyTo(previous_frame);
            // нажата любая кнопка или эксейп(точно не помню) -> выход
            if (waitKey(33) >= 0) {
                break;
            }
            k4a_image_release(ir_image);
            k4a_capture_release(capture);
        }
    }
    save_vector(hsv_set, filename_hsv);
    k4a_device_stop_cameras(device);
    k4a_device_close(device);
    return 0;
}


