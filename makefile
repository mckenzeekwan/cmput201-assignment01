#Name: McKenzee Kwan
#CCID: mckenze1
#Lecture Section: A1
#Instructor Name: Henry Tang
#Lab Section: D05
arc_analyzer: src/assg.c
	gcc -Wall -Werror -std=c99 src/assg.c -o arc_analyzer
clean:
	rm -f arc_analyzer output.txt
