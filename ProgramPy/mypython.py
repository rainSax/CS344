############################################################
# Author: Navine Rai					   #
# Date: 2-1-20						   #
# Description: ProgramPy which is meant to create three    #
#	       files and populate them with 10 lowercase   #
#	       random characters.			   #
############################################################

#import random, and string modules
from random import seed
from random import random
from random import randrange
import string

#generate a new seed based on the system time
seed()

#create files for writing and reading
f1 = open("file1.txt", "w+")

f2 = open("file2.txt", "w+")

f3 = open("file3.txt", "w+")

#create three files with the newline already appropriately placed
s1= ''
s2= ''
s3= ''

#randomly select ASCII values between 97 and 122 (lowercase alphabet) then convert them to chars
for x in range(10):
	s1 += chr(randrange(97, 123, 1))
	s2 += chr(randrange(97, 123, 1))
	s3 += chr(randrange(97, 123, 1))

#print the contents of the three files
print(s1)
print(s2)
print(s3)

#append a newline as the 11th character to each string
s1 += '\n'
s2 += '\n'
s3 += '\n'

#write the strings of random characters to each file via the file descriptors
f1.write(s1)
f2.write(s2)
f3.write(s3)

#calculate and print out two random integers each on a separate line
#ranging from 1 - 42 inclusive
val1 = randrange(1, 43, 1)
val2 = randrange(1, 43, 1)
print(val1)
print(val2)
print(val1 * val2)

#close the files
f1.close()
f2.close()
f3.close()
