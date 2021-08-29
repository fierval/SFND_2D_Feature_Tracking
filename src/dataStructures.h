#ifndef dataStructures_h
#define dataStructures_h

#include <vector>
#include <opencv2/core.hpp>


struct DataFrame { // represents the available sensor information at the same time instance
    
    cv::Mat cameraImg; // camera image
    
    std::vector<cv::KeyPoint> keypoints; // 2D keypoints within camera image
    cv::Mat descriptors; // keypoint descriptors
    std::vector<cv::DMatch> kptMatches; // keypoint matches between previous and current frame
};

struct DetectorTypes {
  constexpr static char SHITOMASI[] = "SHITOMASI";
  constexpr static char HARRIS[] = "HARRIS";
  constexpr static char FAST[] = "FAST";
  constexpr static char BRISK[] = "BRISK";
  constexpr static char ORB[] = "ORB";
  constexpr static char AKAZE[] = "AKAZE";
  constexpr static char SIFT[] = "SIFT";
};
#endif /* dataStructures_h */
