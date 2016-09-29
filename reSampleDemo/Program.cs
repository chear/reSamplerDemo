using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace reSampleDemo
{
    class Program
    {
        #region Kiss FFT DLL

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void ProgressCallback(int value); 

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct ResamplerStruct
        {
            [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.I2, SizeConst = 18)]
            short[] y;
            [MarshalAs(UnmanagedType.I2)]
            short x;
            [MarshalAs(UnmanagedType.U2)]
            ushort fs;
            [MarshalAs(UnmanagedType.U2)]
            ushort ofs;
            [MarshalAs(UnmanagedType.U4)]
            uint a1;
            [MarshalAs(UnmanagedType.U4)]
            uint a2;
        };
        //delegate void ProgressCallback(int value);
         
        /*  kiss fft dll func */
        [DllImport("resampler.dll", EntryPoint = "nel_resampler_init", CallingConvention = CallingConvention.Cdecl)]
        public static extern void nel_resampler_init(ref ResamplerStruct s, int fs, int ofs);

        [DllImport("resampler.dll", EntryPoint = "nel_resampler_add_sample", CallingConvention = CallingConvention.Cdecl)]
        static extern int nel_resampler_add_sample(ref ResamplerStruct s, int x, [MarshalAs(UnmanagedType.FunctionPtr)] ProgressCallback callback);

        [DllImport("resampler.dll", EntryPoint = "nskECGreSamplerInit", CallingConvention = CallingConvention.Cdecl)]
        public static extern void nskECGreSamplerInit(int fs, int ofs);

        [DllImport("resampler.dll", EntryPoint = "nskECGreSampler", CallingConvention = CallingConvention.Cdecl)]
        static extern int nskECGreSampler(int ECGsample, [MarshalAs(UnmanagedType.FunctionPtr)] ProgressCallback callbackPointer);       

        /* Begin Test Func */
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct Parameters
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 20)]
            public String Param1;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 20)]
            public String Param2;
        }

        [DllImport(@"resampler.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int GetValue(StringBuilder sbOut, Parameters sbIn);

        [StructLayout(LayoutKind.Sequential)]
        public struct Complex
        {
            public double re;
            public double im;
        };
        [DllImport("resampler.dll")]
        public static extern void TryComplex(Complex inputVar,ref Complex outputVar, int n, [In, Out]  Complex[] array);
        public static void CallTryComplex(Complex inputVar,
                ref Complex outputVar, Complex[] array)
        {
            int n = 2;
            TryComplex(inputVar, ref outputVar, n, array);
        }
        /* End Test Func */

        /* Begin Test2 Func*/
        [DllImport("resampler.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern void DoWork([MarshalAs(UnmanagedType.FunctionPtr)] TestProgressCallback callbackPointer);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void TestProgressCallback(int value);
        
        #endregion

        public static void MyCallBackFunc(int v)
        {
            Console.WriteLine("Progress = {0}", v);
        }

        /* End Test2 Fun*/

        static ResamplerStruct re;       
        //[STAThread]
        static void Main(string[] args)
        {
#if(false)
            /* Test 1 */
            //Complex inputVar = new Complex();
            //Complex outputVar = new Complex();
            //Complex[] array = new Complex[10];
            //inputVar.re = 1.0;
            //inputVar.im = 2.0;
            //CallTryComplex(inputVar, ref outputVar, array);

            /* Test 2 */
            TestProgressCallback callback =
           (value) =>
           {
               Console.WriteLine("Progress = {0}", value);
           };
            //TestProgressCallback callback = new TestProgressCallback(MyCallBackFunc);
            Console.WriteLine("Press any key to run DoWork....");
            Console.ReadKey(true);
            string line;
            int line_number = 0;
            System.IO.StreamReader file = new System.IO.StreamReader(@"testcallback.txt");
            while ((line = file.ReadLine()) != null)
            {
                line_number++;
                //Console.WriteLine(line);
                try
                {
                    Console.WriteLine("DoWork {0} times", line_number);
                    DoWork(callback);
                    
                }
                catch (Exception ex)
                {
                    Console.WriteLine("DoWork.Exception=\"{0}\"", ex.Message);
                }
            }
            // call DoWork in C code
            
            Console.WriteLine();
            Console.WriteLine("Press any key to run ProcessFile....");
            Console.ReadKey(true);

#else
            int inputCounter = 0;
            int outputCounter = 0;
            // define a progress callback delegate
            ProgressCallback callback =
                (value) =>
                {
                    try
                    {
                        outputCounter++;
                        Console.WriteLine("reSample ECG = {0}", value);
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine("call back ex."+ex.Message);
                    }
                };

            Console.WriteLine("NeuroSky Resampler Demo v1\n\n");
            Console.WriteLine("Please enter your input sample rate: (256-5120)");
            int inputSR = Int32.Parse(Console.ReadLine());
            Console.WriteLine("Please enter your output sample rate: (256-5120)");
            int outputSR = Int32.Parse(Console.ReadLine());

            //nskECGreSamplerInit(inputSR, outputSR);
            re = new ResamplerStruct(); ;
            ////nel_resampler_init(ref re, inputSR, outputSR);
            nskECGreSamplerInit(inputSR, outputSR);

            string line;
            int line_number = 0;
            System.IO.StreamReader file = new System.IO.StreamReader(@"rawEcg.txt");
            while ((line = file.ReadLine()) != null)
            {
                line_number++;
                //Console.WriteLine(line);
                try
                {
                    inputCounter++;
                    //nskECGreSampler(Int32.Parse(line), callback);
                   
                    ////nel_resampler_add_sample(ref re, Int32.Parse(line),callback);
                    nskECGreSampler(Int32.Parse(line), callback);
                }
                catch (FormatException)
                {
                    Console.WriteLine("line: " + line_number + " - " + line);
                    Console.ReadLine();
                }
            }

            file.Close();

            Console.WriteLine("\nSample Rate");
            Console.WriteLine("==============");
            Console.WriteLine("Input: " + inputSR);
            Console.WriteLine("Output: " + outputSR);
            Console.WriteLine("\nSample Count");
            Console.WriteLine("==============");
            Console.WriteLine("Input: " + inputCounter);
            Console.WriteLine("Output: " + outputCounter);
            Console.ReadKey(true);

#endif
        }
    }
}
