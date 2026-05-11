# Use a standard, stable Linux environment
FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install essential C++ tools, SFML, and X11 graphical dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    gdb \
    valgrind \
    strace \
    htop \
    libsfml-dev \
    x11-apps \
    mesa-utils \
    libgl1-mesa-glx \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container
WORKDIR /usr/src/app

# Copy the current directory contents into the container
COPY . .

# Keep the container running so we can attach to it
CMD ["tail", "-f", "/dev/null"]