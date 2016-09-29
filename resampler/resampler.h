/**
 * @file resampler.h
 * @brief The ECG resampling using linear interpolation.
 */
/*
 * Raw ECG resampler.
 * Copyright (c) NeuroSky Inc. All rights reserved.
 */
#define NAG_CALL __stdcall 
#include <string.h>
#include "param.h"
#include <stdio.h>
#include "nsk_defines.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define DLL __declspec(dllexport)

typedef void (__stdcall *ProgressCallback)(int);

/** The resampler state structure. */
typedef struct nr_s {
    int16_t y[2];
    int16_t x;
    uint16_t fs;
    uint16_t ofs;
    uint32_t a1;
    uint32_t a2;
} nr_t;


/**
 * Initialize the resampler.
 * @param[in, t] s the state structure.
 * @param[in] fs the sampling rate of input data.
 * @param[in] ofs the sampling rate of output data.
 */
//DLL void nel_resampler_init(nr_t* s, const uint16_t fs, const uint16_t ofs);
DLL void nel_resampler_init(nr_t* s, const int fs, const int ofs);


/**
 * Add a raw sample to the resampler and perform resampling.
 * @param[in, t] s the state structure.
 * @param[in] x raw ECG sample.
 * @return number of output, the resampled output is in @c s.y.
 */
DLL int8_t nel_resampler_add_sample(nr_t* s, const int x,ProgressCallback callback);


nr_t nsk;
 

/**
 * Initialize the resampler , use nr_t* nsk; to store data .
 * @param[in] fs the sampling rate of input data.
 * @param[in] ofs the sampling rate of output data.
 */
DLL void nskECGreSamplerInit(const int fs, const int ofs);
//DLL void nskECGreSamplerInit(nr_t* ns,const uint16_t fs, const uint16_t ofs)

/**
 * Add a raw sample to the resampler and perform resampling.
 * @param[in] x raw ECG sample.  
 * @param ProgressCallback: function call back function
 * @return number of output, the resampled output is in @c s.y.
 */
DLL  int8_t nskECGreSampler(const int32_t x,ProgressCallback callback);



/* Begin Test Func  */
/* Test1: struct param call back on c#*/
struct Parameter{
    char param1[20];
    char param2[20];
};
DLL int GetValue(char* opt, struct Parameter params);


typedef struct {                /* NAG Complex Data Type */
    double re,im;
} Complex;
DLL void NAG_CALL TryComplex(Complex inputVar, Complex *outputVar,int n, Complex array[]);

/* Test2: test ProgressCallback */
typedef void (__cdecl * TestProgressCallback)(int);
typedef char* (__stdcall * TestGetFilePathCallback)(char* filter);
 
//DLL void DoWork(TestProgressCallback progressCallback);
__declspec(dllexport) void __cdecl DoWork(TestProgressCallback progressCallback);

DLL void ProcessFile(TestGetFilePathCallback getPath);


/* End Test Func  */


#ifdef __cplusplus
}
#endif
