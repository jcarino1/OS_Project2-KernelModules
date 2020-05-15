/*  Jeffrey Carino
    chess.c device driver/kernel module implementation
*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeffrey Carino <jcarino1@umbc.edu>");
#define BUF_LEN 80
#define N 8
#define inner 2
#define MIS_DEVICE_MINOR 144

//This is where the prototypes for the methods go.
static int turn; //If 1 turn belongs to the user and if 2 to computer.
static char board[8][8][2]; //Contains the game board. had *
static int read_bytes;
static char comm[13];
static char color; //Used in the character functions to determine which to move.
static int cpu_move = 7;
static char user_color;
static int is_game = false;
static int __init chess_init(void);
static void __exit chess_exit(void);
static int device_open(struct inode *, struct file*);
static int device_release(struct inode *, struct file*);
static ssize_t device_read(struct file*, char *, size_t, loff_t*);
static ssize_t device_write(struct file*, const char*, size_t, loff_t *);
static int pawn(char*, char*, char);
static int Device_Open = 0; //Checks if the device is open to prevent multiple access to the device.
static char msg[BUF_LEN];
static char* msg_ptr;

//These are used for file operations and must be implemented.
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

struct miscdevice chess_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "chess",
	.fops = &fops,
	.mode = 0666,
};

static int __init chess_init(void){
	printk(KERN_INFO "Module loaded\n");
	int ret;
	
	ret = misc_register(&chess_dev);
	if (ret){
		printk(KERN_ERR "Unable to register \"chess\" misc device\n");
}
	return ret;
}

static void __exit chess_exit(void){
	printk(KERN_INFO "Module unloaded\n");
	misc_deregister(&chess_dev);
	
}

//Functions that help keep track of everything.
//This checks to see if there is a device open already. If so, returns negative value and if there is not returns 0.
static int device_open(struct inode* inode, struct file* file){
	static int counter = 0;
	if(Device_Open){
		return -EBUSY;
	}

	Device_Open++;
	printk(msg, "This device is now opened.\n", counter++);
	
	return 0;
}
static int device_release(struct inode* inode, struct file* file){
	Device_Open--; //Now ready for next caller.

	return 0;
}
static ssize_t device_read(struct file* file, char* cmd, size_t len, loff_t* loff_t){
	char* respbuf = "OK \n";
	char* no_game = "NOGAME \n";
	char* unknown = "UNKCMD \n";
	int i = 0;
	int z = 0;
	
	if(comm[0] == '0' && comm[1] == '0' && (comm[3] == 'W' || comm[3] == 'B')){
		while(i < 5){
			put_user(*(respbuf),cmd++);
			respbuf++;
			i++;
		}
		return 3;
	}
	if(comm[0] == '0' && comm[1] == '1'){
		if(is_game == false){
			while(z < 9){
				put_user(*(no_game),cmd++);
				no_game++;
				z++;
			}
			return 7;
		}
		else if(is_game == true){
			int j,k,l;
		
			for(j = 0; j < N; j++){
				for(k = 0; k < N; k++){
					for(l = 0; l < inner; l++){
						put_user(board[j][k][l],cmd++);		
					}
				}
			}
			put_user("/n", cmd++);
			return 129;
		}
	}
	if((comm[0] == '0') && (comm[1] == '3')){
		if(is_game == 0){
			int u = 0;
			while(u < 9){
				put_user(*(no_game),cmd++);
				no_game++;
				u++;
			}
			return 7;
		}	
		while(i < 5){
			put_user(*(respbuf),cmd++);
			respbuf++;
			i++;
		}
		return 3;
	}
	if(comm[0] == '0' && comm[1] == '4'){
		if(is_game == 0){
			int u = 0;
			while(u < 9){
				put_user(*(no_game),cmd++);
				no_game++;
				u++;
			}
			return 7;
		}
		int u = 0;
		while(u < 5){
		put_user(*(respbuf),cmd++);
		respbuf++;
		u++;
		}
		
		is_game = 0;
		return 3;
		
	}
	int c = 0;
	while(c < 9){
		put_user(*(unknown),cmd++);
		unknown++;
		c++;
	}
	return 7;
		
		
}
static ssize_t device_write(struct file* file, const char* cmd, size_t len, loff_t* loff_t){
	size_t bytes_read = 0;
	read_bytes = 0;
	char commands[len];
	
	printk(KERN_INFO "size of cmd: %d\n", len);
	if(copy_from_user(commands, cmd, len))
		return -EFAULT; 

	while(bytes_read < len){
		printk(KERN_INFO "Char elements: %c\n", commands[bytes_read]);
		comm[bytes_read]=commands[bytes_read];
		bytes_read++;
	}
	//If the second character is a 0 then begin new game and the third character determines who goes first for the '00 B/W\n' command.
	if((commands[0] = "0") && (commands[1] = "0")){
		
		read_bytes = 5;
		int i,j,k;

		for(i = 0; i < N; i++){
			for(j = 0; j < N; j++){
				for(k = 0; k < inner; k++){
					board[i][j][k] = '*';
				}
			}
		}

		if(comm[3] == 'W'){
			user_color = 'W';
			is_game = true;
			turn = 1;
			printk(KERN_INFO "Player 1 goes first\n");
			int m,n;
			for(m = 0; m < N; m++){
				for(n = 0; n < N; n++){
					if(m == 0 && n == 0 || m == 0 && n == 7){
						board[m][n][0] = 'W';
						board[m][n][1] = 'R';
					}
					if(m == 0 && n == 1 || m == 0 && n == 6){
						board[m][n][0] = 'W';
						board[m][n][1] = 'N';
					}
					if(m == 0 && n == 2 || m == 0 && n == 5){
						board[m][n][0] = 'W';
						board[m][n][1] = 'B';
					}
					if(m == 0 && n == 3){
						board[m][n][0] = 'W';
						board[m][n][1] = 'Q';
					}
					if(m == 0 && n == 4){
						board[m][n][0] = 'W';
						board[m][n][1] = 'K';
					}
					if(m == 1){
						board[m][n][0] = 'W';
						board[m][n][1] = 'P';
					}
					if(m == 6){
						board[m][n][0] = 'B';
						board[m][n][1] = 'P';
					}
					if(m == 7 && n == 0 || m == 7 && n == 7){
						board[m][n][0] = 'B';
						board[m][n][1] = 'R';
					}
					if(m == 7 && n == 1 || m == 7 && n == 6){
						board[m][n][0] = 'B';
						board[m][n][1] = 'N';
					}
					if(m == 7 && n == 2 || m == 7 && n == 5){
						board[m][n][0] = 'B';
						board[m][n][1] = 'B';
					}
					if(m == 7 && n == 3){
						board[m][n][0] = 'B';
						board[m][n][1] = 'Q';
					}
					if(m == 7 && n == 4){
						board[m][n][0] = 'B';
						board[m][n][1] = 'K';
					}
				}
			}
		}
		if(comm[3] == 'B'){
			user_color = 'B';
			is_game = true;
			turn = 2;	
			printk(KERN_INFO "Player 2 goes first\n");
						int m,n;
			for(m = 0; m < N; m++){
				for(n = 0; n < N; n++){
					if(m == 0 && n == 0 || m == 0 && n == 7){
						board[m][n][0] = 'B';
						board[m][n][1] = 'R';
					}
					if(m == 0 && n == 1 || m == 0 && n == 6){
						board[m][n][0] = 'B';
						board[m][n][1] = 'N';
					}
					if(m == 0 && n == 2 || m == 0 && n == 5){
						board[m][n][0] = 'B';
						board[m][n][1] = 'B';
					}
					if(m == 0 && n == 3){
						board[m][n][0] = 'B';
						board[m][n][1] = 'Q';
					}
					if(m == 0 && n == 4){
						board[m][n][0] = 'B';
						board[m][n][1] = 'K';
					}
					if(m == 1){
						board[m][n][0] = 'B';
						board[m][n][1] = 'P';
					}
					if(m == 6){
						board[m][n][0] = 'W';
						board[m][n][1] = 'P';
					}
					if(m == 7 && n == 0 || m == 7 && n == 7){
						board[m][n][0] = 'W';
						board[m][n][1] = 'R';
					}
					if(m == 7 && n == 1 || m == 7 && n == 6){
						board[m][n][0] = 'W';
						board[m][n][1] = 'N';
					}
					if(m == 7 && n == 2 || m == 7 && n == 5){
						board[m][n][0] = 'W';
						board[m][n][1] = 'B';
					}
					if(m == 7 && n == 3){
						board[m][n][0] = 'W';
						board[m][n][1] = 'Q';
					}
					if(m == 7 && n == 4){
						board[m][n][0] = 'W';
						board[m][n][1] = 'K';
					}
				}
			}
		}
		
		return bytes_read;
	}
	if(comm[1] == "1"){
		if(is_game == true){
			printk(KERN_INFO "Return current state of the board\n");
			return bytes_read;
		}
		if(is_game == false){
			return 9;
		}
	}
	if((commands[0] = "0") && (commands[1] = "3")){
		if(is_game == 0){
			return 9;
		}
		board[6][4][0] = '*';
		board[6][4][1] = '*';
		board[4][4][0] = 'W';
		board[4][4][1] = 'P';
		return bytes_read;
	}
	if((comm[0] = "0") && (comm[1] = "4")){
		if(is_game == true){
			printk(KERN_INFO "Forfeit Game \n");
			return bytes_read;
		}
		if(is_game == false){
			printk(KERN_INFO "GAME IS OFF \n");
			return 9;
		}
	}
	
	return 9;
}
static int pawn(char* start, char* end, char player_color){
	return 0;
}

//Necessary things.
module_init(chess_init);
module_exit(chess_exit);
