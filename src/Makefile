all: tsh

tsh: Job.cc Shell.cc Main.cc Job.h Shell.h Process.h
	$(CXX) -std=c++11 $^ -o tsh

clean:
	$(RM) tsh

debug: Job.cc Shell.cc Main.cc Job.h Shell.h Process.h
	$(CXX) -g -std=c++11 $^ -o tsh
