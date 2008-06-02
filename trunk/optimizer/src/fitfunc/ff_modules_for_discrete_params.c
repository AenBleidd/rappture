/*
* This module contains functions which will help a user write his own Fitness Function in case of discrete variables
* Have not written the code yet. Need to verify some things first. 
*/
#include<stdio.h>
#include<math.h>
#include<stdlib.h>
#include<time.h>

typedef struct Gene{
	int geneID;
	double weight;
	double value;
	double rangemin;
	double rangemax;
}Gene;


/*Idea behind this function: Whenever a user would want to get the value of a certain i/p geneeter
*during the course of a GA, he would use this function. The work of interfacing it to the GA Algo
* is not yet complete.
*/
double get_gene(int geneID){
	return 0;
}

/*
 * Return the actual values of all the genes in the current string
 */
double* get_all_genes(){
	return 0;
}

/*
 * Return the gene value raised to a power
 */
double gene_value_raised_to_power(int geneID){
	return 0;
}

/*
 * Return the absolute value of a geneeter.
 */
 
double gene_abs_value(int geneID){
 	return 0;
}
 
double weighted_sum_of_genes(){
	return 0;
}

/*
 * I need to verify this yet. 
 */
double weighted_sum_of_nth_power_distances(){
	return 0;
}

int main(){
}
