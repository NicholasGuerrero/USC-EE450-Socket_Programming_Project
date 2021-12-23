all: serverC serverT serverS serverP
	g++ -o central central.cpp
	g++ -o serverT serverT.cpp
	g++ -o serverS serverS.cpp
	g++ -o serverP serverP.cpp
	g++ -o clientA clientA.cpp
	g++ -o clientB clientB.cpp

serverC:
		./central

serverT:
		./serverT

serverS:
		./serverS

serverP:
		./serverP


.PHONY: all serverC serverT serverS serverP
