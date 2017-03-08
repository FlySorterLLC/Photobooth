#include <iostream>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;

int main() {
  Mat img;
  img = Mat::zeros(3840, 2748, CV_8UC(3));

  imwrite("images/test.png", img);

  return 0;
}
