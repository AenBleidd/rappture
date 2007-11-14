/*
 * Ganesh Hegde: GENES Learning
 * This is a sample optimization written for simple parameter values
 * The aim is to fit a given I-V for a MOSFET
 * The variables that are entering into the optimization problem are Vgs - Gate to Source Voltage and W/L-
 * The Width of the oxide layer divided by its length
 * using given Vgs -- 2V and W/L == 10, an I-V is obtained and fet as the function to be optimized
 * 
 */
 
#include <pgapack.h>
#include <stdlib.h>

double evaluationFunction(PGAContext *, int, int);


int main(int argc, char **argv){
	PGAContext *ctx;
	int strLen;
	double low[]={1.5,5} ,high[]={5,12.0};

    ctx = PGACreate(&argc, argv, PGA_DATATYPE_REAL, 2, PGA_MINIMIZE); 
    PGASetRealInitRange(ctx,low,high);
    PGASetPopSize(ctx,1000);
    PGASetCrossoverType(ctx,PGA_CROSSOVER_ONEPT);
    PGASetRandomSeed(ctx, 1);
    PGASetStoppingRuleType(ctx, PGA_STOP_NOCHANGE);
    PGASetUp(ctx);
    PGARun(ctx, evaluationFunction);
    PGADestroy(ctx);
	return 0;
}

/*
 * Sample Evaluation function for evaluating the parameters returned by the PGARun function on one cycle
 * ctx--> context variable
 * index--> chromosome index in the population
 * populationNumber --> Which population to look at
 */

double evaluationFunction(PGAContext *ctx, int index, int populationNumber){
	int i, stringLen;
	double realAllele,vGS=0.0,widthToLengthRatio=0.0;
	double Vds[21]={0}, Ids[21]={0};
	double mu = 300 ;/*In cm^2/V-s*/
	int Vt=1,j=0;
	double tox= 20e-7;
	double Cox= (3.9*8.85e-14)/tox;
	double k,slope,targetslope,fitness=0;
	double targetIds[] = {0,0.0005,0.0010,0.0016,0.0021,0.0026,0.0031,0.0036,0.0041,0.0047,0.0052,0.0057,0.0062,0.0067,0.0072,0.0078,0.0083,0.0088,0.0093,0.0098,0.0104};
	stringLen = PGAGetStringLength(ctx);
	for(i=0;i<stringLen;i++){
		realAllele = PGAGetRealAllele(ctx, index, populationNumber, i);
		if(i==0){
			vGS=realAllele;
		}else{
			widthToLengthRatio = realAllele;
		}
		
	}
	/*
	 * Initing the Vds array
	 */
	k=mu*Cox*widthToLengthRatio;
	
	for(i=0;i<21;i++){
		if(i>0){
		Vds[i]=Vds[i-1]+0.5;
		}else{
			Vds[i]=0;
		}		
		Ids[i]=k*((vGS-Vt)*Vds[i]);
	}
	slope = (Ids[20]-Ids[0])/(Vds[20]-Vds[0]);
	targetslope = (targetIds[20]-targetIds[0])/(Vds[20]-Vds[0]);
	for(j=0;j<21;j++){
//		printf("%lf ",Ids[j]);
	}
	return (double)abs(slope-targetslope);
	
}