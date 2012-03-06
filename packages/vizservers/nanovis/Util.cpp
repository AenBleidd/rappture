/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <Vector3.h>

void evaluate(float fraction, float* key, int count, Vector3* keyValue, Vector3* ret)
{
    int limit = (int) count - 1;
    if (fraction <= key[0]) 
	{
		*ret = keyValue[0];
	}
    else if (fraction >= key[limit]) 
	{
		*ret = keyValue[limit];
	}
	else 
	{
		int n;
        for (n = 0;n < limit; n++){
                if (fraction >= key[n] && fraction < key[n+1]) break;
        }

        if (n >= limit){
			*ret = keyValue[limit];
        }
        else {
            float inter = (fraction - key[n]) / (key[n + 1] - key[n]);
            ret->set(
				inter * (keyValue[n + 1].x - keyValue[n].x) + keyValue[n].x,
				inter * (keyValue[n + 1].y - keyValue[n].y) + keyValue[n].y,
				inter * (keyValue[n + 1].z - keyValue[n].z) + keyValue[n].z);
        }
    }
}

void evaluate(float fraction, float* key, int count, float* keyValue, float* ret)
{
    int limit = (int) count - 1;
    if (fraction <= key[0]) 
	{
		*ret = keyValue[0];
	}
    else if (fraction >= key[limit]) 
	{
		*ret = keyValue[limit];
	}
	else 
	{
		int n;
        for (n = 0;n < limit; n++){
                if (fraction >= key[n] && fraction < key[n+1]) break;
        }

        if (n >= limit){
			*ret = keyValue[limit];
        }
        else {
            float inter = (fraction - key[n]) / (key[n + 1] - key[n]);
            *ret = inter * (keyValue[n + 1] - keyValue[n]) + keyValue[n];
        }
    }
}
