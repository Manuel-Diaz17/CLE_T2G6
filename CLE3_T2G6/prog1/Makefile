CU_APPS=prog1

all: ${CU_APPS}

%: %.cu
	nvcc -O2 -Wno-deprecated-gpu-targets -o $@ $<
clean:
	rm -f ${CU_APPS}