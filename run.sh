#!/bin/bash

# ==========================================
# PART 1: THE DOCKER LAUNCHER (Runs in WSL)
# ==========================================
if [ ! -f /.dockerenv ]; then
    echo "[Host] Spinning up Chrono Rift Docker Environment..."

    docker run -it --rm \
        -e DISPLAY=host.docker.internal:0 \
        -v /tmp/.X11-unix:/tmp/.X11-unix \
        -v /mnt/wslg:/mnt/wslg \
        -e PULSE_SERVER=/mnt/wslg/PulseServer \
        -v $(pwd):/usr/src/app \
        chrono_rift_os bash -c "
            echo '======================================'
            echo '    CHRONO RIFT OS - LIVE'
            echo '======================================'
            echo '[1/3] Compiling...'
            make
            echo '[2/3] Booting OS...'
            pkill -f arbiter/arbiter   2>/dev/null
            pkill -f hip/hip           2>/dev/null
            pkill -f asp/asp           2>/dev/null
            pkill -f hip/render_thread 2>/dev/null
            ./arbiter/arbiter &
            ARBITER_PID=\$!
            sleep 1
            echo '[3/3] Launching Visual Engine...'
            ./hip/render_thread
            echo '[System] Window closed. Shutting down...'
            kill \$ARBITER_PID 2>/dev/null
            wait \$ARBITER_PID 2>/dev/null
        "

    echo "[Host] Container destroyed. Goodbye!"
    exit 0
fi