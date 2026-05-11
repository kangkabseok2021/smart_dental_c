FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# universe repo carries Qt6 Charts and other extras
RUN echo "deb http://archive.ubuntu.com/ubuntu jammy universe" >> /etc/apt/sources.list && \
    apt-get update && apt-get install -y \
    build-essential cmake ninja-build git \
    qt6-base-dev \
    qt6-declarative-dev \
    libqt6charts6-dev \
    libgl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -B build -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON && \
    cmake --build build && \
    ctest --test-dir build --output-on-failure

ENV QT_QPA_PLATFORM=offscreen
RUN mkdir -p data
CMD ["./build/SmartDentalHandpiece"]
