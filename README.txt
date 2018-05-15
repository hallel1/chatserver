
Name: Hallel Davidian

Name of the exercise: Ex4 - Chat Server with select
-----------------------------------------------------
The list of the submitted files:

#chatserver.c- this file implement a simple chat server using TCP and select(), 
		Helped by list of int, all node hold sd of one client 
		

the functions: 


//fuctions of list
	1.the function - createNode--------create node and insrert his values
	2.the function - slist_init--------Initialize a single linked list
	3.the function - slist_destroy-----Destroy and de-allocate the memory hold by a list
	4.the function - slist_pop_first---Pop the first element in the list
	5.the function - slist_append------Append data to list 
	6.the function - pop_by_sd---------Pop element by sd from the list
//other function:
	7.the function - errorInput--------input error
	8.the function - errorMsg----------exit with messege
	9.the function - stringToInt-------check if the input str is a integer - if yes return the number else call errorInput
	10.the function - handler----------catch the signal: ^c in order to release the list
-----------------------------------------------------
