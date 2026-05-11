CC      = g++
CFLAGS  = -Wall -Wextra -std=c++17 -pthread
LDFLAGS = -lrt -pthread -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

.PHONY: all clean

all: arbiter/arbiter hip/hip asp/asp hip/render_thread

# ── Arbiter ────────────────────────────────────────────────────────────────
arbiter/arbiter: arbiter/arbiter.cpp shared_data.h
	$(CC) $(CFLAGS) -o arbiter/arbiter arbiter/arbiter.cpp $(LDFLAGS)

# ── HIP – Human Interface Process ─────────────────────────────────────────
hip/hip: hip/hip.cpp shared_data.h
	$(CC) $(CFLAGS) -o hip/hip hip/hip.cpp $(LDFLAGS)

# ── ASP – Artificial System Process ───────────────────────────────────────
asp/asp: asp/asp.cpp shared_data.h
	$(CC) $(CFLAGS) -o asp/asp asp/asp.cpp $(LDFLAGS)

# ── Render Thread ──────────────────────────────────────────────────────────
hip/render_thread: hip/render_thread.cpp shared_data.h
	$(CC) $(CFLAGS) -o hip/render_thread hip/render_thread.cpp $(LDFLAGS)

# ── Clean ──────────────────────────────────────────────────────────────────
clean:
	rm -f arbiter/arbiter hip/hip asp/asp hip/render_thread
