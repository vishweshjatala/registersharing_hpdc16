#ifndef OPEN_SOURCE_H
#define OPEN_SOURCE_H
static unsigned int kernel_timer = 0;
double overall_kernel_time = 0;

#define BEFORE_KERNEL(k_name) {\
	cutCreateTimer(&kernel_timer);\
	cudaThreadSynchronize();\
	cutStartTimer(kernel_timer);\
}


#define AFTER_KERNEL(k_name) {\
	cudaError_t kernel_err = cudaThreadSynchronize();\
	cutStopTimer(kernel_timer);\
	float kerneltime = cutGetTimerValue(kernel_timer);\
	overall_kernel_time += kerneltime;\
	cutDeleteTimer(kernel_timer);\
    if (kernel_err != cudaSuccess) { printf("%s kernel execution failed\n", k_name); }\
	else { printf("%s kernel execution %f ms, %f ms\n", k_name, kerneltime, overall_kernel_time); }\
}
#endif

