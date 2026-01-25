TARGET = nfadc400_update

CC       = gcc
# [수정됨] -DNKC 플래그 추가 (ROOT C++ 헤더 의존성 제거)
CFLAGS   = -O2 -Wall -DNKC -I$(NKHOME)/include
LDFLAGS  = -L$(NKHOME)/lib -lNoticeNFADC400_c -lNotice6UVME_c -llinux_interface_c

all: $(TARGET)

$(TARGET): updateNFADC400.o
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Installing $(TARGET) to $(NKHOME)/bin"
	cp $(TARGET) $(NKHOME)/bin/

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o
