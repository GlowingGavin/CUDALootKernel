#include <stdint.h>
#include <stdio.h>

__device__ __forceinline__ int next(uint64_t &seed, int bits){
    seed = (seed * 0x5deece66dULL + 0xbULL) & ((1ULL<<48)-1);
    return (int)(seed >> (48 - bits));
}
__device__ __forceinline__ int nextInt(uint64_t &seed, int n){
    int bits,val,m=n-1;
    if((m&n)==0) return (int)((n*(int64_t)next(seed,31))>>31);
    do{bits=next(seed,31);val=bits%n;}while(bits-val+m<0);
    return val;
}
__device__ __forceinline__ float nextFloat(uint64_t &seed){
    return next(seed,24)/(float)(1<<24);
}
__device__ __forceinline__ void nextSeed(uint64_t &seed){
    seed = (seed * 0x5deece66dULL + 0xbULL) & ((1ULL<<48)-1);
}

__device__ __forceinline__ int simulate(uint64_t seed){
int total_0 = 0;
int total_1 = 0;
int rolls0 = 5;
rolls0 += nextInt(seed, 6);
for(int r=0;r<rolls0;r++){
int entry = nextInt(seed,86);
if(entry < 1){
int count = 1;
count = nextInt(seed, 2) + 1;
total_0 += count;
} else if(entry < 2){
} else if(entry < 4){
int count = 1;
count = 1;
total_1 += count;
} else if(entry < 6){
nextInt(seed, 2);
} else if(entry < 8){
} else if(entry < 10){
int level = 30;
level += nextInt(seed, 21);
int delta = level / 4 + 1;
level += 1 + nextInt(seed, delta) + nextInt(seed, delta);
float amp = (nextFloat(seed) + nextFloat(seed) - 1.0f) * 0.15f;
level = (int)(level + level * amp + 0.5f);
nextSeed(seed);
while (nextInt(seed, 50) <= level) {
    nextSeed(seed);
    level /= 2;
}
} else if(entry < 12){
} else if(entry < 14){
} else if(entry < 16){
nextInt(seed, 5);
} else if(entry < 18){
} else if(entry < 20){
} else if(entry < 22){
int level = 30;
level += nextInt(seed, 21);
int delta = level / 4 + 1;
level += 1 + nextInt(seed, delta) + nextInt(seed, delta);
float amp = (nextFloat(seed) + nextFloat(seed) - 1.0f) * 0.15f;
level = (int)(level + level * amp + 0.5f);
nextSeed(seed);
while (nextInt(seed, 50) <= level) {
    nextSeed(seed);
    level /= 2;
}
} else if(entry < 25){
int ench = nextInt(seed, 5);
nextSeed(seed);
} else if(entry < 28){
nextInt(seed, 7);
} else if(entry < 31){
nextInt(seed, 3);
} else if(entry < 34){
nextInt(seed, 4);
} else if(entry < 37){
nextInt(seed, 15);
} else if(entry < 40){
nextInt(seed, 3);
} else if(entry < 43){
nextInt(seed, 15);
} else if(entry < 46){
int level = 20;
level += nextInt(seed, 20);
int delta = level / 4 + 1;
level += 1 + nextInt(seed, delta) + nextInt(seed, delta);
float amp = (nextFloat(seed) + nextFloat(seed) - 1.0f) * 0.15f;
level = (int)(level + level * amp + 0.5f);
nextSeed(seed);
while (nextInt(seed, 50) <= level) {
    nextSeed(seed);
    level /= 2;
}
} else if(entry < 50){
nextInt(seed, 3);
} else if(entry < 54){
nextInt(seed, 3);
} else if(entry < 59){
nextInt(seed, 3);
} else if(entry < 64){
int ench = nextInt(seed, 5);
nextSeed(seed);
} else if(entry < 69){
nextInt(seed, 8);
} else if(entry < 74){
nextInt(seed, 15);
} else if(entry < 79){
nextInt(seed, 15);
} else if(entry < 86){
nextInt(seed, 10);
} else { }
}
int rolls1 = 1;
for(int r=0;r<rolls1;r++){
int entry = nextInt(seed,80);
if(entry < 75){
} else if(entry < 79){
} else if(entry < 80){
} else { }
}
if(total_0 >= 6 && total_1 >= 5) return 1;
return 0;}


__global__ void kernel(uint64_t start){
    uint64_t tid = blockIdx.x*blockDim.x + threadIdx.x;
    uint64_t stride = gridDim.x * blockDim.x;

    uint64_t seed = start + tid;

	const uint64_t MAX_SEED = (1ULL << 48);

	while (seed < MAX_SEED)
	{
		#pragma unroll 8
		for (int i = 0; i < 8 && seed < MAX_SEED; i++)
		{
			uint64_t s = (seed ^ 0x5deece66dULL) & ((1ULL<<48)-1);

			int result = simulate(s);

			if(result){
				printf("%llu\n", seed);
			}

			seed += stride;
		}
	}
}

#include <cuda_runtime.h>

int main()
{
    const int BLOCKS = 2048;
    const int THREADS = 256;

    uint64_t start = 0;

    printf("Starting scan...\n");

    kernel<<<BLOCKS, THREADS>>>(start);

    cudaDeviceSynchronize();
	printf("Done.\n");

    return 0;
}
