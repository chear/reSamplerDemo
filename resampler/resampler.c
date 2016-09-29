#include "resampler.h"

 //DLL void nel_resampler_init(nr_t* s, const uint16_t fs, const uint16_t ofs)
DLL void nel_resampler_init(nr_t* s, const int fs, const int ofs)
{
    memset(s, 0, sizeof(nr_t));
    s->fs = fs;
    s->ofs = ofs;
}


DLL int8_t nel_resampler_add_sample(nr_t* s, const int x,ProgressCallback callback)
{
    int8_t r = 0;
    if (s->fs == s->ofs<<1) {
        s->a2++;
        if (s->a2&1) {
            s->y[0] = x;
            r = 1;
        }
    }
    else if (s->fs == s->ofs) {
        s->y[0] = x;
        r = 1;
    }
    else if (s->fs == s->ofs/2) {
        s->y[0] = (s->x+x)/2;
        s->y[1] = x;
        s->x = x;
        r = 2;
    }
    else {
        while (s->a2 >= s->a1) {
            s->y[r] = x+((int32_t)(s->a2-s->a1)*(s->x-x))/(int32_t)s->ofs;
            if (s->a2 == s->a1) {
                s->a1 = 0;
                s->a2 = 0;
            }
            s->a1 += s->fs;
            r++;
        }
        s->a2 += s->ofs;
        s->x = x;
    }

	if( s->fs > s->ofs )
	{
		/* resample from less to more ,such as 512 to 600 */
		if(r ==1 ){
			callback(s->y[0]);
		}else{
			callback(s->y[0]);
			callback(s->y[1]);
		}
	} else{
		/* resample from more to less ,such as 600 to 512 */
		if(r == 1)
		{
			callback(s->y[0]);
		}
	}

    return r;
}

//DLL void nskECGreSamplerInit(nr_t* ns,const uint16_t fs, const uint16_t ofs)
DLL void nskECGreSamplerInit(const int fs, const int ofs)
{
	/* initial the *nsk ,the deault of the values all are  (int)0 */
    memset(&nsk, 0, sizeof(nr_t));
	nsk.fs = fs;
	nsk.ofs = ofs;
    //nsk.fs = fs;
    //nsk.ofs = ofs;
}


DLL int8_t nskECGreSampler(const int32_t x,ProgressCallback callback)
{	
	int8_t r = 0;
    if (nsk.fs == nsk.ofs<<1) {
        nsk.a2++;
        if (nsk.a2&1) {
            nsk.y[0] = x;
            r = 1;
        }
    }
    else if (nsk.fs == nsk.ofs) {
        nsk.y[0] = x;
        r = 1;
    }
    else if (nsk.fs == nsk.ofs/2) {
        nsk.y[0] = (nsk.x+x)/2;
        nsk.y[1] = x;
        nsk.x = x;
        r = 2;
    }
    else {
        while (nsk.a2 >= nsk.a1) {
            nsk.y[r] = x+((int32_t)(nsk.a2-nsk.a1)*(nsk.x-x))/(int32_t)nsk.ofs;
            if (nsk.a2 == nsk.a1) {
                nsk.a1 = 0;
                nsk.a2 = 0;
            }
            nsk.a1 += nsk.fs;
            r++;
        }
        nsk.a2 += nsk.ofs;
        nsk.x = x;
    }

	
	if( nsk.fs > nsk.ofs )
	{
		/* resample from less to more ,such as 512 to 600 */
		if(r ==1 ){
			callback(nsk.y[0]);
		}else{
			callback(nsk.y[0]);
			callback(nsk.y[1]);
		}
	} else{
		/* resample from more to less ,such as 600 to 512 */
		if(r == 1)
		{
			callback(nsk.y[0]);
		}
	}
    return r;
}

void nskECGMultReSamplerInit(nr_t* s, const uint16_t fs, const uint16_t ofs)
{
    memset(s, 0, sizeof(nr_t));
    s->fs = fs;
    s->ofs = ofs;
}


int8_t nskECGMultReSampler(nr_t* s, const int32_t x)
{
    int8_t r = 0;
    if (s->fs == s->ofs<<1) {
        s->a2++;
        if (s->a2&1) {
            s->y[0] = x;
            r = 1;
        }
    }
    else if (s->fs == s->ofs) {
        s->y[0] = x;
        r = 1;
    }
    else if (s->fs == s->ofs/2) {
        s->y[0] = (s->x+x)/2;
        s->y[1] = x;
        s->x = x;
        r = 2;
    }
    else {
        while (s->a2 >= s->a1) {
            s->y[r] = x+((int32_t)(s->a2-s->a1)*(s->x-x))/(int32_t)s->ofs;
            if (s->a2 == s->a1) {
                s->a1 = 0;
                s->a2 = 0;
            }
            s->a1 += s->fs;
            r++;
        }
        s->a2 += s->ofs;
        s->x = x;
    }
    return r;
}



/* Begin Test Func  */
/* Test 1: */
DLL int GetValue(char* opt, struct Parameter params)
{
	return 300;
	//print("param1: %s, param2: %s, string: %s", params.param1, params.param2, opt);
}

DLL void NAG_CALL TryComplex(Complex inputVar, Complex *outputVar, 
                                              int n, Complex array[])
{
  outputVar->re = ++inputVar.re;
  outputVar->im = ++inputVar.im;

  array[0].re = 99.0;
  array[0].im = 98.0;
  array[1].re = 97.0;
  array[1].im = 96.0;  
}

/* Test 2: */
DLL void DoWork(TestProgressCallback progressCallback)
{
    int counter = 0;
    for(; counter<=2; counter++)
    {
        // do the work...
        //if (progressCallback)
        //{
            // send progress update
            progressCallback(counter);
        //}
    }
}

/* End Test Func  */