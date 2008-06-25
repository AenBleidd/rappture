/*
* This module contains functions which will help a user write his own Fitness Function in case of discrete variables
* Have not written the code yet. Need to verify some things first. 
*/
#include<stdio.h>
#include<math.h>
#include<stdlib.h>
#include<time.h>
#include "pgapack.h"
#include "rp_optimizer.h"


/*Idea behind this function: Whenever a user would want to get the value of a certain i/p geneeter
*during the course of a GA, he would use this function. The work of interfacing it to the GA Algo
* is not yet complete.
*/
double get_gene(char* name){
	return 0;
}

/*
 * Return the actual values of all the genes in the currently evaluated chromosome
 */
double* get_all_genes(){
	return 0;
}

/*
 * The way to go about doing this is to fetch  
 */
double weighted_sum_of_variances(double* array_of_current_values, double* array_of_desired_values){
	return 0;
}


double abs_nth_power_distance(double actual_value, double desired_value,double power){
	return pow(fabs(actual_value-desired_value),power);
}

int main(){
	double actual=5,desired=30,power=0.2;
	printf("Actual - %lf, Desired - %lf, (Actual-Desired)^ %lf is %lf\n", actual,desired,power,abs_nth_power_distance(actual,desired,power));
	return 0;
}
