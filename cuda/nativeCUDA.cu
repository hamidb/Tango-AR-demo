#include "nativeCUDA.cuh"
#include <cuda_runtime.h>
#include <cuda.h>

#define DEBUG

inline cudaError_t checkCuda(cudaError_t result) {
#if defined(DEBUG)
	if (result != cudaSuccess) {
		LOGI("CUDA Runtime Error: %sn", cudaGetErrorString(result));
	}
#endif
	return result;
}

__global__
void greyKernel(uchar* d_input, uchar* d_output, int rows, int cols);

void launchGreyKernel(uchar* d_input, uchar* d_output, int rows, int cols) {
	const dim3 blockSize(TPB, TPB, 1); 
	const dim3 gridSize( (cols + TPB - 1)/TPB, (rows + TPB - 1)/TPB, 1);
    greyKernel<<<gridSize, blockSize>>>(d_input, d_output, rows, cols);
}

void CUDA_greyCvt(uchar* input, uchar** output, int rows, int cols) {

    size_t pixel_size = rows * cols;

    // Allocate device space
    uchar *d_input, *d_output;
    checkCuda (cudaMalloc((void**) &d_input, 3 * pixel_size) );
    checkCuda (cudaMalloc((void**) &d_output, pixel_size) );
	checkCuda (cudaMemset((void*) d_output, 0, pixel_size));

    // Copy input image to device memory asynchronously
    checkCuda( cudaMemcpyAsync(d_input, input, 3 * pixel_size, cudaMemcpyHostToDevice) );
    checkCuda( cudaMemcpyAsync(d_output, *output, pixel_size, cudaMemcpyHostToDevice) );

    // Wait for copies to complete
    cudaDeviceSynchronize();

    // Launch device kernel
    launchGreyKernel(d_input, d_output, rows, cols);

    // Wait for kernel to finish
    cudaDeviceSynchronize();

    // Check for any errors created by kernel
    checkCuda(cudaGetLastError());

    // Copy back sum array
    checkCuda( cudaMemcpy(*output, d_output, pixel_size, cudaMemcpyDeviceToHost) );

    // Free allocated memory
    cudaFree(d_input);
    cudaFree(d_output);
}

// GPU kernel
__global__ 
void greyKernel(uchar* d_input, uchar* d_output, int rows, int cols){
    int index_x = threadIdx.x + blockIdx.x * blockDim.x;
	int index_y = threadIdx.y + blockIdx.y * blockDim.y;

    if (index_x >= cols || index_y >= rows) {
        return;
    }

	uchar* rgb = &d_input[3*(index_x + index_y * cols)];
    d_output[index_x + index_y*cols] = rgb[0] * .299f + rgb[1] * .587f + rgb[2] * .114f;
}
