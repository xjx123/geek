#make project
GCC= arm-linux-gcc
main: camera.o simplified_jpeg_encoder.o control.o gprs.o
	$(GCC) -lpthread $^ -o $@

gprs.o:./gprs/gprs.c ./gprs/gprs.h
	$(GCC) -c $<

camera.o: ./camera/camera.c ./camera/camera.h \
./camera/simplified_jpeg_encoder.h ./camera/simplified_jpeg_encoder.c \
./initalization/init.h 
	$(GCC) -c $^

control.o:./control/control.c ./control/control.h
	$(GCC) -c $<

clean:
	rm  *.o