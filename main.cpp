#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include "json.hpp"

using json = nlohmann::json;

struct Function {
    std::string type;
    int min = 0;
    int max = 0;
};

struct Entry {
    std::string item;
    int weight = 1;
    std::vector<Function> functions;
};

struct Pool {
    int min_rolls = 1;
    int max_rolls = 1;
    std::vector<Entry> entries;
};

std::unordered_map<std::string, std::string> load_constraints()
{
    std::unordered_map<std::string, std::string> m;
    std::ifstream f("constraints.txt");

    std::string line;
    while (std::getline(f, line))
    {
        std::istringstream iss(line);
		std::string key, value;

		if (iss >> key >> value)
		{
			m[key] = value;
		}
    }
    return m;
}

Function parse_function(const json& f)
{
    Function fn;

    std::string name = f["function"];

    if (name == "minecraft:set_count")
    {
        if (f["count"].is_number())
        {
            fn.type = "constant";
            fn.min = f["count"];
        }
        else
        {
            fn.type = "uniform";
            fn.min = f["count"]["min"];
            fn.max = f["count"]["max"];
        }
    }
    else if (name == "minecraft:enchant_with_levels")
    {
        fn.type = "enchant_levels";
        fn.min = f["levels"]["min"];
        fn.max = f["levels"]["max"];
    }
    else if (name == "minecraft:enchant_randomly")
    {
        fn.type = "enchant_random";
    }
	else if (name == "minecraft:set_damage")
	{
		fn.type = "damage";

		fn.min = 0;
		fn.max = 0;
	}
    else
    {
        fn.type = "other";
    }

    return fn;
}

Pool parse_pool(const json& p)
{
    Pool pool;

    if (p["rolls"].is_number())
    {
        pool.min_rolls = pool.max_rolls = p["rolls"];
    }
    else
    {
        pool.min_rolls = p["rolls"]["min"];
        pool.max_rolls = p["rolls"]["max"];
    }

	for (auto& e : p["entries"])
	{
		Entry entry;

		if (e["type"] == "minecraft:empty")
		{
			entry.item = "empty";
			entry.weight = e["weight"];
			pool.entries.push_back(entry);
			continue;
		}

		entry.item = e["name"];

        if (e.contains("weight"))
            entry.weight = e["weight"];

        if (e.contains("functions"))
        {
            for (auto& f : e["functions"])
                entry.functions.push_back(parse_function(f));
        }

        pool.entries.push_back(entry);
    }

    return pool;
}

void emit_function(std::ofstream& out, const Function& f, bool isTarget)
{
    if (f.type == "uniform")
    {
        if (isTarget)
            out << "count = nextInt(seed, " << (f.max - f.min + 1) << ") + " << f.min << ";\n";
        else
            out << "nextInt(seed, " << (f.max - f.min + 1) << ");\n";
    }
    else if (f.type == "constant")
    {
        if (isTarget)
            out << "count = " << f.min << ";\n";
    }
	else if (f.type == "enchant_levels")
	{
		out << "int level = " << f.min << ";\n";

		if (f.min != f.max)
			out << "level += nextInt(seed, " << (f.max - f.min + 1) << ");\n";

		out << "int delta = level / 4 + 1;\n";
		out << "level += 1 + nextInt(seed, delta) + nextInt(seed, delta);\n";

		out << "float amp = (nextFloat(seed) + nextFloat(seed) - 1.0f) * 0.15f;\n";
		out << "level = (int)(level + level * amp + 0.5f);\n";

		out << "nextSeed(seed);\n";

		out << "while (nextInt(seed, 50) <= level) {\n";
		out << "    nextSeed(seed);\n";
		out << "    level /= 2;\n";
		out << "}\n";
	}
    else if (f.type == "enchant_random")
    {
        out << "int ench = nextInt(seed, 5);\n";
		out << "nextSeed(seed);\n";
    }
	else if (f.type == "damage")
	{
		out << "nextFloat(seed);\n";
	}
}

void emit_pool(std::ofstream& out, const Pool& pool,
               const std::vector<std::pair<std::string,int>>& targets,
               int poolIndex)
{
    int totalWeight = 0;
    for (auto& e : pool.entries) totalWeight += e.weight;

    out << "int rolls" << poolIndex << " = " << pool.min_rolls << ";\n";

    if (pool.min_rolls != pool.max_rolls)
        out << "rolls" << poolIndex << " += nextInt(seed, "
            << (pool.max_rolls - pool.min_rolls + 1) << ");\n";

    out << "for(int r=0;r<rolls" << poolIndex << ";r++){\n";

	if (totalWeight > 1)
	{
		out << "int entry = nextInt(seed," << totalWeight << ");\n";
	}

    int acc = 0;

    for (size_t i = 0; i < pool.entries.size(); i++)
    {
        auto& e = pool.entries[i];
        int next = acc + e.weight;

        if (totalWeight > 1)
		{
			if (i == 0)
				out << "if(entry < " << next << "){\n";
			else
				out << "else if(entry < " << next << "){\n";
		}
		else
		{
			out << "{\n";
		}

        int targetIndex = -1;
		for (int t = 0; t < targets.size(); t++)
		{
			if (e.item == targets[t].first)
			{
				targetIndex = t;
				break;
			}
		}

		if (targetIndex != -1)
		{
			out << "int count = 1;\n";

			for (auto& f : e.functions)
				emit_function(out, f, true);

			out << "total_" << targetIndex << " += count;\n";
		}
        else
        {
            for (auto& f : e.functions)
                emit_function(out, f, false);
        }
		
		out << "}\n";
        acc = next;
    }
	
	if (totalWeight > 1)
    	out << "else { }\n";

    out << "}\n";
}

int main()
{
    auto cfg = load_constraints();
    std::vector<std::pair<std::string,int>> targets;

	for (auto& [k, v] : cfg)
	{
		targets.push_back({k, std::stoi(v)});
	}

    std::ifstream f("table.json");
    json j;
    f >> j;

    std::vector<Pool> pools;
    for (auto& p : j["pools"])
        pools.push_back(parse_pool(p));

    std::ofstream out("kernel.cu");

	out << R"(#include <stdint.h>
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
)";

for (int i = 0; i < targets.size(); i++)
{
    out << "int total_" << i << " = 0;\n";
}

    for (int i = 0; i < pools.size(); i++)
	{
		emit_pool(out, pools[i], targets, i);
	}

	if (!targets.empty())
	{
		out << "if(";
		for (int i = 0; i < targets.size(); i++)
		{
			if (i > 0) out << " && ";
			out << "total_" << i << " >= " << targets[i].second;
		}
		out << ") return 1;\n";
	}
	else
	{
		out << "return 1;\n";
	}

	out << "return 0;}\n";

	out << R"(

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
    const int BLOCKS = 8192;
    const int THREADS = 256;

    uint64_t start = 0;

    printf("Starting scan...\n");

    kernel<<<BLOCKS, THREADS>>>(start);

    cudaDeviceSynchronize();
	printf("Done.\n");

    return 0;
}
)";

    std::cout << "Generated kernel.cu\n";
}
