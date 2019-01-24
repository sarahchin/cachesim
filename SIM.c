// Sarah Chin

// Cache Simulator written in C
// Reads in text files of format:
// R/W 0x0000000000000000
// and simulates hits or misses based on associativity, replacement policy,
// and write back policy.
// Input format: <CACHE_SIZE> <ASSOC> <REPLACEMENT> <WB> <TRACE_FILE>

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

// Functional Prototypes
int **create_array(int row, int col);
unsigned long long int **create_cache(int row, int col);
int find_max(int **array, int row);
int find_min(int **array, int row);
void print_output();
int update_lru(int row, int column, int operation);
int update_fifo(int row, int column, int operation);
void simulate();

// Global Variables
char op;
unsigned long long int address;
unsigned long long int tag;
int cache_size, assoc, replace, wb, writes, reads, misses, num_sets, accesses;
unsigned long long int **cache_array;
int **lru;
int **dirty;
int **fifo;

// Helper function to initialize 2D int arrays.
int **create_array(int row, int col)
{
	int i, j;
	int **array = NULL;
	
	array = (int **)malloc(sizeof(int*) * row);

	for (i = 0; i < row; i++)
	{
		array[i] = (int *)malloc(sizeof(int) * col);
	}
	
	for (i = 0; i < row; i++)
	{
		for (j = 0; j < col; j++)
			array[i][j] = 0;
	}
	
	return array;
}

// Helper function to initialize the cache (tag array)
// Initializes 2D long long int arrays.
unsigned long long int **create_cache(int row, int col)
{
	int i, j;
	unsigned long long int **array = NULL;
	
	array = (unsigned long long int **)malloc(sizeof(unsigned long long int*) * row);

	for (i = 0; i < row; i++)
	{
		array[i] = (unsigned long long int *)malloc(sizeof(unsigned long long int) * col);
	}
	
	for (i = 0; i < row; i++)
	{
		for (j = 0; j < col; j++)
			array[i][j] = 0;
	}
	
	return array;
}

// Helper function to print any 2D int arrays.
// Used for debugging.
/* void print_array(int **array, int row, int col)
{
	int i, j;
	
	for (i = 0; i < row; i++)
	{
		for (j = 0; j < col; j++)
		{
			printf("[%d][%d] = %d", i, j, array[i][j]);
			printf("%s", "\t");
		}
		printf("\n");
	}
} */

// Helper function to print the cache (tag) array.
// Used for debugging.
/* void print_cache(unsigned long long int **array, int row, int col)
{
	int i, j;
	
	for (i = 0; i < row; i++)
	{
		for (j = 0; j < col; j++)
		{
			printf("[%d][%d] = %lld", i, j, array[i][j]);
			printf("%s", "\t");
		}
		printf("\n");
	}
} */

// Prints program output.
void print_output()
{
	float miss_ratio = misses;
	
	// Calculate the miss ratio. 
	// Divide total number of misses by total number of accesses. 
	miss_ratio = miss_ratio/accesses;
	
	

	printf("The current cache is %d bytes with %d associativity with %d sets\n",
		cache_size, assoc, num_sets);
	printf("Current replacement policy is %s\n", (replace == 0) ? "LRU" : "FIFO");
	printf("Current write back policy is %s\n", (wb == 0) ? "Write-Through" : " Write-Back");
	printf("The total number of misses: %d\n", misses);
	printf("The total number of hits: %d\n", accesses-misses);
	printf("The total number of accesses: %d\n", accesses);
}

// Helper function. Updates the LRU array as needed.
// The LRU implementation works like a time stamp.
// Most recently accessed block in a set should have the
// largest value in the lru array. Anytime a block is accessed,
// update it so that it holds the largest value in the set.
int update_lru(int row, int column, int operation)
{
	// evicted_block holds the column number of the evicted block.
	int max, evicted_block;
	
	// The indicated block has been recently accessed.
	// Update it to hold the most recent time stamp.
	// No evictions.
	if (operation == 1)
	{
		max = find_max(lru, row);
		lru[row][column] = max + 1;
		
		return 0;
	}
	// We need to evict a block.
	// Evict the block with the lowest number.
	// This is the block that has been least recently accessed.
	if (operation == 2)
	{
		evicted_block = find_min(lru, row);
		
		// If the WB policy is Write Back
		if (wb == 1)
		{
			if (dirty[row][evicted_block] == 1)
			{
				writes++;
			}	
		}
		
		// Evict the block. Nothing there, so set the dirty bit to 0.
		dirty[row][evicted_block] = 0;
		
		// Replace the evicted block with the new data.
		// (Update the status in the LRU array as the MRU)
		max = find_max(lru, row);
		lru[row][evicted_block] = max + 1;

		return evicted_block;
	}	
}

// Helper function to update the FIFO array.
// Works similarly to the LRU implementation but only
// updates when new data is brought to the cache, not when
// existing cache data is accessed.
int update_fifo(int row, int column, int operation)
{
	int max, evicted_block;
	
	// Update the FIFO array when a new block is brought in.
	if (operation == 1)
	{
		max = find_max(fifo, row);
		fifo[row][column] = max + 1;
		
		return 0;
	}
	// When a block needs to be evicted.
	if (operation == 2)
	{
		evicted_block = find_min(lru, row);
		
		// If the WB policy is Write Back
		if (wb == 1)
		{
			if (dirty[row][evicted_block] == 1)
			{
				writes++;
			}	
		}
		
		// Evict the block. Set the dirty bit and the fifo
		// array to 0.
		dirty[row][evicted_block] = 0;
		fifo[row][evicted_block] = 0;
		
		// Move in the new block. 
		// Update it's value to be the largest in its set.
		// (aka Last One In)
		max = find_max(fifo, row);
		fifo[row][evicted_block] = max + 1;
		
		return evicted_block;
	}
}

// A helper function that will return the max value in a 
// particular row in an array.
int find_max(int **array, int row)
{
	int i, max = 0;
	
	for (i = 0; i < assoc; i++)
	{
		if (array[row][i] > max)
			max = array[row][i];
	}
	
	return max;
}

// A helper function that will return the index of the block
// in a set with the minimum value.
int find_min(int **array, int row)
{
	int i, block, min = INT_MAX;
	
	for (i = 0; i < assoc; i++)
	{
		if (array[row][i] < min)
		{
			min = array[row][i];
			block = i;
		}
	}
	
	return block;
}

// Simulate the cache for every memory access. 
void simulate()
{
	unsigned long long int index;
	int i, column; 
	
	// Find the set where the block belongs.
	index = address/64;
	index = index % num_sets;
	
	// Find the tag. 
	tag = address/64;
	tag = tag/num_sets;
	
	// Check if the data exists in the cache already.
	for (i = 0; i < assoc; i++)
	{	
		// The data exists.
		if (cache_array[index][i] == tag)
		{
			// We're writing to memory.
			if(op == 'W')
			{
				// Update LRU
				if (replace == 0)
				{
					update_lru(index, i, 1);
				}
				// Update FIFO
				// Data already existed, so nothing to do.
				if (replace == 1)
				{
					// Do nothing.
				}
				// If it's a write through policy, update main memory now.
				if (wb == 0)
					writes++;
				// If it's a write back policy, mark it dirty and update
				// memory when it's evicted. 
				if (wb == 1)
				{
					dirty[index][i] = 1;
				}
				// We're done. Return.
				return;
			}
			// We're reading from the cache.
			if(op == 'R')
			{
				// Update the LRU array with this block being most
				// recently accessed. 
				if (replace == 0)
				{
					update_lru(index, i, 1);
				}
				// Update FIFO.
				// The data already existed, so nothing to update.
				if (replace == 1)
				{
					// Do nothing.
				}
				return;
			}
		}	
	}
	
	// The data didn't exist. 
	for (i = 0; i < assoc; i++)
	{
		// There is an empty block in the set. 
		if (cache_array[index][i] == 0)
		{
			// We're writing to memory.
			if (op == 'W')
			{
				// Bring the data from 'memory'
				// (Save the tag in the cache block.)
				cache_array[index][i] = tag;
				misses++;
				reads++;
				
				// Update LRU
				if (replace == 0)
				{
					update_lru(index, i, 1);
				}
				// Update FIFO
				if (replace == 1)
				{
					update_fifo(index, i, 1);
				}
				// If it's a write through policy, update main memory now.
				if (wb == 0)
					writes++;
				// If it's a write back policy, mark it dirty and update
				// memory when it's evicted. 
				if (wb == 1)
				{
					dirty[index][i] = 1;
				}
				// We're done. Return.
				return;
			}
			if(op == 'R')
			{
				// Bring the data from memory into an empty block in the set.
				//printf("Found an empty block.\n");
				cache_array[index][i] = tag;
				misses++;
				reads++;
				
				// Update LRU
				if (replace == 0)
				{
					update_lru(index, i, 1);
				}
				// Update FIFO
				if (replace == 1)
				{
					update_fifo(index, i, 1);
				}
				return;
			}
		}
	}
	
	// The data doesn't exist and we have no empty blocks in the set.
	// Evict and replace with the accessed data.
	
	// Writing to Memory
	if (op == 'W')
	{
		misses++;
		reads++;
		// LRU Replacement policy
		if (replace == 0)
		{
			// Find the LRU block within the set.
			// Evict it and replace it with the new block. 
			column = update_lru(index, 0, 2);
			cache_array[index][column] = tag;
		}
		// FIFO Replacement policy
		if (replace == 1)
		{
			column = update_fifo(index, i, 2);
			cache_array[index][column] = tag;
		}
		if (wb == 0)
			writes++;
		if (wb == 1)
			dirty[index][column] = 1;
		return;
	}
	// Reading from memory.
	if (op == 'R')
	{
		reads++;
		misses++;
		// LRU Replacement policy
		if (replace == 0)
		{
			column = update_lru(index, 0, 2);
			cache_array[index][column] = tag;
		}
		// FIFO Replacement policy
		if (replace == 1)
		{
			column = update_fifo(index, 0, 2);
			cache_array[index][column] = tag;
		}
		return;
	}

}

int main(int argc, char **argv)
{
	FILE * input = NULL; 
	
	// Ensure that the input format is correct.
	if(argc != 6)
	{
		fprintf(stderr, "Input format: <CACHE_SIZE> <ASSOC> <REPLACEMENT> <WB> <TRACE_FILE>\n");
		return -1;
	}
	// Error if input file cannot be opened.
	if (!(input = fopen(argv[5], "r")))
	{
		fprintf(stderr, "Could not open trace file: %s\n", argv[5]);
		return -1;
	}
	
	// Set the global variables.
	cache_size = atoi(argv[1]);
	assoc = atoi(argv[2]);
	replace = atoi(argv[3]);
	wb = atoi(argv[4]);
	
	// Calculate the number of sets we have. 
	// Set the global.
	num_sets = cache_size/64;
	num_sets = num_sets/assoc;
	
	// Initialize the cache and the other arrays.
	cache_array = create_cache(num_sets, assoc);
	dirty = create_array(num_sets, assoc);
	lru = create_array(num_sets, assoc);
	fifo = create_array(num_sets, assoc);
	
	// Scan in the accesses one by one.
	// Call the simulation for each access.
	// Update the total amount of accesses. 
	while (1)
	{
		fscanf(input, "%c", &op);
		fscanf(input, "%llx", &address);
		fscanf(input, "\n");
		
		accesses++;
		
		simulate();
		
		if(feof(input))
			break;
	}
	
	// Print the final output.
	print_output();
	
	// Close the input stream.
	fclose(input);
	
	return 0;
}