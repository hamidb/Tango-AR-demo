#environment setup
NVCC = $(TADP)/cuda-6.0/bin/nvcc -ccbin $(GCC)  -target-cpu-arch=ARM -arch=sm_20 -m32 -O3 -Xptxas '-dlcm=ca' -target-os-variant=Android
GCC = $(NDK_ROOT)/toolchains/arm-linux-androideabi-4.6/gen_standalone/linux-x86_64/bin/arm-linux-androideabi-g++
INCLUDES += -I$(TADP)/cuda-6.0/include/ -include $(NDK_ROOT)/platforms/android-19/arch-arm/usr/include/android/log.h

OBJS := nativeCUDA.o

%.o: %.cu
	    $(NVCC) $(CFLAGS) $(INCLUDES) $(EXTRA_CFLAGS) -c -o "$@" "$<"

lib_cudaCode.a: $(OBJS)
	    $(NVCC) -lib -o "$@" $(OBJS)

clean:
	    rm -rf *.a $(OBJS)
