# docker build -t kandle_tests .
# docker run -it -v ${PWD}/..:/home/kandle kandle_tests

FROM ubuntu

# Update
RUN apt-get update && apt-get upgrade -y

# Dependencies
RUN apt-get install -y \
	'build-essential' \
	'wget' \
    'cmake' \
	'git' \
    'unzip' \
	'make'

WORKDIR /home

# Open to bash
CMD [ "/bin/bash" ]
