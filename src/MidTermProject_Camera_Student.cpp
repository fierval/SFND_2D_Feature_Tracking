/* INCLUDES FOR THIS PROJECT */
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <limits>
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>

#include "dataStructures.h"
#include "matching2D.hpp"

#include "logger.hpp"
using namespace std;

void run_data_collection(CsvLogger<int>& log_keys, CsvLogger<int>& log_det_desc_keys, CsvLogger<float>& log_det_desc_times, string& detectorType, string& descriptorType, bool bVis = false) {
  /* INIT VARIABLES AND DATA STRUCTURES */

// data location
  string dataPath = "../";

  // camera
  string imgBasePath = dataPath + "images/";
  string imgPrefix = "KITTI/2011_09_26/image_00/data/000000"; // left camera, color
  string imgFileType = ".png";
  int imgStartIndex = 0; // first file index to load (assumes Lidar and camera names have identical naming convention)
  int imgEndIndex = 9;   // last file index to load
  int imgFillWidth = 4;  // no. of digits which make up the file index (e.g. img-0001.png)

  // misc
  int dataBufferSize = 2;       // no. of images which are held in memory (ring buffer) at the same time
  vector<DataFrame> dataBuffer; // list of data frames which are held in memory at the same time

    // data output files

  for (size_t imgIndex = 0; imgIndex <= imgEndIndex - imgStartIndex; imgIndex++)
  {
    /* LOAD IMAGE INTO BUFFER */

    // assemble filenames for current index
    ostringstream imgNumber;
    imgNumber << setfill('0') << setw(imgFillWidth) << imgStartIndex + imgIndex;
    string imgFullFilename = imgBasePath + imgPrefix + imgNumber.str() + imgFileType;

    // load image from file and convert to grayscale
    cv::Mat img, imgGray;
    img = cv::imread(imgFullFilename);
    cv::cvtColor(img, imgGray, cv::COLOR_BGR2GRAY);

    //// STUDENT ASSIGNMENT
    //// TASK MP.1 -> replace the following code with ring buffer of size dataBufferSize

    // push image into data frame buffer
    DataFrame frame;
    frame.cameraImg = imgGray;
    if (imgIndex >= dataBufferSize) {
      // rotate out
      std::rotate(dataBuffer.begin(), dataBuffer.begin() + 1, dataBuffer.end());

      // insert at the end
      *dataBuffer.rbegin() = frame;
    }
    else {
      dataBuffer.push_back(frame);
    }

    //// EOF STUDENT ASSIGNMENT
    std::cout << "#1 : LOAD IMAGE INTO BUFFER done" << endl;

    /* DETECT IMAGE KEYPOINTS */

    // extract 2D keypoints from current image
    vector<cv::KeyPoint> keypoints; // create empty feature list for current image

    //// STUDENT ASSIGNMENT
    //// TASK MP.2 -> add the following keypoint detectors in file matching2D.cpp and enable string-based selection based on detectorType
    //// -> HARRIS, FAST, BRISK, ORB, AKAZE, SIFT
    bool bVisDetector = false;

    auto startTime = std::chrono::steady_clock::now();

    if (detectorType.compare(DetectorTypes::SHITOMASI) == 0)
    {
      detKeypointsShiTomasi(keypoints, imgGray, bVisDetector);
    }
    else if (detectorType.compare(DetectorTypes::HARRIS) == 0)
    {
      detKeypointsHarris(keypoints, imgGray, bVisDetector);
    }
    else
    {
      detKeypointsModern(keypoints, imgGray, detectorType, bVisDetector);
    }

    //// EOF STUDENT ASSIGNMENT

    //// STUDENT ASSIGNMENT
    //// TASK MP.3 -> only keep keypoints on the preceding vehicle

    // only keep keypoints on the preceding vehicle
    bool bFocusOnVehicle = true;
    cv::Rect vehicleRect(535, 180, 180, 150);
    if (bFocusOnVehicle)
    {
      auto new_end = std::remove_if(keypoints.begin(), keypoints.end(), [&vehicleRect](cv::KeyPoint& p) {
        return !vehicleRect.contains(p.pt);
        });
      keypoints.erase(new_end, keypoints.end());
    }
    //// EOF STUDENT ASSIGNMENT

    // optional : limit number of keypoints (helpful for debugging and learning)
    bool bLimitKpts = false;
    if (bLimitKpts)
    {
      int maxKeypoints = 50;

      if (detectorType.compare(DetectorTypes::SHITOMASI) == 0)
      { // there is no response info, so keep the first 50 as they are sorted in descending quality order
        keypoints.erase(keypoints.begin() + maxKeypoints, keypoints.end());
      }
      cv::KeyPointsFilter::retainBest(keypoints, maxKeypoints);
      std::cout << " NOTE: Keypoints have been limited!" << endl;
    }

    // push keypoints and descriptor for current frame to end of data buffer
    (dataBuffer.end() - 1)->keypoints = keypoints;
    std::cout << "#2 : DETECT KEYPOINTS done" << endl;

    /* EXTRACT KEYPOINT DESCRIPTORS */

    //// STUDENT ASSIGNMENT
    //// TASK MP.4 -> add the following descriptors in file matching2D.cpp and enable string-based selection based on descriptorType
    //// -> BRIEF, ORB, FREAK, AKAZE, SIFT

    cv::Mat descriptors;
    descKeypoints((dataBuffer.end() - 1)->keypoints, (dataBuffer.end() - 1)->cameraImg, descriptors, descriptorType);
    //// EOF STUDENT ASSIGNMENT

    // push descriptors for current frame to end of data buffer
    (dataBuffer.end() - 1)->descriptors = descriptors;

    std::cout << "#3 : EXTRACT DESCRIPTORS done" << endl;

    if (dataBuffer.size() > 1) // wait until at least two images have been processed
    {

      /* MATCH KEYPOINT DESCRIPTORS */

      vector<cv::DMatch> matches;
      string matcherType = MatcherTypes::MAT_FLANN;        // MAT_BF, MAT_FLANN
      string descriptorClass = getDescriptorClass(descriptorType); // DES_BINARY, DES_HOG
      string selectorType = SelectorTypes::SEL_KNN;       // SEL_NN, SEL_KNN

      //// STUDENT ASSIGNMENT
      //// TASK MP.5 -> add FLANN matching in file matching2D.cpp
      //// TASK MP.6 -> add KNN match selection and perform descriptor distance ratio filtering with t=0.8 in file matching2D.cpp

      matchDescriptors((dataBuffer.end() - 2)->keypoints, (dataBuffer.end() - 1)->keypoints,
        (dataBuffer.end() - 2)->descriptors, (dataBuffer.end() - 1)->descriptors,
        matches, descriptorClass, matcherType, selectorType);

      //// EOF STUDENT ASSIGNMENT

      // store matches in current data frame
      (dataBuffer.end() - 1)->kptMatches = matches;

      std::cout << "#4 : MATCH KEYPOINT DESCRIPTORS done" << endl;

      // visualize matches between current and previous image
      if (bVis)
      {
        cv::Mat matchImg = ((dataBuffer.end() - 1)->cameraImg).clone();
        cv::drawMatches((dataBuffer.end() - 2)->cameraImg, (dataBuffer.end() - 2)->keypoints,
          (dataBuffer.end() - 1)->cameraImg, (dataBuffer.end() - 1)->keypoints,
          matches, matchImg,
          cv::Scalar::all(-1), cv::Scalar::all(-1),
          vector<char>(), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

        string windowName = "Matching keypoints between two camera images";
        cv::namedWindow(windowName, 7);
        cv::imshow(windowName, matchImg);
        std::cout << "Press key to continue to next image" << endl;
        cv::waitKey(0); // wait for key to be pressed
      }
    }
    auto endTime = std::chrono::steady_clock::now();
    if (!bVis) {
      log_keys.add_result(detectorType, dataBuffer.rbegin()->keypoints.size());
      if (dataBuffer.size() < 2) {
        log_det_desc_keys.add_result(detectorType, descriptorType, 0);
        log_det_desc_times.add_result(detectorType, descriptorType, 0.f);

      }
      else {
        log_det_desc_keys.add_result(detectorType, descriptorType, dataBuffer.rbegin()->kptMatches.size());
        log_det_desc_times.add_result(detectorType, descriptorType, std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() / 1000.f);
      }
    }

  } // eof loop over all images

}
/* MAIN PROGRAM */
int main(int argc, const char* argv[])
{
  bool bVis = false;            // visualize results
  string outFile = "../doc/";

  CsvLogger<int> log_keypoints("Detector", outFile + "keypoints.csv");
  CsvLogger<int> log_det_desc_keypoints("Detector_Descriptor", outFile + "det_desc_keys.csv");
  CsvLogger<float> log_det_desc_timings("Detector_Descriptor", outFile + "det_desc_times.csv");

  /* MAIN LOOP OVER ALL IMAGES */

  vector<string> detectors{ DetectorTypes::AKAZE, DetectorTypes::BRISK, DetectorTypes::FAST, DetectorTypes::HARRIS, DetectorTypes::ORB, DetectorTypes::SHITOMASI, DetectorTypes::SIFT };
  vector<string> descriptors{ DescriptorTypes::BRIEF, DescriptorTypes::BRISK, DescriptorTypes::FREAK, DescriptorTypes::ORB, DescriptorTypes::SIFT };

  vector<std::pair<string, string>> detector_descriptor;

  unordered_map<string, vector<int>> keypoint_map;
  unordered_map<string, vector<float>> timing_map;

  // AKAZE descriptor requires AKAZE points
  detector_descriptor.push_back(std::make_pair(DetectorTypes::AKAZE, DescriptorTypes::AKAZE));

  // for each of the project tasks
  for (string& detectorType  : detectors) {
    vector<string> actual_descriptors;
    std::copy(descriptors.begin(), descriptors.end(), back_inserter(actual_descriptors));

    // AKAZE descriptor can only be used with AKAZE points
    if (detectorType == DetectorTypes::AKAZE) {
      actual_descriptors.push_back(DescriptorTypes::AKAZE);
    }

    for (string& descriptorType : actual_descriptors) {
      if (detectorType == DetectorTypes::SIFT && descriptorType == DescriptorTypes::ORB) {
        continue;
      }
      std::cout << std::endl << "==========================================================" << std::endl;
      std::cout << "Running: " << detectorType << "/" << descriptorType << std::endl;
      run_data_collection(log_keypoints, log_det_desc_keypoints, log_det_desc_timings, detectorType, descriptorType, bVis);
    }
  }

  log_keypoints.dump();
  log_det_desc_timings.dump();
  log_det_desc_keypoints.dump();

  std::cout << std::endl << "==========================================================" << std::endl;
  std::cout << "                    Visual Run" << std::endl;

  bVis = true;
  run_data_collection(log_keypoints, log_det_desc_keypoints, log_det_desc_timings, detectors[1], descriptors[1], bVis);
  return 0;
}