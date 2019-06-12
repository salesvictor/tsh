all: tsh

tsh: Job.cc Shell.cc Main.cc Job.h Shell.h Process.h
	$(CXX) $^ -o tsh

clean:
	$(RM) tsh

debug: Job.cc Shell.cc Main.cc Job.h Shell.h Process.h
	$(CXX) -g $^ -o tsh
