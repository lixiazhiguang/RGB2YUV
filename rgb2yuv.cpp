#include <cstdio>
#include <cstdint>
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"

using namespace std;

const int AIM_FPS = 15;
const int WIDTH = 320;
const int HEIGHT = 240;

inline float clamp(float val) {
    return val <= 255 ? (val >= 0 ? val : 0) : 255;
}

void rgb2yuv(cv::Mat& image, uint8_t* y_plane, uint8_t* u_plane, uint8_t* v_plane) {
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            cv::Vec3b rgb(image.at<cv::Vec3b>(i, j));
            *y_plane++ = uint8_t(0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2]);
            if (j % 2 == 0) {
                if (i % 2 == 0)
                    *u_plane++ = uint8_t(clamp(-0.169 * rgb[0] + -0.331 * rgb[1] + 0.500 * rgb[2] + 128));
                else
                    *v_plane++ = uint8_t(clamp(0.500 * rgb[0] + -0.419 * rgb[1] + 0.081 * rgb[2] + 128));
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
    int buf_size = floor(frame_num * AIM_FPS / fps) + 10;
   
    uint8_t *y_plane = new uint8_t[WIDTH * HEIGHT * buf_size]; 
    uint8_t *u_plane = new uint8_t[WIDTH * HEIGHT * (buf_size>>2)]; 
    uint8_t *v_plane = new uint8_t[WIDTH * HEIGHT * (buf_size>>2)]; 
    long p_num = 0;

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

        rgb2yuv(image, y_plane + p_num, u_plane + (p_num>>2), v_plane + (p_num>>2));
        p_num += WIDTH * HEIGHT;

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
    fwrite(y_plane, sizeof(uint8_t), p_num, fp);
    fwrite(u_plane, sizeof(uint8_t), p_num>>2, fp);
    fwrite(v_plane, sizeof(uint8_t), p_num>>2, fp);
    fclose(fp);

    return 0;
}

