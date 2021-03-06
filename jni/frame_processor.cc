/*
 * Copyright 2015. All Rights Reserved.
 * Author: Hamid Bazargani
 */

#include "tango-video-handler/param.h"
#include "tango-video-handler/frame_processor.h"

using namespace cv;

frameProcessor::frameProcessor()
{
}

frameProcessor::~frameProcessor()
{
}

int frameProcessor::processFrame(const Mat& input, Mat& output)
{
	output = input.clone();
	int status = RET_SUCCESS;

	Mat gray;
	cvtColor(input, gray, CV_RGB2GRAY);
	//GaussianBlur(gray, gray, Size(3,3), 1, 1);

	vector<Mat> scales;
	scales.push_back(gray);
	scales.push_back(Mat(gray.rows/2, gray.cols/2, CV_8UC1));
	scales.push_back(Mat(gray.rows/4, gray.cols/4, CV_8UC1));

	/**
	 * Pyramid down to the half- and quarter-scales of the frame.
	 */
	resize(scales[0], scales[1], Size(), 0.5, 0.5, INTER_AREA);
	resize(scales[1], scales[2], Size(), 0.5, 0.5, INTER_AREA);

	int i, pyramid = 0;
	for( ; pyramid < 3 && matches.size() <= MAX_TOTAL_MATCH; pyramid++){
		vector<Feature> rtFeature;

		/**
		 * Extract features for the query pyramid level
		 */
		extractFeatures(scales[pyramid], rtFeature);

		/**
		 * Match extracted features against the target model
		 */
		findBRIEFMatches(rtFeature, pyramid);
	}

	/**
	 * Sort matches by Hamming distance (PROSAC mode)
	 */
	vector<Point2f> srcSorted, dstSorted;
	std::sort(matches.begin(), matches.end());

	for(i = 0; i < matches.size(); i++){
		srcSorted.push_back(srcPoints[matches[i].queryIdx]);
		dstSorted.push_back(dstPoints[matches[i].trainIdx]);
	}

	/**
	 * Clear unnecessary data
	 */
	matches.clear();
	srcPoints.clear();
	dstPoints.clear();

	status |= estimateH(dstSorted, srcSorted);

	if(!status){
		/**
		 * Draw bounding polygon around the target
		 */
		status |= drawTargetBox(output, CV_RGB(0, 0, 255));
	}

	return status;
}

void frameProcessor::findBRIEFMatches(vector<Feature>& features, int pyramid)
{
    uint32_t i, j, k;
    uint32_t mc = matches.size();
    for (i = 0; i < features.size(); i++){

    	Feature&  feature_i	= features[i];
    	uint32_t minDistIdx = 0;	    			//Minimum distance index
    	uint16_t minDist    = DESCRIPTOR_LENGTH;   	//Minimum distance set at maximum value
    	uint32_t comp[DESCRIPTOR_SIZE];

    	// Index table corresponding to the current index
    	vector<uint32_t> dstIdxTbl 	= indexTbl[feature_i.index];

		/**
		 * Scan the table and compare with "features <vector>"
		 * to find the one with minimum Hamming distance.
		 */
    	for (j = 0; j < dstIdxTbl.size(); j++ ){

    		// Get the index of featureTable pointing to the right location
    		uint32_t tIdx 		= dstIdxTbl[j];
			Feature& feature_t 	= featureTable[tIdx];

    		for (k = 0; k < DESCRIPTOR_SIZE; k++){
    			comp[k] = (feature_i.descriptor[k] ^ feature_t.descriptor[k]);
    		}

    		int dist = descriptorBitCount(comp);

    		if (dist < minDist){
    			minDist    = dist;
    			minDistIdx = tIdx;
    		}
    	}

    	// Push the best match to "matches <vector>"
    	if (minDist <= MIN_HAMMING_DIST){
    		Feature& feature_t = featureTable[minDistIdx];
    		D_MATCH match = {mc, mc++, minDist};
    		matches.push_back(match);
    		srcPoints.push_back(Point2f(feature_i.x, feature_i.y)*(1<<pyramid));
			dstPoints.push_back(Point2f(feature_t.x, feature_t.y));
    	}
    }
}

int frameProcessor::configProcessor(void)
{
	return RET_SUCCESS;
}

int frameProcessor::releaseProcessor(void)
{
	return RET_SUCCESS;
}

void frameProcessor::extractFeatures(const Mat& input,
									 vector<Feature>& features) const
{
	vector<KeyPoint> 	keypoints;
	uint32_t			i, kpnt;

    /**
     * extract FAST features (non-max suppression enabled)
     */
	FAST(input, keypoints, FAST_THRSH);

	uint8_t* data 	= (uint8_t*)input.data;
	uint32_t step 	= input.step;
	uint32_t width 	= input.cols;
	uint32_t height = input.rows;

	/**
	 * compute BRIEF descriptor with index
	 */
	for( kpnt = 0; kpnt < keypoints.size(); kpnt++ ){

		int x = keypoints[kpnt].pt.x;
		int y = keypoints[kpnt].pt.y;

		/**
		 * boundary check
		 */
		if(std::min(x,y) < HALF_PATCH_WIDTH || x + HALF_PATCH_WIDTH >= width ||
		     y + HALF_PATCH_WIDTH >= height) {
		    continue;
		}

	    /**
	     * calculate 13-bit index for the patch
	     */
		Feature feature;
		feature.index	= calcHashIndex(input, keypoints[kpnt].pt);
		feature.x 		= x;
		feature.y 		= y;

		/**
		 * compute BRIEF descriptor for the patch
		 */
		for (i = 0; i < DESCRIPTOR_LENGTH; i++){

		    unsigned test = (data[(y + BRIEFLoc[i][1]) * width + x + BRIEFLoc[i][0]]
		                     <
		                     data[(y + BRIEFLoc[i][3]) * width + x + BRIEFLoc[i][2]]);

		    feature.descriptor[i / 32] |= test << (i & 31);
		}
		features.push_back(feature);
	}
}

uint16_t frameProcessor::calcHashIndex(const Mat& input, const Point& pt) const
{
	uint8_t* 	data	= input.data;
	uint32_t 	step	= input.step[0];
	int 		xc		= pt.x;
	int 		yc		= pt.y;

	/* mean of patch */
	int mean = (data[(yc-5)*step+(xc-5)] +
				data[(yc-5)*step+(xc+5)] +
				data[(yc-3)*step+(xc-1)] +
				data[(yc-3)*step+(xc+1)] +
				data[(yc-1)*step+(xc-3)] +
				data[(yc-1)*step+(xc+3)] +
				data[(yc)*step+(xc)] 	+
				data[(yc+1)*step+(xc-3)] +
				data[(yc+1)*step+(xc+3)] +
				data[(yc+3)*step+(xc-1)] +
				data[(yc+3)*step+(xc+1)] +
				data[(yc+5)*step+(xc-5)] +
				data[(yc+5)*step+(xc+5)]) / 13;

	uint16_t index = 0;
	index |= data[(yc-5)*step+(xc-5)] > mean ? 1<<12 : 0;
	index |= data[(yc-5)*step+(xc+5)] > mean ? 1<<11 : 0;
	index |= data[(yc-3)*step+(xc-1)] > mean ? 1<<10 : 0;
	index |= data[(yc-3)*step+(xc+1)] > mean ? 1<<9  : 0;
	index |= data[(yc-1)*step+(xc-3)] > mean ? 1<<8  : 0;
	index |= data[(yc-1)*step+(xc+3)] > mean ? 1<<7  : 0;
	index |= data[(yc)*step+(xc)]     > mean ? 1<<6  : 0;
	index |= data[(yc+1)*step+(xc-3)] > mean ? 1<<5  : 0;
	index |= data[(yc+1)*step+(xc+3)] > mean ? 1<<4  : 0;
	index |= data[(yc+3)*step+(xc-1)] > mean ? 1<<3  : 0;
	index |= data[(yc+3)*step+(xc+1)] > mean ? 1<<2  : 0;
	index |= data[(yc+5)*step+(xc-5)] > mean ? 1<<1  : 0;
	index |= data[(yc+5)*step+(xc+5)] > mean ? 1<<0  : 0;

	return index;
}

/*
* @return	return error code (0 if succeed).
*/
int frameProcessor::estimateH(const std::vector<cv::Point2f>& srcPoints,
		  	  	  	  	  	  const std::vector<cv::Point2f>& dstPoints)
{
	unsigned npoints = dstPoints.size();
	if (npoints < MIN_NUM_MATCHES){
		return RET_FAILED;
	}

	/**
	 * Create temporary output matrix.
	 * RHO outputs a single-precision H only.
	 */
	Mat tmpH = Mat(3, 3, CV_32FC1);

	/**
	 * Make use of the RHO estimator API.
	 *
	 * This is where the math happens. A homography estimation context is
	 * initialized, used, then finalized.
	 */

	RHO_HEST_REFC* p = rhoRefCInit();

	if(p == NULL){
		return RET_FAILED;
	}

	/**
	 * Optional. Ideally, the context would survive across calls to
	 * findHomography(), but no clean way appears to exit to do so. The price
	 * to pay is marginally more computational work than strictly needed.
	 */
	if(rhoRefCEnsureCapacity(p, npoints, (float)RANSAC_NR_BETA) != 1){
		return RET_FAILED;
	}

	/**
	 * The critical call. All parameters are heavily documented in rhorefc.h.
	 *
	 * Currently, NR (Non-Randomness criterion) and Final Refinement (with
	 * internal, optimized Levenberg-Marquardt method) are enabled.
	 */
	Mat tempMask = Mat(npoints, 1, CV_8U);
	unsigned minInliers = npoints * RANSAC_MIN_INL_RATIO;

	LOG_E("rho 0\n");
	int numInliers = rhoRefC(p,
			(const float*)	&srcPoints[0],
			(const float*)	&dstPoints[0],
			(char*)			tempMask.data,
			(unsigned)		npoints,
			(float)			RANSAC_REPROJ_THRSH,
			(unsigned)		RANSAC_MAX_ITER,
			(unsigned)		RANSAC_MAX_ITER,
			(double)		RANSAC_CONFIDENCE,
			std::max(4U, (unsigned)minInliers),
			(double)		RANSAC_NR_BETA,
			RHO_FLAG_ENABLE_NR | RHO_FLAG_ENABLE_FINAL_REFINEMENT,
			NULL,
			(float*)		tmpH.data);

	/**
     * Cleanup.
     */
    rhoRefCFini(p);


    /**
     * Copy the result to homography.
     */
    if(numInliers >= std::max(4U, (unsigned)minInliers)){
    	homography = tmpH.clone();
    	return RET_SUCCESS;
    }
    else{
    	return RET_FAILED;
    }
}

/**
* Draw a closed bounding region around the detected target
* given the computed Homography. It also checks if the bounding
* region is a convex polygon.
*/
int frameProcessor::drawTargetBox(Mat& src, const Scalar& color) const
{
	/**
	 * Compute output contours.
	 */
	 vector<Point2f> outCorners;
	 if(trgCorners.size() != 4){
		 LOG_E("Error: The model target corners are not initialized!");
		 return RET_FAILED;
	 }

	 cv::perspectiveTransform(trgCorners, outCorners, homography);

	 /**
	  * Draw located target to display.
	  */

	 if(outCorners.size() > 0 && isContourConvex(outCorners)){

		 vector<vector<Point> > contours;
		 vector<Point> contour;
		 contour.push_back(Point((float)outCorners[0].x, (float)outCorners[0].y));
		 contour.push_back(Point((float)outCorners[1].x, (float)outCorners[1].y));
		 contour.push_back(Point((float)outCorners[2].x, (float)outCorners[2].y));
		 contour.push_back(Point((float)outCorners[3].x, (float)outCorners[3].y));
		 contours.push_back(contour);
		 polylines(src, contours, true, color, 4);
	 }

	 return RET_SUCCESS;
}

//this reads the model from binary file
int frameProcessor::loadModelFromFile(const string& filename)
{
	FILE * dFile = fopen(filename.c_str(), "rb");
	if(!dFile){
		return RET_FAILED;
	}
	uint64_t len, actualRead, i;

	//Read the image size
	Size targetSize;
	fread(&targetSize, 1, sizeof(Size), dFile);

	//Read Features
	fread(&len,8,1,dFile);								// Read length in bytes
	int numFeatures = (int)len/ (int)sizeof(Feature);	// Number of features

	featureTable.resize(numFeatures);

	actualRead = fread(&featureTable[0], sizeof(Feature), numFeatures, dFile);

	if(numFeatures != (int)actualRead){
		LOG_E("Error: Could not read model completely! \n");
		fclose(dFile);
		return RET_FAILED;
	}

	//Read feature index table
	for(i = 0; i < 8192; i++){	// 8192 = (2^13) 13-bits index

		fread(&len, 8, 1, dFile);
		len /= (int)sizeof(uint32_t);
		indexTbl[i].resize((int)len);
		if(len){
			actualRead = fread(&indexTbl[i][0], sizeof(uint32_t), (int)len, dFile);
			if((int)len != (int)actualRead){
				LOG_E("Error: Could not read the feature LUT completely!\n");
				fclose(dFile);
				return RET_FAILED;
			}
		}
	}

	// Fill trgCorners with four corners of the model image
	trgCorners.push_back(Point2f(0, targetSize.height));
	trgCorners.push_back(Point2f(targetSize.width, targetSize.height));
	trgCorners.push_back(Point2f(targetSize.width, 0));
	trgCorners.push_back(Point2f(0, 0));
	fclose(dFile);
	return RET_SUCCESS;
}

const int8_t frameProcessor::BRIEFLoc[256][4] = {
		{0,0,-4,-2},        {7,-1,-3,1},        {3,-6,4,0},         {7,-11,-11,-3},     {-4,11,1,-8},   {9,-4,-14,-9},
		{-8,0,8,4},         {-1,-3,10,2},       {7,-3,-5,6},        {0,-1,0,-2},        {6,2,0,6},      {-4,-3,-1,6},
		{-11,4,-1,0},       {1,-5,3,-1},        {9,-1,-4,4},        {3,4,-2,-3},        {0,-5,-5,7},    {3,2,-7,7},
		{7,8,-3,-6},        {6,2,-5,-3},        {0,-5,3,0},         {0,-5,5,7},         {-4,3,13,15},   {10,0,7,-11},
		{7,0,8,-8},         {0,5,-4,-9},        {-8,-7,-6,0},       {2,-2,0,2},         {2,1,2,0},      {6,0,-13,-8},
		{-7,-1,3,0},        {-5,1,1,-8},        {1,-4,7,0},         {3,0,1,3},          {-10,4,0,-9},   {-13,2,2,8},
		{2,-1,-14,0},       {-7,10,2,-1},       {1,-3,7,-2},        {-10,-2,6,4},       {0,-1,-6,-7},   {8,1,3,3},
		{-1,2,0,0},         {2,-2,-2,-8},       {11,-14,-4,-1},     {11,1,1,-3},        {5,-6,3,3},     {-13,12,4,14},
		{7,2,-2,0},         {-4,-3,0,0},        {2,0,0,1},          {4,-1,-5,-6},       {-1,1,-5,0},    {0,-10,6,-2},
		{7,-1,0,3},         {7,1,5,-3},         {1,-2,0,6},         {-1,3,-4,5},        {9,-4,5,11},    {-8,7,0,0},         {-3,-3,6,0},
		{-5,-6,-4,7},       {7,1,-1,-7},        {-6,7,-7,-2},       {9,0,4,-4},         {7,2,-11,2},    {0,-1,5,0},         {-8,3,-7,-2},
		{6,0,-3,-1},        {0,7,-2,6},         {0,0,-3,2},         {14,1,-9,-1},       {0,-5,9,-3},    {-8,-4,5,-11},      {0,9,-4,-3},
		{1,2,0,-2},         {-1,6,0,4},         {10,2,5,-2},        {4,-9,10,5},        {0,4,-4,0},     {9,5,-7,-2},        {2,4,7,-5},
		{-5,9,0,3},         {-3,1,0,13},        {-1,4,-2,-4},       {0,-2,3,-6},        {13,1,0,-7},    {-3,-5,-1,3},       {-9,6,-1,-3},
		{-1,-4,-6,-10},     {7,-1,0,3},         {2,-5,5,1},         {0,-14,4,2},        {3,-3,-10,3},   {5,-9,4,-5},        {1,10,5,-10},
		{-5,1,-4,6},        {0,4,4,13},         {-3,-1,10,0},       {-3,0,3,-6},        {-2,-2,0,-3},   {-1,-2,1,3},        {6,-1,7,0},
		{-1,-6,0,4},        {8,4,11,4},         {0,-4,1,-2},        {1,-11,7,1},        {-1,-6,4,-5},   {2,-2,0,-2},        {4,1,-6,-1},
		{-5,2,8,2},         {-4,2,-1,4},        {3,-10,-2,0},       {-1,13,-7,-5},      {6,10,-3,-3},   {8,-2,-12,5},       {-5,4,5,-4},
		{-6,3,-8,-12},      {4,-4,-5,9},        {-4,11,-2,-5},      {3,3,-1,-6},        {-1,3,-7,4},    {7,-1,7,0},         {-2,-7,3,-1},
		{-1,-10,-2,-13},    {-3,-9,-5,-5},      {-13,6,0,9},        {-4,7,-4,6},        {4,7,3,3},      {-13,6,4,-3},       {-4,-2,0,6},
		{-4,0,-1,0},        {3,4,-6,-5},        {-1,1,10,0},        {-3,7,4,0},         {-2,8,3,2},     {-2,-2,4,-2},       {4,-6,-12,1},
		{5,0,12,1},         {-7,-2,-6,-15},     {2,-6,5,0},         {0,-5,-8,11},       {-5,-6,0,6},    {6,3,-3,0},         {0,0,8,-6},
		{-1,5,0,-2},        {-8,2,-2,-7},       {1,-7,-1,0},        {4,-8,3,-1},        {11,2,-2,4},    {2,-2,15,5},        {-9,8,11,4},
		{-1,-7,5,2},        {2,7,-10,2},        {3,4,4,-4},         {2,-2,-4,-1},       {-1,-8,-5,-6},  {1,4,11,-4},        {-8,-4,3,-2},
		{-8,-3,5,-5},       {14,5,1,-6},        {10,-4,0,-4},       {6,7,-9,4},         {-7,0,-1,-8},   {-5,-3,4,-3},       {5,-2,-4,2},
		{-7,8,4,-3},        {8,1,0,-4},         {4,3,0,-1},         {13,9,-2,-9},       {0,4,7,1},      {0,2,5,10},         {-14,1,12,-3},
		{-7,10,7,9},        {-5,3,0,6},         {-8,-2,-6,-1},      {1,9,5,0},          {-13,-1,0,0},   {-7,2,-1,8},        {0,-5,-5,2},
		{-1,-6,-5,-5},      {0,-4,-7,-4},       {2,5,-4,1},         {0,2,4,7},          {4,0,4,-1},     {4,-7,12,11},       {9,0,8,-4},
		{3,-5,1,-4},        {13,8,0,2},         {-4,3,-4,3},        {7,-3,0,2},         {1,1,0,-8},     {2,-1,4,1},         {-6,6,-4,5},
		{13,6,8,1},         {-2,-5,-3,-1},      {2,1,3,-1},         {-1,-2,3,-11},      {0,-6,-2,1},    {1,-2,4,4},         {-4,1,1,0},
		{-3,-5,0,-3},       {2,1,8,-1},         {0,-7,2,3},         {-3,-2,13,-2},      {3,-1,8,-9},    {-5,2,-11,11},      {0,2,4,5},
		{9,0,0,1},          {5,-4,2,6},         {-1,-2,-15,7},      {-4,2,-3,-11},      {-6,7,0,-2},    {4,-4,4,-5},        {-8,5,3,3},
		{-2,8,5,10},        {0,0,-4,-2},        {2,0,4,-5},         {0,4,2,-3},         {-12,0,-4,-2},  {-10,-3,-5,0},      {-4,-8,-2,-3},
		{2,1,1,7},          {-13,15,3,-10},     {0,2,5,-3},         {-4,1,-6,-1},       {11,-10,6,8},   {0,-2,-1,10},       {-2,6,0,-15},
		{8,-10,0,4},        {-1,1,0,0},         {1,0,3,6},          {5,3,-2,0},         {5,-7,5,0},     {-1,1,4,-9},        {1,-2,-11,0},
		{0,-4,11,-6},       {10,-9,5,1},        {5,0,1,8},          {-3,5,1,4},         {-4,-1,2,-5},   {8,5,13,2},         {2,8,3,-3},
		{4,-5,6,4},         {6,-5,-10,2},       {7,-1,-2,-6},       {-1,5,-4,0},        {1,5,5,2},      {2,0,-2,-2},        {4,-4,5,10},
		{0,3,3,0},          {9,-1,1,-13},       {0,5,-6,1},         {-4,-1,1,3},        {1,11,12,9},    {-5,7,0,3}
};
