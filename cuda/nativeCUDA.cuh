#ifdef __CUDACC__
#define LOG_TAG "tango_jni"
#ifndef LOGI
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#endif
#endif

#define TPB	16 
typedef unsigned char uchar;

void launchGreyKernel(uchar* d_input, uchar* d_output, int rows, int cols);

void CUDA_greyCvt(uchar* input, uchar** output, int rows, int cols);
