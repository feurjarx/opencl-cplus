__kernel void findMinValue(
	__global int * myArray,
	__global int * mins,
	__global int * chunkSize)
{
	int gid = get_global_id(0);

	int startOffset = (*chunkSize) * gid;
	int endOffset = startOffset + (*chunkSize);
	
	int myMin = INFINITY;
	for (int i = startOffset; i < endOffset; i++)
	{
		mins[i] = chunkSize[0];
	}
}


ulong lcm(ulong a, ulong b);

__kernel void lcmThroughCycle(
	__global ulong * inp,
	__global ulong * results,
	__global int *chunkSize)
{

	int gid = get_global_id(0);

	int startOffset = (*chunkSize) * gid;
	int endOffset = startOffset + (*chunkSize);

	ulong result = 1;
	for (int i = startOffset; i < endOffset; i++) {
		result = lcm(result, (ulong)inp[i]);
	}
	
	results[gid] = result;
}

ulong lcm(ulong a, ulong b) {
	ulong _a = a;
	ulong _b = b;
	
	ulong temp;
	for (;;)
    {
        if (_a == 0) {
			temp = _b;
			break;
		}
        _b %= _a;
        if (_b == 0) {
			temp = _a;
			break;
		}
        _a %= _b;
    }
	return temp ? (a / temp * b) : 0;

}


