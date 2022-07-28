//MT2021057 TRAIN RESERVATION PROJECT//


give execute permissions to reset.sh
>chmod u+x reset.sh 

run reset.sh(to create database)
>./reset.sh 

run compile.sh to create object file of server and client
>./client.sh

#Here we are using port 8000
you can change the port of server and client
you can also change ip address in client under variable name server_ip.

run the server:
>./server

run the client:
>./client

first sign up and create admin account. 
The secret pin for admin account is "secret".

now login into admin account and add trains.

create customers/agents and login to book ticket.

