#from ubuntu
FROM ubuntu:22.04
RUN apt update
RUN apt install -y vim cmake g++ wget unzip nlohmann-json3-dev libboost-all-dev valgrind pip
RUN pip install numpy pandas

#try local download
#RUN wget https://download.pytorch.org/libtorch/nightly/cpu/libtorch-shared-with-deps-latest.zip
#RUN unzip libtorch-shared-with-deps-latest.zip
RUN wget https://download.pytorch.org/libtorch/nightly/cpu/libtorch-shared-with-deps-2.1.0.dev20230601%2Bcpu.zip
RUN unzip libtorch-shared-with-deps-2.1.0.dev20230601+cpu.zip

# to run
#docker run -it -v Network-Simulator:/simulator sim3
# experiment example
#nohup docker run -v ~/Network-Simulator:/simulator sim3 bash -c "cd simulator ; bash many_runs.sh" &

# to compile
# following https://pytorch.org/cppdocs/installing.html
# to setup cmake run `cmake -DCMAKE_PREFIX_PATH=/Network-Simulator/src/libtorch/ -S src/ -B build/`
# in the build directory run `make`
