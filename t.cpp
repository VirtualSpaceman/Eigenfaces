#include <bits/stdc++.h>
#define bug(x) cout << #x << " = " << (x) << '\n'
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <octave/oct.h>

using namespace cv;
using namespace std;

using tensor = vector<Mat>;
using kernel = tensor;

void cv2octave(const Mat &src, Matrix &dst){

    dst = Matrix(src.rows, src.cols);

    for(int i= 0; i < src.rows; ++i)
      for(int j = 0; j < src.cols; ++j)
           dst(i, j) = src.at<double>(i, j);
}

void octave2cv(const Matrix &src, Mat &dst){

  int n = src.rows(), m = src.cols();
  bug(n), bug(m);
  //dst = Mat(n, m);
  /*for(int i= 0; i < dim1(src); ++i)
    for(int j = 0; j < dim2(src); ++j)
        dst.at<double>(i, j) = src.elem(i, j);*/

}

Rect2i cut_image(const Mat &frame)
{
    Mat _frame;
    frame.copyTo(_frame);
    putText(_frame, "Minimum size", Point(10, 10), FONT_HERSHEY_COMPLEX_SMALL, 0.6, Scalar(0, 255, 0));
    rectangle(_frame, Rect(20, 20, 21, 21), Scalar(255, 0, 0));
    Rect2i roi = selectROI("Tracker", _frame, false, false);

    roi.width = max(roi.width, 21), roi.height = max(roi.height, 21);

    if(roi.width % 2 == 0) roi.width++;
    if(roi.height % 2 == 0) roi.height++;

    return roi;
}
tensor get_features(const Mat &frame, const Rect2i &roi)
{
    Mat img = frame(roi), greyImg, d[4], aux(img.rows, img.cols, CV_64F), taux[3];
    tensor features;
    split(img, taux);

    cvtColor(img, greyImg, CV_BGR2GRAY);
    Sobel(greyImg, d[0], CV_64F, 1, 0, 1);
    Sobel(greyImg, d[1], CV_64F, 0, 1, 1);
    Sobel(greyImg, d[2], CV_64F, 2, 0, 1);
    Sobel(greyImg, d[3], CV_64F, 0, 2, 1);
    for(int i = 0; i < 3; ++i)
        taux[i].convertTo(aux, CV_64F), features.push_back(aux), aux.release();
    for(int i = 0; i < 4; ++i)
        d[i].convertTo(aux, CV_64F), features.push_back(aux), aux.release();
    return features;
}

double diss(const Mat &obj, const Mat &c1)
{
    Matrix aux1, aux2;
    ComplexColumnVector eigen_vals;
    cv2octave(obj, aux1);
    cv2octave(c1, aux2);

    EIG eig = (aux1, aux2);
    eigen_vals = eig.eigenvalues();

    for(octave_idx_type i = 0; i < eigen_vals.rows(); ++i)
      bug(real(eigen_vals.elem(i, 0)));


    return 1;

}


Mat get_cov(const tensor &roi_tensor)
{

    int m = roi_tensor[0].rows, n = roi_tensor[0].cols, sz = roi_tensor.size();
    Mat cov = Mat::zeros(sz, sz, CV_64F),
        mean = Mat::zeros(1, sz, CV_64F),
        aux = Mat::zeros(1, sz, CV_64F),
        aux2 = Mat::zeros(1, sz, CV_64F);

    for(uint i = 0; i < roi_tensor.size(); ++i)
        mean.at<double>(0, i) = sum(roi_tensor[i])[0] / (m * n);
    for(int i = 0;  i < m; ++i)
        for(int j = 0; j < n; ++j)
        {
            for(int k = 0; k < sz; ++k)
                aux.at<double>(0, k) = roi_tensor[k].at<double>(i, j);
            //bug(aux);
            mulTransposed(aux, aux2, true, mean, CV_64F);
            cov += aux2 / (m * n);
        }
    //bug(cov);
    return cov;
}



int main(int argc, char** argv)
{

    VideoCapture cap;
    if(argc < 2)
        cap = VideoCapture(0);
    else
        cap = VideoCapture(argv[1]);
    if(!cap.isOpened())
        return -1;

    Mat frame;
    int x, y;
    Rect2i roi;
    /*if(argc < 2)
        for(int i = 0; i < 20; ++i) cap >> frame;
    cap >> frame;
    */
    tensor feat;
    vector<Mat> covs;
    for(;;)
    {
        cap >> frame;
        if(waitKey(10) == 't')
        {
            roi = selectROI("First", frame, false, false);
            x = roi.x + roi.width / 2 + 1, y = roi.y + roi.height / 2 + 1;
            feat = get_features(frame, roi);
            //for(auto &f: feat) bug(f);

            /*for(size_t i = 0; i < feat.size(); ++i)
                imshow("Feature" + to_string(i), feat[i]);*/
            rectangle(frame, roi, Scalar(0, 255, 0), 0.5);
            circle(frame, Point2i(x, y), 1.5, Scalar(255, 0, 0), 2);
            covs.push_back(get_cov(feat));
            if(covs.size() == 2)
            {
                diss(covs[0], covs[1]);
            }
        }
        imshow("Tracking", frame);
        if(waitKey(30) == 27) break;
    }
    return 0;
}
