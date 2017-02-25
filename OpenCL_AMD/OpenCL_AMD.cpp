#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "locale.h"
#include "Calculator.h"

#include <CL/opencl.h>

#include <windows.h>
#include <fstream>

#define MAXSIZE 100000000

void printfinfo() {
	char* value;
	size_t valueSize;
	
	cl_uint platformCount;
	clGetPlatformIDs(0, NULL, &platformCount);
	
	cl_platform_id* platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) *platformCount);
	
	clGetPlatformIDs(platformCount, platforms, NULL);
	
	for (int p = 0; p < platformCount; p++) {
		
		cl_uint deviceCount;
		clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
		
		cl_device_id* devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
		clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);
		
		for (int d = 0; d < deviceCount; d++) {
			
			clGetDeviceInfo(devices[d], CL_DEVICE_NAME, 0, NULL, &valueSize);
			
			value = (char*) malloc(valueSize);
			clGetDeviceInfo(devices[d], CL_DEVICE_NAME, valueSize, value, NULL);
			free(value);
			
			clGetDeviceInfo(devices[d], CL_DEVICE_VERSION, 0, NULL, &valueSize);
			value = (char*) malloc(valueSize);
			clGetDeviceInfo(devices[d], CL_DEVICE_VERSION, valueSize, value, NULL);
			free(value);
			
			clGetDeviceInfo(devices[d], CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &valueSize);
			value = (char*) malloc(valueSize);
			clGetDeviceInfo(devices[d], CL_DEVICE_OPENCL_C_VERSION, valueSize, value, NULL);
			free(value);
			
			cl_uint maxComputeUnits;
			clGetDeviceInfo(devices[d], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(maxComputeUnits), &maxComputeUnits, NULL);
		}
		
		free(devices);
	}
	
	free(platforms);
}

void testOpenCL() 
{
	int *array = (int *)malloc(MAXSIZE * sizeof(int));
	for (int i = 0; i < MAXSIZE; i++) {
		array[i] = rand()%MAXSIZE;
	}
	
	cl_platform_id platform;
	cl_device_id device; 
	cl_context context;
	cl_program program;
	cl_kernel kernel; 
	cl_command_queue queue;

	FILE* programHandle;
	char *programBuffer;
	size_t programSize;

	int numberDevice = 1;
	clGetPlatformIDs(numberDevice, &platform, NULL);
	clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numberDevice, &device, NULL);
	cl_uint units;
	clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(units), &units, NULL);

	int *mins = (int *)malloc(units * sizeof(int));
	context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);

	fopen_s(&programHandle, "cl_core.cl", "rb");
	fseek(programHandle, 0L, SEEK_END);
	programSize = ftell(programHandle);
	rewind(programHandle);

	programBuffer = (char*) malloc(programSize + 1);
	programBuffer[programSize] = '\0';
	
	fread(programBuffer, sizeof(char), programSize, programHandle);
	fclose(programHandle);

	program = clCreateProgramWithSource(context, 1, (const char**) &programBuffer, &programSize, NULL);
	free(programBuffer);

	clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);

	int err = 0;
	
	// findMinValue
	kernel = clCreateKernel(program, "findMinValue", &err);
	if (err == CL_INVALID_KERNEL_NAME)  {
		return;
	}
	//queue = clCreateCommandQueue(context, device, 0, NULL);
	queue = clCreateCommandQueueWithProperties(context, device, 0, NULL);
	
	cl_mem clArray = clCreateBuffer(context, CL_MEM_READ_WRITE, MAXSIZE * sizeof(int), NULL, NULL);
	clEnqueueWriteBuffer(queue, clArray, CL_TRUE, 0, MAXSIZE * sizeof(int), array, 0, NULL, NULL);

	cl_mem clMins = clCreateBuffer(context, CL_MEM_READ_WRITE, units * sizeof(int), NULL, NULL);
	
	int chunkSize = MAXSIZE / units;
	
	cl_mem clChunkSize = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int), NULL, NULL);
	clEnqueueWriteBuffer(queue, clChunkSize, CL_TRUE, 0, sizeof(int), &chunkSize, 0, NULL, NULL);

	clSetKernelArg(kernel, 0, sizeof(cl_mem), &clArray);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), &clMins);
	clSetKernelArg(kernel, 2, sizeof(cl_mem), &clChunkSize);

	size_t global_item_size = units;
	size_t local_item_size = 1;

	clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
	clEnqueueReadBuffer(queue, clMins, CL_TRUE, 0, units * sizeof(int), mins, 0, NULL, NULL);
	
	int min = INT_MAX;
	for (int i = 0; i < units; i++) {
		printf("analisyng min[%d] = %d\n", i, mins[i]);
		if (mins[i]<min) min = mins[i];
	}
	printf("Total min = %d\n",min);

	free(mins);
	free(array);
	clFlush(queue);
	clFinish(queue);
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseMemObject(clArray);
	clReleaseMemObject(clMins);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
}

template <typename type>
void* cl_execute(int nDevice, char * nameClFile, char* nameClFunction, type inp, int sizeInp, int &numberUnit) 
{
	cl_platform_id platform;
	cl_device_id device; 
	cl_context context;
	cl_program program;
	cl_kernel kernel; 
	cl_command_queue queue;

	FILE* programHandle;
	char *programBuffer;
	size_t programSize;

	clGetPlatformIDs(nDevice, &platform, NULL);
	clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, nDevice, &device, NULL);
	
	cl_uint units;
	clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(units), &units, NULL);
	numberUnit = units;

	context = clCreateContext(NULL, nDevice, &device, NULL, NULL, NULL);

	fopen_s(&programHandle, nameClFile, "rb");
	fseek(programHandle, 0, SEEK_END);
	programSize = ftell(programHandle);
	rewind(programHandle);

	programBuffer = (char*) malloc(programSize + 1);
	memset(programBuffer, 0, programSize);
	programBuffer[programSize] = '\0';
	fread(programBuffer, sizeof(char), programSize, programHandle);
	fclose(programHandle);

	program = clCreateProgramWithSource(context, 1, (const char**) &programBuffer, &programSize, NULL);
	free(programBuffer);

	int err = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);

	if (err != CL_SUCCESS)
	{
		printf("clBuildProgram íå ñìîãëà ñêîìïèëèðîâàòü ÿäðî: %d\n", err);
		char log[0x10000];
		clGetProgramBuildInfo( program, device, CL_PROGRAM_BUILD_LOG, 0x10000, log, NULL);
		printf("\n%s\n", log);
		return NULL;
	}

	err = 0;

	// findMinValue
	kernel = clCreateKernel(program, nameClFunction, &err);
	
	if (err == CL_INVALID_KERNEL_NAME)  {
		return NULL;
	}
	 
	//queue = clCreateCommandQueue(context, device, 0, NULL);
	queue = clCreateCommandQueueWithProperties(context, device, 0, NULL);
	
	int sizeTypeInp = sizeof(inp);
	cl_mem clInp = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeInp * sizeTypeInp, NULL, NULL);
	clEnqueueWriteBuffer(queue, clInp, CL_TRUE, 0, sizeInp * sizeof(inp), inp, 0, NULL, NULL);

	cl_mem clResults = clCreateBuffer(context, CL_MEM_READ_WRITE, units * sizeTypeInp, NULL, NULL);	
	
	int chunkSize = sizeInp / units;

	cl_mem clChunkSize = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeTypeInp, NULL, NULL);
	clEnqueueWriteBuffer(queue, clChunkSize, CL_TRUE, 0, sizeTypeInp, &chunkSize, 0, NULL, NULL);
	
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &clInp);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), &clResults);
	clSetKernelArg(kernel, 2, sizeof(cl_mem), &clChunkSize);

	size_t global_item_size = units;
	size_t local_item_size = 1;

	clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
	
	int *results = (int *)malloc(units * sizeTypeInp);

	clEnqueueReadBuffer(queue, clResults, CL_TRUE, 0, units * sizeTypeInp, results, 0, NULL, NULL);

	clFlush(queue);
	clFinish(queue);
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseMemObject(clInp);
	clReleaseMemObject(clResults);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);

	return results;
}

int _tmain(int argc, _TCHAR* argv[])
{
	setlocale(LC_ALL, "rus");
	printfinfo();

	double intv;
	int sizeMemory = 1024*1024; // bytes 512
	printf("Need memory %d Bytes for %d units of type '__int64 ...'\n", sizeMemory, sizeMemory/sizeof(big));
	
	Calculator * calculator = new Calculator(sizeMemory, intv, 15);
	std::cout << "Done. [interval = " << intv << endl;
	printf("\nStart calculation LCM OpenCL\n");
	intv = clock();
	int * results = NULL, nUnits = 0;

	results = (int*)cl_execute(1, "cl_core.cl", "lcmThroughCycle", calculator->getNumberSet(), calculator->lengthNumberSet, nUnits);

	intv = (double) (clock() - intv) / CLOCKS_PER_SEC;
	calculator->setNumberSet((big *)results, nUnits);
	std::cout << "Done calculation. Answer: " << calculator->lcmThroughCycle() << " [interval = "<< intv << "]" << endl;

	return 0;
}

