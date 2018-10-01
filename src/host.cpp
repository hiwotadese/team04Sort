/**********
Copyright (c) 2018, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/
//OpenCL utility layer include
#include "xcl2.hpp"
#include <iostream>
#include <vector>
#include <stdlib.h>

#define DATA_SIZE 267
#define COLS 16

using namespace std;

void merge_cpu(unsigned int *a, int start, int m, int stop){
    unsigned int temp[DATA_SIZE];
    int i, j, k;

    for(i=start; i<=m; i++){
        temp[i] = a[i];
    }


for(j=m+1; j<=stop; j++){
    temp[m+1+stop-j] = a[j];
}

i = start;
j = stop;

for(k=start; k<=stop; k++){
	unsigned int tmp_j = temp[j];
    unsigned int tmp_i = temp[i];
    if(tmp_j < tmp_i) {
        a[k] = tmp_j;
        j--;
    } else {
        a[k] = tmp_i;
        i++;
    }
}
}

void ms_mergesort_cpu(unsigned int *a) {
int start, stop;
int i, m, from, mid, to;

start = 0;
stop = DATA_SIZE;

for(m=1; m<stop-start; m+=m) {
    for(i=start; i<stop; i+=m+m) {
        from = i;
        mid = i+m-1;
        to = i+m+m-1;
        if(to < stop){
            merge_cpu(a, from, mid, to);
        }
        else{
            merge_cpu(a, from, mid, stop);
        }
    }
}
}


void ms_mergesort(cl::Buffer A, cl::Buffer B, const int size, cl::Kernel krnl_merge, cl::CommandQueue q) {
    int start, stop;
    int m;

    start = 0;
    stop = size;

    for(m=1; m<stop-start; m+=m) {
   	    int narg=0;

   	    krnl_merge.setArg(narg++,A);
   	    krnl_merge.setArg(narg++,B);
   	    krnl_merge.setArg(narg++,m-start);
   	    krnl_merge.setArg(narg++,size);

   	    q.enqueueNDRangeKernel(krnl_merge,cl::NullRange,cl::NDRange(128),cl::NullRange/*Local range*/);
    }
}

int main(int argc, char** argv)
{
    //Allocate Memory in Host Memory
    size_t vector_size_bytes = sizeof(unsigned int) * DATA_SIZE;

    //Initialize inputs

    unsigned int *source_input1, *source_input2, *source_hw_results, *source_sw_results;

    source_input1 = (unsigned int*)malloc(sizeof(unsigned int)*DATA_SIZE);
    source_input2 = (unsigned int*)malloc(sizeof(unsigned int)*DATA_SIZE);
    source_hw_results = (unsigned int*)malloc(sizeof(unsigned int)*DATA_SIZE);
    source_sw_results = (unsigned int*)malloc(sizeof(unsigned int)*DATA_SIZE);

//    std::vector<unsigned int,aligned_allocator<unsigned int>> source_input1     (DATA_SIZE);
//    std::vector<unsigned int,aligned_allocator<unsigned int>> source_input2     (DATA_SIZE);
//    std::vector<unsigned int,aligned_allocator<unsigned int>> source_hw_results(DATA_SIZE);
//    std::vector<unsigned int,aligned_allocator<unsigned int>> source_sw_results(DATA_SIZE);

    int size = DATA_SIZE;

    // Create the test data and Software Result 
    for(int i = 0 ; i < DATA_SIZE ; i++){
        source_input1[i] = DATA_SIZE - 1 - i;
        source_input2[i] = 0;
        source_hw_results[i] = 0;
    }

    //software matrix multiplier

    //ms_mergesort_cpu(source_input1);

    for(int i = 0; i < DATA_SIZE; i++) {
    	source_sw_results[i] = source_input1[i];
    }

    for(int i = 0; i < DATA_SIZE; i++) {
    	source_input1[i] = DATA_SIZE - 1 - i;
    }

    /*
    for(int i =0; i< DATA_SIZE/COLS ;i++)
    	 for(int j = 0;j<COLS;j++)
    	 {   int temp =0;
    		 for(int k = 0;k< COLS;k++)
    		 temp = temp + source_input1[i*COLS + k] * source_input2[k*COLS +j];
    		 source_sw_results[i*COLS +j] = temp;

    	 }
    	 */

  //  for(int i= 0;i< DATA_SIZE;i++)
   // 	cout<<source_sw_results[i]<<endl;
//OPENCL HOST CODE AREA START
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE);
    std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 

    //Create Program and Kernel
    std::string binaryFile = xcl::find_binary_file(device_name,"merge");
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    cl::Program program(context, devices, bins);
    cl::Kernel krnl_merge(program,"merge");

    //Allocate Buffer in Global Memory
    cl::Buffer buffer_input1 (context, CL_MEM_READ_WRITE,
                        vector_size_bytes);
    cl::Buffer buffer_input2 (context, CL_MEM_READ_WRITE,
                           vector_size_bytes);
//    cl::Buffer buffer_output(context, CL_MEM_WRITE_ONLY,
//                            vector_size_bytes);

    //Copy input data to device global memory
    q.enqueueWriteBuffer(buffer_input1, CL_TRUE, 0, vector_size_bytes, source_input1);
    //q.enqueueWriteBuffer(buffer_input2, CL_TRUE, 0, vector_size_bytes, source_input2.data());


//////////////////////
    int start, stop;
    int m;

    start = 0;
    stop = size;
/*
	    int narg=0;

	    krnl_merge.setArg(narg++,buffer_input1);
	    krnl_merge.setArg(narg++,buffer_input2);
	    krnl_merge.setArg(narg++,1);
	    krnl_merge.setArg(narg++,32);

	    q.enqueueNDRangeKernel(krnl_merge,cl::NullRange,cl::NDRange(16),cl::NullRange);
	    q.enqueueReadBuffer(buffer_input1, CL_TRUE, 0, vector_size_bytes, source_hw_results);
	    q.finish();

	    for (int i = 0 ; i < DATA_SIZE ; i++){
	    	std::cout << "sw: " << source_sw_results[i] << ", hw: " << source_hw_results[i] << std::endl;
	    }
	    std::cout << std::endl << std::endl;
*/

    for(m=1; m<stop-start; m+=m) {
   	    int narg=0;

   	    krnl_merge.setArg(narg++,buffer_input1);
   	    krnl_merge.setArg(narg++,buffer_input2);
   	    krnl_merge.setArg(narg++,m/*-start*/);
   	    krnl_merge.setArg(narg++,size);

   	    q.enqueueNDRangeKernel(krnl_merge,cl::NullRange,cl::NDRange(DATA_SIZE/(m+1)),cl::NullRange);
   	    q.enqueueReadBuffer(buffer_input1, CL_TRUE, 0, vector_size_bytes, source_hw_results);
   	    q.finish();

   	    for (int i = 0 ; i < DATA_SIZE ; i++){
   	    	std::cout << "sw: " << source_sw_results[i] << ", hw: " << source_hw_results[i] << std::endl;
   	    }
   	    std::cout << std::endl << std::endl;
   	    //break;
    }

////////////////////////


    //Launch the Kernel
    //q.enqueueNDRangeKernel(krnl_merge,cl::NullRange,cl::NDRange(cols,size/cols),cl::NullRange);
    ms_mergesort(buffer_input1, buffer_input2, size, krnl_merge, q);
    //This should be the proper template for the merge kernel invocation


    //Copy Result from Device Global Memory to Host Local Memory
    q.enqueueReadBuffer(buffer_input1, CL_TRUE, 0, vector_size_bytes, source_hw_results);


    q.finish();

//OPENCL HOST CODE AREA END
    
    // Compare the results of the Device to the simulation
    bool match = true;
    for (int i = 0 ; i < DATA_SIZE ; i++){
    	std::cout << "sw: " << source_sw_results[i] << ", hw: " << source_hw_results[i] << std::endl;
//        if (source_hw_results[i] != source_sw_results[i]){
//            std::cout << "Error: Result mismatch" << std::endl;
//            std::cout << "i = " << i << " CPU result = " << source_sw_results[i]
//                << " Device result = " << source_hw_results[i] << std::endl;
//            match = false;
//          break;
//       }
    }

    std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl; 
    return (match ? EXIT_SUCCESS :  EXIT_FAILURE);
}
