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
	//Получить количество доступных платформ
	cl_uint platformCount;
	clGetPlatformIDs(0, NULL, &platformCount);
	//Получить информацию для всех платформ
	cl_platform_id* platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) *platformCount);
	clGetPlatformIDs(platformCount, platforms, NULL);
	for (int p = 0; p < platformCount; p++) {
		// Получить число устройств для p-й платформы
		cl_uint deviceCount;
		clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
		// Получить информацию о всех устройствах для p-й платформы
		cl_device_id* devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
		clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);
		// Вывести атрибуты устройств
		for (int d = 0; d < deviceCount; d++) {
			// Вывести название устройства
			clGetDeviceInfo(devices[d], CL_DEVICE_NAME, 0, NULL, &valueSize);
			value = (char*) malloc(valueSize);
			clGetDeviceInfo(devices[d], CL_DEVICE_NAME, valueSize, value, NULL);
			printf("%d. Устройство: %s\n", d+1, value);
			free(value);
			// Вывести версию стандарта, поддерживаемую аппаратно
			clGetDeviceInfo(devices[d], CL_DEVICE_VERSION, 0, NULL, &valueSize);
			value = (char*) malloc(valueSize);
			clGetDeviceInfo(devices[d], CL_DEVICE_VERSION, valueSize, value, NULL);
			printf(" %d.1 Аппаратная версия стандарта: %s\n", d+1, value);
			free(value);
			// Вывести версию OpenCL C для компилятора программы-ядра
			clGetDeviceInfo(devices[d], CL_DEVICE_OPENCL_C_VERSION, 0, NULL,
			&valueSize);
			value = (char*) malloc(valueSize);
			clGetDeviceInfo(devices[d], CL_DEVICE_OPENCL_C_VERSION, valueSize, value, NULL);
			printf(" %d.2 OpenCL C версия: %s\n", d+1, value);
			free(value);
			// Вывести число вычислителей
			cl_uint maxComputeUnits;
			clGetDeviceInfo(devices[d], CL_DEVICE_MAX_COMPUTE_UNITS,
			sizeof(maxComputeUnits), &maxComputeUnits, NULL);
			printf(" %d.3 Количество параллельных вычислителей: %d\n", d+1,
			maxComputeUnits);
		}
		free(devices);
	}
	free(platforms);
}

void testOpenCL() 
{
	//Генерируем большой массив исходных данных
	int *array = (int *)malloc(MAXSIZE * sizeof(int));
	for (int i = 0; i < MAXSIZE; i++) {
		array[i] = rand()%MAXSIZE;
	}
	//Объекты OpenCL
	cl_platform_id platform;
	cl_device_id device; 
	cl_context context;
	cl_program program;
	cl_kernel kernel; 
	cl_command_queue queue;

	FILE* programHandle;
	char *programBuffer;
	size_t programSize;

	// Получаем первое устройство типа GPU первой доступной платформы
	int numberDevice = 1;
	clGetPlatformIDs(numberDevice, &platform, NULL);
	clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numberDevice, &device, NULL);
	// Получаем число вычислителей для этого устройства
	cl_uint units;
	clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(units), &units, NULL);

	// Создаем массив минимумов такого размера
	int *mins = (int *)malloc(units * sizeof(int));

	// Получаем контекст по устройству
	context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);

	// Открываем файл с ядром и вычисляем его размер
	fopen_s(&programHandle, "cl_core.cl", "rb");
	fseek(programHandle, 0L, SEEK_END);
	programSize = ftell(programHandle);
	rewind(programHandle);

	// Читаем файл с ядром
	programBuffer = (char*) malloc(programSize + 1);
	programBuffer[programSize] = '\0';
	fread(programBuffer, sizeof(char), programSize, programHandle);
	fclose(programHandle);

	// Создаем объект программы и компилируем файл с ядром
	program = clCreateProgramWithSource(context, 1, (const char**) &programBuffer, &programSize, NULL);
	free(programBuffer);

	// CL1.2 – флаг компилятора, поддерживаемый стандарт
	clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);

	// Создаем объект ядра и очередь команд
	int err = 0;
	// findMinValue – функция поиска минимума в ядре
	kernel = clCreateKernel(program, "findMinValue", &err);

	if (err == CL_INVALID_KERNEL_NAME)  {
		// ошибка создания ядра
		return;
	}
	//queue = clCreateCommandQueue(context, device, 0, NULL);
	queue = clCreateCommandQueueWithProperties(context, device, 0, NULL);
	
	// Создаем представление для исходного массива
	cl_mem clArray = clCreateBuffer(context, CL_MEM_READ_WRITE, MAXSIZE * sizeof(int), NULL, NULL);
	// Копируем данные из исходного массива в глобальные данные
	clEnqueueWriteBuffer(queue, clArray, CL_TRUE, 0, MAXSIZE * sizeof(int), array, 0, NULL, NULL);

	// Создаем представление для результирующего массива
	cl_mem clMins = clCreateBuffer(context, CL_MEM_READ_WRITE, units * sizeof(int), NULL, NULL);
	
	// Определяем размер куска данных для каждого вычислителя
	int chunkSize = MAXSIZE / units;

	// Создаем представление для этого параметра
	cl_mem clChunkSize = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int), NULL, NULL);
	
	// Копируем его в глобальную память
	clEnqueueWriteBuffer(queue, clChunkSize, CL_TRUE, 0, sizeof(int), &chunkSize, 0, NULL, NULL);

	// Устанавливаем агрументы для ядра
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &clArray);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), &clMins);
	clSetKernelArg(kernel, 2, sizeof(cl_mem), &clChunkSize);

	// Создаем одномерную сетку из вычислителей (units штук)
	size_t global_item_size = units;
	size_t local_item_size = 1;

	// Запускаем ядро в режиме параллельных данных с заданной сеткой
	clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
	
	// Команда на считываение результата из представления clMins в локальный массив с минимумами
	clEnqueueReadBuffer(queue, clMins, CL_TRUE, 0, units * sizeof(int), mins, 0, NULL, NULL);
	// Ищем минимум из минимумов
	int min = INT_MAX;
	for (int i = 0; i < units; i++) {
		printf("analisyng min[%d] = %d\n", i, mins[i]);
		if (mins[i]<min) min = mins[i];
	}
	printf("Total min = %d\n",min);

	// Очистка
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
	//Объекты OpenCL
	cl_platform_id platform;
	cl_device_id device; 
	cl_context context;
	cl_program program;
	cl_kernel kernel; 
	cl_command_queue queue;

	FILE* programHandle;
	char *programBuffer;
	size_t programSize;

	// Получаем устройство типа GPU
	clGetPlatformIDs(nDevice, &platform, NULL);
	clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, nDevice, &device, NULL);
	
	// Получаем число вычислителей для этого устройства
	cl_uint units;
	clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(units), &units, NULL);
	numberUnit = units;

	// Получаем контекст по устройству
	context = clCreateContext(NULL, nDevice, &device, NULL, NULL, NULL);

	// Открываем файл с ядром и вычисляем его размер
	fopen_s(&programHandle, nameClFile, "rb");
	fseek(programHandle, 0, SEEK_END);
	programSize = ftell(programHandle);
	rewind(programHandle);

	// Читаем файл с ядром
	programBuffer = (char*) malloc(programSize + 1);
	memset(programBuffer, 0, programSize);
	programBuffer[programSize] = '\0';
	fread(programBuffer, sizeof(char), programSize, programHandle);
	fclose(programHandle);

	// Создаем объект программы и компилируем файл с ядром
	program = clCreateProgramWithSource(context, 1, (const char**) &programBuffer, &programSize, NULL);
	free(programBuffer);

	// CL1.2 – флаг компилятора, поддерживаемый стандарт
	int err = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);

	if (err != CL_SUCCESS)
	{
		printf("clBuildProgram не смогла скомпилировать ядро: %d\n", err);
		char log[0x10000];
		clGetProgramBuildInfo( program, device, CL_PROGRAM_BUILD_LOG, 0x10000, log, NULL);
		printf("\n%s\n", log);
		return NULL;
	}
	// Создаем объект ядра и очередь команд
	err = 0;
	// findMinValue – функция поиска минимума в ядре
	kernel = clCreateKernel(program, nameClFunction, &err);
	// Проверка ошибок компиляции ядра
	if (err == CL_INVALID_KERNEL_NAME)  {
		// ошибка создания ядра
		return NULL;
	}
	 
	//queue = clCreateCommandQueue(context, device, 0, NULL);
	queue = clCreateCommandQueueWithProperties(context, device, 0, NULL);
	
	// Создаем представление для исходного массива
	int sizeTypeInp = sizeof(inp);
	cl_mem clInp = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeInp * sizeTypeInp, NULL, NULL);
	// Копируем данные из исходного массива в глобальные данные
	clEnqueueWriteBuffer(queue, clInp, CL_TRUE, 0, sizeInp * sizeof(inp), inp, 0, NULL, NULL);

	// Создаем представление для результирующего массива
	cl_mem clResults = clCreateBuffer(context, CL_MEM_READ_WRITE, units * sizeTypeInp, NULL, NULL);
	
	// Определяем размер куска данных для каждого вычислителя
	int chunkSize = sizeInp / units;

	// Создаем представление для этого параметра
	cl_mem clChunkSize = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeTypeInp, NULL, NULL);
	
	// Копируем его в глобальную память
	clEnqueueWriteBuffer(queue, clChunkSize, CL_TRUE, 0, sizeTypeInp, &chunkSize, 0, NULL, NULL);

	// Устанавливаем агрументы для ядра
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &clInp);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), &clResults);
	clSetKernelArg(kernel, 2, sizeof(cl_mem), &clChunkSize);

	// Создаем одномерную сетку из вычислителей (units штук)
	size_t global_item_size = units;
	size_t local_item_size = 1;

	// Запускаем ядро в режиме параллельных данных с заданной сеткой
	clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
	
	// Создаем массив результатов такого размера
	int *results = (int *)malloc(units * sizeTypeInp);

	// Команда на считываение результата из представления clResults в локальный массив
	clEnqueueReadBuffer(queue, clResults, CL_TRUE, 0, units * sizeTypeInp, results, 0, NULL, NULL);
	
	// Очистка
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

