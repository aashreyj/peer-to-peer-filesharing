tracker: tracker.o client.o clientFile.o clientOperations.o group.o peerDownload.o seeder.o torrentFile.o tokenize.o user.o user_request_processing.o
	g++ tracker.o group.o seeder.o tokenize.o torrentFile.o user.o user_request_processing.o -o tracker.out
	g++ client.o clientFile.o clientOperations.o peerDownload.o tokenize.o -o client.out -lcrypto
	rm *.o

tracker.o: tracker/tracker.cpp tracker/constants.h tracker/group.h tracker/tokenize.h tracker/user.h tracker/user_request_processing.h
	g++ -c tracker/tracker.cpp

client.o: client/client.cpp client/clientFile.h client/clientOperations.h tracker/constants.h client/peerDownload.h tracker/tokenize.h
	g++ -c client/client.cpp

clientFile.o: client/clientFile.cpp client/clientFile.h
	g++ -c client/clientFile.cpp

clientOperations.o: client/clientOperations.cpp client/clientOperations.h tracker/constants.h tracker/tokenize.h
	g++ -c client/clientOperations.cpp -lcrypto

group.o: tracker/group.cpp tracker/group.h tracker/torrentFile.h tracker/user.h
	g++ -c tracker/group.cpp

peerDownload.o: client/peerDownload.cpp client/peerDownload.h tracker/constants.h tracker/tokenize.h client/clientOperations.h
	g++ -c client/peerDownload.cpp -lcrypto

seeder.o: tracker/seeder.cpp tracker/seeder.h
	g++ -c tracker/seeder.cpp

tokenize.o: tracker/tokenize.cpp tracker/tokenize.h
	g++ -c tracker/tokenize.cpp

torrentFile.o: tracker/torrentFile.cpp tracker/torrentFile.h
	g++ -c tracker/torrentFile.cpp

user.o: tracker/user.cpp tracker/user.h
	g++ -c tracker/user.cpp

user_request_processing.o: tracker/user_request_processing.cpp tracker/user_request_processing.h tracker/constants.h tracker/group.h tracker/tokenize.h tracker/user.h
	g++ -c tracker/user_request_processing.cpp
