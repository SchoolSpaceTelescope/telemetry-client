#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h> // needed for memset
#include <bitset>
#include <iostream>

using namespace std;

int main(int argc, char** argv)
{
	struct termios tio;
	int tty_fd;

	printf("Please start with %s /dev/ttyS1 (for example)\n", argv[0]);

	memset(&tio, 0, sizeof(tio));
	tio.c_iflag = 0;
	tio.c_oflag = 0;
	tio.c_cflag = CS8 | CREAD | CLOCAL; // 8n1, see termios.h for more information
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 5;
	
	tty_fd = open(argv[1], O_RDWR | O_NONBLOCK); // O_NONBLOCK might override VMIN and VTIME, so read() may return immediately.
	
	cfsetospeed(&tio, B9600); // 9600 baud
	cfsetispeed(&tio, B9600); // 9600 baud
	
	tcsetattr(tty_fd, TCSANOW, &tio);	
	
	/*
		INPUT
		RAW       DESCR               CMD
		00000000  ver                 v
		00001000  all val             p
		00010xxx  spi ch xxx read     sx (s0-s7)
		00011xxx  analog ch xxx read  ax (a0-a7)
		-         quit                q
		
		OUTPUT
		b15   b14   b13      b12      b11      b10    b9    b8    b7    b6    b5    b4    b3    b2    b1    b0
		cmd1  cmd0  ch_num2  ch_num1  ch_num0  dat10  dat9  dat8  dat7  dat6  dat5  dat4  dat3  dat2  dat1  dat0 
	*/
	
	const char CMD_VER = 0;       // 0b00000000
	const char CMD_ALL_VAL = 0x8; // 0b00001000
	const char CMD_CH = 0x10;    // 0b00010000
	
	int ch = 0;
	unsigned char cmd = 0;    
	unsigned char answ[2];
	char c = 0;
	
	short int ch_answ = 0;
	short int raw_val = 0;
	float val = 0;
	short int int_part = 0;
	short int fract_part = 0;
	
	while (read(tty_fd, &answ, 1) != -1) answ[0] = 0; // clear buffer	
	
	while( c != 'q'){
        cin >> c;

        if ((c == 's') || (c == 'a')){
            cin >> ch;
            
            if (c == 'a') ch += 8;
			
			cmd = CMD_CH | ch;
			
			write(tty_fd, &cmd, 1);
			usleep(100000); //100 ms
			read(tty_fd, &answ, 2);
			
			ch_answ = answ[1] & 0x0F; //0b00001111
				
			raw_val = answ[0] << 4;
			raw_val |= (answ[1] & 0xF0) >> 4;
			
			if (ch_answ < 8) {
				fract_part = raw_val & 0x07; //0b00000111 fractional part
				int_part = raw_val >> 3; //remove fract
				
				if (int_part & 0x0100) int_part |= 0xFE00; //if negative, complete with 0b1111
				
				
				val = int_part;
				
				if (fract_part) val += fract_part / 8.0;
			} else {
				val = raw_val;
			}
			
			cout << ch_answ << ": " << val << endl;			

            c = 0;
            ch = 0;
        }		

        if (c == 'v') {
            cmd = CMD_VER;
			
			write(tty_fd, &cmd, 1);
			usleep(100000); //100 ms
			read(tty_fd, &answ, 1);
			
			cout << static_cast<int>(answ[0]) << endl;
			
			c = 0;
            ch = 0;
        }

        if (c == 'p') {	
			//cmd = CMD_ALL_VAL;	
			
			for (cmd = 0x10; cmd < 0x20; cmd++){ // from 0b00010000 to 0b00111111
				write(tty_fd, &cmd, 1);
				usleep(100000); //100 ms
				read(tty_fd, &answ, 2);
				
				ch_answ = answ[1] & 0x0F; //0b00001111
				
				raw_val = answ[0] << 4;
				raw_val |= (answ[1] & 0xF0) >> 4;
				
				if (ch_answ < 8) {
					fract_part = raw_val & 0x07; //0b00000111 fractional part
					int_part = raw_val >> 3; //remove fract
					
					if (int_part & 0x0100) int_part |= 0xFE00; //if negative, complete with 0b1111
					
					
					val = int_part;
					
					if (fract_part) val += fract_part / 8.0;
				} else {
					val = raw_val;
				}
				
				cout << ch_answ << ": " << val << endl;
			}

            c = 0;
            ch = 0;
        }
		
		usleep(100000); //100 ms
		
		while (read(tty_fd, &answ, 1) != -1) answ[0] = 0; // clear buffer
    }

	close(tty_fd);
}