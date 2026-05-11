FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV QT_VERSION=6.7.0

RUN apt-get update && apt-get install -y \
    build-essential cmake ninja-build git python3 python3-pip wget \
    libgl1-mesa-dev libgles2-mesa-dev \
    libxcb-xinerama0-dev libxcb-icccm4-dev libxcb-image0-dev \
    libxcb-keysyms1-dev libxcb-randr0-dev libxcb-render-util0-dev \
    libxcb-xfixes0-dev libxkbcommon-dev libxkbcommon-x11-dev \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install aqtinstall
RUN aqt install-qt linux desktop ${QT_VERSION} gcc_64 \
    -m qtcharts -O /opt/Qt

ENV PATH="/opt/Qt/${QT_VERSION}/gcc_64/bin:${PATH}"
ENV Qt6_DIR="/opt/Qt/${QT_VERSION}/gcc_64/lib/cmake/Qt6"

WORKDIR /src
COPY . .

RUN cmake -B build -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DCMAKE_PREFIX_PATH=/opt/Qt/${QT_VERSION}/gcc_64
RUN cmake --build build
RUN ctest --test-dir build --output-on-failure

# ── Runtime stage ─────────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y \
    libgl1 libgles2 xvfb \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/SmartDentalHandpiece /app/SmartDentalHandpiece
COPY --from=builder /opt/Qt /opt/Qt

ENV QT_VERSION=6.7.0
ENV LD_LIBRARY_PATH="/opt/Qt/${QT_VERSION}/gcc_64/lib"
ENV QT_QPA_PLATFORM=offscreen

RUN mkdir -p /app/data
WORKDIR /app

CMD ["./SmartDentalHandpiece"]
