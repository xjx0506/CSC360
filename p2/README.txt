University of Victoria
2018 Fall
CSC 360 Assignment 2
Zijian Chen
V00867494

--------------------------------------------------------------------------------

Program Instructions
This code is a about airline check-in system simulation. In this code include
two Queues business queue (1) and economy queue (0). The business queue has high
priority than economy queue, so the business class will check-in first than
economy queue.

--------------------------------------------------------------------------------

Instructions of Running source CODE
1. Ubuntu 64bit System
2. gcc support
3. To compile the code, can execute 'make' from the terminal
4. to run the program, execute './ACS customer.txt' from the terminal

--------------------------------------------------------------------------------

Usage Function Support

1.main: the main function include all sub-functino and thread creations.
2.customer_entry: the function is use to support the create customer threads.
3.clerk_pick: the function is use to support the create clerk threads.

--------------------------------------------------------------------------------

Learning Process

1. Queue linkedlist part use my perious course Seng265 source code and improve it by myself change some data type to customer information.
2. pthread create learn from the course and lab ppt.
3. mutex learn from course and prof provided sample code.
4. convar learn from course and prof provided sample code and some are learn from web.
5. destroy learn from course and prof provided sample code.
