all:

	nvcc -gencode arch=compute_52,code=sm_52 -Xptxas=-v -o main main.cu
test:
	nvcc -gencode arch=compute_52,code=sm_52 -Xptxas=-v -o main_test main_test.cu

fixed:
	nvcc -gencode arch=compute_52,code=sm_52 -Xptxas=-v -o main_fixed main_fixed.cu
#-gencode arch=compute_13,\"code=sm_13,compute_13\" -gencode arch=compute_20,\"code=sm_20,compute_20\" -gencode arch=compute_21,\"code=sm_21,compute_21\"

debug:

	nvcc -gencode arch=compute_52,code=sm_52 -Xptxas=-v -o main main.cu -g -G -lineinfo

clean:

	rm -f *.o main arch_check
