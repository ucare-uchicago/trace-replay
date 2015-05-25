Replayer: Replayer.cpp
	g++-4.8 Replayer.cpp -pthread -lrt -o Replayer -std=c++11

clean:	
	rm -f Replayer
