/*
 * Copyright 2015. All Rights Reserved.
 * Author: Hamid Bazargani
 */

#ifndef FRAME_PROCESSOR_H_
#define FRAME_PROCESSOR_H_

#include <jni.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <vector>
#include "rhorefc.h"

#define DESCRIPTOR_LENGTH 	256		// Length of BRIEF descriptor in bits
#define DESCRIPTOR_SIZE		8		// Size of the descriptor (256/32 = 8)
#define MAX_TOTAL_MATCH		1000	// Maximum number of required matches
#define FAST_THRSH			30		// FAST9 threshold (smaller -> more features)
#define HALF_PATCH_WIDTH	15		// Half of 30 patch used in BRIEF
#define MIN_HAMMING_DIST	46		// Hamming threshold used for matching
using namespace cv;

/**
* Struct similar to OpenCV DMatch
* for holding matching pair
*/
typedef struct	d_match  D_MATCH;
struct	d_match {
    uint32_t queryIdx; // query descriptor index
    uint32_t trainIdx; // train descriptor index
    uint16_t distance; // matching distance

    /* compare operator */
    bool operator < (const D_MATCH &m) const{
    	return distance < m.distance;
    }
};

/**
* Struct for holding feature points
*/
struct Feature {
	uint32_t descriptor[DESCRIPTOR_SIZE];
	unsigned short x, y; 	// feature location
	uint32_t index;			// feature index id

	Feature(): index(0), x(0), y(0) {
		memset(descriptor, 0, sizeof(descriptor));
	}
	Feature(const Feature& obj) : index(obj.index), x(obj.x), y(obj.y){
		if(obj.descriptor != NULL)
			memcpy(descriptor, obj.descriptor, sizeof(descriptor));
	}
	Feature& operator =(const Feature& obj){
		x = obj.x;
		y = obj.y;
		index = obj.index;
		if(obj.descriptor != NULL)
			memcpy(descriptor, obj.descriptor, sizeof(descriptor));
	}
};

class frameProcessor {
public:

	// Constructor and deconstructor.
	frameProcessor();
	~frameProcessor();

	// Main process loop
	// Takes as input an RGB Mat
	int processFrame(const cv::Mat&, cv::Mat&);

	// Setup configuration parameters
	int configProcessor(void);

	// Release and clean memory
	int releaseProcessor(void);

	// Load features table from a binary file
	int loadModelFromFile(const std::string&);

private:
	// Extract FAST9 features and BRIEF descriptor
	void extractFeatures(const cv::Mat& gray, std::vector<Feature>& features) const;

	// Match runtime features with the model features (local binary features)
	void findBRIEFMatches(std::vector<Feature>& features, int pyramid);

	// Calculate 13-bit index for using local patch
	uint16_t calcHashIndex(const cv::Mat& input, const cv::Point& pt) const;

	int estimateH(const std::vector<cv::Point2f>& srcPoints,
				  const std::vector<cv::Point2f>& dstPoints);

	int drawTargetBox(cv::Mat& src, const cv::Scalar& color) const;

	// 3x3 matrix of homography
	cv::Mat homography;
	vector<D_MATCH> matches;
	// 2D feature points from query frame
	std::vector<cv::Point2f> srcPoints;

	// 2D feature points from trained model
	std::vector<cv::Point2f> dstPoints;

	// Pairwise test location used in BRIEF descriptor
	static const int8_t BRIEFLoc[256][4];

	// Look-up table to store model features and indices
	std::vector<Feature> featureTable;
	std::vector<uint32_t> indexTbl[8192];	// 2^(13bits)

	// A Vector stroring four corners of the target image
	vector<Point2f> trgCorners;
};

static inline int int32BitCount(unsigned v) {
    // http://www-graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
    v = v - ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;	// count
}

static inline int descriptorBitCount(unsigned* desc) {
    int dword_count = DESCRIPTOR_SIZE;
    int i, bitCount = 0;
    for (i = 0; i < dword_count; i++){
    	bitCount += int32BitCount(desc[i]);
    }
    return bitCount;
}

#endif  // FRAME_PROCESSOR_H_
