#include <cstdio>
#include <cstdint>
#include "opencv2/opencv.hpp"
//#include "opencv2/core.hpp"

using namespace std;

const int AIM_FPS = 15;
const int WIDTH = 320;
const int HEIGHT = 240;

inline float clamp(float val) {
    return val <= 255 ? (val >= 0 ? val : 0) : 255;
}

void rgb2yuv(cv::Mat& image, uint8_t* buffer) {
    uint8_t* y_plane = buffer;
    uint8_t* v_plane = buffer + WIDTH * HEIGHT;
    uint8_t* u_plane = buffer + WIDTH * HEIGHT * 5 / 4;

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            cv::Vec3b rgb(image.at<cv::Vec3b>(i, j));
            *y_plane++ = uint8_t(0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2]);
            if (j % 2 == 0) {
                if (i % 2 == 0)
                    *v_plane++ = uint8_t(clamp(0.500 * rgb[0] + -0.419 * rgb[1] + -0.081 * rgb[2] + 128));
                else
                    *u_plane++ = uint8_t(clamp(-0.169 * rgb[0] + -0.331 * rgb[1] + 0.500 * rgb[2] + 128));
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc == 1)
        return -1;
    string video = argv[1];

    cv::VideoCapture cap(video);
    if (!cap.isOpened()) {
        printf("Cannot open the video!");
        cap.release();
        return -2;
    }

    float fps = cap.get(CV_CAP_PROP_FPS);
    if (fps < AIM_FPS) {
        printf("Original frame rate[%f fps] is lower than %d fps!", fps, AIM_FPS);
        cap.release();
        return -3;
    }

    int width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
    int height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
    printf("%s: %d*%d, %f fps\n", video.c_str(), width, height, fps);

    cv::Mat frame(height, width, CV_8UC3);
    cv::Mat image(HEIGHT, WIDTH, CV_8UC3);

    int frame_num = cap.get(CV_CAP_PROP_FRAME_COUNT);
    long buf_size = (WIDTH * HEIGHT * frame_num * AIM_FPS / fps + 10) * 3 / 2;
    long buf_len = 0;
    uint8_t *buffer = new uint8_t[buf_size]; 

    int cur_idx = 0;
    int cur_sec = 0;

    bool flag = cap.grab();
    float pos_mses = cap.get(CV_CAP_PROP_POS_MSEC);
    while (flag) {
        while (pos_mses < (float)(cur_idx % AIM_FPS) / AIM_FPS * 1000 + cur_idx / AIM_FPS * 1000 && flag) {
            flag = cap.grab();
            pos_mses = cap.get(CV_CAP_PROP_POS_MSEC);
        }
        if (!flag)
            break;
        cap.retrieve(frame);
        cv::resize(frame, image, cv::Size(WIDTH, HEIGHT), cv::INTER_CUBIC);

        rgb2yuv(image, buffer + buf_len);
        buf_len += WIDTH * HEIGHT * 3 / 2;

        cur_idx++;
        if (cur_idx % AIM_FPS == 0) {
            cur_sec++;
            printf("processed %ds\n", cur_sec);
        }
    }

    cap.release();

    FILE *fp = fopen("result.yuv", "wb");
    if (fp == NULL) {
        printf("Cannot write the yuv!");
        return 0;
    }
    fwrite(buffer, sizeof(uint8_t), buf_len, fp);
    fclose(fp);

    return 0;
}

