#define _USE_MATH_DEFINES

#include <cmath>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "iostream"
#include <stdlib.h>
#include <stdio.h>
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"

using namespace std;
using namespace cv;

String cascade_name = "./dartcascade/cascade.xml";
CascadeClassifier cascade;

void combineDetections(vector<Rect> viola_dartboards, int a, int b, int r,Mat &frame ) {
    //Point center(a,b);
    for (int detect=0; detect<viola_dartboards.size();detect++){
      bool circleInBox = (viola_dartboards[detect].x < a && a < (viola_dartboards[detect].x + viola_dartboards[detect].width) && viola_dartboards[detect].y < b && b < (viola_dartboards[detect].y + viola_dartboards[detect].height));
      if (circleInBox || ((viola_dartboards[detect].width < (2*r) && viola_dartboards[detect].width > (1.75*r))))
      {
      rectangle(frame, Point(viola_dartboards[detect].x, viola_dartboards[detect].y), Point(viola_dartboards[detect].x +
        viola_dartboards[detect].width, viola_dartboards[detect].y + viola_dartboards[detect].height), Scalar( 0, 255, 0 ), 2);
      }
    }
  return;
}

void convertGrey(Mat src, Mat &src_gray) {
  // Prepare Image by turning it into Grayscale and normalising lighting
  cvtColor( src, src_gray, CV_BGR2GRAY );
  // equalizeHist(src_gray, src_gray);
}

void thresh(Mat input, Mat &output, uchar thresh) {
  // Threshold by looping through all pixels
  output.create(input.size(), input.type());

  for (int i = 0; i < input.rows; i++) {
    for (int j = 0; j < input.cols; j++) {
      uchar pixel = input.at<uchar>(i, j);
      if (pixel > thresh) output.at<uchar>(i, j) = 255;
      else output.at<uchar>(i, j) = 0;
    }
  }
}

void sobelEdges(Mat src_gray, Mat &thresh_mag) {
  int scale = 1;
  int delta = 0;
  int ddepth = CV_16S;
  /// Generate gradientX and gradientY
  Mat gradientX, gradientY;
  Mat abs_gradientX, abs_gradientY;
  Mat magnitude;

  /// Gradient X
  Sobel( src_gray, gradientX, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT );
  convertScaleAbs( gradientX, abs_gradientX );

  /// Gradient Y
  Sobel( src_gray, gradientY, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT );
  convertScaleAbs( gradientY, abs_gradientY );

  /// Total Gradient
  addWeighted( abs_gradientX, 0.5, abs_gradientY, 0.5, 0, magnitude );

  // Create threshold from absolute image
  thresh(magnitude, thresh_mag, 130); //good value for img 1
}

// TODO
void houghCircles(Mat thresh_mag,vector<Rect> viola_dartboards, Mat &frame) {
  int min_radius = 50;
  int max_radius = 120;

  int dim1 = thresh_mag.cols;
  int dim2 = thresh_mag.rows;
  int dim3 = max_radius - min_radius;

  int sizes[3] = {dim1,dim2,dim3};
  cv::Mat accumulator = cv::Mat(3, sizes, CV_32F, cv::Scalar(0));
  cv::Mat thresh_accumulator = cv::Mat(3, sizes, CV_32F, cv::Scalar(0));

 //VOTING
  int a;
  int b;

  for (int i = 0; i < dim2; i++) {
    for (int j = 0; j < dim1; j++) {
      if (thresh_mag.at<uchar>(i,j) == 255) {
        for (int r = 0; r < dim3; r++) {
          for (int theta = 0; theta <= 360; theta+=3) {
            a = j - ((r+min_radius) * cos(theta * M_PI / 180));
            b = i - ((r+min_radius) * sin(theta * M_PI / 180));
            if(((0 <= a) && (a < dim1)) && ((0 <= b) && (b < dim2))) {
              accumulator.at<float>(a,b,r) += 1;
            }
          }
        }
      }
    }
  }


  int max = 0;
  int max_r=0;
  for (int i = 0; i< dim1; i++) {
        for (int j = 0; j < dim2; j++) {
          for (int k = 0; k < dim3; k++) {
            if (accumulator.at<float>(i,j,k) > max) {
              max = accumulator.at<float>(i,j,k);
            }
          }
        }
      }

      //printf("%d\n",max);

  for(int a=0; a<dim1; a++){
    for(int b=0; b<dim2;b++){
      for(int r=0;r<dim3;r++){
        if(accumulator.at<float>(a,b,r)>=(max*0.85)){
          thresh_accumulator.at<float>(a,b,r) =accumulator.at<float>(a,b,r);
          combineDetections(viola_dartboards,a,b,(r+min_radius), frame);
        }

      }
    }
  }

  //imshow("blalb", frame);
  //waitKey(0);





  //Creating the 2D space
  Mat hough2D;
  hough2D.create(dim2,dim1, thresh_mag.type());      // use this to create 2D space
  float rad_total;
  for (int a = 0; a < dim1; a++) {
    for (int b = 0; b < dim2; b++) {
      rad_total=0;
      for (int r = 0; r < dim3; r++) {
        rad_total += thresh_accumulator.at<float>(a,b,r);
      }
      hough2D.at<uchar>(b,a)=rad_total;
    }
  }
  imshow("hough2D",hough2D);
}

void violaJonesDetector(Mat src, vector<Rect> &viola_dartboards) {
  // Perform Viola-Jones Object Detection
  cascade.detectMultiScale( src, viola_dartboards, 1.1, 1, 0|CV_HAAR_SCALE_IMAGE, Size(50, 50), Size(500,500) );

  // Draw box around faces found
  // for( int i = 0; i < viola_dartboards.size(); i++ ) {
  //   rectangle(out, Point(viola_dartboards[i].x, viola_dartboards[i].y), Point(viola_dartboards[i].x + viola_dartboards[i].width, viola_dartboards[i].y + viola_dartboards[i].height), Scalar( 0, 255, 0 ), 2);
  // }
  return;
}

// TODO


int main( int argc, const char** argv )
{
   // Read Input Image
  Mat frame = imread(argv[1], CV_LOAD_IMAGE_COLOR);
  cv::Mat src_gray, thresh_mag;
  std::vector<Rect> viola_dartboards;


  // Load the Strong Classifier in a structure called `Cascade'
  if( !cascade.load( cascade_name ) ){ printf("--(!)Error loading\n"); return -1; };

  convertGrey(frame, src_gray);

  sobelEdges(src_gray, thresh_mag);

  //imshow("threshold", thresh_mag);
  //waitKey(0);
  violaJonesDetector(src_gray, viola_dartboards);
  houghCircles(thresh_mag,viola_dartboards,frame);

  imshow("detections", frame);
  waitKey(0);

  //combineDetections();

  // 4. Save Result Image
  imwrite( "detected.jpg", frame );

  return 0;
}
