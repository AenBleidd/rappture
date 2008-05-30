/**********fitnessfunctions.c************************************************************************************
fitfunc.c is a program that contains modules for enabling the user to write his own fitness functions
//TODO: Unresolved Issues so far:
//1)Inclusion of extreme points on the curve
//2)Inclusion of consecutive zero-gradient points in the list of maximas minimas
//3)Regarding mathematically non differentiable functions, like triangular functions, etc.
********************************************************************************************************/

#include<stdio.h>
#include<math.h>
#include<stdlib.h>
#include<time.h>

#define EPSILON 0.005
#define TRUE 1
#define FALSE 0
#define INCREMENT 0.01
#define PI 3.14
#define NO_OF_POINTS 1500
#define INVALID_SLOPE 99999.9999
#define TOLERANCE 1e-5

typedef struct Curve{
	int curveID;
	double *xvals;
	double *yvals;
	int number_of_points;
}Curve;

/*
 * Creating a struct for Maxima and Minima Lists
 * Can be used as an argument for functions that give both local and global max/min
 * The responsibility for initially allocating and finally freeing memory to *xval and *yval arrays
 * lies with the calling function. Reallocation of memory for these arrays and updating size_of_list
 * will happen inside called functions.
 */
 
typedef struct MaxOrMinList{
	double *yvals;
	double *xvals;
	int no_of_actual_max_or_mins;	
}MaxOrMinList;


double X_Axis_Values[NO_OF_POINTS], Y_Axis_Values[NO_OF_POINTS];
double slope_calc(int index);
void initdisplay();
 
void InitializeCurves(int ID){
	int index;
//	double init;
	switch(ID){
		case 1:// Y = SIN(X);
			for(index=0;index<NO_OF_POINTS;index++){
				X_Axis_Values[index] =(double)(index*INCREMENT);
				Y_Axis_Values[index] = sin(X_Axis_Values[index]);
			}
			break;
		case 2://Y = EXP(-X);
			for(index=0;index<NO_OF_POINTS;index++){
				X_Axis_Values[index] =(double)(index*INCREMENT);
				Y_Axis_Values[index] = exp(-X_Axis_Values[index]);
			}
			break;
		case 3://Y = EXP(-x)*SINC(X);
//			init = -5;
			for(index=0;index<NO_OF_POINTS;index++){
//				init+=INCREMENT;
//				X_Axis_Values[index] = init;
				X_Axis_Values[index] =(double)(index*INCREMENT);
				Y_Axis_Values[index] = exp(-X_Axis_Values[index])*sin(10*X_Axis_Values[index]); 		
			}
			break;
		default:
		printf("\nError: Invalid Value Entered\n");
		exit(0);
	}
}


double y_at_x(int curveID, float xval){
	return 0;
}

double y_at_x_temp(double xval){
	int i;
	for(i=0;i<NO_OF_POINTS;i++){
		if(fabs(xval-X_Axis_Values[i]<=EPSILON)){
			return Y_Axis_Values[i];
		}
	}
	return 0;
}

double slope_at_x(int curveID, double xval){
	return 0;
}

double slope_at_x_temp(double xval){
	int i;
	double slope_val = INVALID_SLOPE; 
	for(i=0;i<NO_OF_POINTS;i++){
                if(fabs(xval-X_Axis_Values[i]<=EPSILON)){
					slope_val = slope_calc(i);
					break;
                }
        }
	return slope_val;
}

int max_at_x(int curveID, double xval){
	return FALSE;
}

/*
*have made an implicit assumption here
*only the first point of a series of zero gradient points will be a minima/maxima point
*need some clarifications on whether this is acceptable.This will be important w.r.t square wave like curves, etc.
*/


int max_at_x_temp(double xval){
	double slope_at_prev_point = INVALID_SLOPE, slope_at_point = INVALID_SLOPE;
    slope_at_prev_point = slope_at_x_temp(xval-INCREMENT);
    slope_at_point =  slope_at_x_temp(xval);
	if(slope_at_prev_point!=INVALID_SLOPE && slope_at_point!=INVALID_SLOPE){
	    if(slope_at_prev_point > 0 && slope_at_point <= 0){
	        return TRUE;
	    }else{
	        return FALSE;
	    }
	}
	printf("Error: The point is either outside the range of values for the curve or is an extreme point");
    return FALSE;
}

int min_at_x(int curveID, double xval){
	return FALSE;
}

int min_at_x_temp(double xval){
	double slope_at_prev_point = INVALID_SLOPE, slope_at_point = INVALID_SLOPE;
    slope_at_prev_point= slope_at_x_temp(xval-INCREMENT);
    slope_at_point = slope_at_x_temp(xval);
    if(slope_at_prev_point!=INVALID_SLOPE && slope_at_point!=INVALID_SLOPE){
		if(slope_at_prev_point < 0 && slope_at_point >= 0){
			return TRUE;
		}else{
			return FALSE;
		}
    }
	printf("Error: The point is either outside the range of values for the curve or is an extreme point");
	return FALSE;
}

double** local_maxima_list(int curveID){
	return NULL;
}

/*
*inputs required regarding the inclusion of extreme points on the curve
*
*/

double slope_calc(int i){
	double slope = INVALID_SLOPE;
	if(i>=0 && i<NO_OF_POINTS){	
		if(i!=NO_OF_POINTS-1){
			slope = ((Y_Axis_Values[i+1]-Y_Axis_Values[i])/(X_Axis_Values[i+1]-X_Axis_Values[i]));
		}else{
			slope = ((Y_Axis_Values[i]-Y_Axis_Values[i-1])/(X_Axis_Values[i]-X_Axis_Values[i-1]));
		}
	}
	return slope;
}

/*
 *Let Allocation be handled by calling function 
 *This function only reallocates if necessary 
 */
MaxOrMinList* get_local_max_list(MaxOrMinList* buffer){
	int no_of_actual_max = 0,i;
	double slope_at_prev_point = INVALID_SLOPE,slope_at_point = INVALID_SLOPE;
	if(buffer!=NULL){
		if(buffer->xvals!=NULL && buffer->yvals!=NULL){
			for(i=0;i<NO_OF_POINTS;i++){
				slope_at_prev_point = slope_calc(i-1);
				slope_at_point = slope_calc(i);
				if(slope_at_point != INVALID_SLOPE && slope_at_prev_point != INVALID_SLOPE){
					if(slope_at_prev_point>0 && slope_at_point<=0){
						++no_of_actual_max;
						if(no_of_actual_max>1){
							buffer->xvals = realloc(buffer->xvals,(sizeof(double))*(no_of_actual_max));
							buffer->yvals = realloc(buffer->yvals,(sizeof(double))*(no_of_actual_max));
						}
						buffer->yvals[no_of_actual_max-1]=Y_Axis_Values[i];
						buffer->xvals[no_of_actual_max-1]=X_Axis_Values[i];
					}				
				}
			}
			buffer->no_of_actual_max_or_mins = no_of_actual_max;
		}else{
			printf("\nError: XValue And Yvalue Array Not Initialized");
			return NULL; 
		}
	}
	return buffer;
}

/*
 *Let Allocation be handled by calling function 
 *This function only reallocates if necessary 
 */
MaxOrMinList* get_local_min_list(MaxOrMinList* buffer){
	int no_of_actual_min = 0,i;
	double slope_at_prev_point = INVALID_SLOPE,slope_at_point = INVALID_SLOPE;
	if(buffer!=NULL){
		if(buffer->xvals!=NULL && buffer->yvals!=NULL){
			for(i=0;i<NO_OF_POINTS;i++){
				slope_at_prev_point = slope_calc(i-1);
				slope_at_point = slope_calc(i);
				if(slope_at_point != INVALID_SLOPE && slope_at_prev_point != INVALID_SLOPE){
					if(slope_at_prev_point<0 && slope_at_point>=0){
						++no_of_actual_min;
						if(no_of_actual_min>1){
							buffer->xvals = realloc(buffer->xvals,(sizeof(double))*(no_of_actual_min));
							buffer->yvals = realloc(buffer->yvals,(sizeof(double))*(no_of_actual_min));
						}
						buffer->yvals[no_of_actual_min-1]=Y_Axis_Values[i];
						buffer->xvals[no_of_actual_min-1]=X_Axis_Values[i];
					}				
				}
			}
			buffer->no_of_actual_max_or_mins = no_of_actual_min;
		}else{
			printf("\nError: XValue And Yvalue Array Not Initialized");
			return NULL; 
		}
	}
	return buffer;
}


MaxOrMinList* get_global_max_list(MaxOrMinList* buffer){
	MaxOrMinList *local_max_list,local_max_list_buffer;
	int i,no_of_actual_global_max;
	double temp_glob_max;
	if(buffer!=NULL){
		local_max_list_buffer.xvals = malloc(sizeof(double));
		local_max_list_buffer.yvals = malloc(sizeof(double));
		if(local_max_list_buffer.xvals!=NULL && local_max_list_buffer.yvals!=NULL){
			local_max_list = get_local_max_list(&local_max_list_buffer);
			if(local_max_list != NULL){
				if(local_max_list->no_of_actual_max_or_mins > 0){
						temp_glob_max = local_max_list->yvals[0];
						buffer->yvals[0] = temp_glob_max;
						buffer->xvals[0] = local_max_list->xvals[0];
						buffer->no_of_actual_max_or_mins = 1;
						no_of_actual_global_max=1;
					if(local_max_list->no_of_actual_max_or_mins == 1){
						return buffer;
					}
					for(i=1;i<local_max_list->no_of_actual_max_or_mins;i++){
						if(fabs(temp_glob_max-local_max_list->yvals[i])<TOLERANCE){
							++no_of_actual_global_max;
						}else{
							if(temp_glob_max < local_max_list->yvals[i]){
								no_of_actual_global_max =1;
								temp_glob_max = local_max_list->yvals[i];
							}else{
								continue;
							}
						}
						if(no_of_actual_global_max>1){
							//Reallocate space if necessary...
							buffer->yvals = realloc(buffer->yvals,no_of_actual_global_max*sizeof(double));
							buffer->xvals = realloc(buffer->xvals,no_of_actual_global_max*sizeof(double));
							if(buffer->xvals == NULL || buffer->yvals == NULL){
								printf("\nError: Could not Reallocate space for the Global Maxima List.\n");
								return NULL;
							}
						}
						buffer->yvals[no_of_actual_global_max-1] = temp_glob_max;
						buffer->xvals[no_of_actual_global_max-1] = local_max_list->xvals[i];
						buffer->no_of_actual_max_or_mins = no_of_actual_global_max;
					}
				}else{
					printf("\nThere are no Local Maxima, consequently a Global Maxima List cannot be generated\n");
					buffer->no_of_actual_max_or_mins = 0;
				}
			}else{
				printf("\nError: Could not obtain Local Maxima List\n");
				free(local_max_list_buffer.xvals);
				free(local_max_list_buffer.yvals);
				return NULL;
			}	
			free(local_max_list_buffer.xvals);
			free(local_max_list_buffer.yvals);
		}else{
			printf("\nError: Could not allocate space for the list\n");
			return NULL;
		}
	}else{
		printf("\nError: Uninitialized Buffer\n");
	}
	return buffer;
}


MaxOrMinList* get_global_min_list(MaxOrMinList* buffer){
	MaxOrMinList *local_min_list,local_min_list_buffer;
	int i,no_of_actual_global_min;
	double temp_glob_min;
	if(buffer!=NULL){
		local_min_list_buffer.xvals = malloc(sizeof(double));
		local_min_list_buffer.yvals = malloc(sizeof(double));
		if(local_min_list_buffer.xvals!=NULL && local_min_list_buffer.yvals!=NULL){
			local_min_list = get_local_min_list(&local_min_list_buffer);
			if(local_min_list != NULL){
				if(local_min_list->no_of_actual_max_or_mins > 0){
						temp_glob_min = local_min_list->yvals[0];
						buffer->yvals[0] = temp_glob_min;
						buffer->xvals[0] = local_min_list->xvals[0];
						buffer->no_of_actual_max_or_mins = 1;
						no_of_actual_global_min=1;
					if(local_min_list->no_of_actual_max_or_mins == 1){
						return buffer;
					}
					for(i=1;i<local_min_list->no_of_actual_max_or_mins;i++){
						if(fabs(temp_glob_min-local_min_list->yvals[i])<TOLERANCE){
							++no_of_actual_global_min;
						}else{
							if(temp_glob_min > local_min_list->yvals[i]){
								no_of_actual_global_min =1;
								temp_glob_min = local_min_list->yvals[i];
							}else{
								continue;
							}
						}
						if(no_of_actual_global_min>1){
							//Reallocate space if necessary...
							buffer->yvals = realloc(buffer->yvals,no_of_actual_global_min*sizeof(double));
							buffer->xvals = realloc(buffer->xvals,no_of_actual_global_min*sizeof(double));
							if(buffer->xvals == NULL || buffer->yvals == NULL){
								printf("\nError: Could not Reallocate space for the Global Minima List.\n");
								return NULL;
							}
						}
						buffer->yvals[no_of_actual_global_min-1] = temp_glob_min;
						buffer->xvals[no_of_actual_global_min-1] = local_min_list->xvals[i];
						buffer->no_of_actual_max_or_mins = no_of_actual_global_min;
					}
				}else{
					printf("\nThere are no Local Minima, consequently a Global Minima List cannot be generated\n");
					buffer->no_of_actual_max_or_mins = 0;
				}
			}else{
				printf("\nError: Could not obtain Local Minima List\n");
				free(local_min_list_buffer.xvals);
				free(local_min_list_buffer.yvals);
				return NULL;
			}	
			free(local_min_list_buffer.xvals);
			free(local_min_list_buffer.yvals);
		}else{
			printf("\nError: Could not allocate space for the list\n");
			return NULL;
		}
	}else{
		printf("\nError: Uninitialized Buffer\n");
	}
	return buffer;
}

MaxOrMinList* get_max_list_bet_xvals(MaxOrMinList* buffer, double xval1, double xval2){
	int i,no_of_actual_max=0,start_index=0,end_index=0;
	double slope_at_prev_point = INVALID_SLOPE,slope_at_point = INVALID_SLOPE;
	if(fabs(xval1-X_Axis_Values[0])<EPSILON && fabs(xval2-X_Axis_Values[NO_OF_POINTS-1])<EPSILON){
		return get_local_max_list(buffer);
	}
	if(buffer!=NULL){
		if(buffer->xvals!=NULL && buffer->yvals!=NULL){
			for(i=0;i<NO_OF_POINTS;i++){
				if (fabs(X_Axis_Values[i]-xval1)<EPSILON){
					start_index = i;
				}
				if(fabs(X_Axis_Values[i]-xval2)<EPSILON){
					end_index = i;
				}
			}
			if(start_index == 0 || end_index == 0){
				printf("\nError: Invalid XValues Provided\n");
				return NULL;
			}
			for(i=start_index;i<=end_index;i++){
				slope_at_prev_point = slope_calc(i-1);
				slope_at_point = slope_calc(i);
				if(slope_at_point != INVALID_SLOPE && slope_at_prev_point != INVALID_SLOPE){
					if(slope_at_prev_point>0 && slope_at_point<=0){
						++no_of_actual_max;
						if(no_of_actual_max>1){
							buffer->xvals = realloc(buffer->xvals,(sizeof(double))*(no_of_actual_max));
							buffer->yvals = realloc(buffer->yvals,(sizeof(double))*(no_of_actual_max));
						}
						buffer->yvals[no_of_actual_max-1]=Y_Axis_Values[i];
						buffer->xvals[no_of_actual_max-1]=X_Axis_Values[i];
					}				
				}
				
			}
			buffer->no_of_actual_max_or_mins = no_of_actual_max;
		}else{
			printf("\nError: XValue And Yvalue Array Not Initialized");
			return NULL; 
		}
	}else{
		printf("\nError: Buffer passed to function get_max_list_bet_xvals() is NULL\n");
	}
	return buffer;	
}

MaxOrMinList* get_min_list_bet_xvals(MaxOrMinList* buffer, double xval1, double xval2){
	int i,no_of_actual_min=0,start_index=0,end_index=0;
	double slope_at_prev_point = INVALID_SLOPE,slope_at_point = INVALID_SLOPE;
	if(fabs(xval1-X_Axis_Values[0])<EPSILON && fabs(xval2-X_Axis_Values[NO_OF_POINTS-1])<EPSILON){
		return get_local_min_list(buffer);
	}
	if(buffer!=NULL){
		if(buffer->xvals!=NULL && buffer->yvals!=NULL){
			for(i=0;i<NO_OF_POINTS;i++){
				if (fabs(X_Axis_Values[i]-xval1)<EPSILON){
					start_index = i;
				}
				if(fabs(X_Axis_Values[i]-xval2)<EPSILON){
					end_index = i;
				}
			}
			if(start_index == 0 || end_index == 0){
				printf("\nError: Invalid XValues Provided\n");
				return NULL;
			}
			for(i=start_index;i<=end_index;i++){
				slope_at_prev_point = slope_calc(i-1);
				slope_at_point = slope_calc(i);
				if(slope_at_point != INVALID_SLOPE && slope_at_prev_point != INVALID_SLOPE){
					if(slope_at_prev_point<0 && slope_at_point>=0){
						++no_of_actual_min;
						if(no_of_actual_min>1){
							buffer->xvals = realloc(buffer->xvals,(sizeof(double))*(no_of_actual_min));
							buffer->yvals = realloc(buffer->yvals,(sizeof(double))*(no_of_actual_min));
						}
						buffer->yvals[no_of_actual_min-1]=Y_Axis_Values[i];
						buffer->xvals[no_of_actual_min-1]=X_Axis_Values[i];
					}				
				}
				
			}
			buffer->no_of_actual_max_or_mins = no_of_actual_min;
		}else{
			printf("\nError: XValue And Yvalue Array Not Initialized");
			return NULL; 
		}
	}else{
		printf("\nError: Buffer passed to function get_min_list_bet_xvals() is NULL\n");
	}
	return buffer;	
}


//TODO Myself: Delete This function, it has been written only for testing

void display_max_min_lists(){
	MaxOrMinList list,*buffer;
	int i; 
	double xval1,xval2;
	list.xvals = malloc(sizeof(double));
	list.yvals = malloc(sizeof(double));
	if(list.xvals!=NULL && list.yvals!=NULL){
		buffer = get_local_max_list(&list);
		if(buffer!=NULL){
			printf("\nDisplaying Local Maxima List........\n");
			sleep(1);
			if(list.no_of_actual_max_or_mins == 0){
				printf("The function has ZERO local maximas");
			}
			for(i=0;i<list.no_of_actual_max_or_mins;i++){
				printf("\nLocal Maxima Number %d = %lf Corresponding to Xval of %lf",i+1,buffer->yvals[i],buffer->xvals[i]);
			}
			printf("\n\n\n\n");
		}else{
			printf("\nLocal Max List Returned Null\n");
		}
		sleep(1);
		buffer = get_local_min_list(buffer);
		if(buffer!=NULL){
			printf("\nDisplaying Local Minima List........\n");
			if(buffer->no_of_actual_max_or_mins == 0){
				printf("The function has ZERO local maximas");
			}
			for(i=0;i<buffer->no_of_actual_max_or_mins;i++){
				printf("\nLocal Minima Number %d = %lf Corresponding to Xval of %lf",i+1,buffer->yvals[i],buffer->xvals[i]);
			}
			printf("\n\n\n\n");
		}else{
			printf("\nLocal Minima List returned Null\n");
		}
		sleep(1);
		buffer = get_global_max_list(buffer);
		if(buffer!=NULL){
			printf("\nDisplaying Global Maxima List...");
			if(buffer->no_of_actual_max_or_mins == 0){
				printf("The function has ZERO global maximas");
			}
			for(i=0;i<buffer->no_of_actual_max_or_mins;i++){
				printf("\nGlobal Maxima Number %d = %lf Corresponding to Xval of %lf",i+1,buffer->yvals[i],buffer->xvals[i]);
			}
			printf("\n\n\n\n");
		}else{
			printf("\nGlobal Maxima List returned Null\n");
		}
		sleep(1);
		buffer = get_global_min_list(buffer);
		if(buffer!=NULL){
			printf("\nDisplaying Global Minima List...\n");
			if(buffer->no_of_actual_max_or_mins == 0){
				printf("The function has ZERO global minimas");
			}
			for(i=0;i<buffer->no_of_actual_max_or_mins;i++){
				printf("\nGlobal Minima Number %d = %lf Corresponding to Xval of %lf",i+1,buffer->yvals[i],buffer->xvals[i]);
			}
			printf("\n\n\n\n");
		}else{
			printf("\nGlobal Minima List returned Null\n");
		}
		sleep(2);
		printf("\nTo find Maxima between two Xvalues....\n");
		printf("Enter the first X Value\n");
		scanf("%lf",&xval1);
		printf("\nEnter the Second X Value\n");
		scanf("%lf",&xval2);
		buffer = get_max_list_bet_xvals(buffer,xval1,xval2);
		if(buffer!=NULL){
			printf("\n\n\n\nDisplaying List of Maxima Between %lf and %lf...",xval1,xval2);
			if(buffer->no_of_actual_max_or_mins == 0){
				printf("The function has ZERO maxima between the entered XValues");
			}
			for(i=0;i<buffer->no_of_actual_max_or_mins;i++){
				printf("\nMaxima Number %d = %lf Corresponding to Xval of %lf",i+1,buffer->yvals[i],buffer->xvals[i]);
			}
			printf("\n\n\n\n");
		}else{
			printf("\nMaxima List returned Null\n");
		}
		sleep(2);
		printf("\nTo find Minima between two Xvalues....\n");
		printf("Enter the first X Value\n");
		scanf("%lf",&xval1);
		printf("\nEnter the Second X Value\n");
		scanf("%lf",&xval2);
		buffer = get_min_list_bet_xvals(buffer,xval1,xval2);
		if(buffer!=NULL){
			printf("\n\n\n\nDisplaying list of Minima between %lf and %lf...",xval1,xval2);
			if(buffer->no_of_actual_max_or_mins == 0){
				printf("The function has ZERO minima between the entered XValues");
			}
			for(i=0;i<buffer->no_of_actual_max_or_mins;i++){
				printf("\nMinima Number %d = %lf Corresponding to Xval of %lf",i+1,buffer->yvals[i],buffer->xvals[i]);
			}
			printf("\n\n\n\n");
		}else{
			printf("\nMinima List returned Null\n");
		}
		free(list.xvals);
		free(list.yvals);
	}
}

int main(){
	initdisplay();	
	return 0;
}

//Display related Jazz, totally unnecessary once actual interfacing to API is done TODO: Delete
void initdisplay(){
	int choice;
	double xval;
	printf("\n\n\n\n\n\n\n\n\n\n");
	printf("********************************************************");
	printf("PROGRAM FOR FITNESS MODULES");
	printf("********************************************************\n");
	printf("\n\n\n\n\n\n");
	sleep(1);
	printf("This PROGRAM illustrates some of the modules written to enable a user to write his own fitness function\n");
	sleep(1);
	printf("\nThree types of functions are available for testing\n");
	sleep(1);
	printf("(1)  Sine Function (y = sinx)\n(2)  Exponential Decay Function (y = exp(-x))\n(3)  Rapidly Exponentially decaying Sine Function (y = exp(-x)*sin(10x))\n");
	sleep(1);
	printf("\nEnter Your Choice\n");
	scanf("%d",&choice);
	InitializeCurves(choice);
	sleep(1);
	printf("\n\nEnter a point BETWEEN %lf and %lf at which you would like slope to be found.....\n",X_Axis_Values[0],X_Axis_Values[NO_OF_POINTS-1]);
	scanf("%lf",&xval);
	sleep(1);
	printf("Slope at  %lf is %lf\n", xval, slope_at_x_temp(xval));
	sleep(1);
	if(max_at_x_temp(xval)){
		printf("There is a maxima at given XVal\n");
	}else if(min_at_x_temp(xval)){
		printf("There is a minima at given Xval\n");
	}else{
		printf("There is neither a maxima nor a minima at given XVal\n");
	}
	display_max_min_lists();
	
}
