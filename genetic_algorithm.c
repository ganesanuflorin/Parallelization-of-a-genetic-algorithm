#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "genetic_algorithm.h"
#include "thread_str.h"
#include <math.h>

#define min(a,b)  (((a)<(b))?(a):(b))

int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int argc, char *argv[])
{
	FILE *fp;

	if (argc < 3) {
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
	
	if (*generations_count == 0) {
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function(const sack_object *objects, individual *generation, int object_count, int sack_capacity, threadStr thread)
{	
	int start,end;
	int profit;
	int weight;

	start = thread.id * ceil((double)thread.object_count/thread.P);
	end = min(thread.object_count,(thread.id + 1) * ceil((double)thread.object_count/thread.P));
	pthread_barrier_wait(thread.barrier);
	
	for (int i = start; i < end; ++i) {
		
		weight = 0;
		profit = 0;
		
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				
				weight += objects[j].weight;
				profit += objects[j].profit;
				
			}
		}
	
		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

void mutate_bit_string_1(individual *ind, int generation_index, threadStr thread)
{	
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);
	
	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;

		int start = thread.id * ceil((double)mutation_size/thread.P);
		int end = min(mutation_size,(thread.id + 1) * ceil((double)mutation_size/thread.P));

		for (i = start; i < end; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
			ind->chrom_count = (ind->chromosomes[i] == 0) ? 
					(ind->chrom_count - 1) : (ind->chrom_count + 1);
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;

		int start = thread.id * ceil((double)(ind->chromosome_length - mutation_size)/thread.P);
		int end = min(mutation_size,(thread.id + 1) * ceil((double)mutation_size/thread.P));

		for (i = start; i < end; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
			ind->chrom_count = (ind->chromosomes[i] == 0) ? 
					(ind->chrom_count - 1) : (ind->chrom_count + 1);
		}
	}
}

void mutate_bit_string_2(individual *ind, int generation_index, threadStr thread)
{	
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step

	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
		ind->chrom_count = (ind->chromosomes[i] == 0) ? 
					(ind->chrom_count - 1) : (ind->chrom_count + 1);
	}
}

void crossover(individual *parent1, individual *child1, int generation_index, threadStr thread)
{	
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	int* curr = child1->chromosomes;

	for (;curr != &child1->chromosomes[child1->chromosome_length - 1]; curr++) {
		child1->chrom_count += *curr;
	}
	child1->chrom_count += child1->chromosomes[child1->chromosome_length - 1];

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	curr = child2->chromosomes;
	
	for (;curr != &child2->chromosomes[child2->chromosome_length - 1];curr++) {
		child2->chrom_count += *curr;
	}
	child2->chrom_count += child2->chromosomes[child2->chromosome_length - 1];
}

void copy_individual(const individual *from, const individual *to, threadStr thread)
{	
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

int cmp(const void *a, const void *b) {
	individual* indiv1 = (individual *) a;
	individual* indiv2 = (individual *) b;

	int res = indiv2->fitness - indiv1->fitness;

	if (res == 0) {
		res = indiv1->chrom_count - indiv2->chrom_count; // increasing by number of objects in the sack

		if (res == 0) {
			return indiv1->index - indiv2->index;
		}
	}

	return res;
}

void* run_genetic_algorithm(void *arg)
{	
	
	threadStr thread = *(threadStr *) arg;
	
	pthread_barrier_wait(thread.barrier);
	int count, cursor;
	int start,end;
	start = thread.id * ceil((double)thread.object_count/thread.P);
	end = min(thread.object_count,(thread.id + 1) * ceil((double)thread.object_count/thread.P));
	individual *tmp = NULL;
	
	// set initial generation (composed of object_count individuals with a single item in the sack)
	
	for (int i = start; i < end; ++i) {
		
		thread.current_generation[i].fitness = 0;
		thread.current_generation[i].chromosomes = (int*) calloc(thread.object_count, sizeof(int));
		thread.current_generation[i].chromosomes[i] = 1;
		thread.current_generation[i].index = i;
		thread.current_generation[i].chromosome_length = thread.object_count;
		thread.current_generation[i].chrom_count = 1;

		thread.next_generation[i].fitness = 0;
		thread.next_generation[i].chromosomes = (int*) calloc(thread.object_count, sizeof(int));
		thread.next_generation[i].index = i;
		thread.next_generation[i].chromosome_length = thread.object_count;
		thread.current_generation[i].chrom_count = 1;
	}
	
	// iterate for each generation
	for (int k = 0; k < thread.generations_count; ++k) {
		cursor = 0;	
		
		// compute fitness and sort by it
		pthread_barrier_wait(thread.barrier);
		compute_fitness_function(thread.objects, thread.current_generation, end - start, thread.sack_capacity, thread);
		pthread_barrier_wait(thread.barrier);


		int start = thread.id * (double)thread.object_count / thread.P;
		int end = min((thread.id + 1) * (double)thread.object_count / thread.P, thread.object_count);


		pthread_barrier_wait(thread.barrier);
		if (thread.id == 0) {
			qsort(thread.current_generation, thread.object_count, sizeof(individual), cmp);
		}
		pthread_barrier_wait(thread.barrier);

		// keep first 30% children (elite children selection)
		count = thread.object_count * 3 / 10;
		start = thread.id * ceil((double)count/thread.P);
		end = min(count,(thread.id + 1) * ceil((double)count/thread.P));
		
		pthread_barrier_wait(thread.barrier);
		for (int i = start; i < end; ++i) {
			copy_individual(thread.current_generation + i, thread.next_generation + i,thread);
		}
		
		cursor = count;
		
		// mutate first 20% children with the first version of bit string mutation
		count = thread.object_count * 2 / 10;
		start = thread.id * ceil((double)count/thread.P);
		end = min(count,(thread.id + 1) * ceil((double)count/thread.P));
		
		
		pthread_barrier_wait(thread.barrier);
		for (int i = start; i < end; ++i) {
			copy_individual(thread.current_generation + i, thread.next_generation + cursor + i, thread);
			mutate_bit_string_1(thread.next_generation + cursor + i, k, thread);
		}
		cursor += count;
		
		// mutate thread.next 20% children with the second version of bit string mutation
		count = thread.object_count * 2 / 10;
		start = thread.id * ceil((double)count/thread.P);
		end = min(count,(thread.id + 1) * ceil((double)count/thread.P));
		
		for (int i = start; i < end; ++i) {
			copy_individual(thread.current_generation + i + count, thread.next_generation + cursor + i, thread);
			mutate_bit_string_2(thread.next_generation + cursor + i, k, thread);

		}

		pthread_barrier_wait(thread.barrier);
		cursor += count;
		
		// crossover first 30% parents with one-point crossover
		// (if there is an odd number of parents, the last one is kept as such)
		count = thread.object_count * 3 / 10;
		
		if (count % 2 == 1) {
			
			copy_individual(thread.current_generation + thread.object_count - 1, thread.next_generation + cursor + count - 1, thread);
			count--;
		}
		
		start = thread.id * ceil((double)count/thread.P);
		end = min(count,(thread.id + 1) * ceil((double)count/thread.P));
		
		if (start % 2 == 1) {
			start++;
		}

		for (int i = start; i < end; i += 2) {
			
			crossover(thread.current_generation + i, thread.next_generation + cursor + i, k, thread);
		}
		pthread_barrier_wait(thread.barrier);
		
		// switch to new generation
		tmp = thread.current_generation;
		thread.current_generation = thread.next_generation;
		thread.next_generation = tmp;

		start = thread.id * ceil((double)thread.object_count/thread.P);
		end = min(thread.object_count,(thread.id + 1) * ceil((double)thread.object_count/thread.P));
		
		for (int i = start; i < end; ++i) {
			
			thread.current_generation[i].index = i;
			
		}
		pthread_barrier_wait(thread.barrier);
		
		if (k % 5 == 0) {
			if(thread.id == 0) {
				print_best_fitness(thread.current_generation);
			} 
		}
		
	}
	pthread_barrier_wait(thread.barrier);
	compute_fitness_function(thread.objects, thread.current_generation, thread.object_count, thread.sack_capacity, thread);
	pthread_barrier_wait(thread.barrier);
	

	start = thread.id * (double)thread.object_count / thread.P;
	end = min((thread.id + 1) * (double)thread.object_count / thread.P, thread.object_count);


	pthread_barrier_wait(thread.barrier);
	if (thread.id == 0) {
		qsort(thread.current_generation, thread.object_count, sizeof(individual), cmp);
	}
	pthread_barrier_wait(thread.barrier);

	if(thread.id == 0) {
		print_best_fitness(thread.current_generation);
	}
	
	pthread_exit(NULL);
}
