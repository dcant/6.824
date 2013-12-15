################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../extent_client.o \
../extent_server.o \
../extent_smain.o \
../fuse.o \
../gettime.o \
../inode_manager.o \
../lock_client.o \
../lock_demo.o \
../lock_server.o \
../lock_smain.o \
../lock_tester.o \
../yfs_client.o 

C_SRCS += \
../test-lab-3-a.c \
../test-lab-3-b.c 

CC_SRCS += \
../extent_client.cc \
../extent_server.cc \
../extent_smain.cc \
../fuse.cc \
../gettime.cc \
../inode_manager.cc \
../lock_client.cc \
../lock_demo.cc \
../lock_server.cc \
../lock_smain.cc \
../lock_tester.cc \
../yfs_client.cc 

OBJS += \
./extent_client.o \
./extent_server.o \
./extent_smain.o \
./fuse.o \
./gettime.o \
./inode_manager.o \
./lock_client.o \
./lock_demo.o \
./lock_server.o \
./lock_smain.o \
./lock_tester.o \
./test-lab-3-a.o \
./test-lab-3-b.o \
./yfs_client.o 

C_DEPS += \
./test-lab-3-a.d \
./test-lab-3-b.d 

CC_DEPS += \
./extent_client.d \
./extent_server.d \
./extent_smain.d \
./fuse.d \
./gettime.d \
./inode_manager.d \
./lock_client.d \
./lock_demo.d \
./lock_server.d \
./lock_smain.d \
./lock_tester.d \
./yfs_client.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


