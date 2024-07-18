#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

#define MAX_PLATFORMS 10
#define MAX_DEVICES 10

int main() {
    cl_platform_id platforms[MAX_PLATFORMS];
    cl_device_id devices[MAX_DEVICES];
    cl_uint num_platforms, num_devices;
    cl_context context;
    cl_command_queue command_queue;
    cl_program program;
    cl_kernel kernel;
    cl_int ret;

    // 获取平台数量
    ret = clGetPlatformIDs(MAX_PLATFORMS, platforms, &num_platforms);
    if (ret != CL_SUCCESS) {
        printf("Failed to get platform IDs\n");
        return -1;
    }

    printf("Number of platforms: %u\n", num_platforms);

    // 遍历打印平台信息
    for (cl_uint i = 0; i < num_platforms; i++) {
        char platform_name[128];
        char platform_vendor[128];

        ret = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
        if (ret != CL_SUCCESS) {
            printf("Failed to get platform name for platform %u\n", i);
        }

        ret = clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, sizeof(platform_vendor), platform_vendor, NULL);
        if (ret != CL_SUCCESS) {
            printf("Failed to get platform vendor for platform %u\n", i);
        }

        printf("Platform %u:\n", i);
        printf("    Name: %s\n", platform_name);
        printf("    Vendor: %s\n", platform_vendor);
        printf("\n");
    }

    // 获取设备数量
    ret = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, MAX_DEVICES, devices, &num_devices);
    if (ret != CL_SUCCESS) {
        printf("Failed to get device IDs\n");
        return -1;
    }

    // 创建OpenCL上下文
    context = clCreateContext(NULL, num_devices, devices, NULL, NULL, &ret);
    if (ret != CL_SUCCESS) {
        printf("Failed to create context\n");
        return -1;
    }

    // 创建命令队列
    command_queue = clCreateCommandQueue(context, devices[0], 0, &ret);
    if (ret != CL_SUCCESS) {
        printf("Failed to create command queue\n");
        return -1;
    }

    // 定义和构建OpenCL内核
    const char *kernel_source = "__kernel void hello_world() {\n"
                                "    printf(\"Hello, World!\\n\");\n"
                                "}\n";
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &ret);
    if (ret != CL_SUCCESS) {
        printf("Failed to create program\n");
        return -1;
    }

    ret = clBuildProgram(program, num_devices, devices, NULL, NULL, NULL);
    if (ret != CL_SUCCESS) {
        printf("Failed to build program\n");
        return -1;
    }

    // 创建OpenCL内核对象
    kernel = clCreateKernel(program, "hello_world", &ret);
    if (ret != CL_SUCCESS) {
        printf("Failed to create kernel\n");
        return -1;
    }

    // 执行内核函数
    ret = clEnqueueTask(command_queue, kernel, 0, NULL, NULL);
    if (ret != CL_SUCCESS) {
        printf("Failed to enqueue task\n");
        return -1;
    }

    // 等待执行完成
    ret = clFinish(command_queue);
    if (ret != CL_SUCCESS) {
        printf("Failed to finish execution\n");
        return -1;
    }

    printf("Kernel executed successfully\n");

    // 清理资源
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);

    return 0;
}
