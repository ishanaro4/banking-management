#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct train{                     //structures of train and user and booking to be stored in database 
		int train_number;
		char train_name[50];
		int total_seats;
		int available_seats;
		};
struct user{
		int login_id;
		char password[50];
		char name[50];
		int type;
		};

struct booking{
		int booking_id;
		int type;
		int uid;
		int tid;
		int seats;
		};


void service_cli(int sock);//explicit functions declaration to avoid function calling errors
void login(int client_sock);
void signup(int client_sock);
int menu(int client_sock,int type,int id);
void crud_train(int client_sock);
void crud_user(int client_sock);
int user_function(int client_sock,int choice,int type,int id);


void service_cli(int sock){
	int choice;
	printf("\n\tClient [%d] Connected\n", sock);//1.SIGN IN 2.SIGN UP 3.EXIT
	do{
		read(sock, &choice, sizeof(int));	//read and write are blocking system calls so here it will wait for choice from client and not move forward till then	
		if(choice==1)
			login(sock);
		if(choice==2)
			signup(sock);
		if(choice==3)
			break;
	}while(1);

	close(sock);
	printf("\n\tClient [%d] Disconnected\n", sock);
}

//-------------------- Login function-----------------------------//

void login(int client_sock){
	int fd_user = open("db/db_user.dat",O_RDWR);
	int id,type,valid=0,user_valid=0;
	char password[50];
	struct user u;
	read(client_sock,&id,sizeof(id));
	read(client_sock,&password,sizeof(password));
	
	struct flock lock;
	
	lock.l_start = (id-1)*sizeof(struct user);//as id(primary key) starts from 1 therefore to reach ith block of struct in db_user,we gotta do id-1
	lock.l_len = sizeof(struct user);
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();

	lock.l_type = F_WRLCK;
	fcntl(fd_user,F_SETLKW, &lock);
	
	while(read(fd_user,&u,sizeof(u))){//checking id,password validations
		if(u.login_id==id){
			user_valid=1;
			if(!strcmp(u.password,password)){
				valid = 1;
				type = u.type;
				break;
			}
			else{
				valid = 0;
				break;
			}	
		}		
		else{
			user_valid = 0;
			valid=0;
		}
	}
	
	// same agent is allowed from multiple terminals, so unlocking his user record just after checking his credentials and allowing further
	if(type!=2){//if type is not customer i.e  it is agent or admin
		lock.l_type = F_UNLCK;
		fcntl(fd_user, F_SETLK, &lock);
		close(fd_user);
	}
	
	// if valid user, show him menu
	if(user_valid)//uservalid corresponds to correct id and valid =1 corresponds to both(correct id and password)
	{
		write(client_sock,&valid,sizeof(valid));
		if(valid){//if valid ==1
			write(client_sock,&type,sizeof(type));
			while(menu(client_sock,type,id)!=-1);//calling menu once both id and password are correct
		}
	}
	else
		write(client_sock,&valid,sizeof(valid));//incase valid==0 send it to client
	
	// same user is not allowed from multiple terminals..so unlocking his user record after he logs out only to not allow him from other terminal
	if(type==2){//if customer client
		lock.l_type = F_UNLCK;
		fcntl(fd_user, F_SETLK, &lock);
		close(fd_user);
	}
} 

//-------------------- Signup function-----------------------------//

void signup(int client_sock){
	int fd_user = open("db/db_user.dat",O_RDWR);
	int type,id=0;
	char name[50],password[50];
	struct user u,temp;

	read(client_sock, &type, sizeof(type));
	read(client_sock, &name, sizeof(name));
	read(client_sock, &password, sizeof(password));

	int fp = lseek(fd_user, 0, SEEK_END);

	struct flock lock;//SETTING WRITE LOCK IN db_user file to append these new sign up user details
	lock.l_type = F_WRLCK;
	lock.l_start = fp;
	lock.l_len = 0;//this lock means from start of end of file,it will be locked as long as we append the contents to file
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();


	fcntl(fd_user,F_SETLKW, &lock);//lock started

	// if file is empty, login id will start from 1 else it will increment from the previous value
	if(fp==0){
		u.login_id = 1;
		strcpy(u.name, name);
		strcpy(u.password, password);
		u.type=type;
		write(fd_user, &u, sizeof(u));//writing user to file
		write(client_sock, &u.login_id, sizeof(u.login_id));
	}
	else{
		fp = lseek(fd_user, -1 * sizeof(struct user), SEEK_END);//going to starting of last user struct in file
		read(fd_user, &u, sizeof(u));//reading the lst user struct to check login id so as to increment by 1 to create new login id for new user
		u.login_id++;
		strcpy(u.name, name);
		strcpy(u.password, password);//overwriting name , password, type in u struct by new values
		u.type=type;
		write(fd_user, &u, sizeof(u));//finally writing new user at the end of file
		write(client_sock, &u.login_id, sizeof(u.login_id));
	}
	lock.l_type = F_UNLCK;//lock unlocked
	fcntl(fd_user, F_SETLK, &lock);

	close(fd_user);
	
}

//-------------------- Main menu function-----------------------------//

int menu(int client_sock,int type,int id){
	int choice,ret;

	// for admin
	if(type==0){
		read(client_sock,&choice,sizeof(choice));
		if(choice==1){					// CRUD options on train
			crud_train(client_sock);
			return menu(client_sock,type,id);	
		}
		else if(choice==2){				// CRUD options on User
			crud_user(client_sock);
			return menu(client_sock,type,id);
		}
		else if (choice ==3)				// Logout
			return -1;
	}
	else if(type==2 || type==1){				// For agent and customer
		read(client_sock,&choice,sizeof(choice));
		ret = user_function(client_sock,choice,type,id);
		if(ret!=5)
			return menu(client_sock,type,id);
		else if(ret==5)
			return -1;
	}		
}

//---------------------- CRUD operation on train--------------------//

void crud_train(int client_sock){
	int valid=0;	
	int choice;
	read(client_sock,&choice,sizeof(choice));
	if(choice==1){  					// Add train  	
		char tname[50];
		int tid = 0;
		read(client_sock,&tname,sizeof(tname));
		struct train tdb,temp;
		struct flock lock;
		int fd_train = open("db/db_train.dat", O_RDWR);
		
		tdb.train_number = tid;
		strcpy(tdb.train_name,tname);
		tdb.total_seats = 10;	// we are taking 10 seats for every train
		tdb.available_seats = 10;

		int fp = lseek(fd_train, 0, SEEK_END); 

		lock.l_type = F_WRLCK;
		lock.l_start = fp;
		lock.l_len = 0;//mandatory write lock after end of file
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();

		fcntl(fd_train, F_SETLKW, &lock);//lock started

		if(fp == 0){//same functionality as signup of user
			valid = 1;
			write(fd_train, &tdb, sizeof(tdb));
			lock.l_type = F_UNLCK;
			fcntl(fd_train, F_SETLK, &lock);
			close(fd_train);
			write(client_sock, &valid, sizeof(valid));
		}
		else{
			valid = 1;
			lseek(fd_train, -1 * sizeof(struct train), SEEK_END);
			read(fd_train, &temp, sizeof(temp));
			tdb.train_number = temp.train_number + 1;
			write(fd_train, &tdb, sizeof(tdb));
			write(client_sock, &valid,sizeof(valid));	
		}
		lock.l_type = F_UNLCK;//lock unlocked
		fcntl(fd_train, F_SETLK, &lock);
		close(fd_train);
		
	}

	else if(choice==2){					// View/ Read trains
		struct flock lock;
		struct train tdb;
		int fd_train = open("db/db_train.dat", O_RDONLY);
		
		lock.l_type = F_RDLCK;
		lock.l_start = 0;//mandatory read lock whole file
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd_train, F_SETLKW, &lock);
		int fp = lseek(fd_train, 0, SEEK_END);
		int no_of_trains = fp / sizeof(struct train);
		write(client_sock, &no_of_trains, sizeof(int));

		lseek(fd_train,0,SEEK_SET);
		while(fp != lseek(fd_train,0,SEEK_CUR)){
			read(fd_train,&tdb,sizeof(tdb));
			write(client_sock,&tdb.train_number,sizeof(int));
			write(client_sock,&tdb.train_name,sizeof(tdb.train_name));
			write(client_sock,&tdb.total_seats,sizeof(int));
			write(client_sock,&tdb.available_seats,sizeof(int));
		}
		valid = 1;
		lock.l_type = F_UNLCK;
		fcntl(fd_train, F_SETLK, &lock);
		close(fd_train);
	}

	else if(choice==3){					// Update train
		crud_train(client_sock);
		int choice,valid=0,tid;
		struct flock lock;
		struct train tdb;
		int fd_train = open("db/db_train.dat", O_RDWR);

		read(client_sock,&tid,sizeof(tid));

		lock.l_type = F_WRLCK;
		lock.l_start = (tid)*sizeof(struct train);
		lock.l_len = sizeof(struct train);
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd_train, F_SETLKW, &lock);

		lseek(fd_train, 0, SEEK_SET);
		lseek(fd_train, (tid)*sizeof(struct train), SEEK_CUR);
		read(fd_train, &tdb, sizeof(struct train));
		
		read(client_sock,&choice,sizeof(int));
		if(choice==1){							// update train name
			write(client_sock,&tdb.train_name,sizeof(tdb.train_name));
			read(client_sock,&tdb.train_name,sizeof(tdb.train_name));
			
		}
		else if(choice==2){						// update total number of seats
			write(client_sock,&tdb.total_seats,sizeof(tdb.total_seats));
			int initial = tdb.total_seats;
			read(client_sock,&tdb.total_seats,sizeof(tdb.total_seats));
			int inc = tdb.total_seats - initial;
			tdb.available_seats+=inc;
		}
	
		lseek(fd_train, -1*sizeof(struct train), SEEK_CUR);
		write(fd_train, &tdb, sizeof(struct train));
		valid=1;
		write(client_sock,&valid,sizeof(valid));
		lock.l_type = F_UNLCK;
		fcntl(fd_train, F_SETLK, &lock);
		close(fd_train);	
	}

	else if(choice==4){						// Delete train
		crud_train(client_sock);
		struct flock lock;
		struct train tdb;
		int fd_train = open("db/db_train.dat", O_RDWR);
		int tid,valid=0;

		read(client_sock,&tid,sizeof(tid));

		lock.l_type = F_WRLCK;
		lock.l_start = (tid)*sizeof(struct train);
		lock.l_len = sizeof(struct train);
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd_train, F_SETLKW, &lock);
		
		lseek(fd_train, 0, SEEK_SET);
		lseek(fd_train, (tid)*sizeof(struct train), SEEK_CUR);
		read(fd_train, &tdb, sizeof(struct train));
		strcpy(tdb.train_name,"deleted");//we adopt methodology of overwriting name as "deleted" in case train is deleted instead of removing it from file itself
		lseek(fd_train, -1*sizeof(struct train), SEEK_CUR);
		write(fd_train, &tdb, sizeof(struct train));
		valid=1;
		write(client_sock,&valid,sizeof(valid));
		lock.l_type = F_UNLCK;
		fcntl(fd_train, F_SETLK, &lock);
		close(fd_train);	
	}	
}

//---------------------- CRUD operation on user--------------------//
void crud_user(int client_sock){
	int valid=0;	
	int choice;
	read(client_sock,&choice,sizeof(choice));
	if(choice==1){    					// Add user
		char name[50],password[50];
		int type;
		read(client_sock, &type, sizeof(type));
		read(client_sock, &name, sizeof(name));
		read(client_sock, &password, sizeof(password));
		
		struct user udb;
		struct flock lock;
		int fd_user = open("db/db_user.dat", O_RDWR);
		int fp = lseek(fd_user, 0, SEEK_END);
		
		lock.l_type = F_WRLCK;//write locking will start from end of the file
		lock.l_start = fp;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();

		fcntl(fd_user, F_SETLKW, &lock);//lock started

		if(fp==0){//login_id starts from 1
			udb.login_id = 1;//incase its the first user
			strcpy(udb.name, name);
			strcpy(udb.password, password);
			udb.type=type;
			write(fd_user, &udb, sizeof(udb));
			valid = 1;
			write(client_sock,&valid,sizeof(int));
			write(client_sock, &udb.login_id, sizeof(udb.login_id));
		}
		else{
			fp = lseek(fd_user, -1 * sizeof(struct user), SEEK_END);
			read(fd_user, &udb, sizeof(udb));
			udb.login_id++;
			strcpy(udb.name, name);
			strcpy(udb.password, password);
			udb.type=type;
			write(fd_user, &udb, sizeof(udb));
			valid = 1;
			write(client_sock,&valid,sizeof(int));
			write(client_sock, &udb.login_id, sizeof(udb.login_id));
		}
		lock.l_type = F_UNLCK;//lock unlocked
		fcntl(fd_user, F_SETLK, &lock);
		close(fd_user);

		
	}

	else if(choice==2){					// View user list
		struct flock lock;
		struct user udb;
		int fd_user = open("db/db_user.dat", O_RDONLY);
		
		lock.l_type = F_RDLCK;
		lock.l_start = 0;
		lock.l_len = 0;//mandatory read lock 
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd_user, F_SETLKW, &lock);//lock started
		int fp = lseek(fd_user, 0, SEEK_END);
		int no_of_users = fp / sizeof(struct user);
		no_of_users--;
		write(client_sock, &no_of_users, sizeof(int));

		lseek(fd_user,0,SEEK_SET);//going to start of file to iterate over entire user struct and send it to client to print it
		while(fp != lseek(fd_user,0,SEEK_CUR)){
			read(fd_user,&udb,sizeof(udb));
			if(udb.type!=0){//if user is not admin then only send data to client
				write(client_sock,&udb.login_id,sizeof(int));
				write(client_sock,&udb.name,sizeof(udb.name));
				write(client_sock,&udb.type,sizeof(int));
			}
		}
		valid = 1;
		lock.l_type = F_UNLCK;//lock unlocked
		fcntl(fd_user, F_SETLK, &lock);
		close(fd_user);

	}

	else if(choice==3){					// Update user
		crud_user(client_sock);
		int choice,valid=0,uid;
		char pass[50];
		struct flock lock;
		struct user udb;
		int fd_user = open("db/db_user.dat", O_RDWR);

		read(client_sock,&uid,sizeof(uid));

		lock.l_type = F_WRLCK;
		lock.l_start =  (uid-1)*sizeof(struct user);//locking the concerned user's struct whose data needs to be updated..and id starts from 1 so uid-1 will go to start of id th record
		lock.l_len = sizeof(struct user);
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd_user, F_SETLKW, &lock);//lock started

		lseek(fd_user, 0, SEEK_SET);
		lseek(fd_user, (uid-1)*sizeof(struct user), SEEK_CUR);
		read(fd_user, &udb, sizeof(struct user));
		
		read(client_sock,&choice,sizeof(int));
		if(choice==1){					// update name
			write(client_sock,&udb.name,sizeof(udb.name));
			read(client_sock,&udb.name,sizeof(udb.name));
			valid=1;
			write(client_sock,&valid,sizeof(valid));		
		}
		else if(choice==2){				// update password
			read(client_sock,&pass,sizeof(pass));
			if(!strcmp(udb.password,pass))
				valid = 1;
			write(client_sock,&valid,sizeof(valid));
			read(client_sock,&udb.password,sizeof(udb.password));
		}
	
		lseek(fd_user, -1*sizeof(struct user), SEEK_CUR);
		write(fd_user, &udb, sizeof(struct user));//writing updated struct back to the file
		if(valid)
			write(client_sock,&valid,sizeof(valid));
		lock.l_type = F_UNLCK;//lock unlocked
		fcntl(fd_user, F_SETLK, &lock);
		close(fd_user);

	}

	else if(choice==4){						// Delete any particular user
		crud_user(client_sock);
		struct flock lock;
		struct user udb;
		int fd_user = open("db/db_user.dat", O_RDWR);
		int uid,valid=0;

		read(client_sock,&uid,sizeof(uid));

		lock.l_type = F_WRLCK;
		lock.l_start =  (uid-1)*sizeof(struct user);
		lock.l_len = sizeof(struct user);
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd_user, F_SETLKW, &lock);
		
		lseek(fd_user, 0, SEEK_SET);
		lseek(fd_user, (uid-1)*sizeof(struct user), SEEK_CUR);
		read(fd_user, &udb, sizeof(struct user));
		strcpy(udb.name,"deleted");//we adopt methodology of overwriting name as "deleted" in case user is deleted instead of removing it from file itself
		strcpy(udb.password,"");
		lseek(fd_user, -1*sizeof(struct user), SEEK_CUR);
		write(fd_user, &udb, sizeof(struct user));
		valid=1;
		write(client_sock,&valid,sizeof(valid));
		lock.l_type = F_UNLCK;
		fcntl(fd_user, F_SETLK, &lock);
		close(fd_user);

	}

}


//---------------------- User functions -----------------------//
int user_function(int client_sock,int choice,int type,int id){
	int valid=0;
	if(choice==1){						// book ticket
		crud_train(client_sock);
		struct flock lockt;
		struct flock lockb;
		struct train tdb;
		struct booking bdb;
		int fd_train = open("db/db_train.dat", O_RDWR);
		int fd_book = open("db/db_booking.dat", O_RDWR);
		int tid,seats;
		read(client_sock,&tid,sizeof(tid));		
				
		lockt.l_type = F_WRLCK;
		lockt.l_start = tid*sizeof(struct train);//as train_id in our case starts from 0 thereofre no need to do tid-1
		lockt.l_len = sizeof(struct train);
		lockt.l_whence = SEEK_SET;
		lockt.l_pid = getpid();
		
		lockb.l_type = F_WRLCK;
		lockb.l_start = 0;
		lockb.l_len = 0;
		lockb.l_whence = SEEK_END;
		lockb.l_pid = getpid();
		
		fcntl(fd_train, F_SETLKW, &lockt);
		lseek(fd_train,tid*sizeof(struct train),SEEK_SET);
		
		read(fd_train,&tdb,sizeof(tdb));
		read(client_sock,&seats,sizeof(seats));

		if(tdb.train_number==tid)
		{		
			if(tdb.available_seats>=seats){
				valid = 1;
				tdb.available_seats -= seats;//decrementing the seats booked from total available seats 
				fcntl(fd_book, F_SETLKW, &lockb);
				int fp = lseek(fd_book, 0, SEEK_END);
				
				if(fp > 0){
					lseek(fd_book, -1*sizeof(struct booking), SEEK_CUR);
					read(fd_book, &bdb, sizeof(struct booking));
					bdb.booking_id++;
				}
				else 
					bdb.booking_id = 0;

				bdb.type = type;
				bdb.uid = id;
				bdb.tid = tid;
				bdb.seats = seats;
				write(fd_book, &bdb, sizeof(struct booking));
				lockb.l_type = F_UNLCK;//lock unlocked for booking
				fcntl(fd_book, F_SETLK, &lockb);
			 	close(fd_book);
			}
		
		lseek(fd_train, -1*sizeof(struct train), SEEK_CUR);
		write(fd_train, &tdb, sizeof(tdb));
		}

		lockt.l_type = F_UNLCK;
		fcntl(fd_train, F_SETLK, &lockt);//lock unlocked for train file
		close(fd_train);
		write(client_sock,&valid,sizeof(valid));
		return valid;		
	}
	
	else if(choice==2){							// View bookings
		struct flock lock;
		struct booking bdb;
		int fd_book = open("db/db_booking.dat", O_RDONLY);
		int no_of_bookings = 0;
	
		lock.l_type = F_RDLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		
		fcntl(fd_book, F_SETLKW, &lock);
	
		while(read(fd_book,&bdb,sizeof(bdb))){
			if (bdb.uid==id&&bdb.seats!=0)
				no_of_bookings++;
		}
		int zero_booking=0;
		if(no_of_bookings==0){//if no bookking then send flag 0 to client and unlock the file and return 2 
			write(client_sock, &zero_booking, sizeof(int));
			lock.l_type = F_UNLCK;
		fcntl(fd_book, F_SETLK, &lock);
		close(fd_book);
			return 2;
		}
		else{
		zero_booking=1;//else send flag 1 to client i.e to continue ahead
		write(client_sock, &zero_booking, sizeof(int));
		}

		write(client_sock, &no_of_bookings, sizeof(int));
		lseek(fd_book,0,SEEK_SET);

		while(read(fd_book,&bdb,sizeof(bdb))){
			if(bdb.uid==id){
				write(client_sock,&bdb.booking_id,sizeof(int));
				write(client_sock,&bdb.tid,sizeof(int));
				write(client_sock,&bdb.seats,sizeof(int));
			}
		}
		lock.l_type = F_UNLCK;
		fcntl(fd_book, F_SETLK, &lock);
		close(fd_book);
		return valid;
	}

	else if (choice==3){							// update booking
		int choice = 2,bid,val;
		int check = user_function(client_sock,choice,type,id);//getting value
		if(check==2){//if returned is 2 it means no booking therefore return as update is not possible
		return valid;
		}
		struct booking bdb;
		struct train tdb;
		struct flock lockb;
		struct flock lockt;
		int fd_book = open("db/db_booking.dat", O_RDWR);
		int fd_train = open("db/db_train.dat", O_RDWR);
		read(client_sock,&bid,sizeof(bid));

		lockb.l_type = F_WRLCK;
		lockb.l_start = bid*sizeof(struct booking);
		lockb.l_len = sizeof(struct booking);
		lockb.l_whence = SEEK_SET;
		lockb.l_pid = getpid();
		
		fcntl(fd_book, F_SETLKW, &lockb);
		lseek(fd_book,bid*sizeof(struct booking),SEEK_SET);
		read(fd_book,&bdb,sizeof(bdb));
		lseek(fd_book,-1*sizeof(struct booking),SEEK_CUR);
		
		lockt.l_type = F_WRLCK;
		lockt.l_start = (bdb.tid)*sizeof(struct train);
		lockt.l_len = sizeof(struct train);
		lockt.l_whence = SEEK_SET;
		lockt.l_pid = getpid();

		fcntl(fd_train, F_SETLKW, &lockt);
		lseek(fd_train,(bdb.tid)*sizeof(struct train),SEEK_SET);
		read(fd_train,&tdb,sizeof(tdb));
		lseek(fd_train,-1*sizeof(struct train),SEEK_CUR);

		read(client_sock,&choice,sizeof(choice));
	
		if(choice==1){							// increase number of seats required of booking id
			read(client_sock,&val,sizeof(val));
			if(tdb.available_seats>=val){
				valid=1;
				tdb.available_seats -= val;
				bdb.seats += val;
			}
		}
		else if(choice==2){						// decrease number of seats required of booking id
			valid=1;
			read(client_sock,&val,sizeof(val));
			tdb.available_seats += val;
			bdb.seats -= val;	
		}
		
		write(fd_train,&tdb,sizeof(tdb));
		lockt.l_type = F_UNLCK;
		fcntl(fd_train, F_SETLK, &lockt);
		close(fd_train);
		
		write(fd_book,&bdb,sizeof(bdb));
		lockb.l_type = F_UNLCK;
		fcntl(fd_book, F_SETLK, &lockb);
		close(fd_book);
		
		write(client_sock,&valid,sizeof(valid));
		return valid;
	}
	else if(choice==4){							// Cancel an entire booking
		int choice = 2,bid;
		int check = user_function(client_sock,choice,type,id);
		if(check==2){//if returned is 2 it means no booking therefore return as cancel is not possible
		return valid;
		}
		struct booking bdb;
		struct train tdb;
		struct flock lockb;
		struct flock lockt;
		int fd_book = open("db/db_booking.dat", O_RDWR);
		int fd_train = open("db/db_train.dat", O_RDWR);
		read(client_sock,&bid,sizeof(bid));

		lockb.l_type = F_WRLCK;
		lockb.l_start = bid*sizeof(struct booking);
		lockb.l_len = sizeof(struct booking);
		lockb.l_whence = SEEK_SET;
		lockb.l_pid = getpid();
		
		fcntl(fd_book, F_SETLKW, &lockb);
		lseek(fd_book,bid*sizeof(struct booking),SEEK_SET);
		read(fd_book,&bdb,sizeof(bdb));
		lseek(fd_book,-1*sizeof(struct booking),SEEK_CUR);
		
		lockt.l_type = F_WRLCK;
		lockt.l_start = (bdb.tid)*sizeof(struct train);
		lockt.l_len = sizeof(struct train);
		lockt.l_whence = SEEK_SET;
		lockt.l_pid = getpid();

		fcntl(fd_train, F_SETLKW, &lockt);
		lseek(fd_train,(bdb.tid)*sizeof(struct train),SEEK_SET);
		read(fd_train,&tdb,sizeof(tdb));
		lseek(fd_train,-1*sizeof(struct train),SEEK_CUR);

		tdb.available_seats += bdb.seats;
		bdb.seats = 0;
		valid = 1;

		write(fd_train,&tdb,sizeof(tdb));
		lockt.l_type = F_UNLCK;
		fcntl(fd_train, F_SETLK, &lockt);
		close(fd_train);
		
		write(fd_book,&bdb,sizeof(bdb));
		lockb.l_type = F_UNLCK;
		fcntl(fd_book, F_SETLK, &lockb);
		close(fd_book);
		
		write(client_sock,&valid,sizeof(valid));
		return valid;
		
	}
	else if(choice==5)										// Logout
		return 5;

}
