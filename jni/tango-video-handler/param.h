/*
 * param.h
 *
 *  Created on: Oct 25, 2015
 *      Author: hamid
 */

#ifndef PARAM_H_
#define PARAM_H_

#include <android/log.h>

#define TAG_LOG "tano-AR:native"
#define LOG_I(...) __android_log_print(ANDROID_LOG_INFO,TAG_LOG,__VA_ARGS__)
#define LOG_E(...) __android_log_print(ANDROID_LOG_ERROR,TAG_LOG,__VA_ARGS__)

// Minimum number of matches required to estimate homography
#define MIN_NUM_MATCHES		30

#define RANSAC_REPROJ_THRSH			3.0
#define RANSAC_MAX_ITER				2000
#define RANSAC_CONFIDENCE			0.995
#define RANSAC_NR_BETA				0.35

#define RET_FAILED			-1
#define RET_SUCCESS			0

#endif /* PARAM_H_ */
