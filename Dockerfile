#from ubuntu
FROM ubuntu:22.04
RUN apt update
RUN apt install -y vim cmake g++ wget unzip nlohmann-json3-dev libboost-all-dev valgrind pip
RUN pip install numpy

#try local download
RUN wget https://download.pytorch.org/libtorch/nightly/cpu/libtorch-shared-with-deps-latest.zip
RUN unzip libtorch-shared-with-deps-latest.zip

# to run
#docker run -it -v Network-Simulator:/simulator sim
# experiment example
#nohup docker run -v ~/Network-Simulator:/simulator sim2 bash -c "cd simulator ; bash many_runs.sh" &

# to compile
# following https://pytorch.org/cppdocs/installing.html
#cmake -DCMAKE_PREFIX_PATH=/Network-Simulator/src/libtorch/ -S src/ -B build/
