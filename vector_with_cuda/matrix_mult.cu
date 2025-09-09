#include <iostream>
#include <cstdlib>
#include <chrono>
#include <random>
#include <cuda_runtime.h>
#include "vector.hpp"
// Проверка ошибок CUDA
void checkCudaError(cudaError_t err, const char* msg) {
if (err != cudaSuccess) {
std::cerr << "CUDA Error (" << msg << "): " << cudaGetErrorString(err) << std::endl;
exit(1);
}
}
// Функция для генерации случайной матрицы
void generateMatrix(Vector<double>& mat, size_t size) {
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<double> dis(0.0, 1.0);
for (size_t i = 0; i < size * size; ++i) {
mat[i] = dis(gen);
}
}
// Умножение матриц на CPU
void matrixMultCPU(const Vector<double>& A, const Vector<double>& B, Vector<double>& C,
size_t size) {
for (size_t i = 0; i < size; ++i) {
for (size_t j = 0; j < size; ++j) {
double sum = 0.0;
for (size_t k = 0; k < size; ++k) {
sum += A[i * size + k] * B[k * size + j];
}
C[i * size + j] = sum;
}
}
}
// Базовое CUDA ядро для умножения матриц
__global__ void matrixMultKernel(const double* A, const double* B, double* C, int size) {
int row = blockIdx.y * blockDim.y + threadIdx.y;
int col = blockIdx.x * blockDim.x + threadIdx.x;
if (row < size && col < size) {
double sum = 0.0;
for (int k = 0; k < size; ++k) {
sum += A[row * size + k] * B[k * size + col];
}
C[row * size + col] = sum;
}
}
// Оптимизированное CUDA ядро с использованием shared memory
__global__ void matrixMultKernelOptimized(const double* A, const double* B, double* C, int size) {
__shared__ double sA[16][16];
__shared__ double sB[16][16];
int row = blockIdx.y * blockDim.y + threadIdx.y;
int col = blockIdx.x * blockDim.x + threadIdx.x;
double sum = 0.0;
for (int tile = 0; tile < (size + 15) / 16; ++tile) {
if (row < size && tile * 16 + threadIdx.x < size) {
sA[threadIdx.y][threadIdx.x] = A[row * size + tile * 16 + threadIdx.x];
} else {
sA[threadIdx.y][threadIdx.x] = 0.0;
}
if (col < size && tile * 16 + threadIdx.y < size) {
sB[threadIdx.y][threadIdx.x] = B[(tile * 16 + threadIdx.y) * size + col];
} else {
sB[threadIdx.y][threadIdx.x] = 0.0;
}
__syncthreads();
for (int k = 0; k < 16; ++k) {
sum += sA[threadIdx.y][k] * sB[k][threadIdx.x];
}
__syncthreads();
}
if (row < size && col < size) {
C[row * size + col] = sum;
}
}
// Функция для умножения на GPU
void matrixMultGPU(const Vector<double>& A, const Vector<double>& B, Vector<double>& C,
size_t size, bool optimized = false) {
double *d_A, *d_B, *d_C;
checkCudaError(cudaMalloc(&d_A, size * size * sizeof(double)), "cudaMalloc A");
checkCudaError(cudaMalloc(&d_B, size * size * sizeof(double)), "cudaMalloc B");
checkCudaError(cudaMalloc(&d_C, size * size * sizeof(double)), "cudaMalloc C");
checkCudaError(cudaMemcpy(d_A, A.data(), size * size * sizeof(double), cudaMemcpyHostToDevice),
"cudaMemcpy A");
checkCudaError(cudaMemcpy(d_B, B.data(), size * size * sizeof(double), cudaMemcpyHostToDevice),
"cudaMemcpy B");
dim3 threadsPerBlock(16, 16);
dim3 blocksPerGrid((size + threadsPerBlock.x - 1) / threadsPerBlock.x,
(size + threadsPerBlock.y - 1) / threadsPerBlock.y);
if (optimized) {
matrixMultKernelOptimized<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_B, d_C, size);
} else {
matrixMultKernel<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_B, d_C, size);
}
checkCudaError(cudaGetLastError(), "Kernel launch");
checkCudaError(cudaMemcpy(C.data(), d_C, size * size * sizeof(double), cudaMemcpyDeviceToHost),
"cudaMemcpy C");
checkCudaError(cudaFree(d_A), "cudaFree A");
checkCudaError(cudaFree(d_B), "cudaFree B");
checkCudaError(cudaFree(d_C), "cudaFree C");
}
int main(int argc, char* argv[]) {
// Получаем размер матрицы из аргументов командной строки
size_t N = 1024; // Значение по умолчанию
if (argc > 1) {
N = std::atoi(argv[1]);
if (N <= 0) {
std::cerr << "Invalid matrix size. Using default N=1024." << std::endl;
N = 1024;
}
}
// Проверяем наличие GPU
int deviceCount;
checkCudaError(cudaGetDeviceCount(&deviceCount), "cudaGetDeviceCount");
if (deviceCount == 0) {
std::cerr << "No CUDA-capable GPU found." << std::endl;
return 1;
}
// Инициализируем матрицы
Vector<double> A(N * N);
Vector<double> B(N * N);
Vector<double> C_cpu(N * N);
Vector<double> C_gpu(N * N);
Vector<double> C_gpu_opt(N * N);
generateMatrix(A, N);
generateMatrix(B, N);
// CPU умножение
auto start = std::chrono::high_resolution_clock::now();
matrixMultCPU(A, B, C_cpu, N);
auto end = std::chrono::high_resolution_clock::now();
double cpu_time = std::chrono::duration<double>(end - start).count();
std::cout << "CPU time: " << cpu_time << " sec" << std::endl;
// GPU умножение (базовая версия)
start = std::chrono::high_resolution_clock::now();
matrixMultGPU(A, B, C_gpu, N, false);
end = std::chrono::high_resolution_clock::now();
double gpu_time = std::chrono::duration<double>(end - start).count();
std::cout << "GPU time (basic): " << gpu_time << " sec" << std::endl;
// GPU умножение (оптимизированная версия)
start = std::chrono::high_resolution_clock::now();
matrixMultGPU(A, B, C_gpu_opt, N, true);
end = std::chrono::high_resolution_clock::now();
double gpu_opt_time = std::chrono::duration<double>(end - start).count();
std::cout << "GPU time (optimized): " << gpu_opt_time << " sec" << std::endl;
// Проверка результатов и вычисление максимальной ошибки
double max_error = 0.0;
for (size_t i = 0; i < N * N; ++i) {
max_error = std::max(max_error, std::abs(C_cpu[i] - C_gpu[i]));
max_error = std::max(max_error, std::abs(C_cpu[i] - C_gpu_opt[i]));
}
bool correct = max_error <= 1e-5;
std::cout << "Results match: " << (correct ? "Yes" : "No") << " (Max error: " << max_error << ")" <<
std::endl;
// Вывод ускорения
std::cout << "Speedup (basic GPU vs CPU): " << cpu_time / gpu_time << "x" << std::endl;
std::cout << "Speedup (optimized GPU vs CPU): " << cpu_time / gpu_opt_time << "x" << std::endl;
return 0;
}